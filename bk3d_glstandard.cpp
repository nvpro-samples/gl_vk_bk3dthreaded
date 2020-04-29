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

namespace glstandard {

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
static const char *s_glslf_mesh_line = 
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
"layout(location=0) out vec4 outColor;\n"
"void main() {\n"
"\n"
"   outColor = vec4(0.5,0.5,0.2,1);\n"
"}\n"
;
GLSLShader s_shaderMesh;
GLSLShader s_shaderMeshLine;

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
GLSLShader s_shaderGrid;

struct BO
{
  GLuint Id;
  GLuint Sz;
};

BO g_uboMatrix = {0, 0};
BO g_uboLight  = {0, 0};

static GLuint s_vboGrid;
static GLuint s_vboGridSz;

static GLuint s_vboCross;
static GLuint s_vboCrossSz;

static LightBuffer s_light = {vec3f(0.4f, 0.8f, 0.3f)};

static GLuint s_vao = 0;

class Bk3dModelStandard;
//------------------------------------------------------------------------------
// Renderer: can be OpenGL or other
//------------------------------------------------------------------------------
class RendererStandard : public Renderer
{
private:
  bool                            m_bValid;
  std::vector<Bk3dModelStandard*> m_pModels;

  bool initResourcesGrid();
  bool deleteResourcesGrid();

  GLuint m_depthTexture;
  GLuint m_colorTexture;
  GLuint m_FBO;
  GLuint m_MSAA;

  void fboResize(int w, int h);
  void fboInitialize(int w, int h, int MSAA);
  void fboFinish();

  int m_winSize[2];

  nvgl::ProfilerGL         m_profilerGL;
  nvh::Profiler::SectionID m_slot;

public:
  RendererStandard()
  {
    m_bValid                      = false;
    g_renderers[g_numRenderers++] = this;
  }
  virtual ~RendererStandard() {}
  virtual const char* getName() { return "Naive Standard VBO"; }
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

  virtual bool buildPrimaryCmdBuffer();
  virtual bool deleteCmdBuffers();
  virtual bool buildCmdBufferGrid();
  virtual bool buildCmdBufferModel(Bk3dModel* pModel, int bufIdx, int mstart, int mend);
  virtual void consolidateCmdBuffersModel(Bk3dModel* pModel, int numCmdBuffers);
  virtual bool deleteCmdBufferModel(Bk3dModel* pModel);

  virtual bool updateForChangedRenderTarget(Bk3dModel* pModel);

  virtual bool invalidateCmdBufferGrid();

  virtual void displayStart(const mat4f& world, const InertiaCamera& camera, const mat4f& projection, bool bTimingGlitch);
  virtual void displayEnd();
  virtual void displayGrid(const InertiaCamera& camera, const mat4f projection);
  virtual void displayBk3dModel(Bk3dModel* pModel, const mat4f& cameraView, const mat4f projection, unsigned char topologies);
  virtual void blitToBackbuffer();

  virtual void updateViewport(GLint x, GLint y, GLsizei width, GLsizei height);
};

RendererStandard s_renderer;

//------------------------------------------------------------------------------
// Class for Object (made of 1 to N meshes)
//------------------------------------------------------------------------------
class Bk3dModelStandard
{
public:
  Bk3dModelStandard(Bk3dModel* pGenericModel);
  ~Bk3dModelStandard();

private:
  Bk3dModel* m_pGenericModel;

  BO m_uboObjectMatrices;
  BO m_uboMaterial;

public:
  bool initResourcesObject();
  bool deleteResourcesObject();
  void displayObject(Renderer* pRenderer, const mat4f& cameraView, const mat4f projection, GLuint fboMSAA8x, unsigned char topologies);

};  //Class Bk3dModelStandard


//------------------------------------------------------------------------------
// FBO FBO FBO FBO FBO FBO FBO FBO FBO FBO FBO
//------------------------------------------------------------------------------
void RendererStandard::fboResize(int w, int h)
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
void RendererStandard::fboInitialize(int w, int h, int MSAA)
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

void RendererStandard::fboFinish()
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
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool RendererStandard::initResourcesGrid()
{
  //
  // Grid floor
  //
  glCreateBuffers(1, &s_vboGrid);
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
  s_vboGridSz = sizeof(vec3f) * GRIDDEF * 4;
  glNamedBufferData(s_vboGrid, s_vboGridSz, data[0].vec_array, GL_STATIC_DRAW);
  delete[] data;
  //
  // Target Cross
  //
  glCreateBuffers(1, &s_vboCross);
  vec3f crossVtx[6] = {
      vec3f(-CROSSSZ, 0.0f, 0.0f), vec3f(CROSSSZ, 0.0f, 0.0f),  vec3f(0.0f, -CROSSSZ, 0.0f),
      vec3f(0.0f, CROSSSZ, 0.0f),  vec3f(0.0f, 0.0f, -CROSSSZ), vec3f(0.0f, 0.0f, CROSSSZ),
  };
  s_vboCrossSz = sizeof(vec3f) * 6;
  glNamedBufferData(s_vboCross, s_vboCrossSz, crossVtx[0].vec_array, GL_STATIC_DRAW);
  return true;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool RendererStandard::deleteResourcesGrid()
{
  glDeleteBuffers(1, &s_vboGrid);
  glDeleteBuffers(1, &s_vboCross);
  return true;
}
//------------------------------------------------------------------------------
// build commandList for the Grid
//------------------------------------------------------------------------------
bool RendererStandard::invalidateCmdBufferGrid()
{
  return true;
}
bool RendererStandard::buildPrimaryCmdBuffer()
{
  return true;
}
bool RendererStandard::deleteCmdBuffers()
{
  return true;
}
//------------------------------------------------------------------------------
// build commandList for the Grid
//------------------------------------------------------------------------------
bool RendererStandard::buildCmdBufferGrid()
{
  return true;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void RendererStandard::displayStart(const mat4f& world, const InertiaCamera& camera, const mat4f& projection, bool bTimingGlitch)
{
  m_slot = m_profilerGL.beginSection("scene");

  glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

  glEnable(GL_MULTISAMPLE);
  glViewport(0, 0, m_winSize[0], m_winSize[1]);

  float r = 0.0f; //bTimingGlitch ? 1.0f : 0.0f;
  glClearColor(r, 0.1f, 0.15f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  //glDepthFunc(GL_LEQUAL);
  glDisable(GL_CULL_FACE);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void RendererStandard::displayEnd()
{
  m_profilerGL.endSection(m_slot);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void RendererStandard::displayGrid(const InertiaCamera& camera, const mat4f projection)
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
  glNamedBufferSubData(s_vboCross, 0, sizeof(vec3f) * 6, crossVtx);
  // ------------------------------------------------------------------------------------------
  // Case of regular rendering
  //
  s_shaderGrid.bindShader();
  glEnableVertexAttribArray(0);
  glDisableVertexAttribArray(1);

  // --------------------------------------------------------------------------------------
  // Using regular VBO
  //
  glBindBufferBase(GL_UNIFORM_BUFFER, UBO_MATRIX, g_uboMatrix.Id);
  glBindVertexBuffer(0, s_vboGrid, 0, sizeof(vec3f));
  glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, 0);
  //
  // Draw!
  //
  glDrawArrays(GL_LINES, 0, GRIDDEF * 4);

  glBindVertexBuffer(0, s_vboCross, 0, sizeof(vec3f));
  glDrawArrays(GL_LINES, 0, 6);

  glDisableVertexAttribArray(0);
  //s_shaderGrid.unbindShader();
}


//------------------------------------------------------------------------------
// this is an example of creating a piece of token buffer that would be put
// as a header before every single glDrawCommandsStatesAddressNV so that
// proper view setup (viewport) get properly done without relying on any
// typical OpenGL command.
// this approach is good avoid messing with OpenGL state machine and later could
// prevent extra driver validation
//------------------------------------------------------------------------------
void RendererStandard::updateViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
  m_winSize[0] = width;
  m_winSize[1] = height;
  fboResize(width, height);
  glLineWidth(1.0);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool RendererStandard::initGraphics(int w, int h, int MSAA)
{
  //
  // some offscreen buffer
  //
  fboInitialize(w, h, MSAA);
  //
  // Shader compilation
  //
  if(!s_shaderGrid.addVertexShaderFromString(g_glslv_grid))
    return false;
  if(!s_shaderGrid.addFragmentShaderFromString(g_glslf_grid))
    return false;
  if(!s_shaderGrid.link())
    return false;
  if(!s_shaderMesh.addVertexShaderFromString(s_glslv_mesh))
    return false;
  if(!s_shaderMesh.addFragmentShaderFromString(s_glslf_mesh))
    return false;
  if(!s_shaderMesh.link())
    return false;
  if(!s_shaderMeshLine.addVertexShaderFromString(s_glslv_mesh))
    return false;
  if(!s_shaderMeshLine.addFragmentShaderFromString(s_glslf_mesh_line))
    return false;
  if(!s_shaderMeshLine.link())
    return false;

  //
  // Create some UBO for later share their 64 bits
  //
  glCreateBuffers(1, &g_uboMatrix.Id);
  g_uboMatrix.Sz = sizeof(MatrixBufferGlobal);
  glNamedBufferData(g_uboMatrix.Id, g_uboMatrix.Sz, &g_globalMatrices, GL_STREAM_DRAW);
  //glBindBufferBase(GL_UNIFORM_BUFFER,UBO_MATRIX, g_uboMatrix.Id);
  //
  // Trivial Light info...
  //
  glCreateBuffers(1, &g_uboLight.Id);
  g_uboLight.Sz = sizeof(LightBuffer);
  glNamedBufferData(g_uboLight.Id, g_uboLight.Sz, &s_light, GL_STATIC_DRAW);
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
  //
  // OpenGL timers
  //
  m_profilerGL = nvgl::ProfilerGL(&g_profiler);
  m_profilerGL.init();

  LOGOK("Initialized renderer %s\n", getName());
  m_bValid = true;
  return true;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool RendererStandard::terminateGraphics()
{
  fboFinish();
  detachModels();
  deleteResourcesGrid();
  glDeleteVertexArrays(1, &s_vao);
  s_vao = 0;
  glDeleteBuffers(1, &g_uboLight.Id);
  g_uboLight.Id = 0;
  glDeleteBuffers(1, &g_uboMatrix.Id);
  g_uboMatrix.Id = 0;
  s_shaderGrid.cleanup();
  s_shaderMesh.cleanup();
  s_shaderMeshLine.cleanup();
  m_profilerGL.deinit();
  m_bValid = false;
  return true;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool RendererStandard::attachModel(Bk3dModel* pGenericModel)
{
  Bk3dModelStandard* pModel = new Bk3dModelStandard(pGenericModel);
  m_pModels.push_back(pModel);  // keep track of allocated stuff
  return true;
}
bool RendererStandard::detachModels()
{
  bool b = true;
  for(int i = 0; i < m_pModels.size(); i++)
  {
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
bool RendererStandard::initResourcesModel(Bk3dModel* pGenericModel)
{
  return ((Bk3dModelStandard*)pGenericModel->m_pRendererData)->initResourcesObject();
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int  g_AAAA;
bool RendererStandard::buildCmdBufferModel(Bk3dModel* pModel, int bufIdx, int mstart, int mend)
{
  g_AAAA = mend;
  return false;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void RendererStandard::consolidateCmdBuffersModel(Bk3dModel* pModel, int numCmdBuffers)
{
  //((Bk3dModelCMDList*)pModel->m_pRendererData)->setNumCmdBuffers(numCmdBuffers);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool RendererStandard::deleteCmdBufferModel(Bk3dModel* pGenericModel)
{
  return true;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool RendererStandard::updateForChangedRenderTarget(Bk3dModel* pModel)
{
  return false;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void RendererStandard::displayBk3dModel(Bk3dModel* pGenericModel, const mat4f& cameraView, const mat4f projection, unsigned char topologies)
{
  const nvgl::ProfilerGL::Section profile(m_profilerGL, "display.Bk3dModel");
  ((Bk3dModelStandard*)pGenericModel->m_pRendererData)->displayObject(this, cameraView, projection, m_FBO, topologies);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void RendererStandard::blitToBackbuffer()
{
  const nvgl::ProfilerGL::Section profile(m_profilerGL, "blitToBackbuffer");
  glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FBO);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glBlitFramebuffer(0, 0, m_winSize[0], m_winSize[1], 0, 0, m_winSize[0], m_winSize[1], GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
Bk3dModelStandard::Bk3dModelStandard(Bk3dModel* pGenericModel)
{
  memset(&m_uboObjectMatrices, 0, sizeof(BO));
  memset(&m_uboMaterial, 0, sizeof(BO));

  // keep track of the stuff related to this renderer
  pGenericModel->m_pRendererData = this;
  // keep track of the generic model data in the part for this renderer
  m_pGenericModel = pGenericModel;
}

Bk3dModelStandard::~Bk3dModelStandard()
{
  glDeleteBuffers(1, &m_uboObjectMatrices.Id);
  glDeleteBuffers(1, &m_uboMaterial.Id);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool Bk3dModelStandard::deleteResourcesObject()
{
  glDeleteBuffers(1, &m_uboMaterial.Id);
  m_uboMaterial.Id = 0;
  m_uboMaterial.Sz = 0;
  glDeleteBuffers(1, &m_uboObjectMatrices.Id);
  m_uboObjectMatrices.Id = 0;
  m_uboObjectMatrices.Sz = 0;
  bk3d::Mesh* pMesh      = NULL;

  for(int i = 0; i < m_pGenericModel->m_meshFile->pMeshes->n; i++)
  {
    pMesh = m_pGenericModel->m_meshFile->pMeshes->p[i];
    int n = pMesh->pSlots->n;
    for(int s = 0; s < n; s++)
    {
      bk3d::Slot* pS = pMesh->pSlots->p[s];
      GLuint      id = pS->userData;
      pS->userData   = 0;
      glDeleteBuffers(1, &id);
    }
    for(int pg = 0; pg < pMesh->pPrimGroups->n; pg++)
    {
      bk3d::PrimGroup* pPG = pMesh->pPrimGroups->p[pg];
      if(pPG->indexArrayByteSize > 0)
      {
        if((pPG->pOwnerOfIB == pPG) || (pPG->pOwnerOfIB == NULL))  // this primitive group doesn't use other's buffer
        {
          GLuint id    = (GLuint)uintptr_t(pPG->userPtr);
          pPG->userPtr = NULL;
          glDeleteBuffers(1, &id);
        }
      }
    }
  }
  return false;
}
//------------------------------------------------------------------------------
// It is possible to create one VBO for one Mesh; and one EBO for each primitive group
// however, for this sample, we will create only one VBO for all and one EBO
// meshes and primitive groups will have an offset in these buffers
//------------------------------------------------------------------------------
bool Bk3dModelStandard::initResourcesObject()
{

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
    glNamedBufferData(m_uboMaterial.Id, m_uboMaterial.Sz, m_pGenericModel->m_material, GL_STATIC_DRAW);
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
    glNamedBufferData(m_uboObjectMatrices.Id, m_uboObjectMatrices.Sz, m_pGenericModel->m_objectMatrices, GL_STATIC_DRAW);
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
    pMesh = m_pGenericModel->m_meshFile->pMeshes->p[i];
    //
    // Slots: buffers for vertices
    //
    int n = pMesh->pSlots->n;
    for(int s = 0; s < n; s++)
    {
      bk3d::Slot* pS = pMesh->pSlots->p[s];
      GLuint      id;
      glCreateBuffers(1, &id);  // Buffer Object directly kept in the Slot
      pS->userData = id;
#if 1
      glNamedBufferData(id, pS->vtxBufferSizeBytes, NULL, GL_STATIC_DRAW);
#else
      glNamedBufferStorage(id, pS->vtxBufferSizeBytes, NULL, 0);  // Not working with NSight !!! https://www.opengl.org/registry/specs/ARB/buffer_storage.txt
#endif
      glNamedBufferSubData(id, 0, pS->vtxBufferSizeBytes, pS->pVtxBufferData);
    }
    //
    // Primitive groups
    //
    for(int pg = 0; pg < pMesh->pPrimGroups->n; pg++)
    {
      bk3d::PrimGroup* pPG = pMesh->pPrimGroups->p[pg];
      if(pPG->indexArrayByteSize > 0)
      {
        if((pPG->pOwnerOfIB == pPG) || (pPG->pOwnerOfIB == NULL))  // this primitive group doesn't use other's buffer
        {
          GLuint id;
          glCreateBuffers(1, &id);
          pPG->userPtr = (int*)id;
#if 1
          glNamedBufferData(id, pPG->indexArrayByteSize, NULL, GL_STATIC_DRAW);
#else
          glNamedBufferStorage(id, pPG->indexArrayByteSize, NULL, 0);  // Not working with NSight !!! https://www.opengl.org/registry/specs/ARB/buffer_storage.txt
#endif
          glNamedBufferSubData(id, pPG->indexArrayByteOffset, pPG->indexArrayByteSize, pPG->pIndexBufferData);
        }
        else
        {
          pPG->userPtr = pPG->pOwnerOfIB->userPtr;
        }
      }
      else
      {
        pPG->userPtr = NULL;
      }
    }
  }
  //LOGI("meshes: %d in :%d VBOs (%f Mb) and %d EBOs (%f Mb) \n", m_pGenericModel->m_meshFile->pMeshes->n, .size(), (float)totalVBOSz/(float)(1024*1024), m_ObjEBOs.size(), (float)totalEBOSz/(float)(1024*1024));
  return true;
}

//------------------------------------------------------------------------------
// Really dumb display loop: cycling in meshes and primitive groups as they come
//------------------------------------------------------------------------------
void Bk3dModelStandard::displayObject(Renderer* pRenderer, const mat4f& cameraView, const mat4f projection, GLuint fboMSAA8x, unsigned char topologies)
{
  NXPROFILEFUNC(__FUNCTION__);

  g_globalMatrices.mVP = projection * cameraView;
  g_globalMatrices.mW  = mat4f(array16_id);
  cameraView.get_translation(g_globalMatrices.eyePos);
  //g_globalMatrices.mW.rotate(nv_to_rad*180.0f, vec3f(0,1,0));
  g_globalMatrices.mW.rotate(-nv_to_rad * 90.0f, vec3f(1, 0, 0));
  g_globalMatrices.mW.translate(-m_pGenericModel->m_posOffset);
  g_globalMatrices.mW.scale(m_pGenericModel->m_scale);
  glNamedBufferSubData(g_uboMatrix.Id, 0, sizeof(g_globalMatrices), &g_globalMatrices);

  if(m_pGenericModel->m_meshFile)
  {
    GLuint curMaterial = 0;
    GLuint curTransf   = 0;
    glBindBufferBase(GL_UNIFORM_BUFFER, UBO_MATRIX, g_uboMatrix.Id);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBO_LIGHT, g_uboLight.Id);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBO_MATRIXOBJ, m_uboObjectMatrices.Id);
//
// Loop 2 times: for filled topologies, then for lines
// ideally, the models should be pre-sorted by shaders...
// Alternating from one shader to the other is very expensive
// so it is better to do all geometries for one dedicated shader/material
//
#define LINES 1
#define POLYS 2
    for(int s = LINES; s < POLYS + 1; s++)
    {
      if(s == POLYS)
      {
        if((topologies & 0x1C) == 0)
          continue;
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0, 1.0);  // no issue with redundant call here: the state capture will just deal with simplifying things
        s_shaderMesh.bindShader();
      }
      else
      {
        if((topologies & 3) == 0)
          continue;
        glDisable(GL_POLYGON_OFFSET_FILL);
        s_shaderMeshLine.bindShader();
      }
      for(int m = 1; m < m_pGenericModel->m_meshFile->pMeshes->n; m++)  //m_pGenericModel->m_meshFile->pMeshes->n; m++)
      {
        bk3d::Mesh* pMesh = m_pGenericModel->m_meshFile->pMeshes->p[m];
        //
        // First filter to eliminate meshes that aren't relevant for the pass
        //
        char bPrimType = 0;
        for(int pg = 0; pg < pMesh->pPrimGroups->n; pg++)
        {
          bk3d::PrimGroup* pPG = pMesh->pPrimGroups->p[pg];
          switch(pPG->topologyGL)
          {
            case GL_LINES:
            case GL_LINE_STRIP:
              bPrimType |= LINES;
              break;
            default:
              bPrimType |= POLYS;
              break;
          }
        }
        // skip is exclusively for primitives out of the scope of this loop
        if((s == POLYS) && (bPrimType == LINES))
          continue;
        if((s == LINES) && (bPrimType == POLYS))
          continue;

        if(pMesh->pTransforms && (pMesh->pTransforms->n > 0))
        {
          bk3d::Bone* pTransf = pMesh->pTransforms->p[0];
          if(pTransf && (curTransf != pTransf->ID))
          {
            curTransf = pTransf->ID;
            glBindBufferRange(GL_UNIFORM_BUFFER, UBO_MATRIXOBJ, m_uboObjectMatrices.Id,
                              curTransf * sizeof(MatrixBufferObject), sizeof(MatrixBufferObject));
          }
        }
        int n = pMesh->pSlots->n;
        // let's make it simple: for now we assume pos and normal come first:
        // 0: vertex
        // 1: normal
        // the file could give any arbitrary kind of attributes. Normally, we should check the attribute type and make them match with the shader expectation
        int bindingIndex = 0;
        for(int s = 0; s < n; s++)
        {
          bk3d::Slot* pS = pMesh->pSlots->p[s];
          glBindBuffer(GL_ARRAY_BUFFER, pS->userData);
          for(int a = 0; a < pS->pAttributes->n; a++)
          {
            glEnableVertexAttribArray(bindingIndex);
            bk3d::Attribute* pAttr = pS->pAttributes->p[a];
            //pAttr->name would give the attribute name... assuming we are right, here.
            //glBindVertexBuffer(bindingIndex, pS->userData, pAttr->dataOffsetBytes, pAttr->strideBytes);
            glVertexAttribPointer(bindingIndex, pAttr->numComp, pAttr->formatGL, GL_FALSE, pAttr->strideBytes,
                                  (const void*)pAttr->dataOffsetBytes);
            bindingIndex++;
          }
        }
        // disable other attributes... we never know
        for(int j = bindingIndex; j <= 3 /*15*/; j++)
          glDisableVertexAttribArray(bindingIndex);
        //====> render primitive groups
        for(int pg = 0; pg < pMesh->pPrimGroups->n; pg++)
        {
          bk3d::PrimGroup* pPG = pMesh->pPrimGroups->p[pg];
          switch(pPG->topologyGL)
          {
            case GL_LINES:
              if(!(topologies & 0x01))
                continue;
              break;
            case GL_LINE_STRIP:
              if(!(topologies & 0x02))
                continue;
              break;
            case GL_TRIANGLES:
              if(!(topologies & 0x04))
                continue;
              break;
            case GL_TRIANGLE_STRIP:
              if(!(topologies & 0x08))
                continue;
              break;
            case GL_TRIANGLE_FAN:
              if(!(topologies & 0x10))
                continue;
              break;
          }
          //
          // Material: point to the right one in the table
          //
          bk3d::Material* pMat = pPG->pMaterial;
          if(pMat && (curMaterial != pMat->ID))
          {
            curMaterial = pMat->ID;
            glBindBufferRange(GL_UNIFORM_BUFFER, UBO_MATERIAL, m_uboMaterial.Id, (curMaterial * sizeof(MaterialBuffer)),
                              sizeof(MaterialBuffer));
          }
          if(pPG->pTransforms->n > 0)
          {
            bk3d::Bone* pTransf = pPG->pTransforms->p[0];
            if(pTransf && (curTransf != pTransf->ID))
            {
              curTransf = pTransf->ID;
              glBindBufferRange(GL_UNIFORM_BUFFER, UBO_MATRIXOBJ, m_uboObjectMatrices.Id,
                                (curTransf * sizeof(MatrixBufferObject)), sizeof(MatrixBufferObject));
            }
          }
          if(pPG->pIndexBufferData)
          {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, (GLuint)uintptr_t(pPG->userPtr));
            glDrawElements(pPG->topologyGL, pPG->indexCount, pPG->indexFormatGL, (const void*)pPG->indexArrayByteOffset);
          }
          else
          {
            glDrawArrays(pPG->topologyGL, 0, pPG->indexCount);
          }
        }
      }  // for(int m=1; m< m_pGenericModel->m_meshFile->pMeshes->n;m++)
    }    // for(int s=0; s<2; s++)
    // normally we should diable what was really used... simplification for the sample...
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
  }
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool RendererStandard::initThreadLocalVars(int threadId)
{
  return false;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void RendererStandard::releaseThreadLocalVars() {}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void RendererStandard::destroyCommandBuffers(bool bAll) {}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void RendererStandard::waitForGPUIdle() {}

}  // namespace glstandard
