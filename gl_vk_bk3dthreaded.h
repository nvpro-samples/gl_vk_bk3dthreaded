/*
 * Copyright (c) 2016-2021, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2016-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

#define USEWORKERS
#define MAXCMDBUFFERS 100
#ifdef USEWORKERS
#define CCRITICALSECTIONHOLDER(c) CCriticalSectionHolder _cs(c);
#else
#define CCRITICALSECTIONHOLDER(c)
#endif

#include <assert.h>
#include "nvpwindow.hpp"

#include <glm/glm.hpp>
using namespace glm;

#include "GLSLShader.h"
#include "gl_nv_command_list.h"
#include <nvh/profiler.hpp>

#include <nvh/appwindowcamerainertia.hpp>

#include "helper_fbo.h"

#ifdef NVP_SUPPORTS_GZLIB
#include "zlib.h"
#endif
#include "bk3dEx.h"  // a baked binary format for few models

#define PROFILE_SECTION(name) nvh::Profiler::Section _tempTimer(g_profiler, name)

//
// For the case where we work with Descriptor Sets (Vulkan)
//
#define DSET_GLOBAL 0
#define BINDING_MATRIX 0
#define BINDING_LIGHT 1
#define BINDING_NOISE 2

#define DSET_OBJECT 1
#define BINDING_MATRIXOBJ 0
#define BINDING_MATERIAL 1

#define DSET_TOTALAMOUNT 2
//
// For the case where we just assign UBO bindings (cmd-list)
//
#define UBO_MATRIX 0
#define UBO_MATRIXOBJ 1
#define UBO_MATERIAL 2
#define UBO_LIGHT 3
#define NUM_UBOS 4

#define TOSTR_(x) #x
#define TOSTR(x) TOSTR_(x)

//
// Let's assume we would put any matrix that don't get impacted by the local object transformation
//
NV_ALIGN(
    256,
    struct MatrixBufferGlobal {
      mat4 mW;
      mat4 mVP;
      vec3 eyePos;
    });
//
// Let's assume these are the ones that can change for each object
// will used at an array of MatrixBufferObject
//
NV_ALIGN(
    256,
    struct MatrixBufferObject { mat4 mO; });
//
// if we create arrays with a structure, we must be aligned according to
// GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT (to query)
//
NV_ALIGN(
    256,
    struct MaterialBuffer {
      vec3  diffuse;
      float a;
    });

NV_ALIGN(
    256,
    struct LightBuffer { vec3 dir; });

//
// Externs
//
extern nvh::Profiler g_profiler;

extern bool   g_bDisplayObject;
extern GLuint g_MaxBOSz;
extern bool   g_bDisplayGrid;

extern MatrixBufferGlobal g_globalMatrices;

//------------------------------------------------------------------------------
class Bk3dModel;
//------------------------------------------------------------------------------
// Renderer: can be OpenGL or other
//------------------------------------------------------------------------------
class Renderer
{
public:
  Renderer() {}
  virtual ~Renderer() {}
  virtual const char* getName()                            = 0;
  virtual bool        valid()                              = 0;
  virtual bool        initGraphics(int w, int h, int MSAA) = 0;
  virtual bool        terminateGraphics()                  = 0;
  virtual bool        initThreadLocalVars(int threadId)    = 0;
  virtual void        releaseThreadLocalVars()             = 0;
  virtual void        destroyCommandBuffers(bool bAll)     = 0;
  virtual void        resetCommandBuffersPool() {}
  virtual void        waitForGPUIdle() = 0;

  virtual bool attachModel(Bk3dModel* pModel) = 0;
  virtual bool detachModels()                 = 0;

  virtual bool initResourcesModel(Bk3dModel* pModel) = 0;

  virtual bool buildPrimaryCmdBuffer() = 0;
  // bufIdx: index of cmdBuffer to create, containing mesh mstart to mend-1 (for testing concurrent cmd buffer creation)
  virtual bool buildCmdBufferModel(Bk3dModel* pModelcmd, int bufIdx = 0, int mstart = 0, int mend = -1) = 0;
  virtual void consolidateCmdBuffersModel(Bk3dModel* pModelcmd, int numCmdBuffers)                      = 0;
  virtual bool deleteCmdBufferModel(Bk3dModel* pModel)                                                  = 0;

  virtual bool updateForChangedRenderTarget(Bk3dModel* pModel) = 0;


  virtual void displayStart(const mat4& world, const InertiaCamera& camera, const mat4& projection, bool bTimingGlitch) = 0;
  virtual void displayEnd() {}
  virtual void displayGrid(const InertiaCamera& camera, const mat4 projection) = 0;
  // topologies: bits for each primitive type (Lines:1, linestrip:2, triangles:4, tristrips:8, trifans:16)
  virtual void displayBk3dModel(Bk3dModel* pModel, const mat4& cameraView, const mat4 projection, unsigned char topologies = 0xFF) = 0;
  virtual void blitToBackbuffer() = 0;

  virtual void updateViewport(GLint x, GLint y, GLsizei width, GLsizei height) = 0;

  virtual bool bFlipViewport() { return false; }
};
extern Renderer* g_renderers[10];
extern int       g_numRenderers;

//------------------------------------------------------------------------------
// Class for Object (made of 1 to N meshes)
// This class is agnostic to any renderer: just contains the data of geometry
//------------------------------------------------------------------------------
class Bk3dModel
{
public:
  Bk3dModel(const char* name, vec3* pPos = NULL, float* pScale = NULL);
  ~Bk3dModel();

  vec3        m_posOffset;
  float       m_scale;
  std::string m_name;
  struct Stats
  {
    unsigned int primitives;
    unsigned int drawcalls;
    unsigned int attr_update;
    unsigned int uniform_update;
  };

  MatrixBufferObject* m_objectMatrices;
  int                 m_objectMatricesNItems;

  MaterialBuffer* m_material;
  int             m_materialNItems;

  bk3d::FileHeader* m_meshFile;

  Stats m_stats;

  Renderer* m_pRenderer;
  void*     m_pRendererData;

  bool updateForChangedRenderTarget();
  bool loadModel();
  void printPosition();
  void addStats(Stats& stats);
};  //Class Bk3dModel


extern std::vector<Bk3dModel*> g_bk3dModels;

#define FOREACHMODEL(cmd)                                                                                              \
  {                                                                                                                    \
    for(int m = 0; m < g_bk3dModels.size(); m++)                                                                       \
    {                                                                                                                  \
      g_bk3dModels[m]->cmd;                                                                                            \
    }                                                                                                                  \
  }
