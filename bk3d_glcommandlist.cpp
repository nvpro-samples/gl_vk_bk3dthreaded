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
#define EXTERNSVCUI
#define WINDOWINERTIACAMERA_EXTERN
#define EMUCMDLIST_EXTERN
#include "gl_vk_bk3dthreaded.h"
#include "helper_fbo.h"
#include <nvgl/profiler_gl.hpp>

#define GRIDDEF 20
#define GRIDSZ 1.0f
#define CROSSSZ 0.01f

#ifdef _WIN32
#define DEBUGBREAK() DebugBreak()
#else
#define DEBUGBREAK() __builtin_trap()
#endif

namespace glcmdlist {
#include "gl_nv_commandlist_helpers.h"

//------------------------------------------------------------------------------
// Globals
//------------------------------------------------------------------------------
GLuint g_MaxBOSz             = 200000;
int    g_TokenBufferGrouping = 0;

//-----------------------------------------------------------------------------
// Shaders
//-----------------------------------------------------------------------------
static const char *s_glslv_mesh = 
"#version 430\n"
"#extension GL_ARB_separate_shader_objects : enable\n"
"#extension GL_NV_command_list : enable\n"
"layout(std140,commandBindableNV,binding=" TOSTR(UBO_MATRIX) ") uniform matrixBuffer {\n"
"   uniform mat4 mW;\n"
"   uniform mat4 mVP;\n"
"   vec3 eyePos;\n"
"} matrix;\n"
"layout(std140,commandBindableNV,binding=" TOSTR(UBO_MATRIXOBJ) ") uniform matrixObjBuffer {\n"
"   uniform mat4 mO;\n"
"} object;\n"
"layout(location=0) in  vec3 pos;\n"
"layout(location=1) in  vec3 N;\n"
"\n"
"layout(location=1) out vec3 outN;\n"
"layout(location=2) out vec3 outWPos;\n"
"layout(location=3) out vec3 outEyePos;\n"
"out gl_PerVertex {\n"
"    vec4  gl_Position;\n"
"};\n"
"void main()\n"
"{\n"
"  outN = N.xzy;\n"
"  vec4 wpos     = matrix.mW  * (object.mO * vec4(pos,1)); \n"
"  gl_Position   = matrix.mVP * wpos;\n"
"  outWPos       = wpos.xyz;\n"
"  outEyePos     = matrix.eyePos;\n"
"}\n"
;
static const char *s_glslf_mesh = 
"#version 430\n"
"#extension GL_ARB_separate_shader_objects : enable\n"
"#extension GL_NV_command_list : enable\n"
"layout(std140,commandBindableNV,binding=" TOSTR(UBO_MATERIAL) ") uniform materialBuffer {\n"
"   uniform vec3 diffuse;"
"} material;\n"
"layout(std140,commandBindableNV,binding=" TOSTR(UBO_LIGHT) ") uniform lightBuffer {\n"
"   uniform vec3 dir;"
"} light;\n"
"layout(location=1) in  vec3 N;\n"
"layout(location=2) in  vec3 inWPos;\n"
"layout(location=3) in  vec3 inEyePos;\n"
"layout(location=0,index=0) out vec4 outColor;\n"
"\n"
"vec3 Sky( vec3 ray )\n"
"{\n"
"    return mix( vec3(.8), vec3(0), exp2(-(1.0/max(ray.y,.01))*vec3(.4,.6,1.0)) );\n"
"}\n"
"\n"
"mat2 mm2(in float a){float c = cos(a), s = sin(a);return mat2(c,-s,s,c);}\n"
"\n"
"vec3 Voronoi( vec3 pos )\n"
"{\n"
"    vec3 d[8];\n"
"    d[0] = vec3(0,0,0);\n"
"    d[1] = vec3(1,0,0);\n"
"    d[2] = vec3(0,1,0);\n"
"    d[3] = vec3(1,1,0);\n"
"    d[4] = vec3(0,0,1);\n"
"    d[5] = vec3(1,0,1);\n"
"    d[6] = vec3(0,1,1);\n"
"    d[7] = vec3(1,1,1);\n"
"    \n"
"    const float maxDisplacement = .7; //tweak this to hide grid artefacts\n"
"    \n"
"   vec3 pf = floor(pos);\n"
"\n"
"    const float phi = 1.61803398875;\n"
"\n"
"    float closest = 12.0;\n"
"    vec3 result;\n"
"    for ( int i=0; i < 8; i++ )\n"
"    {\n"
"        vec3 v = (pf+d[i]);\n"
"        vec3 r = fract(phi*v.yzx+17.*fract(v.zxy*phi)+v*v*.03);//Noise(ivec3(floor(pos+d[i])));\n"
"        vec3 p = d[i] + maxDisplacement*(r.xyz-.5);\n"
"        p -= fract(pos);\n"
"        float lsq = dot(p,p);\n"
"        if ( lsq < closest )\n"
"        {\n"
"            closest = lsq;\n"
"            result = r;\n"
"        }\n"
"    }\n"
"    return fract(result.xyz);//+result.www); // random colour\n"
"}\n"
"\n"
"vec3 shade( vec3 pos, vec3 norm, vec3 rayDir, vec3 lightDir )\n"
"{\n"
"    vec3 paint = material.diffuse;\n"
"\n"
"    vec3 norm2 = normalize(norm+.02*(Voronoi(pos*800.0)*2.0-1.0));\n"
"    \n"
"    if ( dot(norm2,rayDir) > 0.0 ) // we shouldn't see flecks that point away from us\n"
"        norm2 -= 2.0*dot(norm2,rayDir)*rayDir;\n"
"\n"
"\n"
"    // diffuse layer, reduce overall contrast\n"
"    vec3 result = paint*.6*(pow(max(0.0,dot(norm,lightDir)),2.0)+.2);\n"
"\n"
"    vec3 h = normalize( lightDir-rayDir );\n"
"    vec3 s = pow(max(0.0,dot(h,norm2)),50.0)*10.0*vec3(1);\n"
"\n"
"    float rdotn = dot(rayDir,norm2);\n"
"    vec3 reflection = rayDir-2.0*rdotn*norm;\n"
"    s += Sky( reflection );\n"
"\n"
"    float f = pow(1.0+rdotn,5.0);\n"
"    f = mix( .2, 1.0, f );\n"
"    \n"
"    result = mix(result,paint*s,f);\n"
"    \n"
"    // gloss layer\n"
"    s = pow(max(0.0,dot(h,norm)),1000.0)*32.0*vec3(1);\n"
"    \n"
"    rdotn = dot(rayDir,norm);\n"
"    reflection = rayDir-2.0*rdotn*norm;\n"
"    \n"
"    return result;\n"
"}\n"
"\n"
"void main()\n"
"{\n"
"   vec3 lightDir = vec3(0, 0.707, 0.707);\n"
"   vec3 ray = inWPos;\n"
"   ray -= inEyePos;\n"
"   ray = normalize(ray);\n"
"   vec3 shaded = shade( inWPos, N, ray, lightDir );\n"
"   outColor = vec4(shaded, 1);\n"
"}\n"
;
static const char* s_glslf_mesh_line =
    "#version 430\n"
    "#extension GL_ARB_separate_shader_objects : enable\n"
    "#extension GL_NV_command_list : enable\n"
    "layout(location=0) out vec4 outColor;\n"
    "void main() {\n"
    "\n"
    "   outColor = vec4(0.5,0.5,0.2,1);\n"
    "}\n";

//-----------------------------------------------------------------------------
// Grid
//-----------------------------------------------------------------------------
static const char *g_glslv_grid =
"#version 430\n"
"#extension GL_ARB_separate_shader_objects : enable\n"
"#extension GL_NV_command_list : enable\n"
"layout(std140,commandBindableNV,binding=" TOSTR(UBO_MATRIX) ") uniform matrixBuffer {\n"
"   uniform mat4 mW;\n"
"   uniform mat4 mVP;\n"
"   vec3 eyePos;\n"
"} matrix;\n"
"layout(location=0) in  vec3 P;\n"
"out gl_PerVertex {\n"
"    vec4  gl_Position;\n"
"};\n"
"void main() {\n"
"   gl_Position = matrix.mVP * (matrix.mW * ( vec4(P, 1.0)));\n"
"}\n"
;
static const char* g_glslf_grid =
    "#version 430\n"
    "#extension GL_ARB_separate_shader_objects : enable\n"
    "#extension GL_NV_command_list : enable\n"
    "layout(location=0) out vec4 outColor;\n"
    "void main() {\n"
    "   outColor = vec4(0.5,0.7,0.5,1);\n"
    "}\n";

struct BufO
{
  GLuint   Id;
  GLuint   Sz;
  GLuint64 Addr;
  void     release()
  {
    if(Id)
    {
      glMakeNamedBufferNonResidentNV(Id);
      glDeleteBuffers(1, &Id);
    }
    memset(this, 0, sizeof(BufO));
  }
};

BufO g_uboMatrix = {0, 0, 0};
BufO g_uboLight  = {0, 0, 0};

static LightBuffer s_light = {vec3f(0, 0.707, 0.707)};

static GLuint s_vao = 0;

namespace grid {
static CommandStatesBatch command;
static TokenBuffer        tokenBuffer;
GLSLShader                shader;
static GLuint             vbo;
static GLuint             vboSz;
static GLuint64           vboAddr;
static GLuint             vboCross;
static GLuint             vboCrossSz;
static GLuint64           vboCrossAddr;
};  // namespace grid

TokenBuffer g_tokenBufferViewport;

class Bk3dModelCMDList;

//------------------------------------------------------------------------------
// Renderer: can be OpenGL or other
//------------------------------------------------------------------------------
class RendererCMDList : public Renderer
{
private:
  bool                           m_bValid;
  std::vector<Bk3dModelCMDList*> m_pModels;

  bool initResourcesGrid();
  bool deleteResourcesGrid();

  int m_winSize[2];

  GLuint m_depthTexture;
  GLuint m_colorTexture;
  GLuint m_FBO;
  GLuint m_MSAA;

  void fboResize(int w, int h);
  void fboInitialize(int w, int h, int MSAA);
  void fboFinish();
  void fboMakeResourcesResident();

  GLuint m_stateObjGridLine;
  GLuint m_stateObjMeshTri;
  GLuint m_stateObjMeshLine;

  nvgl::ProfilerGL m_profilerGL;
  nvh::Profiler::SectionID m_slot;

public:
  RendererCMDList()
  {
    m_bValid                      = false;
    g_renderers[g_numRenderers++] = this;
    m_winSize[0]                  = 100;
    m_winSize[1]                  = 100;
  }
  virtual ~RendererCMDList() {}
  virtual const char* getName() { return "OpenGL Command-list"; }
  virtual bool        valid() { return m_bValid; };
  virtual bool        initGraphics(int w, int h, int MSAA);
  virtual bool        terminateGraphics();
  virtual bool        initThreadLocalVars(int threadId);
  virtual void        releaseThreadLocalVars();
  virtual void        destroyCommandBuffers(bool bAll);
  virtual void        waitForGPUIdle();

  virtual bool attachModel(Bk3dModel* pModel);
  virtual bool detachModels();

  virtual bool initResourcesModel(Bk3dModel* pModel);
  virtual bool releaseResourcesModel(Bk3dModel* pModel);

  virtual bool buildPrimaryCmdBuffer();
  virtual bool deleteCmdBuffers();
  virtual bool buildCmdBufferModel(Bk3dModel* pModel, int bufIdx, int mstart, int mend);
  virtual void consolidateCmdBuffersModel(Bk3dModel* pModel, int numCmdBuffers);
  virtual bool deleteCmdBufferModel(Bk3dModel* pModel);

  virtual bool updateForChangedRenderTarget(Bk3dModel* pModel);

  virtual void displayStart(const mat4f& world, const InertiaCamera& camera, const mat4f& projection, bool bTimingGlitch);
  virtual void displayEnd();
  virtual void displayGrid(const InertiaCamera& camera, const mat4f projection);
  virtual void displayBk3dModel(Bk3dModel* pModel, const mat4f& cameraView, const mat4f projection, unsigned char topologies);
  virtual void blitToBackbuffer();

  virtual void updateViewport(GLint x, GLint y, GLsizei width, GLsizei height);

  virtual bool bFlipViewport() { return false; }

  friend class Bk3dModelCMDList;
};

RendererCMDList s_renderer;

//------------------------------------------------------------------------------
// Class for Object (made of 1 to N meshes)
//------------------------------------------------------------------------------
class Bk3dModelCMDList
{
public:
  static GLSLShader shader;
  static GLSLShader shaderLine;

  Bk3dModelCMDList(Bk3dModel* pGenericModel);
  ~Bk3dModelCMDList();

private:
  Bk3dModel*         m_pGenericModel;
  int                m_numUsedCmdBuffers;
  CommandStatesBatch m_commandModel2[MAXCMDBUFFERS];
  std::string        m_tokenBufferModel2[MAXCMDBUFFERS];  // contains the commands to send to the GPU for setup and draw
  CommandStatesBatch m_commandModel;  // used to gather the GPU pointers of a single batch and where states/fbos do change
  TokenBuffer        m_tokenBufferModel;  // contains the commands to send to the GPU for setup and draw

  std::vector<BufO> m_ObjVBOs;
  std::vector<BufO> m_ObjEBOs;

  BufO m_uboObjectMatrices;
  BufO m_uboMaterial;

  GLuint m_commandList;  // the list containing the command buffer(s)

  //-----------------------------------------------------------------------------
  // Store state objects according to what was changed while building the mesh
  // for now: only one argument. But could have more...
  //-----------------------------------------------------------------------------
  struct States
  {
    GLenum topology;
    //GLuint primRestartIndex;
    // we should have more comparison on attribute stride, offset...
  };
  struct StateLess
  {
    bool operator()(const States& _Left, const States& _Right) const
    {
      // check primRestartIndex, too
      return (_Left.topology < _Right.topology);
    }
  };
  std::map<States, GLuint, StateLess> m_glStates;

public:
  void   releaseState(GLuint s);
  bool   deleteCommandListData();
  GLenum topologyWithoutStrips(GLenum topologyGL);
  GLuint findStateOrCreate(bk3d::Mesh* pMesh, bk3d::PrimGroup* pPG);
  bool   buildCmdBuffer(RendererCMDList* pRenderer, int bufIdx, int mstart, int mend);
  void   init_command_list();
  void   update_fbo_target(GLuint fbo);
  void   consolidateCmdBuffers(int numCmdBuffers);
  bool   initResourcesObject();
  bool   deleteResourcesObject();
  void   displayObject(Renderer* pRenderer, const mat4f& cameraView, const mat4f projection, unsigned char topologies);

};  //Class Bk3dModelCMDList
GLSLShader Bk3dModelCMDList::shader;
GLSLShader Bk3dModelCMDList::shaderLine;

//------------------------------------------------------------------------------
// FBO FBO FBO FBO FBO FBO FBO FBO FBO FBO FBO
//------------------------------------------------------------------------------
void RendererCMDList::fboResize(int w, int h)
{
  m_winSize[0] = w;
  m_winSize[1] = h;

  if(m_depthTexture)
  {
    texture::deleteTexture(m_depthTexture);
    m_depthTexture = 0;
  }
  if(m_colorTexture)
    texture::deleteTexture(m_colorTexture);
  if(m_FBO)
  {
    fbo::detachColorTexture(m_FBO, 0, m_MSAA);
    fbo::detachDSTTexture(m_FBO, m_MSAA);
  }
  // initialize color texture
  m_colorTexture = texture::createRGBA8(w, h, m_MSAA, 0);
  fbo::attachTexture2D(m_FBO, m_colorTexture, 0, m_MSAA);
  m_depthTexture = texture::createDST(w, h, m_MSAA, 0);
  fbo::attachDSTTexture2D(m_FBO, m_depthTexture, m_MSAA);
  fbo::CheckStatus();
}
void RendererCMDList::fboInitialize(int w, int h, int MSAA)
{
  m_winSize[0] = w;
  m_winSize[1] = h;
  m_MSAA       = MSAA;

  m_FBO          = fbo::create();
  m_colorTexture = texture::createRGBA8(w, h, MSAA, 0);
  fbo::attachTexture2D(m_FBO, m_colorTexture, 0, MSAA);
  m_depthTexture = texture::createDST(w, h, MSAA, 0);
  fbo::attachDSTTexture2D(m_FBO, m_depthTexture, MSAA);
  fbo::CheckStatus();
}

void RendererCMDList::fboFinish()
{
  if(m_depthTexture)
  {
    texture::deleteTexture(m_depthTexture);
    m_depthTexture = 0;
  }
  if(m_colorTexture)
  {
    texture::deleteTexture(m_colorTexture);
    m_colorTexture = 0;
  }
  if(m_FBO)
  {
    fbo::detachColorTexture(m_FBO, 0, m_MSAA);
    fbo::detachDSTTexture(m_FBO, m_MSAA);
    m_FBO = 0;
  }
  fbo::deleteFBO(m_FBO);
}
void RendererCMDList::fboMakeResourcesResident()
{
  GLuint64 handle;
  handle = glGetTextureHandleARB(m_colorTexture);
  glMakeTextureHandleResidentARB(handle);
  handle = glGetTextureHandleARB(m_depthTexture);
  glMakeTextureHandleResidentARB(handle);
}
//------------------------------------------------------------------------------
// cleanup commandList for the Grid
//------------------------------------------------------------------------------
bool RendererCMDList::deleteResourcesGrid()
{
  if(grid::vbo)
  {
    glMakeNamedBufferNonResidentNV(grid::vbo);
    glDeleteBuffers(1, &grid::vbo);
    grid::vbo     = 0;
    grid::vboSz   = 0;
    grid::vboAddr = 0;
  }
  if(grid::vboCross)
  {
    glMakeNamedBufferNonResidentNV(grid::vboCross);
    glDeleteBuffers(1, &grid::vboCross);
    grid::vboCross     = 0;
    grid::vboCrossSz   = 0;
    grid::vboCrossAddr = 0;
  }
  return true;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool RendererCMDList::initResourcesGrid()
{
  deleteResourcesGrid();
  //
  // Grid floor
  //
  glCreateBuffers(1, &grid::vbo);
  vec3f* data = new vec3f[GRIDDEF * 4];
  vec3f* p    = data;
  int    j    = 0;
  for(int i = 0; i < GRIDDEF; i++)
  {
    *(p++) = vec3f(-GRIDSZ, 0.0, GRIDSZ * (-1.0f + 2.0f * (float)i / (float)GRIDDEF));
    *(p++) = vec3f(GRIDSZ * (1.0f - 2.0f / (float)GRIDDEF), 0.0, GRIDSZ * (-1.0f + 2.0f * (float)i / (float)GRIDDEF));
    *(p++) = vec3f(GRIDSZ * (-1.0f + 2.0f * (float)i / (float)GRIDDEF), 0.0, -GRIDSZ);
    *(p++) = vec3f(GRIDSZ * (-1.0f + 2.0f * (float)i / (float)GRIDDEF), 0.0, GRIDSZ * (1.0f - 2.0f / (float)GRIDDEF));
  }
  grid::vboSz = sizeof(vec3f) * GRIDDEF * 4;
  glNamedBufferData(grid::vbo, grid::vboSz, data[0].vec_array, GL_STATIC_DRAW);
  // make the buffer resident and get its pointer
  glGetNamedBufferParameterui64vNV(grid::vbo, GL_BUFFER_GPU_ADDRESS_NV, &grid::vboAddr);
  glMakeNamedBufferResidentNV(grid::vbo, GL_READ_ONLY);
  delete[] data;
  //
  // Target Cross
  //
  glCreateBuffers(1, &grid::vboCross);
  vec3f crossVtx[6] = {
      vec3f(-CROSSSZ, 0.0f, 0.0f), vec3f(CROSSSZ, 0.0f, 0.0f),  vec3f(0.0f, -CROSSSZ, 0.0f),
      vec3f(0.0f, CROSSSZ, 0.0f),  vec3f(0.0f, 0.0f, -CROSSSZ), vec3f(0.0f, 0.0f, CROSSSZ),
  };
  grid::vboCrossSz = sizeof(vec3f) * 6;
  glNamedBufferData(grid::vboCross, grid::vboCrossSz, crossVtx[0].vec_array, GL_STATIC_DRAW);
  // make the buffer resident and get its pointer
  glGetNamedBufferParameterui64vNV(grid::vboCross, GL_BUFFER_GPU_ADDRESS_NV, &grid::vboCrossAddr);
  glMakeNamedBufferResidentNV(grid::vboCross, GL_READ_WRITE);
  return true;
}
//------------------------------------------------------------------------------
// cleanup commandList for the Grid
//------------------------------------------------------------------------------
void cleanTokenBufferGrid()
{
  glDeleteBuffers(1, &grid::tokenBuffer.bufferID);
  grid::tokenBuffer.bufferID   = 0;
  grid::tokenBuffer.bufferAddr = 0;
  grid::tokenBuffer.data.clear();
  grid::command.release();
}

//------------------------------------------------------------------------------
// delete commandList for general stuff (Grid...)
//------------------------------------------------------------------------------
bool RendererCMDList::deleteCmdBuffers()
{
  // TODO!
  return true;
}
//------------------------------------------------------------------------------
// build commandList for general stuff (Grid...)
//------------------------------------------------------------------------------
bool RendererCMDList::buildPrimaryCmdBuffer()
{
  cleanTokenBufferGrid();
  //
  // build address command for attribute #0
  // - assign UBO addresses to uniform index for a given shader stage
  // - assign a VBO address to attribute 0 (only one needed here)
  // - issue draw commands
  //
  std::string data = buildUniformAddressCommand(UBO_MATRIX, g_uboMatrix.Addr, g_uboMatrix.Sz, STAGE_VERTEX);
  data += buildAttributeAddressCommand(0, grid::vboAddr, grid::vboSz);
  data += buildDrawArraysCommand(GL_LINES, GRIDDEF * 4);
  //
  // build another drawcall for the target cross
  //
  data += buildLineWidthCommand(1.0);  //m_fboBox.getSSFactor());
  data += buildAttributeAddressCommand(0, grid::vboCrossAddr, grid::vboCrossSz);
  data += buildDrawArraysCommand(GL_LINES, 6);
  grid::tokenBuffer.data = data;  // token buffer containing commands
  //
  // Generate the token buffer in which we copy tokenBuffer.data
  //
  glCreateBuffers(1, &grid::tokenBuffer.bufferID);
  glNamedBufferData(grid::tokenBuffer.bufferID, data.size(), &data[0], GL_STATIC_DRAW);
  glGetNamedBufferParameterui64vNV(grid::tokenBuffer.bufferID, GL_BUFFER_GPU_ADDRESS_NV, &grid::tokenBuffer.bufferAddr);
  glMakeNamedBufferResidentNV(grid::tokenBuffer.bufferID, GL_READ_WRITE);
  //
  // Build the tables for the command-state batch
  //
  // token buffer for the viewport setting
  grid::command.pushBatch(m_stateObjGridLine, m_FBO,  // m_fboBox.GetFBO(),
                          g_tokenBufferViewport.bufferAddr, &g_tokenBufferViewport.data[0], g_tokenBufferViewport.data.size());
  // token buffer for drawing the grid
  grid::command.pushBatch(m_stateObjGridLine, m_FBO,  // m_fboBox.GetFBO(),
                          grid::tokenBuffer.bufferAddr, &grid::tokenBuffer.data[0], grid::tokenBuffer.data.size());

  LOGI("Token buffer created for Grid\n");

  glDisableClientState(GL_VERTEX_ATTRIB_ARRAY_UNIFIED_NV);
  glDisableClientState(GL_ELEMENT_ARRAY_UNIFIED_NV);
  glDisableClientState(GL_UNIFORM_BUFFER_UNIFIED_NV);
  return true;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void RendererCMDList::displayStart(const mat4f& world, const InertiaCamera& camera, const mat4f& projection, bool bTimingGlitch)
{
  m_slot = m_profilerGL.beginSection("scene");

  glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
  glEnable(GL_MULTISAMPLE);
  glViewport(0, 0, m_winSize[0], m_winSize[1]);
  float r = 0.0f; //bTimingGlitch ? 1.0f : 0.0f;
  glClearColor(r, 0.1f, 0.15f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}
void RendererCMDList::displayEnd()
{
  m_profilerGL.endSection(m_slot);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void RendererCMDList::blitToBackbuffer()
{
  const nvgl::ProfilerGL::Section profile(m_profilerGL, "blitToBackbuffer");
  glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FBO);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glBlitFramebuffer(0, 0, m_winSize[0], m_winSize[1], 0, 0, m_winSize[0], m_winSize[1], GL_COLOR_BUFFER_BIT, GL_NEAREST);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void RendererCMDList::displayGrid(const InertiaCamera& camera, const mat4f projection)
{
  const nvgl::ProfilerGL::Section profile(m_profilerGL, "display.Grid");
  //
  // Update what is inside buffers
  //
  g_globalMatrices.mVP = projection * camera.m4_view;
  g_globalMatrices.mW  = mat4f(array16_id);
  glNamedBufferSubData(g_uboMatrix.Id, 0, sizeof(g_globalMatrices), &g_globalMatrices);
  //
  // The cross vertex change is an example on how command-list are compatible with changing
  // what is inside the vertex buffers. VBOs are outside of the token buffers...
  //
  const vec3f& p           = camera.curFocusPos;
  vec3f        crossVtx[6] = {
      vec3f(p.x - CROSSSZ, p.y, p.z), vec3f(p.x + CROSSSZ, p.y, p.z), vec3f(p.x, p.y - CROSSSZ, p.z),
      vec3f(p.x, p.y + CROSSSZ, p.z), vec3f(p.x, p.y, p.z - CROSSSZ), vec3f(p.x, p.y, p.z + CROSSSZ),
  };
  glNamedBufferSubData(grid::vboCross, 0, sizeof(vec3f) * 6, crossVtx);
  //
  // execute the commands from the token buffer
  //
  {
    //
    // real Command-list's Token buffer with states execution
    //
    glDrawCommandsStatesAddressNV(&grid::command.dataGPUPtrs[0], &grid::command.sizes[0], &grid::command.stateGroups[0],
                                  &grid::command.fbos[0], int(grid::command.numItems));
  }
  return;
}


//------------------------------------------------------------------------------
// this is an example of creating a piece of token buffer that would be put
// as a header before every single glDrawCommandsStatesAddressNV so that
// proper view setup (viewport) get properly done without relying on any
// typical OpenGL command.
// this approach is good avoid messing with OpenGL state machine and later could
// prevent extra driver validation
//------------------------------------------------------------------------------
void RendererCMDList::updateViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
  m_winSize[0] = width;
  m_winSize[1] = height;
  fboResize(width, height);
  fboMakeResourcesResident();
  // could have way more commands here...
  // ...
  if(g_tokenBufferViewport.bufferAddr == 0)
  {
    // first time: create
    g_tokenBufferViewport.data = buildViewportCommand(x, y, width, height);
    g_tokenBufferViewport.data += buildLineWidthCommand(1.0);
    glCreateBuffers(1, &g_tokenBufferViewport.bufferID);
    glNamedBufferData(g_tokenBufferViewport.bufferID, g_tokenBufferViewport.data.size(), &g_tokenBufferViewport.data[0], GL_STATIC_DRAW);
    glGetNamedBufferParameterui64vNV(g_tokenBufferViewport.bufferID, GL_BUFFER_GPU_ADDRESS_NV, &g_tokenBufferViewport.bufferAddr);
    glMakeNamedBufferResidentNV(g_tokenBufferViewport.bufferID, GL_READ_WRITE);
  }
  else
  {
    // change arguments in the token buffer: better keep the same system memory pointer, too:
    // CPU system memory used by the command-list compilation
    // this is a simple use-case here: only one token cmd with related structure...
    //
    Token_Viewport* dc  = (Token_Viewport*)&g_tokenBufferViewport.data[0];
    dc->cmd.x           = x;
    dc->cmd.y           = y;
    dc->cmd.width       = width;
    dc->cmd.height      = height;
    Token_LineWidth* lw = (Token_LineWidth*)(dc + 1);
    lw->cmd.lineWidth   = 1.0;
    //
    // just update. Offset is always 0 in our simple case
    glNamedBufferSubData(g_tokenBufferViewport.bufferID, 0 /*offset*/, g_tokenBufferViewport.data.size(),
                         &g_tokenBufferViewport.data[0]);
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  buildPrimaryCmdBuffer();
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool RendererCMDList::initGraphics(int w, int h, int MSAA)
{
  //
  // some offscreen buffer
  //
  fboInitialize(w, h, MSAA);
  fboMakeResourcesResident();
  //
  // Check mandatory extensions
  //
  if(!has_GL_NV_command_list)
  {
    LOGE("Failed to initialize NVIDIA command-list\n");
    return false;
  }
  if(!has_GL_NV_vertex_buffer_unified_memory)
  {
    LOGE("Failed to initialize NVIDIA Bindless graphics\n");
    return false;
  }
  initTokenInternals();

  //
  // Shader compilation
  //
  if(!grid::shader.addVertexShaderFromString(g_glslv_grid))
    return false;
  if(!grid::shader.addFragmentShaderFromString(g_glslf_grid))
    return false;
  if(!grid::shader.link())
    return false;
  if(!Bk3dModelCMDList::shader.addVertexShaderFromString(s_glslv_mesh))
    return false;
  if(!Bk3dModelCMDList::shader.addFragmentShaderFromString(s_glslf_mesh))
    return false;
  if(!Bk3dModelCMDList::shader.link())
    return false;
  if(!Bk3dModelCMDList::shaderLine.addVertexShaderFromString(s_glslv_mesh))
    return false;
  if(!Bk3dModelCMDList::shaderLine.addFragmentShaderFromString(s_glslf_mesh_line))
    return false;
  if(!Bk3dModelCMDList::shaderLine.link())
    return false;

  //
  // Create some UBO for later share their 64 bits
  //
  glCreateBuffers(1, &g_uboMatrix.Id);
  g_uboMatrix.Sz = sizeof(MatrixBufferGlobal);
  glNamedBufferData(g_uboMatrix.Id, g_uboMatrix.Sz, &g_globalMatrices, GL_STREAM_DRAW);
  glGetNamedBufferParameterui64vNV(g_uboMatrix.Id, GL_BUFFER_GPU_ADDRESS_NV, (GLuint64EXT*)&g_uboMatrix.Addr);
  glMakeNamedBufferResidentNV(g_uboMatrix.Id, GL_READ_WRITE);
  //glBindBufferBase(GL_UNIFORM_BUFFER,UBO_MATRIX, g_uboMatrix.Id);
  //
  // Trivial Light info...
  //
  glCreateBuffers(1, &g_uboLight.Id);
  g_uboLight.Sz = sizeof(LightBuffer);
  glNamedBufferData(g_uboLight.Id, g_uboLight.Sz, &s_light, GL_STREAM_DRAW);
  glGetNamedBufferParameterui64vNV(g_uboLight.Id, GL_BUFFER_GPU_ADDRESS_NV, (GLuint64EXT*)&g_uboLight.Addr);
  glMakeNamedBufferResidentNV(g_uboLight.Id, GL_READ_WRITE);
  //glBindBufferBase(GL_UNIFORM_BUFFER,UBO_LIGHT, g_uboLight.Id);
  //
  // Misc OGL setup
  //
  glGenVertexArrays(1, &s_vao);
  glBindVertexArray(s_vao);
  //
  // Grid
  //
  if(!initResourcesGrid())
    return false;
  LOGOK("Initialized renderer %s\n", getName());

  //
  // state objects
  // assume that the state machine is correctly setup from there.
  // best would be to exhaustively reset OpenGL states...
  //
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glDisable(GL_CULL_FACE);
  glClearColor(0.0f, 0.1f, 0.15f, 1.0f);
  glGenVertexArrays(1, &s_vao);
  glBindVertexArray(s_vao);
  glBindBufferBase(GL_UNIFORM_BUFFER, UBO_MATRIX, 0);
  glBindBufferBase(GL_UNIFORM_BUFFER, UBO_LIGHT, 0);
  glBindBufferBase(GL_UNIFORM_BUFFER, UBO_MATRIXOBJ, 0);
  glEnableClientState(GL_VERTEX_ATTRIB_ARRAY_UNIFIED_NV);
  glEnableClientState(GL_ELEMENT_ARRAY_UNIFIED_NV);
  glEnableClientState(GL_UNIFORM_BUFFER_UNIFIED_NV);
  //
  // We must capture states in a FBO configuration that will be at least compatible with
  // when we'll use these objects (attachments; formats... *not* sizes)
  glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
  //
  // states - Redundant settings on purpose to be explicit
  //
  {
    glCreateStatesNV(1, &m_stateObjMeshTri);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0, 1.0);
    //
    // Warning: simple approach for the sample: we KNOW that the models won't have other
    // attribute configuration (vec3 pos+ vec3 normal)
    // if the project had many kind of combinations, many state objects should be created
    //
    glEnableVertexAttribArray(0);
    glBindVertexBuffer(0, 0, 0, 24);
    glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, 0);
    glEnableVertexAttribArray(1);
    glBindVertexBuffer(1, 0, 0, 24);
    glVertexAttribFormat(1, 3, GL_FLOAT, GL_FALSE, 12);
    for(int a = 2; a < 16; a++)
      glDisableVertexAttribArray(a);
    Bk3dModelCMDList::shader.bindShader();
    glStateCaptureNV(m_stateObjMeshTri, GL_TRIANGLES);
  }
  //
  // states for lines
  //
  {
    Bk3dModelCMDList::shaderLine.bindShader();
    glDisable(GL_POLYGON_OFFSET_FILL);
    //
    // Warning: simple approach for the sample: we KNOW that the models won't have other
    // attribute configuration (vec3 pos+ vec3 normal)
    // if the project had many kind of combinations, many state objects should be created
    //
    glEnableVertexAttribArray(0);
    glBindVertexBuffer(0, 0, 0, 12);
    glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, 0);
    for(int a = 1; a < 16; a++)
      glDisableVertexAttribArray(a);
    glCreateStatesNV(1, &m_stateObjMeshLine);
    glStateCaptureNV(m_stateObjMeshLine, GL_LINES);
  }
  glDisableClientState(GL_VERTEX_ATTRIB_ARRAY_UNIFIED_NV);
  glDisableClientState(GL_ELEMENT_ARRAY_UNIFIED_NV);
  glDisableClientState(GL_UNIFORM_BUFFER_UNIFIED_NV);

  m_bValid = true;
  {
    cleanTokenBufferGrid();
    grid::shader.bindShader();
    //
    // Again here we know that the grid has a specific vertex Input format
    //
    glEnableVertexAttribArray(0);
    glBindVertexBuffer(0, grid::vbo, 0, sizeof(vec3f));
    glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, 0);
    for(int i = 1; i < 16; i++)
      glDisableVertexAttribArray(i);
    glEnableClientState(GL_VERTEX_ATTRIB_ARRAY_UNIFIED_NV);
    glDisableClientState(GL_ELEMENT_ARRAY_UNIFIED_NV);
    glEnableClientState(GL_UNIFORM_BUFFER_UNIFIED_NV);

    //glViewport(0,0,1280,720);
    glCreateStatesNV(1, &m_stateObjGridLine);
    glStateCaptureNV(m_stateObjGridLine, GL_LINES);
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  //
  // OpenGL timers
  //
  m_profilerGL = nvgl::ProfilerGL(&g_profiler);
  m_profilerGL.init();
  return true;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool RendererCMDList::terminateGraphics()
{
  fboFinish();
  detachModels();
  deleteResourcesGrid();
  cleanTokenBufferGrid();
  glDeleteVertexArrays(1, &s_vao);
  s_vao = 0;
  g_uboLight.release();
  g_uboMatrix.release();
  grid::shader.cleanup();
  Bk3dModelCMDList::shader.cleanup();
  Bk3dModelCMDList::shaderLine.cleanup();
  m_profilerGL.deinit();
  m_bValid = false;
  glDisableClientState(GL_VERTEX_ATTRIB_ARRAY_UNIFIED_NV);
  glDisableClientState(GL_ELEMENT_ARRAY_UNIFIED_NV);
  glDisableClientState(GL_UNIFORM_BUFFER_UNIFIED_NV);
  glDisableClientState(GL_ELEMENT_ARRAY_UNIFIED_NV);
  for(int a = 2; a < 16; a++)
    glDisableVertexAttribArray(a);
  return true;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool RendererCMDList::attachModel(Bk3dModel* pGenericModel)
{
  Bk3dModelCMDList* pModel = new Bk3dModelCMDList(pGenericModel);
  m_pModels.push_back(pModel);  // keep track of allocated stuff
  return true;
}
bool RendererCMDList::detachModels()
{
  bool b = true;
  for(int i = 0; i < m_pModels.size(); i++)
  {
    if(!m_pModels[i]->deleteCommandListData())
      b = false;
    if(!m_pModels[i]->deleteResourcesObject())
      b = false;
    delete m_pModels[i];
  }
  m_pModels.clear();
  return b;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool RendererCMDList::initResourcesModel(Bk3dModel* pGenericModel)
{
  return ((Bk3dModelCMDList*)pGenericModel->m_pRendererData)->initResourcesObject();
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool RendererCMDList::releaseResourcesModel(Bk3dModel* pModel)
{
  return true;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool RendererCMDList::buildCmdBufferModel(Bk3dModel* pModel, int bufIdx, int mstart, int mend)
{
  return ((Bk3dModelCMDList*)pModel->m_pRendererData)->buildCmdBuffer(this, bufIdx, mstart, mend);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void RendererCMDList::consolidateCmdBuffersModel(Bk3dModel* pModel, int numCmdBuffers)
{
  ((Bk3dModelCMDList*)pModel->m_pRendererData)->consolidateCmdBuffers(numCmdBuffers);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool RendererCMDList::deleteCmdBufferModel(Bk3dModel* pGenericModel)
{
  return ((Bk3dModelCMDList*)pGenericModel->m_pRendererData)->deleteCommandListData();
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool RendererCMDList::updateForChangedRenderTarget(Bk3dModel* pModel)
{
  return false;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void RendererCMDList::displayBk3dModel(Bk3dModel* pGenericModel, const mat4f& cameraView, const mat4f projection, unsigned char topologies)

{
  const nvgl::ProfilerGL::Section profile(m_profilerGL, "display.Bk3dModel");
  return ((Bk3dModelCMDList*)pGenericModel->m_pRendererData)->displayObject(this, cameraView, projection, topologies);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
Bk3dModelCMDList::Bk3dModelCMDList(Bk3dModel* pGenericModel)
{
  m_commandList               = 0;
  m_tokenBufferModel.bufferID = 0;
  memset(&m_uboObjectMatrices, 0, sizeof(BufO));
  memset(&m_uboMaterial, 0, sizeof(BufO));

  // keep track of the stuff related to this renderer
  pGenericModel->m_pRendererData = this;
  // keep track of the generic model data in the part for this renderer
  m_pGenericModel = pGenericModel;
}

Bk3dModelCMDList::~Bk3dModelCMDList()
{
  deleteCommandListData();

  for(int i = 0; i < m_ObjVBOs.size(); i++)
  {
    m_ObjVBOs[i].release();
  }
  for(int i = 0; i < m_ObjEBOs.size(); i++)
  {
    m_ObjEBOs[i].release();
  }
  m_uboObjectMatrices.release();
  m_uboMaterial.release();
}
//------------------------------------------------------------------------------
// destroy the command buffers and states
//------------------------------------------------------------------------------
bool Bk3dModelCMDList::deleteCommandListData()
{
  releaseState(0);  // 0 : release all
  if(m_tokenBufferModel.bufferID)
  {
    m_tokenBufferModel.release();
  }
  // delete FBOs... m_tokenBufferModel.fbos
  m_commandModel.release();

  if(m_commandList)
  {
    glDeleteCommandListsNV(1, &m_commandList);
    m_commandList = 0;
  }
  m_commandList = 0;
  memset(&m_pGenericModel->m_stats, 0, sizeof(Bk3dModel::Stats));
  return true;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void Bk3dModelCMDList::releaseState(GLuint s)
{
  std::map<States, GLuint, StateLess>::iterator iM = m_glStates.begin();
  while(iM != m_glStates.end())
  {
    if(s > 0 && (iM->second == s))
    {
      glDeleteStatesNV(1, &iM->second);
      m_glStates.erase(iM);
      return;
    }
    else if(s == 0)
    {
      glDeleteStatesNV(1, &iM->second);
    }
    ++iM;
  }
  if(s <= 0)
    m_glStates.clear();
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
GLenum Bk3dModelCMDList::topologyWithoutStrips(GLenum topologyGL)
{
  switch(topologyGL)
  {
    case GL_TRIANGLE_STRIP:
      topologyGL = GL_TRIANGLES;
      break;
    case GL_TRIANGLES:
      topologyGL = GL_TRIANGLES;
      break;
    case GL_QUAD_STRIP:
      topologyGL = GL_NONE;
      break;
    case GL_QUADS:
      topologyGL = GL_QUADS;
      break;
    case GL_LINE_STRIP:
      topologyGL = GL_LINES;
      break;
    case GL_LINES:
      topologyGL = GL_LINES;
      break;
    case GL_TRIANGLE_FAN:  // Not supported by command-lists
      topologyGL = GL_NONE;
      break;
    case GL_POINTS:
      topologyGL = GL_POINTS;  //GL_NONE;
      break;
    case GL_LINE_LOOP:  // Should be supported but raised errors, so far (?)
      topologyGL = GL_NONE;
      break;
  }
  return topologyGL;
}

//------------------------------------------------------------------------------
// the command-list is the ultimate optimization of the extension called with
// the same name. It is very close from old Display-lists but offer more flexibility
//------------------------------------------------------------------------------
void Bk3dModelCMDList::init_command_list()
{
  if(m_commandList)
    glDeleteCommandListsNV(1, &m_commandList);
  glCreateCommandListsNV(1, &m_commandList);
  {
    glCommandListSegmentsNV(m_commandList, 1);
    glListDrawCommandsStatesClientNV(m_commandList, 0, &m_commandModel.dataPtrs[0], &m_commandModel.sizes[0],
                                     &m_commandModel.stateGroups[0], &m_commandModel.fbos[0], int(m_commandModel.fbos.size()));
  }
  glCompileCommandListNV(m_commandList);
}
//------------------------------------------------------------------------------
// it is possible that the target for rendering end-up to another FBO
// for example when MSAA mode changed from 8x to 1x...
// therefore we must make sure that command list know about it
//------------------------------------------------------------------------------
void Bk3dModelCMDList::update_fbo_target(GLuint fbo)
{
  // simple case now: same for all
  for(int i = 0; i < m_commandModel.fbos.size(); i++)
    m_commandModel.fbos[i] = fbo;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool Bk3dModelCMDList::buildCmdBuffer(RendererCMDList* pRenderer, int bufIdx, int mstart, int mend)
{
  NXPROFILEFUNC(__FUNCTION__);
  if(!m_pGenericModel->m_meshFile)
    return false;

  GLsizei          tokenTableOffset = 0;
  std::vector<int> offsets;

  float  lineWidth = 1.0;
  float  width     = pRenderer->m_winSize[0];
  float  height    = pRenderer->m_winSize[0];
  GLuint FBO       = pRenderer->m_FBO;

  m_commandModel2[bufIdx].release();
  //
  // Walk through meshes as if we were traversing a scene...
  //
  m_tokenBufferModel2[bufIdx].clear();
  m_tokenBufferModel2[bufIdx] += buildLineWidthCommand(lineWidth);
  m_tokenBufferModel2[bufIdx] += buildUniformAddressCommand(UBO_MATRIX, g_uboMatrix.Addr, sizeof(MatrixBufferGlobal), STAGE_VERTEX);
  m_tokenBufferModel2[bufIdx] +=
      buildUniformAddressCommand(UBO_MATRIXOBJ, m_uboObjectMatrices.Addr, sizeof(MatrixBufferObject), STAGE_VERTEX);
  m_tokenBufferModel2[bufIdx] += buildUniformAddressCommand(UBO_LIGHT, g_uboLight.Addr, sizeof(LightBuffer), STAGE_FRAGMENT);
  m_pGenericModel->m_stats.uniform_update += 3;

  int totalDCs = 0;
  int nDCs     = 0;
  // first default state capture
  GLuint           curState           = -1;
  GLuint           curMaterial        = 0xFFFFFFFF;
  GLuint           curObjectTransform = 0xFFFFFFFF;
  bk3d::PrimGroup* pPrevPG            = NULL;
  bk3d::Mesh*      pPrevMesh          = NULL;
  bool             changed            = false;
  int              prevNAttr          = -1;

  BufO curVBO;
  BufO curEBO;

  //////////////////////////////////////////////
  // Loop through meshes
  //
  if((mend <= 0) || (mend > m_pGenericModel->m_meshFile->pMeshes->n))
    mend = m_pGenericModel->m_meshFile->pMeshes->n;
  for(int m = mstart; m < mend; m++)
  {
    bk3d::Mesh* pMesh = m_pGenericModel->m_meshFile->pMeshes->p[m];
    int         idx   = uintptr_t(pMesh->userPtr);
    curVBO            = m_ObjVBOs[idx];
    curEBO            = m_ObjEBOs[idx];
    //
    // the Mesh can (should) have a transformation associated to itself
    // this is the mode where the primitive groups share the same transformation
    // Change the uniform pointer of object transformation if it changed
    //
    if(pMesh->pTransforms && (pMesh->pTransforms->n > 0) && (curObjectTransform != pMesh->pTransforms->p[0]->ID))
    {
      curObjectTransform = pMesh->pTransforms->p[0]->ID;
      m_tokenBufferModel2[bufIdx] +=
          buildUniformAddressCommand(UBO_MATRIXOBJ, m_uboObjectMatrices.Addr + (curObjectTransform * sizeof(MatrixBufferObject)),
                                     sizeof(MatrixBufferObject), STAGE_VERTEX);
      m_pGenericModel->m_stats.uniform_update++;
    }
    //
    // build COMMANDS to assign pointers to attributes
    //
    int s = 0;
    int n = pMesh->pAttributes->n;
    for(; s < n; s++)
    {
      bk3d::Attribute* pA = pMesh->pAttributes->p[s];
      bk3d::Slot*      pS = pMesh->pSlots->p[pA->slot];
      m_tokenBufferModel2[bufIdx] += buildAttributeAddressCommand(s, curVBO.Addr + (GLuint64)pS->userPtr.p, pS->vtxBufferSizeBytes);
      m_pGenericModel->m_stats.attr_update++;
    }
    prevNAttr = n;
    ////////////////////////////////////////
    // Primitive groups in the mesh
    //
    for(int pg = 0; pg < pMesh->pPrimGroups->n; pg++)
    {
      bk3d::PrimGroup* pPG    = pMesh->pPrimGroups->p[pg];
      GLenum           PGTopo = topologyWithoutStrips(pPG->topologyGL);
      if(PGTopo == GL_NONE)
        continue;
      //
      // Change the uniform pointer if material changed
      //
      if(pPG->pMaterial && (curMaterial != pPG->pMaterial->ID))
      {
        curMaterial = pPG->pMaterial->ID;
        m_tokenBufferModel2[bufIdx] +=
            buildUniformAddressCommand(UBO_MATERIAL, m_uboMaterial.Addr + (curMaterial * sizeof(MaterialBuffer)),
                                       sizeof(MaterialBuffer), STAGE_FRAGMENT);
        m_pGenericModel->m_stats.uniform_update++;
      }
      //
      // the Primitive group can also have its own transformation
      // this is the mode where the mesh don't own the transformation but its primitive groups do
      // Change the uniform pointer of object transformation if it changed
      //
      if(pPG->pTransforms && (pPG->pTransforms->n > 0) && (curObjectTransform != pPG->pTransforms->p[0]->ID))
      {
        curObjectTransform = pPG->pTransforms->p[0]->ID;
        m_tokenBufferModel2[bufIdx] +=
            buildUniformAddressCommand(UBO_MATRIXOBJ, m_uboObjectMatrices.Addr + (curObjectTransform * sizeof(MatrixBufferObject)),
                                       sizeof(MatrixBufferObject), STAGE_VERTEX);
        m_pGenericModel->m_stats.uniform_update++;
      }
      //
      // Choose the state Object depending on the primitive type
      //
      GLuint prevState = curState;
      switch(topologyWithoutStrips(pPG->topologyGL))
      {
        case GL_LINES:
          if(curState != pRenderer->m_stateObjMeshLine)
          {
            curState = pRenderer->m_stateObjMeshLine;
          }
          //m_pGenericModel->m_stats.primitives += pPG->indexCount/2;
          break;
        case GL_TRIANGLES:
          if(curState != pRenderer->m_stateObjMeshTri)
          {
            curState = pRenderer->m_stateObjMeshTri;
          }
          //m_pGenericModel->m_stats.primitives += pPG->indexCount/3;
          break;
        case GL_QUADS:
          //m_pGenericModel->m_stats.primitives += pPG->indexCount/4;
          DEBUGBREAK();
          break;
        case GL_QUAD_STRIP:
          //m_pGenericModel->m_stats.primitives += pPG->indexCount-3;          
          DEBUGBREAK();
          break;
        case GL_POINTS:
          //m_pGenericModel->m_stats.primitives += pPG->indexCount;
          DEBUGBREAK();
          break;
        default:
          DEBUGBREAK();
          // not-handled cases...
          break;
      }
      if(prevState == -1)
        prevState = curState;
      if(prevState != curState)
      {
        m_commandModel2[bufIdx].pushBatch(prevState, FBO, 0, NULL, (GLsizei)m_tokenBufferModel2[bufIdx].size() - tokenTableOffset);
        offsets.push_back(tokenTableOffset);
        // new offset
        tokenTableOffset = (GLsizei)m_tokenBufferModel2[bufIdx].size();
      }
      // add other token COMMANDS: elements + drawcall
      if(pPG->indexArrayByteSize > 0)
      {
        m_tokenBufferModel2[bufIdx] += buildElementAddressCommand(curEBO.Addr + (GLuint64)pPG->userPtr, pPG->indexFormatGL);
        m_tokenBufferModel2[bufIdx] += buildDrawElementsCommand(pPG->topologyGL, pPG->indexCount);
        nDCs++;
      }
      else
      {
        m_tokenBufferModel2[bufIdx] += buildDrawArraysCommand(pPG->topologyGL, pPG->indexCount);
        nDCs++;
      }
      m_pGenericModel->m_stats.drawcalls++;

      pPrevPG = pPG;
    }  // for(int pg=0; pg<pMesh->pPrimGroups->n; pg++)
    pPrevMesh = pMesh;
  }  // for(int i=0; i< m_pGenericModel->m_meshFile->pMeshes->n; i++)
  if(pPrevPG)
  {
    //
    // Choose the state Object depending on the primitive type
    //
    switch(topologyWithoutStrips(pPrevPG->topologyGL))
    {
      case GL_LINES:
        if(curState != pRenderer->m_stateObjMeshLine)
        {
          curState = pRenderer->m_stateObjMeshLine;
        }
        //m_pGenericModel->m_stats.primitives += pPG->indexCount/2;
        break;
      case GL_TRIANGLES:
        if(curState != pRenderer->m_stateObjMeshTri)
        {
          curState = pRenderer->m_stateObjMeshTri;
        }
        //m_pGenericModel->m_stats.primitives += pPG->indexCount/3;
        break;
      default:
        //DebugBreak();
        // not-handled cases...
        break;
    }
    m_commandModel2[bufIdx].pushBatch(curState, FBO, 0, NULL, (GLsizei)m_tokenBufferModel2[bufIdx].size() - tokenTableOffset);
    offsets.push_back(tokenTableOffset);

    // new offset and ptr
    tokenTableOffset = (GLsizei)m_tokenBufferModel2[bufIdx].size();
    pPrevPG          = NULL;
  }
  totalDCs += nDCs;
  return true;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool Bk3dModelCMDList::deleteResourcesObject()
{
  glDeleteBuffers(1, &m_uboMaterial.Id);
  m_uboMaterial.Id = 0;
  m_uboMaterial.Sz = 0;
  glDeleteBuffers(1, &m_uboObjectMatrices.Id);
  m_uboObjectMatrices.Id = 0;
  m_uboObjectMatrices.Sz = 0;
  bk3d::Mesh* pMesh      = NULL;

  for(int i = 0; i < m_ObjVBOs.size(); i++)
  {
    glMakeNamedBufferNonResidentNV(m_ObjVBOs[i].Id);
    GLuint id = m_ObjVBOs[i].Id;
    glDeleteBuffers(1, &id);
  }
  m_ObjVBOs.clear();
  for(int i = 0; i < m_ObjEBOs.size(); i++)
  {
    glMakeNamedBufferNonResidentNV(m_ObjEBOs[i].Id);
    GLuint id = m_ObjEBOs[i].Id;
    glDeleteBuffers(1, &id);
  }
  m_ObjEBOs.clear();
  return true;
}
//------------------------------------------------------------------------------
// It is possible to create one VBO for one Mesh; and one EBO for each primitive group
// however, for this sample, we will create only one VBO for all and one EBO
// meshes and primitive groups will have an offset in these buffers
//------------------------------------------------------------------------------
bool Bk3dModelCMDList::initResourcesObject()
{
  NXPROFILEFUNC(__FUNCTION__);
  BufO         curVBO;
  BufO         curEBO;
  unsigned int totalVBOSz = 0;
  unsigned int totalEBOSz = 0;
  memset(&curVBO, 0, sizeof(curVBO));
  memset(&curEBO, 0, sizeof(curEBO));
  glCreateBuffers(1, &curVBO.Id);
  glCreateBuffers(1, &curEBO.Id);

  //m_pGenericModel->m_meshFile->pMeshes->n = 60000;
  //
  // create Buffer Object for materials
  //
  if(m_pGenericModel->m_meshFile->pMaterials && m_pGenericModel->m_meshFile->pMaterials->nMaterials)
  {
    //
    // Material UBO: *TABLE* of multiple materials
    // Then offset in it for various drawcalls
    //
    if(m_uboMaterial.Id == 0)
      glCreateBuffers(1, &m_uboMaterial.Id);

    m_uboMaterial.Sz = sizeof(MaterialBuffer) * m_pGenericModel->m_materialNItems;
    glNamedBufferData(m_uboMaterial.Id, m_uboMaterial.Sz, m_pGenericModel->m_material, GL_STREAM_DRAW);
    glGetNamedBufferParameterui64vNV(m_uboMaterial.Id, GL_BUFFER_GPU_ADDRESS_NV, (GLuint64EXT*)&m_uboMaterial.Addr);
    glMakeNamedBufferResidentNV(m_uboMaterial.Id, GL_READ_WRITE);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBO_MATERIAL, m_uboMaterial.Id);

    LOGI("%d materials stored in %d Kb\n", m_pGenericModel->m_meshFile->pMaterials->nMaterials, (m_uboMaterial.Sz + 512) / 1024);
  }

  //
  // create Buffer Object for Object-matrices
  //
  if(m_pGenericModel->m_meshFile->pTransforms && m_pGenericModel->m_meshFile->pTransforms->nBones)
  {
    //
    // Transformation UBO: *TABLE* of multiple transformations
    // Then offset in it for various drawcalls
    //
    if(m_uboObjectMatrices.Id == 0)
      glCreateBuffers(1, &m_uboObjectMatrices.Id);

    m_uboObjectMatrices.Sz = sizeof(MatrixBufferObject) * m_pGenericModel->m_objectMatricesNItems;
    glNamedBufferData(m_uboObjectMatrices.Id, m_uboObjectMatrices.Sz, m_pGenericModel->m_objectMatrices, GL_STREAM_DRAW);
    glGetNamedBufferParameterui64vNV(m_uboObjectMatrices.Id, GL_BUFFER_GPU_ADDRESS_NV,
                                     (GLuint64EXT*)&m_uboObjectMatrices.Addr);
    glMakeNamedBufferResidentNV(m_uboObjectMatrices.Id, GL_READ_WRITE);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBO_MATRIXOBJ, m_uboObjectMatrices.Id);

    LOGI("%d matrices stored in %d Kb\n", m_pGenericModel->m_meshFile->pTransforms->nBones, (m_uboObjectMatrices.Sz + 512) / 1024);
  }

  //
  // First pass: evaluate the size of the single VBO
  // and store offset to where we'll find data back
  //
  bk3d::Mesh* pMesh = NULL;
  for(int i = 0; i < m_pGenericModel->m_meshFile->pMeshes->n; i++)
  {
    pMesh          = m_pGenericModel->m_meshFile->pMeshes->p[i];
    pMesh->userPtr = (void*)m_ObjVBOs.size();  // keep track of the VBO
    //
    // When we reached the arbitrary limit: switch to another VBO
    //
    if(curVBO.Sz > (g_MaxBOSz * 1024 * 1024))
    {
      glCreateBuffers(1, &curVBO.Id);
#if 1
      glNamedBufferData(curVBO.Id, curVBO.Sz, NULL, GL_STATIC_DRAW);
#else
      glNamedBufferStorage(curVBO.Id, curVBO.Sz, NULL, 0);  // Not working with NSight !!! https://www.opengl.org/registry/specs/ARB/buffer_storage.txt
#endif
      glGetNamedBufferParameterui64vNV(curVBO.Id, GL_BUFFER_GPU_ADDRESS_NV, &curVBO.Addr);
      glMakeNamedBufferResidentNV(curVBO.Id, GL_READ_ONLY);
      //
      // push this VBO and create a new one
      //
      totalVBOSz += curVBO.Sz;
      m_ObjVBOs.push_back(curVBO);
      memset(&curVBO, 0, sizeof(curVBO));
      //
      // At the same time, create a new EBO... good enough for now
      //
      glCreateBuffers(1, &curEBO.Id);  // store directly to a user-space dedicated to this kind of things
#if 1
      glNamedBufferData(curEBO.Id, curEBO.Sz, NULL, GL_STATIC_DRAW);
#else
      glNamedBufferStorage(curEBO.Id, curEBO.Sz, NULL, 0);  // Not working with NSight !!! https://www.opengl.org/registry/specs/ARB/buffer_storage.txt
#endif
      glGetNamedBufferParameterui64vNV(curEBO.Id, GL_BUFFER_GPU_ADDRESS_NV, &curEBO.Addr);
      glMakeNamedBufferResidentNV(curEBO.Id, GL_READ_ONLY);
      //
      // push this VBO and create a new one
      //
      totalEBOSz += curEBO.Sz;
      m_ObjEBOs.push_back(curEBO);
      memset(&curEBO, 0, sizeof(curEBO));
    }
    //
    // Slots: buffers for vertices
    //
    int n = pMesh->pSlots->n;
    for(int s = 0; s < n; s++)
    {
      bk3d::Slot* pS   = pMesh->pSlots->p[s];
      pS->userData     = 0;
      pS->userPtr      = (int*)curVBO.Sz;
      GLuint alignedSz = (pS->vtxBufferSizeBytes >> 8);
      alignedSz += pS->vtxBufferSizeBytes & 0xFF ? 1 : 0;
      alignedSz = alignedSz << 8;
      curVBO.Sz += alignedSz;
    }
    //
    // Primitive groups
    //
    for(int pg = 0; pg < pMesh->pPrimGroups->n; pg++)
    {
      bk3d::PrimGroup* pPG = pMesh->pPrimGroups->p[pg];
      if(pPG->indexArrayByteSize > 0)
      {
        GLuint alignedSz = (pPG->indexArrayByteSize >> 8);
        alignedSz += pPG->indexArrayByteSize & 0xFF ? 1 : 0;
        alignedSz    = alignedSz << 8;
        pPG->userPtr = (void*)curEBO.Sz;
        curEBO.Sz += alignedSz;
      }
      else
      {
        pPG->userPtr = NULL;
      }
    }
  }
  //
  // Finalize the last set of data
  //
  {
    glCreateBuffers(1, &curVBO.Id);
#if 1
    glNamedBufferData(curVBO.Id, curVBO.Sz, NULL, GL_STATIC_DRAW);
#else
    glNamedBufferStorage(curVBO.Id, curVBO.Sz, NULL, 0);  // Not working with NSight !!! https://www.opengl.org/registry/specs/ARB/buffer_storage.txt
#endif
    glGetNamedBufferParameterui64vNV(curVBO.Id, GL_BUFFER_GPU_ADDRESS_NV, &curVBO.Addr);
    glMakeNamedBufferResidentNV(curVBO.Id, GL_READ_ONLY);
    //
    // push this VBO and create a new one
    //
    totalVBOSz += curVBO.Sz;
    m_ObjVBOs.push_back(curVBO);
    memset(&curVBO, 0, sizeof(curVBO));
    //
    // At the same time, create a new EBO... good enough for now
    //
    glCreateBuffers(1, &curEBO.Id);  // store directly to a user-space dedicated to this kind of things
#if 1
    glNamedBufferData(curEBO.Id, curEBO.Sz, NULL, GL_STATIC_DRAW);
#else
    glNamedBufferStorage(curEBO.Id, curEBO.Sz, NULL, 0);  // Not working with NSight !!! https://www.opengl.org/registry/specs/ARB/buffer_storage.txt
#endif
    glGetNamedBufferParameterui64vNV(curEBO.Id, GL_BUFFER_GPU_ADDRESS_NV, &curEBO.Addr);
    glMakeNamedBufferResidentNV(curEBO.Id, GL_READ_ONLY);
    //
    // push this VBO and create a new one
    //
    totalEBOSz += curEBO.Sz;
    m_ObjEBOs.push_back(curEBO);
    memset(&curVBO, 0, sizeof(curVBO));
  }
  //
  // second pass: put stuff in the buffer and store offsets
  //
  for(int i = 0; i < m_pGenericModel->m_meshFile->pMeshes->n; i++)
  {
    bk3d::Mesh* pMesh = m_pGenericModel->m_meshFile->pMeshes->p[i];
    int         idx   = uintptr_t(pMesh->userPtr);
    curVBO            = m_ObjVBOs[idx];
    curEBO            = m_ObjEBOs[idx];
    int n             = pMesh->pSlots->n;
    for(int s = 0; s < n; s++)
    {
      bk3d::Slot* pS = pMesh->pSlots->p[s];
      glNamedBufferSubData(curVBO.Id, (GLuint)uintptr_t((int*)pS->userPtr), pS->vtxBufferSizeBytes, pS->pVtxBufferData);
    }
    for(int pg = 0; pg < pMesh->pPrimGroups->n; pg++)
    {
      bk3d::PrimGroup* pPG = pMesh->pPrimGroups->p[pg];
      if(pPG->indexArrayByteSize > 0)
        glNamedBufferSubData(curEBO.Id, (GLuint)uintptr_t(pPG->userPtr), pPG->indexArrayByteSize, pPG->pIndexBufferData);
    }
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  }
  LOGI("meshes: %d in :%d VBOs (%f Mb) and %d EBOs (%f Mb) \n", m_pGenericModel->m_meshFile->pMeshes->n, m_ObjVBOs.size(),
       (float)totalVBOSz / (float)(1024 * 1024), m_ObjEBOs.size(), (float)totalEBOSz / (float)(1024 * 1024));
  return true;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void Bk3dModelCMDList::displayObject(Renderer* pRenderer, const mat4f& cameraView, const mat4f projection, unsigned char topologies)
{
  RendererCMDList* pRendererCmdList = static_cast<RendererCMDList*>(pRenderer);

  g_globalMatrices.mVP = projection * cameraView;
  g_globalMatrices.mW  = mat4f(array16_id);
  cameraView.get_translation(g_globalMatrices.eyePos);
  //g_globalMatrices.mW.rotate(nv_to_rad*180.0f, vec3f(0,1,0));
  g_globalMatrices.mW.rotate(-nv_to_rad * 90.0f, vec3f(1, 0, 0));
  g_globalMatrices.mW.translate(-m_pGenericModel->m_posOffset);
  g_globalMatrices.mW.scale(m_pGenericModel->m_scale);
  glNamedBufferSubData(g_uboMatrix.Id, 0, sizeof(g_globalMatrices), &g_globalMatrices);

  //
  // execute the commands from the token buffer
  //
  {
    // canceled for now... another feature from command-lists (== display object)
    //if(g_bUseCallCommandListNV)
    //    //
    //    // real compiled Command-list
    //    //
    //    glCallCommandListNV(m_commandList);
    //else
    {
      //
      // real Command-list's Token buffer with states execution
      //
      int nitems = int(m_commandModel.numItems);
      glDrawCommandsStatesAddressNV(&m_commandModel.dataGPUPtrs[0], &m_commandModel.sizes[0],
                                    &m_commandModel.stateGroups[0], &m_commandModel.fbos[0], nitems);
    }
    return;
  }
}
//------------------------------------------------------------------------------
// gather the N pieces of tokenBuffers and state object/FBO tables to build one
// and adjust the # of used CMD buffers
//------------------------------------------------------------------------------
void Bk3dModelCMDList::consolidateCmdBuffers(int numCmdBuffers)
{
  NXPROFILEFUNC(__FUNCTION__);
  m_numUsedCmdBuffers = numCmdBuffers;
  //
  // append the generated pieces of command-buffer
  //
  int prevSz = m_tokenBufferModel.data.size();
  m_tokenBufferModel.data.clear();
  m_commandModel.release();
  //
  // start with adding the viewport reference: using another token buffer
  //
  for(int i = 0; i < numCmdBuffers; i++)
  {
    if(m_commandModel2[i].stateGroups.size() == 0)
      continue;
    m_commandModel.pushBatch(m_commandModel2[i].stateGroups[0], m_commandModel2[i].fbos[0], g_tokenBufferViewport.bufferAddr,
                             &g_tokenBufferViewport.data[0], g_tokenBufferViewport.data.size());
    break;
  }
  for(int i = 0; i < numCmdBuffers; i++)
  {
    if(m_commandModel2[i].stateGroups.size() == 0)
      continue;
    m_tokenBufferModel.data += m_tokenBufferModel2[i];
    m_commandModel += m_commandModel2[i];  // append... but the GPU addresses will be WRONG
  }
  //
  // eventually re-allocate the buffer
  //
  if(prevSz < m_tokenBufferModel.data.size())
  {
    if(m_tokenBufferModel.bufferID == 0)
      glCreateBuffers(1, &m_tokenBufferModel.bufferID);
    else
      glMakeNamedBufferNonResidentNV(m_tokenBufferModel.bufferID);
    glNamedBufferData(m_tokenBufferModel.bufferID, m_tokenBufferModel.data.size(), &m_tokenBufferModel.data[0], GL_STREAM_DRAW);  //GL_STATIC_DRAW);
    // get the 64 bits pointer and make it resident: bedcause we will go through its pointer
    glGetNamedBufferParameterui64vNV(m_tokenBufferModel.bufferID, GL_BUFFER_GPU_ADDRESS_NV, &m_tokenBufferModel.bufferAddr);
    glMakeNamedBufferResidentNV(m_tokenBufferModel.bufferID, GL_READ_ONLY);
  }
  else
  {
    glNamedBufferSubData(m_tokenBufferModel.bufferID, 0, m_tokenBufferModel.data.size(), &m_tokenBufferModel.data[0]);
  }
  //
  // start at i=1 because the first is the Viewport, resolved...
  // others need update
  //
  GLuint64EXT pAddr64GPU = m_tokenBufferModel.bufferAddr;
  char*       pAddr      = &m_tokenBufferModel.data[0];
  for(int i = 1; i < m_commandModel.numItems; i++)
  {
    m_commandModel.dataGPUPtrs[i] = pAddr64GPU;
    m_commandModel.dataPtrs[i]    = pAddr;
    pAddr64GPU += m_commandModel.sizes[i];
    pAddr += m_commandModel.sizes[i];
  }

  //init_command_list();
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Timer
//------------------------------------------------------------------------------
bool RendererCMDList::initThreadLocalVars(int threadId)
{
  return false;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void RendererCMDList::releaseThreadLocalVars() {}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void RendererCMDList::destroyCommandBuffers(bool bAll) {}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void RendererCMDList::waitForGPUIdle() {}

}  // namespace glcmdlist
