/* Copyright (c) 2018, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

#include "nvmath/nvmath.h"
#include "nvmath/nvmath_glsltypes.h"
using namespace nvmath;

#include "GLSLShader.h"
#include "gl_nv_command_list.h"
#include <nvh/profiler.hpp>

#include <nvh/appwindowcamerainertia.hpp>

#include "helper_fbo.h"

#ifndef NOGZLIB
#   include "zlib.h"
#endif
#include "bk3dEx.h" // a baked binary format for few models

#define PROFILE_SECTION(name)   nvh::Profiler::Section _tempTimer(g_profiler ,name)

//
// For the case where we work with Descriptor Sets (Vulkan)
//
#define DSET_GLOBAL  0
#   define BINDING_MATRIX 0
#   define BINDING_LIGHT  1
#   define BINDING_NOISE  2

#define DSET_OBJECT  1
#   define BINDING_MATRIXOBJ   0
#   define BINDING_MATERIAL    1

#define DSET_TOTALAMOUNT 2
//
// For the case where we just assign UBO bindings (cmd-list)
//
#define UBO_MATRIX      0
#define UBO_MATRIXOBJ   1
#define UBO_MATERIAL    2
#define UBO_LIGHT       3
#define NUM_UBOS        4

#define TOSTR_(x) #x
#define TOSTR(x) TOSTR_(x)

//
// Let's assume we would put any matrix that don't get impacted by the local object transformation
//
NV_ALIGN(256, struct MatrixBufferGlobal
{ 
  mat4f mW; 
  mat4f mVP;
  vec3f eyePos;
} );
//
// Let's assume these are the ones that can change for each object
// will used at an array of MatrixBufferObject
//
NV_ALIGN(256, struct MatrixBufferObject
{
    mat4f mO;
} );
//
// if we create arrays with a structure, we must be aligned according to
// GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT (to query)
//
NV_ALIGN(256, struct MaterialBuffer
{
    vec3f diffuse;
    float a;
} );

NV_ALIGN(256, struct LightBuffer
{
    vec3f dir;
} );

//
// Externs
//
extern nvh::Profiler  g_profiler;

extern bool         g_bDisplayObject;
extern GLuint       g_MaxBOSz;
extern bool            g_bDisplayGrid;

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
    virtual const char *getName() = 0;
    virtual bool valid() = 0;
    virtual bool initGraphics(int w, int h, int MSAA) = 0;
    virtual bool terminateGraphics() = 0;
    virtual bool initThreadLocalVars(int threadId) = 0;
    virtual void releaseThreadLocalVars() = 0;
    virtual void destroyCommandBuffers(bool bAll) = 0;
    virtual void resetCommandBuffersPool() {}
    virtual void waitForGPUIdle() = 0;

    virtual bool attachModel(Bk3dModel* pModel) = 0;
    virtual bool detachModels() = 0;

    virtual bool initResourcesModel(Bk3dModel* pModel) = 0;

    virtual bool buildPrimaryCmdBuffer() = 0;
    // bufIdx: index of cmdBuffer to create, containing mesh mstart to mend-1 (for testing concurrent cmd buffer creation)
    virtual bool buildCmdBufferModel(Bk3dModel* pModelcmd, int bufIdx=0, int mstart=0, int mend=-1) = 0;
    virtual void consolidateCmdBuffersModel(Bk3dModel* pModelcmd, int numCmdBuffers) = 0;
    virtual bool deleteCmdBufferModel(Bk3dModel* pModel) = 0;

    virtual bool updateForChangedRenderTarget(Bk3dModel* pModel) = 0;


    virtual void displayStart(const mat4f& world, const InertiaCamera& camera, const mat4f& projection, bool bTimingGlitch) = 0;
    virtual void displayEnd() 
    {}
    virtual void displayGrid(const InertiaCamera& camera, const mat4f projection) = 0;
    // topologies: bits for each primitive type (Lines:1, linestrip:2, triangles:4, tristrips:8, trifans:16)
    virtual void displayBk3dModel(Bk3dModel *pModel, const mat4f& cameraView, const mat4f projection, unsigned char topologies=0xFF) = 0;
    virtual void blitToBackbuffer() = 0;

    virtual void updateViewport(GLint x, GLint y, GLsizei width, GLsizei height) = 0;

    virtual bool bFlipViewport() { return false; }

};
extern Renderer*    g_renderers[10];
extern int            g_numRenderers;

//------------------------------------------------------------------------------
// Class for Object (made of 1 to N meshes)
// This class is agnostic to any renderer: just contains the data of geometry
//------------------------------------------------------------------------------
class Bk3dModel
{
public:
    Bk3dModel(const char *name, vec3f *pPos=NULL, float *pScale=NULL);
    ~Bk3dModel();

    vec3f               m_posOffset;
    float               m_scale;
    std::string         m_name;
    struct Stats {
        unsigned int    primitives;
        unsigned int    drawcalls;
        unsigned int    attr_update;
        unsigned int    uniform_update;
    };

    MatrixBufferObject* m_objectMatrices;
    int                 m_objectMatricesNItems;

    MaterialBuffer*     m_material;
    int                 m_materialNItems;

    bk3d::FileHeader*   m_meshFile;

    Stats m_stats;

    Renderer*            m_pRenderer;
    void*               m_pRendererData;
    
    bool updateForChangedRenderTarget();
    bool loadModel();
    void printPosition();
    void addStats(Stats &stats);
}; //Class Bk3dModel



extern std::vector<Bk3dModel*> g_bk3dModels;

#define FOREACHMODEL(cmd) {\
for (int m = 0; m<g_bk3dModels.size(); m++) {\
    g_bk3dModels[m]->cmd; \
}\
}
