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
#define GRIDDEF 20
#define GRIDSZ 1.0f
#define CROSSSZ 0.01f

// ABOUT USE_VKCMDBINDVERTEXBUFFERS_OFFSET define
//
// the define USE_VKCMDBINDVERTEXBUFFERS_OFFSET
// allows to try 2 different approaches for memory management:
//
// when USE_VKCMDBINDVERTEXBUFFERS_OFFSET is NOT defined, the memory for 3D model will be allocated
// in big chunks of memory, and each 3D Part will *get its own VkBuffer Handle* that we will bind to
// a sub-section of this pre-allocated device memory.
// Of course, this example doesn't implement a full memory allocator.
// Note that this example is also a bit "brute-force", as it may allocate lots of VkBuffers, if the model
// is made of many small buffers, for example. In a more serious situation, small buffers should definitely be
// aggregated togething and you should offsets at drawcall time...
// As a consequence, this approach might reach the maximum # of VkBuffer handles that the driver can contain
//
// when USE_VKCMDBINDVERTEXBUFFERS_OFFSET is defined, the opposite is done: few pairs of (VkBuffer + Device memory)
// will be used for many small parts of the model. Instead of associating many VkBuffers for each 3D parts, we will associate
// offsets to where they can be found in this big Vkbuffer. vkCmdBindVertexBuffers() will use these offsets
//
// The best solution would be to mix both of these approaches. But for clarity of the code, this sample doesn't do it


#define USE_VKCMDBINDVERTEXBUFFERS_OFFSET
#ifdef USE_VKCMDBINDVERTEXBUFFERS_OFFSET
#define VBOIDX userPtr
#define EBOIDX userPtr
#endif

#define EXTERNSVCUI
#define WINDOWINERTIACAMERA_EXTERN
#define EMUCMDLIST_EXTERN
#include "gl_vk_bk3dthreaded.h"
#include <fileformats/nv_dds.h>

#include <nvvk/profiler_vk.hpp>

#include "mt/CThreadWork.h"

#ifdef _WIN32
#define DEBUGBREAK() DebugBreak()
#else
#define DEBUGBREAK() __builtin_trap()
#endif
///////////////////////////////////////////////////////////////////////////////
// VULKAN: NVK.inl > vkfnptrinline.h > vulkannv.h > vulkan.h
//
#include "NVK.h"

///////////////////////////////////////////////////////////////////////////////
// renderer
//
namespace vk {
static GLuint s_vboGrid;
static GLuint s_vboGridSz;

static GLuint s_vboCross;
static GLuint s_vboCrossSz;

static LightBuffer s_light = {vec3f(0.4f, 0.8f, 0.3f)};

static GLuint s_vao = 0;

static NVK nvk;

class Bk3dModelVk;

//------------------------------------------------------------------------------
// Image buffer object
//------------------------------------------------------------------------------
struct ImgO
{
  VkImage        img;
  VkImageView    imgView;
  VkDeviceMemory imgMem;
  size_t         Sz;
  void           release()
  {
    if(imgView)
      nvk.destroyImageView(imgView);
    if(img)
      nvk.destroyImage(img);
    if(imgMem)
      nvk.freeMemory(imgMem);
    memset(this, 0, sizeof(ImgO));
  }
};

//------------------------------------------------------------------------------
// Buffer Object
//------------------------------------------------------------------------------
struct BufO
{
  VkBuffer       buffer;
  VkDeviceMemory bufferMem;
  size_t         Sz;
  void           release()
  {
    if(buffer)
      nvk.destroyBuffer(buffer);
    if(bufferMem)
      nvk.freeMemory(bufferMem);
    memset(this, 0, sizeof(BufO));
  }
};

BufO g_uboMatrix = {0, 0};
BufO g_uboLight  = {0, 0};
//------------------------------------------------------------------------------
// Renderer: can be OpenGL or other
//------------------------------------------------------------------------------
class RendererVk : public Renderer
{
private:
  bool                     m_bValid;
  nvh::Profiler::SectionID m_slot;  // for profiling purpose
  //
  // Vulkan stuff
  //
  struct PerThreadData
  {
    int m_threadId;
    // Command buffer pool must be local the the thread: for allocation and freeing
    NVK::CommandPool m_cmdPoolStatic;
// these pools are for dynamic allocations along the rendering
// we use a ring buffer of CMDPOOL_BUFFER_SZ and loop through it at each
// frame. we also keep track of where is the next available cmd-buffer
// (after a reset of cmd-buffer pool: we reuse allocated cmd-buffers)
#define CMDPOOL_BUFFER_SZ 3  // Ring buffer >= to swapchain so we are safe
    NVK::CommandPool  m_cmdPoolDynamic[CMDPOOL_BUFFER_SZ];
    NVK::CommandPool* m_curCmdPoolDynamic;
    //std::vector<VkCommandBuffer>        m_cmdBufferQueue[2];
  };
#ifdef USEWORKERS
  NThreadLocalVar<PerThreadData*> m_perThreadData;
#else
  PerThreadData*   m_perThreadData;
#endif
  VkDescriptorPool m_descPool;

  VkDescriptorSetLayout m_descriptorSetLayouts[DSET_TOTALAMOUNT];  // general layout and objects layout
  VkDescriptorSet       m_descriptorSetGlobal;                     // descriptor set for general part

  VkPipelineLayout m_pipelineLayout;

  VkPipeline m_pipelineMeshTri;
  VkPipeline m_pipelineMeshTriStrip;
  VkPipeline m_pipelineMeshTriFan;
  VkPipeline m_pipelineMeshLine;
  VkPipeline m_pipelineMeshLineStrip;
  VkPipeline m_pipelineGrid;

  VkRenderPass  m_scenePass;  // render-pass. For now things are simple: only one pass
  ImgO          m_colorImage;
  ImgO          m_colorImageMS;
  ImgO          m_DSTImageMS;
  VkFramebuffer m_framebuffer;

  NVK::CommandBuffer m_cmdScene;
  VkFence            m_cmdPoolFence[CMDPOOL_BUFFER_SZ];
  int                m_frameCounter, m_frameCounter2;  // looping from 0 to CMDPOOL_BUFFER_SZ-1

  // Used for merging Vulkan image to OpenGL backbuffer
  VkSemaphore m_semOpenGLReadDone;
  VkSemaphore m_semVKRenderingDone;


  NVK::CommandBuffer m_cmdSyncAndViewport;
  NVK::CommandBuffer m_cmdBufferGrid;

  BufO m_gridBuffer;
  BufO m_matrix;

  VkSampler m_sampler;

  // Profiler
  nvvk::ProfilerVK m_profilerVK;

  //
  // viewport info
  //
  NVK::Rect2D m_viewRect;

  int m_MSAA;
  //
  // Model
  //
  std::vector<Bk3dModelVk*> m_pModels;

  virtual void initFramebuffer(GLsizei width, GLsizei height, int MSAA);

  void releaseFramebuffer();
  bool initResourcesGrid();
  bool releaseResourcesGrid();

public:
  RendererVk()
  {
    m_frameCounter                = 0;
    m_frameCounter2               = 0;
    m_bValid                      = false;
    g_renderers[g_numRenderers++] = this;
  }
  virtual ~RendererVk() {}

  virtual const char* getName() { return "Vulkan"; }
  virtual bool        valid() { return m_bValid; };
  virtual bool        initRenderpassDependent(int w, int h, int MSAA);
  virtual bool        initGraphics(int w, int h, int MSAA);
  virtual bool        terminateGraphics();
  virtual bool        initThreadLocalVars(int threadId);
  virtual void        releaseThreadLocalVars();
  virtual void        destroyCommandBuffers(bool bAll);
  virtual void        resetCommandBuffersPool();
  virtual void        waitForGPUIdle();

  virtual bool attachModel(Bk3dModel* pModel);
  virtual bool detachModels();

  virtual bool initResourcesModel(Bk3dModel* pModel);
  virtual bool releaseResourcesModel(Bk3dModel* pModel);

  virtual bool buildPrimaryCmdBuffer();
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

  virtual bool bFlipViewport() { return true; }

  friend class Bk3dModelVk;
};

RendererVk s_renderer;

//------------------------------------------------------------------------------
// structure for a model's command buffer
//------------------------------------------------------------------------------
struct ModelCmdBuffers
{
  // one command buffer containing all in one
  NVK::CommandBuffer full;
  // array of command buffers: one for each topology: Lines, linestrip, triangles, tristrips, trifans
  // used when the user only wanted to render specific kind of prims. These would replace 'full' cmd buffer above
  NVK::CommandBuffer SplitTopo[5];
};
//------------------------------------------------------------------------------
// Class for Object (made of 1 to N meshes)
//------------------------------------------------------------------------------
class Bk3dModelVk
{
private:
  Bk3dModel*      m_pGenericModel;
  int             m_numUsedCmdBuffers;
  ModelCmdBuffers m_cmdBuffer[MAXCMDBUFFERS];

#ifdef USE_VKCMDBINDVERTEXBUFFERS_OFFSET
  std::vector<BufO> m_ObjVBOs;
  std::vector<BufO> m_ObjEBOs;
#else
  NVK::MemoryChunk m_memoryVBO;
  NVK::MemoryChunk m_memoryEBO;
#endif

  BufO m_uboObjectMatrices;
  BufO m_uboMaterial;

#define NDSETOBJECT 1
  VkDescriptorSet m_descriptorSets[NDSETOBJECT];  // descriptor sets for things related to this model: local transf+material

public:
  Bk3dModelVk(Bk3dModel* pGenericModel);
  ~Bk3dModelVk();

  bool feedCmdBuffer(RendererVk* pRendererVk, NVK::CommandBuffer& cmdBuffer, NVK::CommandBuffer* cmdBufferSplitTopo, int mstart, int mend);
  bool buildCmdBuffer(Renderer* pRenderer, int bufIdx, int mstart, int mend);
  void consolidateCmdBuffers(int numCmdBuffers);
  bool initResources(Renderer* pRenderer);
  bool releaseResources(Renderer* pRenderer);
  void displayObject(Renderer* pRenderer, const mat4f& cameraView, const mat4f projection, unsigned char topologies);
  Bk3dModel* getGenericModel() { return m_pGenericModel; }

  friend class RendererVk;
};  //Class Bk3dModelVk


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool load_binary(const std::string& name, std::string& data)
{
  FILE*                    fd = NULL;
  std::vector<std::string> paths;
  paths.push_back(name);
  paths.push_back(std::string("GLSL/") + name);
  paths.push_back(std::string(NVPSystem::exePath() + "/" + PROJECT_RELDIRECTORY + "GLSL/") + name);
  paths.push_back(std::string("../GLSL/") + name);  // for when working directory in Debug is $(ProjectDir)
  paths.push_back(std::string("../../" PROJECT_NAME "/GLSL/") + name);          // for when using $(TargetDir)
  paths.push_back(std::string("../../shipped/" PROJECT_NAME "/GLSL/") + name);  // for when using $(TargetDir)
  paths.push_back(std::string("SPV_" PROJECT_NAME "/") + name);
  //paths.push_back(std::string(PROJECT_RELDIRECTORY) + name);
  //paths.push_back(std::string(PROJECT_ABSDIRECTORY) + name);
  for(size_t i = 0; i < paths.size(); i++)
  {
    if((fd = fopen(paths[i].c_str(), "rb")) != NULL)
    {
      break;
    }
  }
  if(fd == NULL)
  {
    //LOGE("error in loading %s\n", name.c_str());
    return false;
  }
  fseek(fd, 0, SEEK_END);
  long  realsize = ftell(fd);
  char* p        = new char[realsize];
  fseek(fd, 0, SEEK_SET);
  fread(p, 1, realsize, fd);
  data = std::string(p, realsize);
  delete[] p;

  return true;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool RendererVk::initRenderpassDependent(int w, int h, int MSAA)
{
  bool bRes = true;
  if(m_scenePass)
    vkDestroyRenderPass(nvk.m_device, m_scenePass, NULL);
  m_scenePass = VK_NULL_HANDLE;
  if(m_pipelineMeshTri)
    vkDestroyPipeline(nvk.m_device, m_pipelineMeshTri, NULL);
  m_pipelineMeshTri = NULL;
  if(m_pipelineMeshTriStrip)
    vkDestroyPipeline(nvk.m_device, m_pipelineMeshTriStrip, NULL);
  m_pipelineMeshTriStrip = NULL;
  if(m_pipelineMeshTriFan)
    vkDestroyPipeline(nvk.m_device, m_pipelineMeshTriFan, NULL);
  m_pipelineMeshTriFan = NULL;
  if(m_pipelineMeshLine)
    vkDestroyPipeline(nvk.m_device, m_pipelineMeshLine, NULL);
  m_pipelineMeshLine = NULL;
  if(m_pipelineMeshLineStrip)
    vkDestroyPipeline(nvk.m_device, m_pipelineMeshLineStrip, NULL);
  m_pipelineMeshLineStrip = NULL;
  if(m_pipelineGrid)
    vkDestroyPipeline(nvk.m_device, m_pipelineGrid, NULL);
  m_pipelineGrid = NULL;

  //
  // Create the render passes
  //
  // example (commented in this case) where the VkSubpassDescription() would use pointers to these things
  //NVK::AttachmentReference color(0/*attachment*/, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL/*layout*/);
  //NVK::AttachmentReference dst(1/*attachment*/, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL/*layout*/);
  //NVK::AttachmentReference colorResolved(2/*attachment*/, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL/*layout*/);
  // later below, you'd reference them by pointer instead of using constructors:
  //NVK::SubpassDescription
  //(   VK_PIPELINE_BIND_POINT_GRAPHICS,//pipelineBindPoint
  //    NULL,                           //inputAttachments
  //    &color,                         //colorAttachments
  //    &colorResolved,                 //resolveAttachments
  //    &dst,                           //depthStencilAttachment
  //    NULL,                            //preserveAttachments
  //    0)
  NVK::RenderPassCreateInfo rpinfo = NVK::RenderPassCreateInfo(
      NVK::AttachmentDescription(VK_FORMAT_R8G8B8A8_UNORM, (VkSampleCountFlagBits)MSAA,      //format, samples
                                 VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,  //loadOp, storeOp
                                 VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,  //stencilLoadOp, stencilStoreOp
                                 VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL  //initialLayout, finalLayout
                                 )(VK_FORMAT_D24_UNORM_S8_UINT, (VkSampleCountFlagBits)MSAA, VK_ATTACHMENT_LOAD_OP_CLEAR,
                                   VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
                                   VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)(
          VK_FORMAT_R8G8B8A8_UNORM, (VkSampleCountFlagBits)1,                  //format, samples
          VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,           //loadOp, storeOp
          VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,   //stencilLoadOp, stencilStoreOp
          VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL  //initialLayout, finalLayout
          ),
      // Easy way
      NVK::SubpassDescription(
          VK_PIPELINE_BIND_POINT_GRAPHICS,  //pipelineBindPoint
          NVK::AttachmentReference(),       //inputAttachments
          NVK::AttachmentReference(0 /*attachment*/, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL /*layout*/),  //colorAttachments
          NVK::AttachmentReference(2 /*attachment*/, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL /*layout*/),  //resolveAttachments
          NVK::AttachmentReference(1 /*attachment*/, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL /*layout*/),  //depthStencilAttachment
          NVK::Uint32Array(),  //preserveAttachments
          0                    //flags
          ),

      //NVK::SubpassDescription
      //(   VK_PIPELINE_BIND_POINT_GRAPHICS,//pipelineBindPoint
      //    NULL,                           //inputAttachments
      //    &color,                         //colorAttachments
      //    &colorResolved,                 //resolveAttachments
      //    &dst,                           //depthStencilAttachment
      //    NULL,                           //preserveAttachments
      //    0                               //flags
      //),
      NVK::SubpassDependency(/*NONE*/));
  m_scenePass = nvk.createRenderPass(rpinfo);

  //--------------------------------------------------------------------------
  // Load SpirV shaders
  //
  std::string spv_GLSL_mesh_lines_frag;
  std::string spv_GLSL_mesh_frag;
  std::string spv_GLSL_mesh_vert;
  std::string spv_GLSL_mesh_lines_vert;
  std::string spv_GLSL_grid_frag;
  std::string spv_GLSL_grid_vert;
  if(!load_binary(std::string("GLSL_mesh_lines_frag.spv"), spv_GLSL_mesh_lines_frag))
    bRes = false;
  if(!load_binary(std::string("GLSL_mesh_frag.spv"), spv_GLSL_mesh_frag))
    bRes = false;
  if(!load_binary(std::string("GLSL_mesh_vert.spv"), spv_GLSL_mesh_vert))
    bRes = false;
  if(!load_binary(std::string("GLSL_mesh_lines_vert.spv"), spv_GLSL_mesh_lines_vert))
    bRes = false;
  if(!load_binary(std::string("GLSL_grid_frag.spv"), spv_GLSL_grid_frag))
    bRes = false;
  if(!load_binary(std::string("GLSL_grid_vert.spv"), spv_GLSL_grid_vert))
    bRes = false;
  if(bRes == false)
  {
    LOGE("Failed loading some SPV files\n");
    m_bValid = false;
    return false;
  }


  //--------------------------------------------------------------------------
  // Init 'pipelines'
  //
  //
  // what is needed to tell which states are dynamic
  //
  NVK::PipelineDynamicStateCreateInfo dynamicStateCreateInfo(NVK::DynamicState(VK_DYNAMIC_STATE_VIEWPORT)(
      VK_DYNAMIC_STATE_SCISSOR)(VK_DYNAMIC_STATE_LINE_WIDTH)(VK_DYNAMIC_STATE_DEPTH_BIAS));

  NVK::PipelineViewportStateCreateInfo vkPipelineViewportStateCreateInfo(
      NVK::Viewport(0.0f, 0.0f, (float)w, (float)h, 0.0f, 1.0f), NVK::Rect2DArray(0.0f, 0.0f, (float)w, (float)h));
  NVK::PipelineRasterizationStateCreateInfo vkPipelineRasterStateCreateInfo(VK_FALSE,  //depthClampEnable
                                                                            VK_FALSE,  //rasterizerDiscardEnable
                                                                            VK_POLYGON_MODE_FILL,             //fillMode
                                                                            VK_CULL_MODE_NONE,                //cullMode
                                                                            VK_FRONT_FACE_COUNTER_CLOCKWISE,  //frontFace
                                                                            VK_TRUE,  //depthBiasEnable
                                                                            0.0,      //depthBias
                                                                            0.0,      //depthBiasClamp
                                                                            0.0,      //slopeScaledDepthBias
                                                                            1.0       //lineWidth
  );
  NVK::PipelineColorBlendStateCreateInfo    vkPipelineColorBlendStateCreateInfo(
      VK_FALSE /*logicOpEnable*/, VK_LOGIC_OP_NO_OP,
      NVK::PipelineColorBlendAttachmentState(VK_FALSE /*blendEnable*/, VK_BLEND_FACTOR_ZERO /*srcBlendColor*/,
                                             VK_BLEND_FACTOR_ZERO /*destBlendColor*/, VK_BLEND_OP_ADD /*blendOpColor*/,
                                             VK_BLEND_FACTOR_ZERO /*srcBlendAlpha*/,
                                             VK_BLEND_FACTOR_ZERO /*destBlendAlpha*/, VK_BLEND_OP_ADD /*blendOpAlpha*/,
                                             VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
                                                 | VK_COLOR_COMPONENT_A_BIT /*colorWriteMask*/),
      NVK::Float4()  //blendConst[4]
  );
  NVK::PipelineDepthStencilStateCreateInfo vkPipelineDepthStencilStateCreateInfo(VK_TRUE,  //depthTestEnable
                                                                                 VK_TRUE,  //depthWriteEnable
                                                                                 VK_COMPARE_OP_LESS_OR_EQUAL,  //depthCompareOp
                                                                                 VK_FALSE,  //depthBoundsTestEnable
                                                                                 VK_FALSE,  //stencilTestEnable
                                                                                 NVK::StencilOpState(), NVK::StencilOpState(),  //front, back
                                                                                 0.0f, 1.0f  //minDepthBounds, maxDepthBounds
  );
  ::VkSampleMask                           sampleMask = 0xFFFF;
  NVK::PipelineMultisampleStateCreateInfo  vkPipelineMultisampleStateCreateInfo(
      (VkSampleCountFlagBits)MSAA /*rasterSamples*/, VK_FALSE /*sampleShadingEnable*/, 1.0 /*minSampleShading*/,
      &sampleMask /*sampleMask*/, VK_FALSE, VK_FALSE);

  //
  // GRID gfx pipeline
  //
  m_pipelineGrid = nvk.createGraphicsPipeline(NVK::GraphicsPipelineCreateInfo(m_pipelineLayout, m_scenePass)(NVK::PipelineVertexInputStateCreateInfo(
      NVK::VertexInputBindingDescription(0 /*binding*/, sizeof(vec3f) /*stride*/, VK_VERTEX_INPUT_RATE_VERTEX),
      NVK::VertexInputAttributeDescription(0 /*location*/, 0 /*binding*/, VK_FORMAT_R32G32B32_SFLOAT, 0)  // pos
      ))(NVK::PipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_LINE_LIST, VK_FALSE))(
      NVK::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT,
                                         nvk.createShaderModule(spv_GLSL_grid_vert.c_str(), spv_GLSL_grid_vert.size()),
                                         "main"))(vkPipelineViewportStateCreateInfo)(vkPipelineRasterStateCreateInfo)(
      vkPipelineMultisampleStateCreateInfo)(NVK::PipelineShaderStageCreateInfo(
      VK_SHADER_STAGE_FRAGMENT_BIT, nvk.createShaderModule(spv_GLSL_grid_frag.c_str(), spv_GLSL_grid_frag.size()),
      "main"))(vkPipelineColorBlendStateCreateInfo)(vkPipelineDepthStencilStateCreateInfo)(dynamicStateCreateInfo));
  //
  // MESH gfx pipelines
  // NVkVertexInputBindingDescription corresponds to a buffer containing some interleaved attributes. Many can be specified
  // NVkVertexInputAttributeDescription describes attributes and how they correlate to shader location and NVkVertexInputBindingDescription (binding)
  //
  NVK::PipelineShaderStageCreateInfo vkPipelineShaderStageCreateInfoVtx(
      VK_SHADER_STAGE_VERTEX_BIT, nvk.createShaderModule(spv_GLSL_mesh_vert.c_str(), spv_GLSL_mesh_vert.size()), "main");
  NVK::PipelineShaderStageCreateInfo vkPipelineShaderStageCreateInfoFrag(
      VK_SHADER_STAGE_FRAGMENT_BIT, nvk.createShaderModule(spv_GLSL_mesh_frag.c_str(), spv_GLSL_mesh_frag.size()),
      "main");
  NVK::PipelineShaderStageCreateInfo vkPipelineShaderStageCreateInfoVtxLine(
      VK_SHADER_STAGE_VERTEX_BIT,
      nvk.createShaderModule(spv_GLSL_mesh_lines_vert.c_str(), spv_GLSL_mesh_lines_vert.size()), "main");
  NVK::PipelineShaderStageCreateInfo vkPipelineShaderStageCreateInfoFragLine(
      VK_SHADER_STAGE_FRAGMENT_BIT,
      nvk.createShaderModule(spv_GLSL_mesh_lines_frag.c_str(), spv_GLSL_mesh_lines_frag.size()), "main");
  NVK::PipelineVertexInputStateCreateInfo vkPipelineVertexInputStateCreateInfo(
      NVK::VertexInputBindingDescription(0 /*binding*/, 2 * sizeof(vec3f) /*stride*/, VK_VERTEX_INPUT_RATE_VERTEX),
      NVK::VertexInputAttributeDescription(0 /*location*/, 0 /*binding*/, VK_FORMAT_R32G32B32_SFLOAT, 0 /*offset*/)  // pos
      (1 /*location*/, 0 /*binding*/, VK_FORMAT_R32G32B32_SFLOAT, sizeof(vec3f) /*offset*/)  // normal
  );
  NVK::PipelineVertexInputStateCreateInfo vkPipelineVertexInputStateCreateInfoLine(
      NVK::VertexInputBindingDescription(0 /*binding*/, sizeof(vec3f) /*stride*/, VK_VERTEX_INPUT_RATE_VERTEX),
      NVK::VertexInputAttributeDescription(0 /*location*/, 0 /*binding*/, VK_FORMAT_R32G32B32_SFLOAT, 0 /*offset*/)  // pos
  );


  m_pipelineMeshTri = nvk.createGraphicsPipeline(NVK::GraphicsPipelineCreateInfo(m_pipelineLayout, m_scenePass)(
      vkPipelineVertexInputStateCreateInfo)(NVK::PipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE))(
      NVK::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT,
                                         nvk.createShaderModule(spv_GLSL_mesh_vert.c_str(), spv_GLSL_mesh_vert.size()),
                                         "main"))(vkPipelineViewportStateCreateInfo)(vkPipelineRasterStateCreateInfo)(
      vkPipelineMultisampleStateCreateInfo)(NVK::PipelineShaderStageCreateInfo(
      VK_SHADER_STAGE_FRAGMENT_BIT, nvk.createShaderModule(spv_GLSL_mesh_frag.c_str(), spv_GLSL_mesh_frag.size()),
      "main"))(vkPipelineColorBlendStateCreateInfo)(vkPipelineDepthStencilStateCreateInfo)(dynamicStateCreateInfo));
  //
  // create Mesh pipelines
  //
  m_pipelineMeshTriStrip = nvk.createGraphicsPipeline(NVK::GraphicsPipelineCreateInfo(m_pipelineLayout, m_scenePass)(
      vkPipelineVertexInputStateCreateInfo)(NVK::PipelineInputAssemblyStateCreateInfo(
      VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, VK_TRUE))(vkPipelineShaderStageCreateInfoVtx)(vkPipelineViewportStateCreateInfo)(
      vkPipelineRasterStateCreateInfo)(vkPipelineMultisampleStateCreateInfo)(vkPipelineShaderStageCreateInfoFrag)(
      vkPipelineColorBlendStateCreateInfo)(vkPipelineDepthStencilStateCreateInfo)(dynamicStateCreateInfo));
  //
  // create Mesh pipelines
  //
  m_pipelineMeshTriFan = nvk.createGraphicsPipeline(NVK::GraphicsPipelineCreateInfo(m_pipelineLayout, m_scenePass)(
      vkPipelineVertexInputStateCreateInfoLine)(NVK::PipelineInputAssemblyStateCreateInfo(
      VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN, VK_TRUE))(vkPipelineShaderStageCreateInfoVtxLine)(vkPipelineViewportStateCreateInfo)(
      vkPipelineRasterStateCreateInfo)(vkPipelineMultisampleStateCreateInfo)(vkPipelineShaderStageCreateInfoFragLine)(
      vkPipelineColorBlendStateCreateInfo)(vkPipelineDepthStencilStateCreateInfo)(dynamicStateCreateInfo));
  //
  // create Mesh pipelines
  //
  m_pipelineMeshLine = nvk.createGraphicsPipeline(NVK::GraphicsPipelineCreateInfo(m_pipelineLayout, m_scenePass)(
      vkPipelineVertexInputStateCreateInfoLine)(NVK::PipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_LINE_LIST, VK_FALSE))(
      vkPipelineShaderStageCreateInfoVtxLine)(vkPipelineViewportStateCreateInfo)(vkPipelineRasterStateCreateInfo)(
      vkPipelineMultisampleStateCreateInfo)(vkPipelineShaderStageCreateInfoFragLine)(
      vkPipelineColorBlendStateCreateInfo)(vkPipelineDepthStencilStateCreateInfo)(dynamicStateCreateInfo));
  //
  // create Mesh pipelines
  //
  m_pipelineMeshLineStrip = nvk.createGraphicsPipeline(NVK::GraphicsPipelineCreateInfo(m_pipelineLayout, m_scenePass)(
      vkPipelineVertexInputStateCreateInfoLine)(NVK::PipelineInputAssemblyStateCreateInfo(
      VK_PRIMITIVE_TOPOLOGY_LINE_STRIP, VK_TRUE))(vkPipelineShaderStageCreateInfoVtxLine)(vkPipelineViewportStateCreateInfo)(
      vkPipelineRasterStateCreateInfo)(vkPipelineMultisampleStateCreateInfo)(vkPipelineShaderStageCreateInfoFragLine)(
      vkPipelineColorBlendStateCreateInfo)(vkPipelineDepthStencilStateCreateInfo)(dynamicStateCreateInfo));
  return bRes;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool RendererVk::initGraphics(int w, int h, int MSAA)
{
  bool bRes;
  if(m_bValid)
    return true;
  m_MSAA   = MSAA;
  m_bValid = true;
  //--------------------------------------------------------------------------
  // Create the Vulkan device
  //
  bRes = nvk.utInitialize(false /*use or not validation layer*/);
  assert(bRes);
  //--------------------------------------------------------------------------
  // extension for merging VULKAN with OpenGL
  //
  VkSemaphoreCreateInfo semCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
  m_semOpenGLReadDone                 = nvk.createSemaphore();
  // Signal Semaphore by default to avoid being stuck
  glSignalVkSemaphoreNV((GLuint64)m_semOpenGLReadDone);
  m_semVKRenderingDone = nvk.createSemaphore();
  //--------------------------------------------------------------------------
  // Command pool for the main thread
  //
  initThreadLocalVars(0);
  //--------------------------------------------------------------------------
  // initialize the Fences for command pools
  //
  for(int i = 0; i < CMDPOOL_BUFFER_SZ; i++)
  {
    m_cmdPoolFence[i] = nvk.createFence(VK_FENCE_CREATE_SIGNALED_BIT);
  }

  //--------------------------------------------------------------------------
  m_profilerVK = nvvk::ProfilerVK(&g_profiler);
  m_profilerVK.init(nvk.m_device, nvk.m_gpu.device);

  //
  // Create a sampler
  //
  m_sampler = nvk.createSampler(NVK::SamplerCreateInfo(VK_FILTER_LINEAR,                         //magFilter
                                                       VK_FILTER_LINEAR,                         //minFilter
                                                       VK_SAMPLER_MIPMAP_MODE_LINEAR,            //mipMode
                                                       VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,  //addressU
                                                       VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,  //addressV
                                                       VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,  //addressW
                                                       0.0,                                      //mipLodBias
                                                       VK_FALSE,                                 // aniso enable
                                                       1,                                        //maxAnisotropy
                                                       VK_FALSE,                                 //compareEnable
                                                       VK_COMPARE_OP_NEVER,                      //compareOp
                                                       0.0,                                      //minLod
                                                       16.0,                                     //maxLod
                                                       VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,       //borderColor
                                                       VK_FALSE));
  //--------------------------------------------------------------------------
  // Texture
  //
  nv_dds::CDDSImage image;
  ImgO              noiseTex3D;

  std::vector<std::string> paths;
  std::string              name("GLSL/noise64x64_RGB.dds");
  paths.push_back(name);
  paths.push_back(NVPSystem::exePath() + std::string(PROJECT_RELDIRECTORY) + name);
  //paths.push_back(std::string(PROJECT_ABSDIRECTORY) + name);
  for(int i = 0; i < paths.size(); i++)
  {
    if(image.load(paths[i].c_str()))
    {
      break;
    }
  }
  if(!image.is_valid())
  {
    assert(!"error in loading texture");
  }
  VkFormat fmt;
  //DXT1_RGB                VK_FORMAT_BC1_RGB_UNORM_BLOCK
  //SRGB_DXT1_RGB           VK_FORMAT_BC1_RGB_SRGB_BLOCK
  //DXT1_RGBA               VK_FORMAT_BC1_RGBA_UNORM_BLOCK
  //SRGB_DXT1_RGBA          VK_FORMAT_BC1_RGBA_SRGB_BLOCK
  //DXT3                    VK_FORMAT_BC2_UNORM_BLOCK
  //SRGB_DXT3               VK_FORMAT_BC2_SRGB_BLOCK
  //DXT5                    VK_FORMAT_BC3_UNORM_BLOCK
  //SRGB_DXT5               VK_FORMAT_BC3_SRGB_BLOCK
  switch(image.get_internal_format())
  {
    case RED:
      fmt = VK_FORMAT_R8_UNORM;
      break;
    case RG8:
      fmt = VK_FORMAT_R8G8_UNORM;
      break;
    case RGBA8:
      fmt = VK_FORMAT_R8G8B8A8_UNORM;
      break;
    case COMPRESSED_RGBA_S3TC_DXT1_EXT:
      fmt = VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
      break;
    case COMPRESSED_RGBA_S3TC_DXT3_EXT:
      fmt = VK_FORMAT_BC2_UNORM_BLOCK;
      break;
    case COMPRESSED_RGBA_S3TC_DXT5_EXT:
      fmt = VK_FORMAT_BC3_UNORM_BLOCK;
      break;
      assert(!"TODO");
  }
  noiseTex3D.img = nvk.utCreateImage3D(image.get_width(), image.get_height(), image.get_depth(), noiseTex3D.imgMem, fmt,
                                       VK_SAMPLE_COUNT_1_BIT, VK_SAMPLE_COUNT_1_BIT, image.get_num_mipmaps() + 1);
  noiseTex3D.imgView = nvk.createImageView(NVK::ImageViewCreateInfo(noiseTex3D.img,               // image
                                                                    VK_IMAGE_VIEW_TYPE_3D,        //viewType
                                                                    fmt,                          //format
                                                                    NVK::ComponentMapping(),      //channels
                                                                    NVK::ImageSubresourceRange()  //subresourceRange
                                                                    ));
  //
  // Perform the copy of the image and possible mipmaps
  // TODO: check mipmap upload !
  //
  NVK::BufferImageCopy imageCopy;
  unsigned char*       aggregatedMipmaps    = NULL;
  size_t               aggregatedMipmaps_Sz = 0;
  for(int i = 0; i < image.get_num_mipmaps() + 1; i++)
  {
    const nv_dds::CSurface& imageSurface = image.get_mipmap(i);
    aggregatedMipmaps_Sz += imageSurface.get_size();
  }
  aggregatedMipmaps    = new unsigned char[aggregatedMipmaps_Sz];
  aggregatedMipmaps_Sz = 0;
  for(int i = 0; i < image.get_num_mipmaps() + 1; i++)
  {
    const nv_dds::CSurface& imageSurface = image.get_mipmap(i);
    memcpy(aggregatedMipmaps + aggregatedMipmaps_Sz, (unsigned char*)imageSurface, imageSurface.get_size());
    imageCopy.add(aggregatedMipmaps_Sz,       //bufferOffset
                  0,                          //bufferRowLength
                  imageSurface.get_height(),  //bufferImageHeight,
                  NVK::ImageSubresourceLayers(VK_IMAGE_ASPECT_COLOR_BIT, i /*mipLevel*/, 0 /*baseArrayLayer*/, 1 /*layerCount*/),  //imageSubresource
                  NVK::Offset3D(0, 0, 0),
                  NVK::Extent3D(imageSurface.get_width(), imageSurface.get_height(), imageSurface.get_depth()));
    aggregatedMipmaps_Sz += imageSurface.get_size();
  }
  nvk.utFillImage(&m_perThreadData->m_cmdPoolStatic, imageCopy, aggregatedMipmaps, aggregatedMipmaps_Sz, noiseTex3D.img);
  delete[] aggregatedMipmaps;

  NVK::DescriptorImageInfo noiseTextureSamplerAndView =
      NVK::DescriptorImageInfo(m_sampler, noiseTex3D.imgView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  //--------------------------------------------------------------------------
  // Buffers for general UBOs
  //
  m_matrix.Sz = sizeof(vec4f) * 4 * 2;
  m_matrix.buffer = nvk.utCreateAndFillBuffer(&m_perThreadData->m_cmdPoolStatic, m_matrix.Sz, NULL, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                              m_matrix.bufferMem, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  //--------------------------------------------------------------------------
  // descriptor set LAYOUTS
  //
  // descriptor layout for general things (projection matrix; view matrix...)
  m_descriptorSetLayouts[DSET_GLOBAL] = nvk.createDescriptorSetLayout(NVK::DescriptorSetLayoutCreateInfo(
      NVK::DescriptorSetLayoutBinding(BINDING_MATRIX, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT)  // BINDING_MATRIX
      //(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT) // BINDING_LIGHT
      (BINDING_NOISE, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)));
  // descriptor layout for object level: buffers related to the object (objec-matrix; material colors...)
  // This part will use the offsets to adjust buffer data
  m_descriptorSetLayouts[DSET_OBJECT] = nvk.createDescriptorSetLayout(NVK::DescriptorSetLayoutCreateInfo(
      NVK::DescriptorSetLayoutBinding(BINDING_MATRIXOBJ, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_VERTEX_BIT)  // BINDING_MATRIXOBJ
      (BINDING_MATERIAL, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_FRAGMENT_BIT)  // BINDING_MATERIAL
      ));
  //
  // PipelineLayout
  //
  m_pipelineLayout = nvk.createPipelineLayout(m_descriptorSetLayouts, DSET_TOTALAMOUNT);
  //
  // Renderpass creation
  //
  initRenderpassDependent(w, h, MSAA);
  //
  // Framebuffer Initialization
  //
  m_viewRect = NVK::Rect2D(NVK::Offset2D(0, 0), NVK::Extent2D(w, h));
  initFramebuffer(w, h, MSAA);

  initResourcesGrid();

  //
  // Descriptor Pool: size is 4 to have enough for global; object and ...
  // TODO: try other VkDescriptorType
  //
  m_descPool = nvk.createDescriptorPool(NVK::DescriptorPoolCreateInfo(
      3, NVK::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3)(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
                                                                       3)(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3)));
  //
  // DescriptorSet allocation
  // Here we allocate only the global descriptor set
  // Objects will do their own allocation later
  //
  nvk.allocateDescriptorSets(NVK::DescriptorSetAllocateInfo(m_descPool, 1, m_descriptorSetLayouts + DSET_GLOBAL), &m_descriptorSetGlobal);
  //
  // update the descriptorset used for Global
  // later we will update the ones local to objects
  //
  NVK::DescriptorBufferInfo descBuffer = NVK::DescriptorBufferInfo(m_matrix.buffer, 0, m_matrix.Sz);

  nvk.updateDescriptorSets(NVK::WriteDescriptorSet(m_descriptorSetGlobal, BINDING_MATRIX, 0, descBuffer, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)(
      m_descriptorSetGlobal, BINDING_NOISE, 0, noiseTextureSamplerAndView, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER));

  return true;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool RendererVk::initResourcesGrid()
{
  //
  // Make a simple grid
  //
  vec3* data = new vec3[GRIDDEF * 4];
  vec3* p    = data;
  int   j    = 0;
  for(int i = 0; i < GRIDDEF; i++)
  {
    *(p++) = vec3(-GRIDSZ, 0.0, GRIDSZ * (-1.0f + 2.0f * (float)i / (float)GRIDDEF));
    *(p++) = vec3(GRIDSZ * (1.0f - 2.0f / (float)GRIDDEF), 0.0, GRIDSZ * (-1.0f + 2.0f * (float)i / (float)GRIDDEF));
    *(p++) = vec3(GRIDSZ * (-1.0f + 2.0f * (float)i / (float)GRIDDEF), 0.0, -GRIDSZ);
    *(p++) = vec3(GRIDSZ * (-1.0f + 2.0f * (float)i / (float)GRIDDEF), 0.0, GRIDSZ * (1.0f - 2.0f / (float)GRIDDEF));
  }
  int vboSz = sizeof(vec3) * GRIDDEF * 4;
  //
  // Create the buffer with these data
  //
  m_gridBuffer.buffer = nvk.utCreateAndFillBuffer(&m_perThreadData->m_cmdPoolStatic, vboSz, data, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                                  m_gridBuffer.bufferMem, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  return true;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool RendererVk::releaseResourcesGrid()
{
  if(m_bValid == false)
    return false;
  return true;
}
//------------------------------------------------------------------------------
// build general command buffers
// 1- a command-buffer for global-uniforms: for camera, view...
// 2- a command buffer for the grid
// 3- the primary command buffer that will call them and call the ones from the 3D models
//------------------------------------------------------------------------------
bool RendererVk::buildPrimaryCmdBuffer()
{
  NXPROFILEFUNC(__FUNCTION__);
  if(m_bValid == false)
    return false;
  VkRenderPass  renderPass  = m_scenePass;
  VkFramebuffer framebuffer = m_framebuffer;
  NVK::Rect2D   viewRect    = m_viewRect;
  float         lineWidth   = 1.0;
  float         width       = m_viewRect.extent.width;
  float         height      = m_viewRect.extent.height;
  //
  // CMD-BUFFER to Synchronize and setup viewport
  //
  if(m_cmdSyncAndViewport)
    m_perThreadData->m_cmdPoolStatic.freeCommandBuffer(m_cmdSyncAndViewport);
  m_cmdSyncAndViewport = m_perThreadData->m_cmdPoolStatic.allocateCommandBuffer(false);
  m_cmdSyncAndViewport.beginCommandBuffer(false, true,
                                          NVK::CommandBufferInheritanceInfo(renderPass, 0, framebuffer, 0 /*occlusionQueryEnable*/,
                                                                            0 /*queryFlags*/, 0 /*pipelineStatistics*/));
  {
    //
    // Prepare the depth+stencil for reading.
    // static because structs get declared once, then...
    //
    /*NVK::ImageMemoryBarrier imageMemoryBarriers = NVK::ImageMemoryBarrier
            (
                VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,        //srcAccessMask 
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,       //dstAccessMask 
                VK_IMAGE_LAYOUT_GENERAL,                    //oldLayout
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,   //newLayout
                0,0,//srcQueueFamilyIndex, dstQueueFamilyIndex
                m_colorImageMS.img,                           //image
                NVK::ImageSubresourceRange                //subresourceRange
                    ( VK_IMAGE_ASPECT_COLOR_BIT,//aspect
                    0,                      //baseMipLevel
                    (~0),                   //mipLevels
                    0,                      //baseArraySlice
                    1                       //arraySize
            ) )
            (
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                VK_IMAGE_LAYOUT_GENERAL,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                0,0,//srcQueueFamilyIndex, dstQueueFamilyIndex
                m_DSTImageMS.img, 
                NVK::ImageSubresourceRange(VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, (~0), 0, 1) )
            (
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                VK_IMAGE_LAYOUT_GENERAL,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                0,0,//srcQueueFamilyIndex, dstQueueFamilyIndex
                m_DSTImageMS.img, 
                NVK::ImageSubresourceRange(VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, (~0), 0, 1)
            );
        m_cmdSyncAndViewport.cmdPipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_FALSE, 0, NULL, 0, NULL, imageMemoryBarriers.size(), imageMemoryBarriers);*/
    m_cmdSyncAndViewport.cmdSetViewport(0, 1, NVK::Viewport(0.0, 0.0, width, height, 0.0f, 1.0f));
    m_cmdSyncAndViewport.cmdSetScissor(0, 1, NVK::Rect2D(0.0, 0.0, width, height));
  }
  m_cmdSyncAndViewport.endCommandBuffer();
  //
  // CMD-BUFFER for the grid
  //
  if(m_cmdBufferGrid)
    m_perThreadData->m_cmdPoolStatic.freeCommandBuffer(m_cmdBufferGrid);
  m_cmdBufferGrid = m_perThreadData->m_cmdPoolStatic.allocateCommandBuffer(false);

  m_cmdBufferGrid.beginCommandBuffer(false, true,
                                     NVK::CommandBufferInheritanceInfo(renderPass, 0, framebuffer, 0 /*occlusionQueryEnable*/,
                                                                       0 /*queryFlags*/, 0 /*pipelineStatistics*/));
  {
    m_cmdBufferGrid.cmdBindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineGrid);
    m_cmdBufferGrid.cmdSetDepthBias(0.0f, 0.0f, 0.0f);  // offset raster
    m_cmdBufferGrid.cmdSetLineWidth(lineWidth);
    m_cmdBufferGrid.cmdSetViewport(0, 1, NVK::Viewport(0.0, 0.0, width, height, 0.0f, 1.0f));
    m_cmdBufferGrid.cmdSetScissor(0, 1, NVK::Rect2D(0.0, 0.0, width, height));
    VkDeviceSize vboffsets[1] = {0};
    vkCmdBindVertexBuffers(m_cmdBufferGrid, 0, 1, &m_gridBuffer.buffer, vboffsets);
    //vkCmdBindIndexBuffer  (m_cmdBufferGrid, ibo, iboOffset, VK_INDEX_TYPE_UINT32);
    //
    // bind the descriptor set for global stuff
    //
    vkCmdBindDescriptorSets(m_cmdBufferGrid, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, DSET_GLOBAL, 1,
                            &m_descriptorSetGlobal, 0, NULL);

    vkCmdDraw(m_cmdBufferGrid, GRIDDEF * 4, 1, 0, 0);
  }
  vkEndCommandBuffer(m_cmdBufferGrid);
  return true;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void RendererVk::displayStart(const mat4f& world, const InertiaCamera& camera, const mat4f& projection, bool bTimingGlitch)
{
  if(m_bValid == false)
    return;
  //NXPROFILEFUNC(__FUNCTION__);
  //
  // Update general params for all sub-sequent operations IN CMD BUFFER #1
  //
  g_globalMatrices.mW     = world;
  g_globalMatrices.mVP    = projection * camera.m4_view;
  g_globalMatrices.eyePos = camera.curEyePos;
  if((m_scenePass == NULL) || (m_framebuffer == NULL))
    return;
  float         w           = m_viewRect.extent.width;
  float         h           = m_viewRect.extent.height;
  VkRenderPass  renderPass  = m_scenePass;
  VkFramebuffer framebuffer = m_framebuffer;
  NVK::Rect2D   viewRect    = m_viewRect;
  float         lineWidth   = 1.0f;
  //
  // Create the primary command buffer
  //
  m_cmdScene = m_perThreadData->m_curCmdPoolDynamic->allocateCommandBuffer(true);

  m_cmdScene.beginCommandBuffer(false, true, NVK::CommandBufferInheritanceInfo(renderPass, 0, framebuffer, VK_FALSE, 0, 0));

  m_slot = m_profilerVK.beginSection("scene", m_cmdScene.m_cmdbuffer);

  m_cmdScene.cmdExecuteCommands(1, m_cmdSyncAndViewport);
  m_cmdScene.cmdUpdateBuffer(m_matrix.buffer, 0, sizeof(g_globalMatrices), (uint32_t*)&g_globalMatrices);
  float r = 0.0f; //bTimingGlitch ? 1.0f : 0.0f;
  m_cmdScene.cmdBeginRenderPass(
      NVK::RenderPassBeginInfo(renderPass, framebuffer, viewRect,
                               NVK::ClearValue(NVK::ClearColorValue(r, 0.1f, 0.15f, 1.0f))(
                                   NVK::ClearDepthStencilValue(1.0, 0))(NVK::ClearColorValue(0.0f, 0.1f, 0.15f, 1.0f))),
      VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void RendererVk::displayEnd()
{
  if(m_bValid == false)
    return;
  //NXPROFILEFUNC(__FUNCTION__);
  {
    m_cmdScene.cmdEndRenderPass();
    // TODO: Create a MACRO
    m_profilerVK.endSection(m_slot, m_cmdScene.m_cmdbuffer);
    m_cmdScene.endCommandBuffer();
  }
  {
    const VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    nvk.queueSubmit(NVK::SubmitInfo(1, &m_semOpenGLReadDone, &waitStages, 1, m_cmdScene, 1, &m_semVKRenderingDone),
                    m_cmdPoolFence[m_frameCounter]);
  }
  // allows us to loop through the next available pool for next frame
  m_frameCounter++;
  m_frameCounter2++;
  if(m_frameCounter >= CMDPOOL_BUFFER_SZ)
    m_frameCounter = 0;  //(m_frameCounter + 1)%CMDPOOL_BUFFER_SZ;
  //vkQueueWaitIdle(nvk.m_queue);
}
//------------------------------------------------------------------------------
//
// blit to gl backbuffer
//
//------------------------------------------------------------------------------
void RendererVk::blitToBackbuffer()
{
  NXPROFILEFUNC(__FUNCTION__);
  if(m_bValid == false)
    return;
  float w = m_viewRect.extent.width;
  float h = m_viewRect.extent.height;
  // NO Depth test
  glDisable(GL_DEPTH_TEST);
  //
  // Wait for the queue of Our VK rendering to signal m_semVKRenderingDone so we know the image is ready
  //
  glWaitVkSemaphoreNV((GLuint64)m_semVKRenderingDone);
  //
  // Blit the image
  //
  glDrawVkImageNV((GLuint64)m_colorImage.img, 0, 0, 0, w, h, 0, 0, 1, 1, 0);
  //
  // Signal m_semOpenGLReadDone to tell the VK rendering queue that it can render the next one
  //
  glSignalVkSemaphoreNV((GLuint64)m_semOpenGLReadDone);
  //
  // Depth test back to ON (assuming we needed to put it back)
  //
  glEnable(GL_DEPTH_TEST);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void RendererVk::displayGrid(const InertiaCamera& camera, const mat4f projection)
{
  if(m_bValid == false)
    return;

  const nvvk::ProfilerVK::Section profile(m_profilerVK, "diplay.Grid", m_cmdScene);
  m_cmdScene.cmdExecuteCommands(1, m_cmdBufferGrid);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void RendererVk::updateViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
  if(m_bValid == false)
    return;
  m_viewRect = NVK::Rect2D(NVK::Offset2D(0, 0), NVK::Extent2D(width, height));
  initFramebuffer(width, height, m_MSAA);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void RendererVk::releaseFramebuffer()
{
  m_DSTImageMS.release();
  m_colorImageMS.release();
  m_colorImage.release();

  if(m_framebuffer)
    vkDestroyFramebuffer(nvk.m_device, m_framebuffer, NULL);
  m_framebuffer = NULL;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void RendererVk::initFramebuffer(GLsizei width, GLsizei height, int MSAA)
{
  if(m_framebuffer)
  {
    releaseFramebuffer();
  }
  //
  // Images: resolve + MSAA
  //
  m_colorImage.img     = nvk.utCreateImage2D(width, height, m_colorImage.imgMem, VK_FORMAT_R8G8B8A8_UNORM);
  m_colorImage.imgView = nvk.createImageView(NVK::ImageViewCreateInfo(m_colorImage.img,             // image
                                                                      VK_IMAGE_VIEW_TYPE_2D,        //viewType
                                                                      VK_FORMAT_R8G8B8A8_UNORM,     //format
                                                                      NVK::ComponentMapping(),      //channels
                                                                      NVK::ImageSubresourceRange()  //subresourceRange
                                                                      ));

  m_colorImageMS.img =
      nvk.utCreateImage2D(width, height, m_colorImageMS.imgMem, VK_FORMAT_R8G8B8A8_UNORM, (VkSampleCountFlagBits)MSAA);
  m_colorImageMS.imgView = nvk.createImageView(NVK::ImageViewCreateInfo(m_colorImageMS.img,           // image
                                                                        VK_IMAGE_VIEW_TYPE_2D,        //viewType
                                                                        VK_FORMAT_R8G8B8A8_UNORM,     //format
                                                                        NVK::ComponentMapping(),      //channels
                                                                        NVK::ImageSubresourceRange()  //subresourceRange
                                                                        ));
  //
  // depth stencil
  //
  m_DSTImageMS.img =
      nvk.utCreateImage2D(width, height, m_DSTImageMS.imgMem, VK_FORMAT_D24_UNORM_S8_UINT, (VkSampleCountFlagBits)MSAA);
  m_DSTImageMS.imgView = nvk.createImageView(
      NVK::ImageViewCreateInfo(m_DSTImageMS.img,             // image
                               VK_IMAGE_VIEW_TYPE_2D,        //viewType
                               VK_FORMAT_D24_UNORM_S8_UINT,  //format
                               NVK::ComponentMapping(),      //channels
                               NVK::ImageSubresourceRange(VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)  //subresourceRange
                               ));
  //
  // create the framebuffer
  //
  m_framebuffer =
      nvk.createFramebuffer(NVK::FramebufferCreateInfo(m_scenePass,       //renderPass
                                                       width, height, 1,  //width, height, layers
                                                       (m_colorImageMS.imgView))(m_DSTImageMS.imgView)(m_colorImage.imgView));
}

//------------------------------------------------------------------------------
// reset the current commandbuffer Pool
// this will start with thread 0 : we will wait for a fence, to make sure we
// don't reset anything that is still in use by the GPU
// then other threads (dispatching workers) must also reset their pool
//------------------------------------------------------------------------------
void RendererVk::resetCommandBuffersPool()
{
  PerThreadData* p = m_perThreadData;
  // when thread #0, let's wait for the Fence to be ready prior to proceed
  if(p->m_threadId == 0)
  {
    VkFence curCmdPoolFence = m_cmdPoolFence[m_frameCounter];
    while(nvk.waitForFences(1, &curCmdPoolFence, VK_TRUE, 100000000) == false)
    {
    };
    nvk.resetFences(1, &curCmdPoolFence);
  }

  p->m_cmdPoolDynamic[m_frameCounter].resetCommandPool(VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
  p->m_curCmdPoolDynamic = &p->m_cmdPoolDynamic[m_frameCounter];
}

//------------------------------------------------------------------------------
// create what is needed for thread-local storage
//------------------------------------------------------------------------------
bool RendererVk::initThreadLocalVars(int threadId)
{
  VkResult result;
  if(!m_bValid)
    return false;
  //--------------------------------------------------------------------------
  // CMDPOOL_BUFFER_SZ Command pools per thread!
  //
  m_perThreadData                     = new PerThreadData;
  m_perThreadData->m_threadId         = threadId;
  VkCommandPoolCreateInfo cmdPoolInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  cmdPoolInfo.queueFamilyIndex        = 0;
  cmdPoolInfo.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  result                              = nvk.createCommandPool(&cmdPoolInfo, NULL, &m_perThreadData->m_cmdPoolStatic);
  for(int i = 0; i < CMDPOOL_BUFFER_SZ; i++)
  {
    result = nvk.createCommandPool(&cmdPoolInfo, NULL, &m_perThreadData->m_cmdPoolDynamic[i]);
  }
  m_perThreadData->m_curCmdPoolDynamic = &(m_perThreadData->m_cmdPoolDynamic[0]);
  return true;
}
//------------------------------------------------------------------------------
// create what is needed for thread-local storage
//------------------------------------------------------------------------------
void RendererVk::releaseThreadLocalVars()
{
  VkCommandPoolCreateInfo cmdPoolInfo  = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  cmdPoolInfo.queueFamilyIndex         = 0;
  m_perThreadData->m_curCmdPoolDynamic = NULL;
  for(int i = 0; i < CMDPOOL_BUFFER_SZ; i++)
  {
    NVK::CommandPool& cmdPoolDynamic = m_perThreadData->m_cmdPoolDynamic[i];
    if(cmdPoolDynamic)
    {
      cmdPoolDynamic.destroyCommandPool();
      cmdPoolDynamic = VK_NULL_HANDLE;
    }
  }
  NVK::CommandPool& cmdPoolStatic = m_perThreadData->m_cmdPoolStatic;
  if(cmdPoolStatic)
    cmdPoolStatic.destroyCommandPool();
  cmdPoolStatic = VK_NULL_HANDLE;
  if (m_perThreadData)
  {
    delete (PerThreadData*)m_perThreadData;
  }
  m_perThreadData = NULL;
}
//------------------------------------------------------------------------------
// release the command buffers
//------------------------------------------------------------------------------
void RendererVk::waitForGPUIdle()
{
  vkQueueWaitIdle(nvk.m_queue);  // need to wait: some command-buffers could be used by the GPU
}
//------------------------------------------------------------------------------
// release the command buffers
//------------------------------------------------------------------------------
void RendererVk::destroyCommandBuffers(bool bAll)
{
  NXPROFILEFUNC(__FUNCTION__);
  if(bAll)
  {
    PerThreadData* perThreadData = m_perThreadData;
    if(perThreadData->m_curCmdPoolDynamic[0])
      perThreadData->m_curCmdPoolDynamic->resetCommandPool(VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
  }
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool RendererVk::terminateGraphics()
{
  if(!m_bValid)
    return true;
  waitForGPUIdle();
  m_bValid = false;

  NVK::CommandPool& cmdPoolStatic = m_perThreadData->m_cmdPoolStatic;
  cmdPoolStatic.destroyCommandPool();  // destroys commands that are inside, obviously
  m_perThreadData->m_cmdPoolStatic = VK_NULL_HANDLE;
  m_cmdBufferGrid                  = VK_NULL_HANDLE;
  m_cmdSyncAndViewport             = VK_NULL_HANDLE;

  for(int i = 0; i < CMDPOOL_BUFFER_SZ; i++)
  {
    NVK::CommandPool& cmdPoolDynamic = m_perThreadData->m_cmdPoolDynamic[i];
    cmdPoolDynamic.destroyCommandPool();  // destroys commands that are inside, obviously
    cmdPoolDynamic = VK_NULL_HANDLE;
    nvk.destroyFence(m_cmdPoolFence[i]);
    m_cmdPoolFence[i] = VK_NULL_HANDLE;
  }
  m_perThreadData->m_curCmdPoolDynamic[0] = VK_NULL_HANDLE;

  for(int i = 0; i < DSET_TOTALAMOUNT; i++)
  {
    vkDestroyDescriptorSetLayout(nvk.m_device, m_descriptorSetLayouts[i], NULL);  // general layout and objects layout
    m_descriptorSetLayouts[i] = 0;
  }
  //vkFreeDescriptorSets(nvk.m_device, m_descPool, 1, &m_descriptorSetGlobal); // no really necessary: we will destroy the pool after that
  m_descriptorSetGlobal = NULL;

  vkDestroyDescriptorPool(nvk.m_device, m_descPool, NULL);
  m_descPool = NULL;

  vkDestroyPipelineLayout(nvk.m_device, m_pipelineLayout, NULL);
  m_pipelineLayout = NULL;

  releaseFramebuffer();

  m_gridBuffer.release();
  m_matrix.release();

  nvk.destroySampler(m_sampler);
  m_sampler = NULL;

  m_profilerVK.deinit();

  nvk.destroySemaphore(m_semOpenGLReadDone);
  nvk.destroySemaphore(m_semVKRenderingDone);
  m_semOpenGLReadDone  = NULL;
  m_semVKRenderingDone = NULL;

  if(m_scenePass)
    vkDestroyRenderPass(nvk.m_device, m_scenePass, NULL);
  m_scenePass = VK_NULL_HANDLE;
  if(m_pipelineMeshTri)
    vkDestroyPipeline(nvk.m_device, m_pipelineMeshTri, NULL);
  m_pipelineMeshTri = NULL;
  if(m_pipelineMeshTriStrip)
    vkDestroyPipeline(nvk.m_device, m_pipelineMeshTriStrip, NULL);
  m_pipelineMeshTriStrip = NULL;
  if(m_pipelineMeshTriFan)
    vkDestroyPipeline(nvk.m_device, m_pipelineMeshTriFan, NULL);
  m_pipelineMeshTriFan = NULL;
  if(m_pipelineMeshLine)
    vkDestroyPipeline(nvk.m_device, m_pipelineMeshLine, NULL);
  m_pipelineMeshLine = NULL;
  if(m_pipelineMeshLineStrip)
    vkDestroyPipeline(nvk.m_device, m_pipelineMeshLineStrip, NULL);
  m_pipelineMeshLineStrip = NULL;
  if(m_pipelineGrid)
    vkDestroyPipeline(nvk.m_device, m_pipelineGrid, NULL);
  m_pipelineGrid = NULL;

  nvk.utDestroy();

  m_bValid = false;
  return false;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool RendererVk::attachModel(Bk3dModel* pGenericModel)
{
  if(m_bValid == false)
    return false;
  Bk3dModelVk* pModel = new Bk3dModelVk(pGenericModel);
  m_pModels.push_back(pModel);  // keep track of allocated stuff
  return true;
}
bool RendererVk::detachModels()
{
  if(m_bValid == false)
    return false;
  bool b = true;
  for(int i = 0; i < m_pModels.size(); i++)
  {
    if(!m_pModels[i]->releaseResources(this))
      b = false;
    delete m_pModels[i];
  }
  m_pModels.clear();
  return b;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool RendererVk::initResourcesModel(Bk3dModel* pGenericModel)
{
  if(m_bValid == false)
    return false;
  return ((Bk3dModelVk*)pGenericModel->m_pRendererData)->initResources(this);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool RendererVk::releaseResourcesModel(Bk3dModel* pGenericModel)
{
  if(m_bValid == false)
    return false;
  return ((Bk3dModelVk*)pGenericModel->m_pRendererData)->releaseResources(this);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool RendererVk::buildCmdBufferModel(Bk3dModel* pGenericModel, int bufIdx, int mstart, int mend)
{
  if(m_bValid == false)
    return false;
  return ((Bk3dModelVk*)pGenericModel->m_pRendererData)->buildCmdBuffer(this, bufIdx, mstart, mend);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void RendererVk::consolidateCmdBuffersModel(Bk3dModel* pModel, int numCmdBuffers)
{
  if(m_bValid == false)
    return;
  ((Bk3dModelVk*)pModel->m_pRendererData)->consolidateCmdBuffers(numCmdBuffers);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool RendererVk::deleteCmdBufferModel(Bk3dModel* pGenericModel)
{
  if(m_bValid == false)
    return false;
  return true;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool RendererVk::updateForChangedRenderTarget(Bk3dModel* pModel)
{
  if(m_bValid == false)
    return false;
  return false;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void RendererVk::displayBk3dModel(Bk3dModel* pGenericModel, const mat4f& cameraView, const mat4f projection, unsigned char topologies)
{
  if(m_bValid == false)
    return;

  const nvvk::ProfilerVK::Section profile(m_profilerVK, "diplay.Grid", m_cmdScene);
  ((Bk3dModelVk*)pGenericModel->m_pRendererData)->displayObject(this, cameraView, projection, topologies);
}

//******************************************************************************
//******************************************************************************
//********* MODEL
//******************************************************************************
//******************************************************************************
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
Bk3dModelVk::Bk3dModelVk(Bk3dModel* pGenericModel)
{
  // keep track of the stuff related to this renderer
  pGenericModel->m_pRendererData = this;
  // keep track of the generic model data in the part for this renderer
  m_pGenericModel     = pGenericModel;
  m_numUsedCmdBuffers = 0;
  memset(m_cmdBuffer, 0, MAXCMDBUFFERS * sizeof(ModelCmdBuffers));
}

Bk3dModelVk::~Bk3dModelVk() {}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool Bk3dModelVk::releaseResources(Renderer* pRenderer)
{
  RendererVk* pRendererVk = static_cast<RendererVk*>(pRenderer);

  m_uboObjectMatrices.release();
  m_uboMaterial.release();
#ifdef USE_VKCMDBINDVERTEXBUFFERS_OFFSET
  for(int i = 0; i < m_ObjVBOs.size(); i++)
  {
    m_ObjVBOs[i].release();
  }
  for(int i = 0; i < m_ObjVBOs.size(); i++)
  {
    m_ObjEBOs[i].release();
  }
#else
  m_memoryVBO.free();
  m_memoryEBO.free();
#endif
  vkFreeDescriptorSets(nvk.m_device, pRendererVk->m_descPool, NDSETOBJECT, m_descriptorSets);
  memset(m_descriptorSets, 0, sizeof(VkDescriptorSet) * NDSETOBJECT);
  //
  // Note: No need to destroy command-buffers: the pools containing them will be destroyed anyways
  //
  for(int m = 0; m < m_numUsedCmdBuffers * 2; m++)
  {
    m_cmdBuffer[m].full = NULL;
    for(int i = 0; i < 5; i++)
    {
      m_cmdBuffer[m].SplitTopo[i] = NULL;
    }
  }
  m_numUsedCmdBuffers = 0;
  //
  // remove attached references of buffers to the meshes/Slots/Primgroups
  //
  bk3d::Mesh* pMesh = NULL;
  for(int i = 0; i < m_pGenericModel->m_meshFile->pMeshes->n; i++)
  {
    pMesh          = m_pGenericModel->m_meshFile->pMeshes->p[i];
    pMesh->userPtr = NULL;
    //
    // Slots: buffers for vertices
    //
    int n = pMesh->pSlots->n;
    for(int s = 0; s < n; s++)
    {
      bk3d::Slot* pS = pMesh->pSlots->p[s];
      pS->userData   = 0;
      pS->userPtr    = NULL;
    }
    //
    // Primitive groups
    //
    for(int pg = 0; pg < pMesh->pPrimGroups->n; pg++)
    {
      bk3d::PrimGroup* pPG = pMesh->pPrimGroups->p[pg];
      pPG->userPtr         = NULL;
    }
  }
  return true;
}


//------------------------------------------------------------------------------
//
// TODO: the chunk allocation that is done here can be deferred to Vulkan
// Allocator, thanks to vkQueueBindObjectMemory: many buffer could be bound
// to a chunk of memory that we'd have allocated with vkAllocMemory
//
//------------------------------------------------------------------------------
bool Bk3dModelVk::initResources(Renderer* pRenderer)
{
  RendererVk* pRendererVk = static_cast<RendererVk*>(pRenderer);

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
    //if(m_uboMaterial.buffer == )...
    m_uboMaterial.Sz     = sizeof(MaterialBuffer) * m_pGenericModel->m_materialNItems;
    m_uboMaterial.buffer = nvk.utCreateAndFillBuffer(&pRendererVk->m_perThreadData->m_cmdPoolStatic, m_uboMaterial.Sz,
                                                     m_pGenericModel->m_material, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                     m_uboMaterial.bufferMem, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
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
    //if(m_uboObjectMatrices.buffer == 0)
    m_uboObjectMatrices.Sz = sizeof(MatrixBufferObject) * m_pGenericModel->m_objectMatricesNItems;
    m_uboObjectMatrices.buffer =
        nvk.utCreateAndFillBuffer(&pRendererVk->m_perThreadData->m_cmdPoolStatic, m_uboObjectMatrices.Sz,
                                  m_pGenericModel->m_objectMatrices, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                  m_uboObjectMatrices.bufferMem, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    LOGI("%d matrices stored in %d Kb\n", m_pGenericModel->m_meshFile->pTransforms->nBones, (m_uboObjectMatrices.Sz + 512) / 1024);
  }
  //
  // DescriptorSet allocation
  //
  (nvk.allocateDescriptorSets(NVK::DescriptorSetAllocateInfo(pRendererVk->m_descPool, NDSETOBJECT /*numLayouts*/,
                                                             pRendererVk->m_descriptorSetLayouts + DSET_OBJECT),
                              m_descriptorSets));
  //
  // update the descriptorset used for Global
  // later we will update the ones local to objects
  //
  // TODO TODO TODO: we should use only one NVK::DescriptorBufferInfo as before
  NVK::DescriptorBufferInfo descBuffers  = NVK::DescriptorBufferInfo(m_uboObjectMatrices.buffer, 0, sizeof(mat4f));
  NVK::DescriptorBufferInfo descBuffers2 = NVK::DescriptorBufferInfo(m_uboMaterial.buffer, 0, sizeof(vec4f));

  nvk.updateDescriptorSets(NVK::WriteDescriptorSet(m_descriptorSets[0], BINDING_MATRIXOBJ, 0, descBuffers,
                                                   VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)(
      m_descriptorSets[0], BINDING_MATERIAL, 0, descBuffers2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC));
#ifdef USE_VKCMDBINDVERTEXBUFFERS_OFFSET
  BufO curVBO;
  BufO curEBO;
  memset(&curVBO, 0, sizeof(curVBO));
  memset(&curEBO, 0, sizeof(curEBO));
#endif
  unsigned int totalVBOSz = 0;
  unsigned int totalEBOSz = 0;
  //
  // First pass: evaluate the size of the single VBO
  // and store offset to where we'll find data back
  //
  bk3d::Mesh* pMesh = NULL;
  for(int i = 0; i < m_pGenericModel->m_meshFile->pMeshes->n; i++)
  {
    //g_pWinHandler->HandleMessageLoop_OnePass();
    pMesh = m_pGenericModel->m_meshFile->pMeshes->p[i];

#ifdef USE_VKCMDBINDVERTEXBUFFERS_OFFSET
    pMesh->VBOIDX = (void*)m_ObjVBOs.size();  // keep track of the VBO
//
// When we reached the arbitrary limit: switch to another VBO
//
#define MAXBOSZ 200000
    if((curVBO.Sz >> 10) > (MAXBOSZ * 1024))
    {
      curVBO.buffer = nvk.utCreateAndFillBuffer(&pRendererVk->m_perThreadData->m_cmdPoolStatic, curVBO.Sz, NULL,
                                                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, curVBO.bufferMem,
                                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
      //
      // push this VBO and create a new one
      //
      totalVBOSz += curVBO.Sz;
      m_ObjVBOs.push_back(curVBO);
      memset(&curVBO, 0, sizeof(curVBO));
      //
      // At the same time, create a new EBO... good enough for now
      //
      curEBO.buffer = nvk.utCreateAndFillBuffer(&pRendererVk->m_perThreadData->m_cmdPoolStatic, curEBO.Sz, NULL,
                                                VK_BUFFER_USAGE_INDEX_BUFFER_BIT, curEBO.bufferMem,
                                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
      //
      // push this VBO and create a new one
      //
      totalEBOSz += curEBO.Sz;
      m_ObjEBOs.push_back(curEBO);
      memset(&curEBO, 0, sizeof(curEBO));
    }
#endif
    //
    // Slots: buffers for vertices
    //
    int n = pMesh->pSlots->n;
    for(int s = 0; s < n; s++)
    {
      bk3d::Slot* pS   = pMesh->pSlots->p[s];
      pS->userData     = 0;
      GLuint alignedSz = (pS->vtxBufferSizeBytes >> 8);
      alignedSz += pS->vtxBufferSizeBytes & 0xFF ? 1 : 0;
      alignedSz = alignedSz << 8;
#ifdef USE_VKCMDBINDVERTEXBUFFERS_OFFSET
      pS->VBOIDX = (int*)curVBO.Sz;
      curVBO.Sz += alignedSz;
#else
      totalVBOSz += alignedSz;
#endif
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
        alignedSz = alignedSz << 8;
#ifdef USE_VKCMDBINDVERTEXBUFFERS_OFFSET
        pPG->EBOIDX = (void*)curEBO.Sz;
        curEBO.Sz += alignedSz;
      }
      else
      {
        pPG->EBOIDX = NULL;
      }
#else
        totalEBOSz += alignedSz;
      }
#endif
    }
  }
#ifdef USE_VKCMDBINDVERTEXBUFFERS_OFFSET
  //
  // Finalize the last set of data
  //
  {
    curVBO.buffer = nvk.utCreateAndFillBuffer(&pRendererVk->m_perThreadData->m_cmdPoolStatic, curVBO.Sz, NULL,
                                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, curVBO.bufferMem,
                                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    //
    // push this VBO and create a new one
    //
    totalVBOSz += curVBO.Sz;
    m_ObjVBOs.push_back(curVBO);
    memset(&curVBO, 0, sizeof(curVBO));
    //
    // At the same time, create a new EBO... good enough for now
    //
    curEBO.buffer = nvk.utCreateAndFillBuffer(&pRendererVk->m_perThreadData->m_cmdPoolStatic, curEBO.Sz, NULL,
                                              VK_BUFFER_USAGE_INDEX_BUFFER_BIT, curEBO.bufferMem,
                                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    //
    // push this EBO and create a new one
    //
    totalEBOSz += curEBO.Sz;
    m_ObjEBOs.push_back(curEBO);
    memset(&curEBO, 0, sizeof(curEBO));
  }
#else
  m_memoryVBO = nvk.allocateMemory(totalVBOSz, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  m_memoryEBO = nvk.allocateMemory(totalEBOSz, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
#endif
  //
  // second pass: put stuff in the buffer(s) and store offsets
  //
  for(int i = 0; i < m_pGenericModel->m_meshFile->pMeshes->n; i++)
  {
    VkResult    result = VK_SUCCESS;
    bk3d::Mesh* pMesh  = m_pGenericModel->m_meshFile->pMeshes->p[i];
#ifdef USE_VKCMDBINDVERTEXBUFFERS_OFFSET
    int idx = uintptr_t(pMesh->VBOIDX);
    curVBO  = m_ObjVBOs[idx];
    curEBO  = m_ObjEBOs[idx];
#endif
    int n = pMesh->pSlots->n;
    for(int s = 0; s < n; s++)
    {
      bk3d::Slot* pS = pMesh->pSlots->p[s];
#ifdef USE_VKCMDBINDVERTEXBUFFERS_OFFSET
      result = nvk.utFillBuffer(&pRendererVk->m_perThreadData->m_cmdPoolStatic, pS->vtxBufferSizeBytes, result,
                                pS->pVtxBufferData, curVBO.buffer, (GLuint)uintptr_t((int *)pS->VBOIDX));
#else
      VkBuffer buffer = m_memoryVBO.createBufferAllocFill(pRendererVk->m_perThreadData->m_cmdPoolStatic, pS->vtxBufferSizeBytes,
                                                          pS->pVtxBufferData, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, NULL);
      pS->userPtr = (int*)buffer;
#endif
    }
    for(int pg = 0; pg < pMesh->pPrimGroups->n; pg++)
    {
      bk3d::PrimGroup* pPG = pMesh->pPrimGroups->p[pg];
      if(pPG->indexArrayByteSize > 0)
      {
#ifdef USE_VKCMDBINDVERTEXBUFFERS_OFFSET
        result = nvk.utFillBuffer(&pRendererVk->m_perThreadData->m_cmdPoolStatic, pPG->indexArrayByteSize, result,
                                  pPG->pIndexBufferData, curEBO.buffer, (GLuint)uintptr_t(pPG->EBOIDX));
#else
        VkBuffer buffer = m_memoryEBO.createBufferAllocFill(pRendererVk->m_perThreadData->m_cmdPoolStatic, pPG->indexArrayByteSize,
                                                            pPG->pIndexBufferData, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, NULL);
        pPG->userPtr = (int*)buffer;
#endif
      }
    }
  }
#ifdef USE_VKCMDBINDVERTEXBUFFERS_OFFSET
  LOGI("meshes: %d in :%d VBOs (%f Mb) and %d EBOs (%f Mb) \n", m_pGenericModel->m_meshFile->pMeshes->n, m_ObjVBOs.size(),
       (float)totalVBOSz / (float)(1024 * 1024), m_ObjEBOs.size(), (float)totalEBOSz / (float)(1024 * 1024));
#else
  LOGI("meshes: %d in : %f Mb VBO and %f Mb EBO \n", m_pGenericModel->m_meshFile->pMeshes->n,
       (float)totalVBOSz / (float)(1024 * 1024), (float)totalEBOSz / (float)(1024 * 1024));
#endif
  return true;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void Bk3dModelVk::consolidateCmdBuffers(int numCmdBuffers)
{
  m_numUsedCmdBuffers = numCmdBuffers;
  // Optional cleanup
  if(numCmdBuffers == 0)
  {
    memset(m_cmdBuffer, 0, sizeof(ModelCmdBuffers) * MAXCMDBUFFERS);
    //for(int m=0; m<MAXCMDBUFFERS*2; m++)
    //{
    //    m_cmdBuffer[m].full = NULL;
    //    for(int i=0; i<5; i++)
    //    {
    //        m_cmdBuffer[m].SplitTopo[i] = NULL;
    //    }
    //}
  }
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool Bk3dModelVk::feedCmdBuffer(RendererVk* pRendererVk, NVK::CommandBuffer& cmdBuffer, NVK::CommandBuffer* cmdBufferSplitTopo, int mstart, int mend)
{
  //NXPROFILEFUNC(__FUNCTION__);
  BufO            curVBO;
  BufO            curEBO;
  GLuint          curMaterial             = 0;
  GLuint          curTransf               = 0;
  GLuint          curMeshTransf           = 0;
  VkPipeline      lastPipeline            = 0;
  VkCommandBuffer m_curCmdBufferSplitTopo = NULL;

  VkRenderPass  renderPass  = pRendererVk->m_scenePass;
  VkFramebuffer framebuffer = pRendererVk->m_framebuffer;
  NVK::Rect2D   viewRect    = pRendererVk->m_viewRect;
  float         lineWidth   = 1.0;
  float         width       = pRendererVk->m_viewRect.extent.width;
  float         height      = pRendererVk->m_viewRect.extent.height;
  //
  // Bind descriptorSet for globals
  //
  {
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pRendererVk->m_pipelineLayout, DSET_GLOBAL, 1,
                            &pRendererVk->m_descriptorSetGlobal, 0, NULL);
    cmdBuffer.cmdSetDepthBias(1.0f, 0.0f, 1.0f);  // offset raster
    cmdBuffer.cmdSetLineWidth(lineWidth);         //lineWidth
    cmdBuffer.cmdSetViewport(0, 1, NVK::Viewport(0.0, 0.0, width, height, 0.0f, 1.0f));
    cmdBuffer.cmdSetScissor(0, 1, NVK::Rect2D(0.0, 0.0, width, height));
    if(cmdBufferSplitTopo)
    {
      for(int i = 0; i < 5; i++)
      {
        vkCmdBindDescriptorSets(cmdBufferSplitTopo[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pRendererVk->m_pipelineLayout,
                                DSET_GLOBAL, 1, &pRendererVk->m_descriptorSetGlobal, 0, NULL);
        cmdBufferSplitTopo[i].cmdSetDepthBias(1.0f, 0.0f, 1.0f);  // offset raster
        cmdBufferSplitTopo[i].cmdSetLineWidth(lineWidth);         //lineWidth
        cmdBufferSplitTopo[i].cmdSetViewport(0, 1, NVK::Viewport(0.0, 0.0, width, height, 0.0f, 1.0f));
        cmdBufferSplitTopo[i].cmdSetScissor(0, 1, NVK::Rect2D(0.0, 0.0, width, height));
        switch(i)
        {
          case 0:
            vkCmdBindPipeline(cmdBufferSplitTopo[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pRendererVk->m_pipelineMeshLine);
            break;
          case 1:
            vkCmdBindPipeline(cmdBufferSplitTopo[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pRendererVk->m_pipelineMeshLineStrip);
            break;
          case 2:
            vkCmdBindPipeline(cmdBufferSplitTopo[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pRendererVk->m_pipelineMeshTri);
            break;
          case 3:
            vkCmdBindPipeline(cmdBufferSplitTopo[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pRendererVk->m_pipelineMeshTriStrip);
            break;
          case 4:
            vkCmdBindPipeline(cmdBufferSplitTopo[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pRendererVk->m_pipelineMeshTriFan);
            break;
        }
      }
    }
  }

  //-------------------------------------------------------------
  // Loop in the meshes of the model
  //
  if((mend <= 0) || (mend > m_pGenericModel->m_meshFile->pMeshes->n))
    mend = m_pGenericModel->m_meshFile->pMeshes->n;
  for(int m = mstart; m < mend; m++)
  {
    bk3d::Mesh* pMesh = m_pGenericModel->m_meshFile->pMeshes->p[m];
#ifdef USE_VKCMDBINDVERTEXBUFFERS_OFFSET
    //
    // get back the buffers that are used by this mesh
    //
    int idx = uintptr_t(pMesh->VBOIDX);
    curVBO  = m_ObjVBOs[idx];
    curEBO  = m_ObjEBOs[idx];
#endif
    //
    // Bind vertex buffer(s) of this mesh
    //
    if(pMesh->pTransforms && (pMesh->pTransforms->n > 0))
    {
      bk3d::Bone* pTransf = pMesh->pTransforms->p[0];
      if(pTransf && (curTransf != pTransf->ID))
      {
        curMeshTransf = pTransf->ID;
      }
    }
    else if(curTransf != 0)
    {
      curMeshTransf = 0;
    }
    // let's make it simple: for now we assume meshes give pos and normal as first and are in the same Buffer, interleaved...
    // bk3d files could give other forms of vertices... but for now we only work with the ones that are ok
    // 0: vertex
    // 1: normal
    bk3d::Slot* pS = pMesh->pSlots->p[0];
#ifdef USE_VKCMDBINDVERTEXBUFFERS_OFFSET
    VkDeviceSize vboffsets[] = {(GLuint64)pS->VBOIDX.p};  // we previously stored the offset in the buffer here...
#else
    VkBuffer buffer;
    buffer                   = (VkBuffer)pS->userPtr.p;
    VkDeviceSize vboffsets[] = {0};  //(GLuint64)pS->VBOIDX.p}; // we previously stored the offset in the buffer here...
#endif
    {
#ifdef USE_VKCMDBINDVERTEXBUFFERS_OFFSET
      vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &curVBO.buffer, vboffsets);
#else
      vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &buffer, vboffsets);
#endif
      if(cmdBufferSplitTopo)
      {
        for(int i = 0; i < 5; i++)
        {
#ifdef USE_VKCMDBINDVERTEXBUFFERS_OFFSET
          vkCmdBindVertexBuffers(cmdBufferSplitTopo[i], 0, 1, &curVBO.buffer, vboffsets);
#else
          vkCmdBindVertexBuffers(cmdBufferSplitTopo[i], 0, 1, &buffer, vboffsets);
#endif
        }
      }
    }
    // the file could give any arbitrary kind of attributes. Normally, we should check the attribute type and make them match with the shader expectation
    //int bindingIndex = 0;
    //for(int s=0; s<n; s++)
    //{
    //    bk3d::Slot* pS = pMesh->pSlots->p[s];
    //for(int a=0; a<pS->pAttributes->n; a++)
    //{
    //    bk3d::Attribute* pAttr = pS->pAttributes->p[a];
    //    bindingIndex++;
    //}
    //}
    //====> render primitive groups
    for(int pg = 0; pg < pMesh->pPrimGroups->n; pg++)
    {
      bool             needUpdateDSetOffsets = false;
      bk3d::PrimGroup* pPG                   = pMesh->pPrimGroups->p[pg];
      // filter unsuported primitives: QUADS + Line loops
      switch(pPG->topologyGL)
      {
        case GL_QUADS:
        case GL_QUAD_STRIP:
        case GL_LINE_LOOP:
          continue;
      }
      //
      // Material: point to the right one in the table
      //
      bk3d::Material* pMat = pPG->pMaterial;
      if(pMat && (curMaterial != pMat->ID))
      {
        curMaterial           = pMat->ID;
        needUpdateDSetOffsets = true;
      }
      if((pPG->pTransforms) && (pPG->pTransforms->n > 0))
      {
        bk3d::Bone* pTransf = pPG->pTransforms->p[0];
        if(pTransf && (curTransf != pTransf->ID))
        {
          curTransf             = pTransf->ID;
          needUpdateDSetOffsets = true;
        }
      }
      else
      {
        if((curMeshTransf > 0) && (curMeshTransf != curTransf))
        {
          needUpdateDSetOffsets = true;
          curTransf             = curMeshTransf;
        }
      }
      {
        switch(pPG->topologyGL)
        {
          case GL_LINES:
            if(cmdBufferSplitTopo)
              m_curCmdBufferSplitTopo = cmdBufferSplitTopo[0];
            if(lastPipeline != pRendererVk->m_pipelineMeshLine)
            {
              lastPipeline = pRendererVk->m_pipelineMeshLine;
              vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pRendererVk->m_pipelineMeshLine);
            }
            break;
          case GL_LINE_STRIP:
            if(cmdBufferSplitTopo)
              m_curCmdBufferSplitTopo = cmdBufferSplitTopo[1];
            if(lastPipeline != pRendererVk->m_pipelineMeshLineStrip)
            {
              lastPipeline = pRendererVk->m_pipelineMeshLineStrip;
              vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pRendererVk->m_pipelineMeshLineStrip);
            }
            break;
          case GL_TRIANGLES:
            if(cmdBufferSplitTopo)
              m_curCmdBufferSplitTopo = cmdBufferSplitTopo[2];
            if(lastPipeline != pRendererVk->m_pipelineMeshTri)
            {
              lastPipeline = pRendererVk->m_pipelineMeshTri;
              vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pRendererVk->m_pipelineMeshTri);
            }
            break;
          case GL_TRIANGLE_STRIP:
            if(cmdBufferSplitTopo)
              m_curCmdBufferSplitTopo = cmdBufferSplitTopo[3];
            if(lastPipeline != pRendererVk->m_pipelineMeshTriStrip)
            {
              lastPipeline = pRendererVk->m_pipelineMeshTriStrip;
              vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pRendererVk->m_pipelineMeshTriStrip);
            }
            break;
          case GL_TRIANGLE_FAN:
            if(cmdBufferSplitTopo)
              m_curCmdBufferSplitTopo = cmdBufferSplitTopo[4];
            if(lastPipeline != pRendererVk->m_pipelineMeshTriFan)
            {
              lastPipeline = pRendererVk->m_pipelineMeshTriFan;
              vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pRendererVk->m_pipelineMeshTriFan);
            }
            break;
          default:
            DEBUGBREAK();
            // not-handled cases...
            break;
        }
      }
      //
      // Bind descriptorSet for local transformation and material
      // offset is the key for proper updates
      //
      if(needUpdateDSetOffsets)
      {
        uint32_t offsets[2] = {static_cast<uint32_t>(curTransf * sizeof(MatrixBufferObject)), static_cast<uint32_t>(curMaterial * sizeof(MaterialBuffer))};
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pRendererVk->m_pipelineLayout, DSET_OBJECT,
                                NDSETOBJECT, m_descriptorSets, 2, offsets);
        // This is not optimal: many vkCmdBindDescriptorSets in cmdBufferSplitTopo[] when not especially needed
        if(cmdBufferSplitTopo)
        {
          for(int i = 0; i < 5; i++)
          {
            vkCmdBindDescriptorSets(cmdBufferSplitTopo[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pRendererVk->m_pipelineLayout,
                                    DSET_OBJECT, NDSETOBJECT, m_descriptorSets, 2, offsets);
          }
        }
      }
      if(pPG->pIndexBufferData)
      {
#ifdef USE_VKCMDBINDVERTEXBUFFERS_OFFSET
        VkBuffer idxBuffer((VkBuffer)pPG->EBOIDX);
        vkCmdBindIndexBuffer(cmdBuffer, curEBO.buffer, (GLuint64)pPG->EBOIDX,
                             pPG->indexFormatGL == GL_UNSIGNED_INT ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16);
#else
        VkBuffer buffer;
        buffer = (VkBuffer)pPG->userPtr;
        vkCmdBindIndexBuffer(cmdBuffer, buffer, 0, pPG->indexFormatGL == GL_UNSIGNED_INT ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16);
#endif
        vkCmdDrawIndexed(cmdBuffer, pPG->indexCount, 1, 0, 0, 0);
        if(m_curCmdBufferSplitTopo)
        {
#ifdef USE_VKCMDBINDVERTEXBUFFERS_OFFSET
          vkCmdBindIndexBuffer(m_curCmdBufferSplitTopo, curEBO.buffer, (GLuint64)pPG->EBOIDX,
                               pPG->indexFormatGL == GL_UNSIGNED_INT ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16);
#else
          vkCmdBindIndexBuffer(m_curCmdBufferSplitTopo, buffer, 0,
                               pPG->indexFormatGL == GL_UNSIGNED_INT ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16);
#endif
          vkCmdDrawIndexed(m_curCmdBufferSplitTopo, pPG->indexCount, 1, 0, 0, 0);
        }
      }
      else
      {
        vkCmdDraw(cmdBuffer, pPG->indexCount, 1, 0, 0);
        if(m_curCmdBufferSplitTopo)
        {
          vkCmdDraw(m_curCmdBufferSplitTopo, pPG->indexCount, 1, 0, 0);
        }
      }
    }
  }
  return true;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool Bk3dModelVk::buildCmdBuffer(Renderer* pRenderer, int bufIdx, int mstart, int mend)
{
  bool        res         = false;
  RendererVk* pRendererVk = static_cast<RendererVk*>(pRenderer);
  NXPROFILEFUNC(__FUNCTION__);
  VkRenderPass  renderPass  = pRendererVk->m_scenePass;
  VkFramebuffer framebuffer = pRendererVk->m_framebuffer;

  // take the right one
  ModelCmdBuffers& cmdBuffer = m_cmdBuffer[bufIdx];
  //
  // Build the command-buffers
  //
  if(m_pGenericModel->m_meshFile)
  {
    //if(topologies & 0x20)
    {
      cmdBuffer.full = pRendererVk->m_perThreadData->m_curCmdPoolDynamic->allocateCommandBuffer(false);
      cmdBuffer.full.beginCommandBuffer(false, true,
                                        NVK::CommandBufferInheritanceInfo(renderPass, 0, framebuffer, VK_FALSE /*occlusionQueryEnable*/,
                                                                          0 /*queryFlags*/, 0 /*pipelineStatistics*/));
    }  //else cmdBuffer.full.beginCommandBuffer(false, true);//, NVK::CommandBufferInheritanceInfo(renderPass, 0, framebuffer, VK_FALSE, 0,0) );
    {
      for(int i = 0; i < 5; i++)
      {
        cmdBuffer.SplitTopo[i] = pRendererVk->m_perThreadData->m_curCmdPoolDynamic->allocateCommandBuffer(false);
        cmdBuffer.SplitTopo[i].beginCommandBuffer(false, true,
                                                  NVK::CommandBufferInheritanceInfo(renderPass, 0, framebuffer, VK_FALSE /*occlusionQueryEnable*/,
                                                                                    0 /*queryFlags*/, 0 /*pipelineStatistics*/));
      }
    }
    //
    // Now keep track of this command-buffer for the thread to release it later
    // we can't release is in this worker: any thread could have been assigned to it
    // and we must use the same thread to release it than the one that created it
    //
    RendererVk::PerThreadData* perThreadData = pRendererVk->m_perThreadData;

    res = feedCmdBuffer(pRendererVk, cmdBuffer.full, cmdBuffer.SplitTopo, mstart, mend);

    //if(topologies & 0x20)
    cmdBuffer.full.endCommandBuffer();
    //else
    for(int i = 0; i < 5; i++)
      cmdBuffer.SplitTopo[i].endCommandBuffer();

  }  //if(m_pGenericModel->m_meshFile)
  return res;
}
//----------------------------------------------------------------------------------------
// In Vulkan, this display command will be for "recording" to a primary command buffer
//----------------------------------------------------------------------------------------
void Bk3dModelVk::displayObject(Renderer* pRenderer, const mat4f& cameraView, const mat4f projection, unsigned char topologies)
{
  RendererVk*     pRendererVk = static_cast<RendererVk*>(pRenderer);
  VkCommandBuffer pCmd        = pRendererVk->m_cmdScene;
  // take the right one
  for(int i = 0; i < m_numUsedCmdBuffers; i++)
  {
    ModelCmdBuffers& cmdBuffer = m_cmdBuffer[i];
    if(topologies & 0x20)
    {
      // Un-sorted primitives case: created the mesh as the primitive groups arrived
      vkCmdExecuteCommands(pCmd, 1, cmdBuffer.full);
    }
    else
    {
      if((topologies & 0x01) && (cmdBuffer.SplitTopo[0]))
        vkCmdExecuteCommands(pCmd, 1, cmdBuffer.SplitTopo[0]);
      if((topologies & 0x02) && (cmdBuffer.SplitTopo[1]))
        vkCmdExecuteCommands(pCmd, 1, cmdBuffer.SplitTopo[1]);
      if((topologies & 0x04) && (cmdBuffer.SplitTopo[2]))
        vkCmdExecuteCommands(pCmd, 1, cmdBuffer.SplitTopo[2]);
      if((topologies & 0x08) && (cmdBuffer.SplitTopo[3]))
        vkCmdExecuteCommands(pCmd, 1, cmdBuffer.SplitTopo[3]);
      if((topologies & 0x10) && (cmdBuffer.SplitTopo[4]))
        vkCmdExecuteCommands(pCmd, 1, cmdBuffer.SplitTopo[4]);
    }
  }
}

}  //namespace vk
