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
#define DEFAULT_RENDERER 2
#include "gl_vk_bk3dthreaded.h"
#include <imgui/imgui_impl_gl.h>
#include <nvgl/contextwindow_gl.hpp>

//-----------------------------------------------------------------------------
// renderers
//-----------------------------------------------------------------------------
Renderer*        g_renderers[10];
int              g_numRenderers = 0;
static Renderer* s_pCurRenderer = NULL;
static int       s_curRenderer  = DEFAULT_RENDERER;
//-----------------------------------------------------------------------------
// timing/profiling
//-----------------------------------------------------------------------------
double        g_statsCpuTime = 0;
double        g_statsGpuTime = 0;
nvh::Profiler g_profiler;
//-----------------------------------------------------------------------------
// Toggles
//-----------------------------------------------------------------------------
bool g_bDisplayObject            = true;
bool g_bRefreshCmdBuffers        = true;
int  g_bRefreshCmdBuffersCounter = 2;
bool g_bDisplayGrid              = true;
bool g_bTopologyLines            = true;
bool g_bTopologylinestrip        = true;
bool g_bTopologytriangles        = true;
bool g_bTopologytristrips        = true;
bool g_bTopologytrifans          = true;
int  g_bUnsortedPrims            = 0;
int  g_MSAA                      = 8;

MatrixBufferGlobal g_globalMatrices;

//-----------------------------------------------------------------------------
// Static variables for setting up the scene...
//-----------------------------------------------------------------------------

std::vector<Bk3dModel*> g_bk3dModels;

static int s_curObject = 0;

template <typename T, size_t N>
inline size_t array_size(T (&x)[N])
{
  return N;
}
//
// Camera animation: captured using '1' in the sample. Then copy and paste...
//
struct CameraAnim
{
  vec3f eye, focus;
  float sleep;
};
static CameraAnim s_cameraAnim[] = {
    // pos                    target                   time to wait
    {vec3f(-0.43, -0.20, -0.01), vec3f(-0.14, -0.34, 0.40), 3.0}, {vec3f(0.00, -0.36, 0.15),  vec3f(0.01, -0.40, 0.39),  2},
    {vec3f(0.15, -0.38, 0.23),   vec3f(0.16, -0.40, 0.38),  2},   {vec3f(0.30, -0.37, 0.22),  vec3f(0.31, -0.39, 0.38),  2},
    {vec3f(0.60, -0.37, 0.32),   vec3f(0.35, -0.41, 0.41),  2},   {vec3f(0.24, -0.39, 0.26),  vec3f(0.23, -0.40, 0.35),  2},
    {vec3f(0.26, -0.41, 0.37),   vec3f(0.23, -0.40, 0.37),  2},   {vec3f(0.09, -0.40, 0.36),  vec3f(0.05, -0.40, 0.37),  2},
    {vec3f(-0.01, -0.38, 0.39),  vec3f(-0.07, -0.38, 0.39), 2},   {vec3f(-0.18, -0.38, 0.38), vec3f(-0.11, -0.38, 0.40), 2},
    {vec3f(-0.12, -0.37, 0.41),  vec3f(-0.06, -0.37, 0.43), 2},   {vec3f(-0.12, -0.29, 0.41), vec3f(-0.12, -0.30, 0.41), 1},
    {vec3f(-0.25, -0.12, 0.25),  vec3f(-0.11, -0.37, 0.40), 2},
};

static int   s_cameraAnimItem      = 0;
static int   s_cameraAnimItems     = array_size(s_cameraAnim);
static float s_cameraAnimIntervals = 0.1;
static bool  s_bCameraAnim         = true;

#define HELPDURATION 5.0
static float s_helpText = 0.0;

static bool s_bStats = true;

#ifdef USEWORKERS
//-----------------------------------------------------------------------------
// Stuff for Multi-threading, using 'Workers'
//-----------------------------------------------------------------------------
#include "mt/CThreadWork.h"
#define NUMTHREADS 8
ThreadWorkerPool* g_mainThreadPool = NULL;
CEvent            g_dataReadyEvent;
TaskQueue*        g_mainThreadQueue = NULL;
CCriticalSection* g_crs_bk3d        = NULL;  // for concurrent access on the model
CCriticalSection* g_crs_VK          = NULL;  // for concurrent access on Vulkan
CEvent*           g_evt_cmdbuf      = NULL;
bool              g_useWorkers      = false;
#endif
int g_numCmdBuffers = 16;
//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
void destroyCommandBuffers(bool bAll);
void releaseThreadLocalVars();
void initThreadLocalVars();
//-----------------------------------------------------------------------------
// Derive the Window for this sample
//-----------------------------------------------------------------------------
class MyWindow : public AppWindowCameraInertia
{
public:
  ImGuiH::Registry      m_guiRegistry;
  nvgl::ContextWindow m_contextWindowGL;

  MyWindow();

  bool open(int posX, int posY, int width, int height, const char* title, const nvgl::ContextWindowCreateInfo& context);
  void processUI(int width, int height, double dt);

  virtual void onWindowClose() override;
  virtual void onWindowResize(int w = 0, int h = 0) override;
  //virtual void motion(int x, int y) override;
  //virtual void mousewheel(short delta) override;
  //virtual void mouse(NVPWindow::MouseButton button, ButtonAction action, int mods, int x, int y) override;
  //virtual void menu(int m) override;
  virtual void onKeyboard(MyWindow::KeyCode key, ButtonAction action, int mods, int x, int y) override;
  virtual void onKeyboardChar(unsigned char key, int mods, int x, int y) override;
  //virtual void idle() override;
  virtual void onWindowRefresh() override;
};

MyWindow::MyWindow()
    : AppWindowCameraInertia(vec3f(-0.43, -0.20, -0.01), vec3f(-0.14, -0.34, 0.40)

      )
{
}

#ifdef USEWORKERS
void initThreads()
{
  //
  // Create a pool
  //
  g_mainThreadPool = new ThreadWorkerPool(NUMTHREADS, false, false, NWTPS_ROUND_ROBIN, std::string("Main Worker Pool"));
  LOGI("Creating %d workers...\n", NUMTHREADS);
  //
  // Create a TaskBatch for this main thread
  //
  g_mainThreadQueue = new TaskQueue((NThreadID)0, &g_dataReadyEvent);  // 0 means that the main thread handle will be taken
  setCurrentTaskQueue(g_mainThreadQueue);
  g_crs_bk3d = new CCriticalSection();
  g_crs_VK   = new CCriticalSection();
  // create N events
  g_evt_cmdbuf = new CEvent[g_bk3dModels.size() * MAXCMDBUFFERS];
}

void terminateThreads()
{
  g_mainThreadPool->FlushTasks();
  g_mainThreadPool->Terminate();  // issue with NWTPS_SHARED_QUEUE
  delete g_mainThreadPool;
  g_mainThreadPool = NULL;
  delete g_mainThreadQueue;
  g_mainThreadQueue = NULL;
  delete g_crs_bk3d;
  g_crs_bk3d = NULL;
  delete g_crs_VK;
  g_crs_VK = NULL;
}
#endif

//-----------------------------------------------------------------------------
// Help
//-----------------------------------------------------------------------------
static const char* s_sampleHelp =
    "space: toggles continuous rendering\n"
    "'c': use toggle command-buffer continuous refresh\n"
    "'l': use glCallCommandListNV\n"
    "'o': toggles object display\n"
    "'g': toggles grid display\n"
    "'s': toggle stats\n"
    "'a': animate camera\n";
static const char* s_sampleHelpCmdLine =
    "---------- Cmd-line arguments ----------\n"
    "-v <VBO max Size>\n-m <bk3d model>\n"
    "-c 0 or 1 : use command-lists\n"
    "-o 0 or 1 : display meshes\n"
    "-g 0 or 1 : display grid\n"
    "-s 0 or 1 : stats\n"
    "-a 0 or 1 : animate camera\n"
    "-d 0 or 1 : debug stuff (ui)\n"
    "-m <bk3d file> : load a specific model\n"
    "<bk3d>    : load a specific model\n"
    "-q <msaa> : MSAA\n"
    "----------------------------------------\n";

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool Bk3dModel::updateForChangedRenderTarget()
{
  if(m_pRenderer)
    return m_pRenderer->updateForChangedRenderTarget(this);
  return false;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
Bk3dModel::Bk3dModel(const char* name, vec3f* pPos, float* pScale)
{
  assert(name);
  m_name                 = std::string(name);
  m_objectMatrices       = NULL;
  m_objectMatricesNItems = 0;
  m_material             = NULL;
  m_materialNItems       = 0;
  m_meshFile             = NULL;
  m_posOffset            = pPos ? *pPos : vec3f(0, 0, 0);
  m_scale                = pScale ? *pScale : 0.0f;
  m_pRenderer            = NULL;
}

Bk3dModel::~Bk3dModel()
{
  delete[] m_objectMatrices;
  delete[] m_material;
  if(m_meshFile)
    free(m_meshFile);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool Bk3dModel::loadModel()
{
  LOGI("Loading Mesh %s..\n", m_name.c_str());
  std::vector<std::string> m_paths;
  m_paths.push_back(m_name);
  m_paths.push_back(std::string("downloaded_resources/") + m_name);
  m_paths.push_back(std::string("../downloaded_resources/") + m_name);
  m_paths.push_back(std::string("../../downloaded_resources/") + m_name);
  m_paths.push_back(std::string("../../../downloaded_resources/") + m_name);
  //paths.push_back(std::string(PROJECT_RELDIRECTORY) + name);
  //paths.push_back(std::string(PROJECT_ABSDIRECTORY) + name);
  for(int i = 0; i < m_paths.size(); i++)
  {
    if((m_meshFile = bk3d::load(m_paths[i].c_str())))
    {
      break;
    }
  }
  if(m_meshFile == NULL)
  {
    LOGE("error in loading mesh %s\n", m_name.c_str());
    return false;
  }
  //
  // Some adjustment for the display
  //
  if(m_scale <= 0.0)
  {
    float min[3] = {1000.0, 1000.0, 1000.0};
    float max[3] = {-1000.0, -1000.0, -1000.0};
    for(int i = 0; i < m_meshFile->pMeshes->n; i++)
    {
      bk3d::Mesh* pMesh = m_meshFile->pMeshes->p[i];
      if(pMesh->aabbox.min[0] < min[0])
        min[0] = pMesh->aabbox.min[0];
      if(pMesh->aabbox.min[1] < min[1])
        min[1] = pMesh->aabbox.min[1];
      if(pMesh->aabbox.min[2] < min[2])
        min[2] = pMesh->aabbox.min[2];
      if(pMesh->aabbox.max[0] > max[0])
        max[0] = pMesh->aabbox.max[0];
      if(pMesh->aabbox.max[1] > max[1])
        max[1] = pMesh->aabbox.max[1];
      if(pMesh->aabbox.max[2] > max[2])
        max[2] = pMesh->aabbox.max[2];
    }
    m_posOffset[0] = (max[0] + min[0]) * 0.5f;
    m_posOffset[1] = (max[1] + min[1]) * 0.5f;
    m_posOffset[2] = (max[2] + min[2]) * 0.5f;
    float bigger   = 0;
    if((max[0] - min[0]) > bigger)
      bigger = (max[0] - min[0]);
    if((max[1] - min[1]) > bigger)
      bigger = (max[1] - min[1]);
    if((max[2] - min[2]) > bigger)
      bigger = (max[2] - min[2]);
    if((bigger) > 0.001)
    {
      m_scale = 1.0f / bigger;
      PRINTF(("Scaling the model by %f...\n", m_scale));
    }
    m_posOffset *= m_scale;
  }
  //
  // Allocate an array of transforms and materials for later use with APIs
  //
  //
  // create Buffer Object for materials
  //
  if(m_meshFile->pMaterials && m_meshFile->pMaterials->nMaterials)
  {
    m_material       = new MaterialBuffer[m_meshFile->pMaterials->nMaterials];
    m_materialNItems = m_meshFile->pMaterials->nMaterials;
    for(int i = 0; i < m_meshFile->pMaterials->nMaterials; i++)
    {
      // 256 bytes aligned...
      // this is for now a fake material: very few items (diffuse)
      // in a real application, material information could contain more
      // we'd need 16 vec4 to fill 256 bytes
      memcpy(m_material[i].diffuse.vec_array, m_meshFile->pMaterials->pMaterials[i]->Diffuse(), sizeof(vec3f));
      //...
      // hack for visual result...
      if(length(m_material[i].diffuse) <= 0.1f)
        m_material[i].diffuse = vec3f(1, 1, 1);
    }
  }
  //
  // create Buffer Object for Object-matrices
  //
  if(m_meshFile->pTransforms && m_meshFile->pTransforms->nBones)
  {
    m_objectMatrices       = new MatrixBufferObject[m_meshFile->pTransforms->nBones];
    m_objectMatricesNItems = m_meshFile->pTransforms->nBones;
    for(int i = 0; i < m_meshFile->pTransforms->nBones; i++)
    {
      // 256 bytes aligned...
      memcpy(m_objectMatrices[i].mO.mat_array, m_meshFile->pTransforms->pBones[i]->Matrix().m, sizeof(mat4f));
    }
  }
  return true;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void Bk3dModel::addStats(Stats& stats)
{
  stats.primitives += m_stats.primitives;
  stats.drawcalls += m_stats.drawcalls;
  stats.attr_update += m_stats.attr_update;
  stats.uniform_update += m_stats.uniform_update;
}
void Bk3dModel::printPosition()
{
  LOGI("%f %f %f %f\n", m_posOffset[0], m_posOffset[1], m_posOffset[2], m_scale);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

#define COMBO_MSAA 0
#define COMBO_SORT 1
#define COMBO_RENDERER 2
#define SCALAR_NCMDBUF 3
#define CHECK_WORKERS 4
void MyWindow::processUI(int width, int height, double dt)
{
  // Update imgui configuration
  auto& imgui_io       = ImGui::GetIO();
  imgui_io.DeltaTime   = static_cast<float>(dt);
  imgui_io.DisplaySize = ImVec2(width, height);

  ImGui::NewFrame();
  ImGui::SetNextWindowBgAlpha(0.5);
  ImGui::SetNextWindowSize(ImVec2(450, 0), ImGuiCond_FirstUseEver);
  if(ImGui::Begin("NVIDIA " PROJECT_NAME, nullptr))
  {

    m_guiRegistry.enumCombobox(COMBO_RENDERER, "Renderer", &s_curRenderer);
#ifdef USEWORKERS
    m_guiRegistry.checkbox(CHECK_WORKERS, "UseWorkers", &g_useWorkers);
#endif
    m_guiRegistry.inputIntClamped(SCALAR_NCMDBUF, "N Objs x&y", &g_numCmdBuffers, 1, MAXCMDBUFFERS);
    ImGui::Separator();
    m_guiRegistry.enumCombobox(COMBO_SORT, "Cmd-Buf style", &g_bUnsortedPrims);
    m_guiRegistry.enumCombobox(COMBO_MSAA, "MSAA", &g_MSAA);
    ImGui::Separator();
    //CreateCtrlButton("CURPRINT", "Ouput Pos-scale", g_pTweakContainer)->Register(&eventUI2);
    //CreateCtrlScalar("CURO", "Cur Object", g_pTweakContainer)->SetBounds(0.0, 10.)->SetIntMode()->Register(&eventUI2), &s_curObject);
    //ImGui::InputFloat("CURX", v, 0.1f, 1.0f, 2);
    // ...
    //ImGui::Separator();
    ImGui::Checkbox("command buffer continuous refresh\n", &g_bRefreshCmdBuffers);
    ImGui::Checkbox("continuous rendering\n", &m_realtime.bNonStopRendering);
    ImGui::Checkbox("object display\n", &g_bDisplayObject);
    ImGui::Checkbox("grid display\n", &g_bDisplayGrid);
    ImGui::Checkbox("stats\n", &s_bStats);
    ImGui::Checkbox("animate camera\n", &s_bCameraAnim);
    ImGui::Checkbox("Topology Lines\n", &g_bTopologyLines);
    ImGui::Checkbox("Topology Line-Strip\n", &g_bTopologylinestrip);
    ImGui::Checkbox("Topology Triangles\n", &g_bTopologytriangles);
    ImGui::Checkbox("Topology Tri-Strips\n", &g_bTopologytristrips);
    ImGui::Checkbox("Topology Tri-Fans\n", &g_bTopologytrifans);
    ImGui::Separator();
    ImGui::Text("('h' to toggle help)");
    //if(s_bStats)
    //    h += m_oglTextBig.drawString(5, m_winSz[1]-h, hudStats.c_str(), 0, vec4f(0.8,0.8,1.0,0.5).vec_array);

    if(s_helpText)
    {
      ImGui::BeginChild("Help", ImVec2(400, 110), true);
      // camera help
      //ImGui::SetNextWindowCollapsed(0);
      const char* txt = getHelpText();
      ImGui::Text("%s",txt);
      ImGui::EndChild();
    }

    int avg = 10;

    if(g_profiler.getTotalFrames() % avg == avg - 1)
    {
      nvh::Profiler::TimerInfo info;
      g_profiler.getTimerInfo("scene", info);
      g_statsCpuTime = info.cpu.average;
      g_statsGpuTime = info.gpu.average;
    }

    float gpuTimeF = float(g_statsGpuTime);
    float cpuTimeF = float(g_statsCpuTime);
    float maxTimeF = std::max(std::max(cpuTimeF, gpuTimeF), 0.0001f);

    ImGui::Text("Frame     [ms]: %2.1f", dt * 1000.0f);
    ImGui::Text("Scene GPU [ms]: %2.3f", gpuTimeF / 1000.0f);
    ImGui::ProgressBar(gpuTimeF / maxTimeF, ImVec2(0.0f, 0.0f));
    ImGui::Text("Scene CPU [ms]: %2.3f", cpuTimeF / 1000.0f);
    ImGui::ProgressBar(cpuTimeF / maxTimeF, ImVec2(0.0f, 0.0f));
  }
  ImGui::End();
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool MyWindow::open(int posX, int posY, int width, int height, const char* title, const nvgl::ContextWindowCreateInfo& context)
{
  if(!AppWindowCameraInertia::open(posX, posY, width, height, title, true))
    return false;
  m_contextWindowGL.init(&context, m_internal, title);
  ImGui::InitGL();
  //
  // UI
  //
  auto& imgui_io       = ImGui::GetIO();
  imgui_io.IniFilename = nullptr;
  m_guiRegistry.enumAdd(COMBO_MSAA, 1, "MSAA OFF");
  m_guiRegistry.enumAdd(COMBO_MSAA, 4, "MSAA 4x");
  m_guiRegistry.enumAdd(COMBO_MSAA, 8, "MSAA 8x");
  m_guiRegistry.enumAdd(COMBO_SORT, 0, "Sort on primitive types");
  m_guiRegistry.enumAdd(COMBO_SORT, 1, "Unsorted primitive types");
  for(int i = 0; i < g_numRenderers; i++)
  {
    m_guiRegistry.enumAdd(COMBO_RENDERER, i, g_renderers[i]->getName());
  }
  m_guiRegistry.checkboxAdd(CHECK_WORKERS);
  m_guiRegistry.inputIntAdd(SCALAR_NCMDBUF, g_numCmdBuffers);
  return true;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void MyWindow::onWindowClose()
{
  AppWindowCameraInertia::onWindowClose();
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void MyWindow::onWindowResize(int w, int h)
{
  if(w == 0)
    w = getWidth();
  if(h == 0)
    h = getHeight();
  AppWindowCameraInertia::onWindowResize(w, h);
  if(s_pCurRenderer)
  {
    if(s_pCurRenderer->bFlipViewport())
    {
      m_projection *= nvmath::scale_mat4(nvmath::vec3(1, -1, 1));
    }
    //
    // update the token buffer in which the viewport setup happens for token rendering
    //
    s_pCurRenderer->updateViewport(0, 0, w, h);
    //
    // rebuild the main Command-buffer: size could have been hard-coded within
    //
    s_pCurRenderer->buildPrimaryCmdBuffer();
    //
    // the FBOs were destroyed and rebuilt
    // associated 64 bits pointers (as resident resources) might have changed
    // we need to rebuild the *command-lists*
    //
    FOREACHMODEL(updateForChangedRenderTarget());
  }
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
#define KEYTAU 0.10f
void MyWindow::onKeyboard(NVPWindow::KeyCode key, MyWindow::ButtonAction action, int mods, int x, int y)
{
  AppWindowCameraInertia::onKeyboard(key, action, mods, x, y);

  if(action == MyWindow::BUTTON_RELEASE)
    return;
  switch(key)
  {
    case NVPWindow::KEY_F1:
      LOGI("Camera: vec3f(%.2f,%.2f,%.2f), vec3f(%.2f,%.2f,%.2f)\n ", m_camera.eyePos.x, m_camera.eyePos.y,
           m_camera.eyePos.z, m_camera.focusPos.x, m_camera.focusPos.y, m_camera.focusPos.z);
      break;
    //...
    case NVPWindow::KEY_F12:
      break;
  }
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void MyWindow::onKeyboardChar(unsigned char key, int mods, int x, int y)
{
  AppWindowCameraInertia::onKeyboardChar(key, mods, x, y);
  switch(key)
  {
    case '1':
      m_camera.print_look_at(true);
      break;
    case '2':  // dumps the position and scale of current object
      if(s_curObject >= g_bk3dModels.size())
        break;
      g_bk3dModels[s_curObject]->printPosition();
      break;
    case '0':
      m_bAdjustTimeScale = true;
    case 'h':
      LOGI(s_sampleHelpCmdLine);
      s_helpText = HELPDURATION;
      break;
  }
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void resetCommandBuffersPool()
{
  // here we reset the primary pool: what is from thread #0
  s_pCurRenderer->resetCommandBuffersPool();
#ifdef USEWORKERS
  if(g_useWorkers)
  {
    // here we reset the secondary pools: what is from thread #0
    //---------------------------------------------------------------------
    // Invoke a Task on all the threads to setup some thread-local variable
    //
    class TaskResetCommandBuffersPool : public TaskBase
    {
    private:
      int m;

    public:
      TaskResetCommandBuffersPool(int _m)
          : TaskBase()
      {
        m = _m;
      }
      void Invoke()
      {  // We are now in the Thread : let's reset the command-buffers Pool
        s_pCurRenderer->resetCommandBuffersPool();
        g_evt_cmdbuf[m].Set();
      }
    };
    //---------------------------------------------------------------------
    // loop in each existing thread
    for(unsigned int i = 0; i < g_mainThreadPool->getThreadCount(); i++)
    {
      // Call in // threads
      TaskResetCommandBuffersPool* taskResetCommandBuffersPool;
      taskResetCommandBuffersPool = new TaskResetCommandBuffersPool(i);
      // explicitly choosing threads. Note: need to check how does it work with NWTPS_SHARED_QUEUE
      g_mainThreadPool->getThreadWorker(i)->GetTaskQueue().pushTask(taskResetCommandBuffersPool);
    }
    // wait for all before continuing
    {
      //NXPROFILEFUNC("Wait for all");
      for(unsigned int m = 0; m < g_mainThreadPool->getThreadCount(); m++)
      {
        //
        // Wait for the worker to be done
        //
        g_evt_cmdbuf[m].WaitOnEvent();
        g_evt_cmdbuf[m].Reset();
      }
    }
  }
#endif
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void destroyCommandBuffers(bool bAll)
{
  NXPROFILEFUNC(__FUNCTION__);
  if(bAll)
  {
    // wait for the GPU to be done: we might erase commands that are still used by the GPU
    s_pCurRenderer->waitForGPUIdle();
    //
    for(int m = 0; m < g_bk3dModels.size(); m++)
    {
      s_pCurRenderer->consolidateCmdBuffersModel(g_bk3dModels[m], 0);
    }
  }
#ifdef USEWORKERS
  if(g_useWorkers)
  {
    //---------------------------------------------------------------------
    // Invoke a Task on all the threads to setup some thread-local variable
    //
    class TaskDestroyCommandBuffers : public TaskBase
    {
    private:
      int  m;
      bool bAll;

    public:
      TaskDestroyCommandBuffers(int _m, bool _bAll)
          : TaskBase()
      {
        m    = _m;
        bAll = _bAll;
      }
      void Invoke()
      {  // We are now in the Thread : let's delete the command-buffers pointed through TLS
        s_pCurRenderer->destroyCommandBuffers(bAll);
        //if(bAll)
        g_evt_cmdbuf[m].Set();
      }
    };
    //---------------------------------------------------------------------
    for(unsigned int i = 0; i < g_mainThreadPool->getThreadCount(); i++)
    {
      // Call in // threads
      TaskDestroyCommandBuffers* taskDestroyCommandBuffers;
      taskDestroyCommandBuffers = new TaskDestroyCommandBuffers(i, bAll);
      // explicitly choosing threads. Note: need to check how does it work with NWTPS_SHARED_QUEUE
      g_mainThreadPool->getThreadWorker(i)->GetTaskQueue().pushTask(taskDestroyCommandBuffers);
      //taskDestroyCommandBuffers->(&g_mainThreadPool->getThreadWorker(i)->GetTaskQueue());
    }
    // wait for completion before continuing
    {
      NXPROFILEFUNC("Wait for all");
      for(unsigned int m = 0; m < g_mainThreadPool->getThreadCount(); m++)
      {
        //
        // Wait for the worker to be done
        //
        g_evt_cmdbuf[m].WaitOnEvent();
        g_evt_cmdbuf[m].Reset();
      }
    }
  }
  else
#endif
  {
    s_pCurRenderer->destroyCommandBuffers(bAll);
  }
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void initThreadLocalVars()
{
#ifdef USEWORKERS
  NXPROFILEFUNC(__FUNCTION__);
  //---------------------------------------------------------------------
  // Invoke a Task on all the threads to setup some thread-local variable
  //
  class SetThreadLocalVars : public TaskSyncCall
  {
  public:
    void Invoke()
    {  // We are now in the Thread : let's create the context, share it and make it current
      int threadId = getThreadNumber() + 1;
      s_pCurRenderer->initThreadLocalVars(threadId);
    }
  };
  //---------------------------------------------------------------------
  static SetThreadLocalVars setThreadLocalVars;
  for(unsigned int i = 0; i < g_mainThreadPool->getThreadCount(); i++)
  {
    // we will wait for the tasks to be done before continuing (Call)
    setThreadLocalVars.Call(&g_mainThreadPool->getThreadWorker(i)->GetTaskQueue());
  }
#endif
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void releaseThreadLocalVars()
{
#ifdef USEWORKERS
  NXPROFILEFUNC(__FUNCTION__);
  //---------------------------------------------------------------------
  // Invoke a Task on all the threads to setup some thread-local variable
  //
  class ReleaseThreadLocalVars : public TaskSyncCall
  {
  public:
    void Invoke()
    {  // We are now in the Thread : let's create the context, share it and make it current
      s_pCurRenderer->releaseThreadLocalVars();
    }
  };
  //---------------------------------------------------------------------
  static ReleaseThreadLocalVars releaseThreadLocalVars;
  for(unsigned int i = 0; i < g_mainThreadPool->getThreadCount(); i++)
  {
    // we will wait for the tasks to be done before continuing (Call)
    releaseThreadLocalVars.Call(&g_mainThreadPool->getThreadWorker(i)->GetTaskQueue());
  }
#endif
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int refreshCmdBuffers()
{
  int totalTasks = 0;
#ifdef USEWORKERS
  //---------------------------------------------
  // Worker for command-buffer update
  //
  class TskUpdateCommandBuffer : public TaskBase
  {
  private:
    int m;
    int cmdBufIdx;
    int mstart, mend;

  public:
    //TskUpdateCommandBuffer() {  }
    TskUpdateCommandBuffer(int modelIndex, int cIdx, int ms, int me)
    {
      m         = modelIndex;
      mstart    = ms;
      mend      = me;
      cmdBufIdx = cIdx;
    }
    virtual void Invoke()
    {
      s_pCurRenderer->buildCmdBufferModel(g_bk3dModels[m], cmdBufIdx, mstart, mend);
      g_evt_cmdbuf[m * MAXCMDBUFFERS + cmdBufIdx].Set();
    }
    //void Done() { /* FIXME: prevent delete to happen */ }
  };
  //---------------------------------------------
  //---------------------------------------------
  if(g_useWorkers)
  {
    for(int m = 0; m < g_bk3dModels.size(); m++)
    {
      int meshgroupsize = g_bk3dModels[m]->m_meshFile->pMeshes->n / g_numCmdBuffers;
      int i             = 0;
      for(int n = 0; n < g_bk3dModels[m]->m_meshFile->pMeshes->n; n += meshgroupsize, i++)
      {
        // worker will be deleted by the default method Done()
        if((i + 1) >= g_numCmdBuffers)
        {
          TskUpdateCommandBuffer* tskUpdateCommandBuffer =
              new TskUpdateCommandBuffer(m, i, n, g_bk3dModels[m]->m_meshFile->pMeshes->n);
          g_mainThreadPool->pushTask(tskUpdateCommandBuffer);
          totalTasks++;
          break;
        }
        TskUpdateCommandBuffer* tskUpdateCommandBuffer = new TskUpdateCommandBuffer(m, i, n, n + meshgroupsize);
        g_mainThreadPool->pushTask(tskUpdateCommandBuffer);
        totalTasks++;
      }
    }
  }  //if(g_useWorkers)
  else
#endif
  {
    for(int m = 0; m < g_bk3dModels.size(); m++)
    {
      int meshgroupsize = 1 + (g_bk3dModels[m]->m_meshFile->pMeshes->n / g_numCmdBuffers);
      int i             = 0;
      for(int n = 0; n < g_bk3dModels[m]->m_meshFile->pMeshes->n; n += meshgroupsize, i++)
      {
        if((i + 1) >= g_numCmdBuffers)
        {
          s_pCurRenderer->buildCmdBufferModel(g_bk3dModels[m], i, n, g_bk3dModels[m]->m_meshFile->pMeshes->n);
          break;
        }
        s_pCurRenderer->buildCmdBufferModel(g_bk3dModels[m], i, n, n + meshgroupsize);
      }
    }
  }  //if(g_useWorkers)
  return totalTasks;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
#ifdef USEWORKERS
void waitRefreshCmdBuffersDone(int totalTasks)
{
  //PROFILE_SECTION("waitRefreshCmdBuffersDone");
  if(g_useWorkers)
  {
    for(int m = 0; m < totalTasks; m++)
    {
      //
      // Wait for the worker to be done with command-buffer creation
      //
      g_evt_cmdbuf[m].WaitOnEvent();
      g_evt_cmdbuf[m].Reset();
    }
  }
}
#endif
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void MyWindow::onWindowRefresh()
{
  if(!s_pCurRenderer)
    return;

  if(g_bRefreshCmdBuffers || (g_bRefreshCmdBuffersCounter > 0))
    resetCommandBuffersPool();
  AppWindowCameraInertia::onWindowRefresh();
  if(!s_pCurRenderer->valid())
  {
    glClearColor(0.5, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    m_contextWindowGL.swapBuffers();
    return;
  }
  float dt = (float)m_realtime.getFrameDT();
  //
  // Simple camera change for animation
  //
  if(s_bCameraAnim)
  {
    if(s_cameraAnimItems == 0)
    {
      LOGE("NO Animation loaded (-i <file>)\n");
      s_bCameraAnim = false;
    }
    s_cameraAnimIntervals -= dt;
    if((m_camera.eyeD <= 0.01 /*m_camera.epsilon*/) && (m_camera.focusD <= 0.01 /*m_camera.epsilon*/)
       && (s_cameraAnimIntervals <= 0.0))
    {
      //LOGI("Anim step %d\n", s_cameraAnimItem);
      s_cameraAnimIntervals = s_cameraAnim[s_cameraAnimItem].sleep;
      m_camera.look_at(s_cameraAnim[s_cameraAnimItem].eye, s_cameraAnim[s_cameraAnimItem].focus);
      m_camera.tau = 0.4f;
      s_cameraAnimItem++;
      if(s_cameraAnimItem >= s_cameraAnimItems)
        s_cameraAnimItem = 0;
    }
  }
  //
  // render the scene
  //
  g_profiler.beginFrame();
  {
    PROFILE_SECTION("frame");

    mat4f mW;
    // somehow a hack for the CAD models to be back on better scale and orientation
    mW.identity();
    if(!g_bk3dModels.empty())
    {
      mW.rotate(-nv_to_rad * 90.0f, vec3f(1, 0, 0));
      mW.translate(-g_bk3dModels[0]->m_posOffset);
      mW.scale(g_bk3dModels[0]->m_scale);
    }
    {
      //
      // This might initiate a primary command-buffer (in Vulkan renderer)
      //
      {
        s_pCurRenderer->displayStart(mW, m_camera, m_projection, m_timingGlitch);
      }
      //
      // kick a bunch of workers to update the command-buffers
      // AFTER displayStart: because displayStart alternate the ping-pong index needed below
      //
      int totalTasks = 0;
      if(g_bDisplayObject)
      {
        PROFILE_SECTION("refresh CmdBuffers");
        if(g_bRefreshCmdBuffers || (g_bRefreshCmdBuffersCounter > 0))
        {
          totalTasks = refreshCmdBuffers();
          if(g_bRefreshCmdBuffersCounter > 0)
            g_bRefreshCmdBuffersCounter--;
#ifdef USEWORKERS
          // NOTE: we could do things in the meantime, rather than waiting...
          waitRefreshCmdBuffersDone(totalTasks);
#endif
          for(int m = 0; m < g_bk3dModels.size(); m++)
          {
            // set the # of command buffers used to display the model and possibly do some consolidation
            s_pCurRenderer->consolidateCmdBuffersModel(g_bk3dModels[m], g_numCmdBuffers);
          }
        }
      }  //if(g_bDisplayObject)
      //
      // Grid floor: might append a sub-command to the primary command-buffer
      //
      if(g_bDisplayGrid)
      {
        s_pCurRenderer->displayGrid(m_camera, m_projection);
      }
      //
      // Display Meshes: might append a sub-commands to the primary command-buffer
      //
      unsigned short topo = (g_bTopologyLines ? 1 : 0) | (g_bTopologylinestrip ? 2 : 0) | (g_bTopologytriangles ? 4 : 0)
                            | (g_bTopologytristrips ? 8 : 0) | (g_bTopologytrifans ? 0x10 : 0) | (g_bUnsortedPrims ? 0x20 : 0);
      if(g_bDisplayObject)
      {
        for(int m = 0; m < g_bk3dModels.size(); m++)
        {
          //
          // Then use it
          //
          s_pCurRenderer->displayBk3dModel(g_bk3dModels[m], m_camera.m4_view, m_projection, topo);
        }
      }
      //
      // This might finalize a primary command-buffer (Vulkan) referring to sub-commands (create in display..() )
      //
      {
        s_pCurRenderer->displayEnd();
      }
      //
      // copy FBO to backbuffer
      //
      s_pCurRenderer->blitToBackbuffer();
    }
    if(1 /*useUI*/)
    {
      int width  = getWidth();
      int height = getHeight();
      processUI(width, height, dt);
      ImDrawData* imguiDrawData;
      ImGui::Render();
      imguiDrawData = ImGui::GetDrawData();
      ImGui::RenderDrawDataGL(imguiDrawData);
      ImGui::EndFrame();
    }
  }  //PROFILE_SECTION("frame");
  m_contextWindowGL.swapBuffers();
  g_profiler.endFrame();
}
//------------------------------------------------------------------------------
// Main initialization point
//------------------------------------------------------------------------------
void readConfigFile(const char* fname)
{
  //  FILE *fp = fopen(fname, "r");
  //  if(!fp)
  //  {
  //      std::string modelPathName;
  //      modelPathName = std::string(PROJECT_RELDIRECTORY) + std::string(fname);
  //      fp = fopen(modelPathName.c_str(), "r");
  //      if(!fp)
  //      {
  //          modelPathName = std::string(PROJECT_ABSDIRECTORY) + std::string(fname);
  //          fp = fopen(modelPathName.c_str(), "r");
  //          if(!fp) {
  //              LOGE("Couldn't Load %s\n", fname);
  //              return;
  //          }
  //      }
  //  }
  //  int nModels = 0;
  //  int res = 0;
  //  // Models and position/scale
  //  fscanf(fp, "%d", &nModels);
  //  for(int i=0; i<nModels; i++)
  //  {
  //      char name[200];
  //      vec3f pos;
  //      float scale;
  //      res = fscanf(fp, "%s\n", &name);
  //      if(res != 1) { LOGE("Error during parsing\n"); return; }
  //      res = fscanf(fp, "%f %f %f %f\n", &pos.x, &pos.y, &pos.z, &scale);
  //      if(res != 4) { LOGE("Error during parsing\n"); return; }
  //      LOGI("Load Model set to %s\n", name);
  //g_bk3dModels.push_back(new Bk3dModel(name, &pos, &scale));
  //  }
  //  // camera movement
  //  int nCameraPos = 0;
  //  res = fscanf(fp, "%d", &nCameraPos);
  //  if(res == 1)
  //  {
  //      for(int i=0; i<nCameraPos; i++)
  //      {
  //          CameraAnim cam;
  //          res = fscanf(fp, "%f %f %f %f %f %f %f\n"
  //              , &cam.eye.x, &cam.eye.y, &cam.eye.z
  //              , &cam.focus.x, &cam.focus.y, &cam.focus.z
  //              , &cam.sleep);
  //          if(res != 7) { LOGE("Error during parsing\n"); return; }
  //          s_cameraAnim.push_back(cam);
  //      }
  //  }
  //  fclose(fp);
}
//------------------------------------------------------------------------------
// Main initialization point
//------------------------------------------------------------------------------
int main(int argc, const char** argv)
{
  NVPSystem system(argv[0], PROJECT_NAME);

  // you can create more than only one
  static MyWindow myWindow;

  // -------------------------------
  // Basic OpenGL settings
  //
  nvgl::ContextWindowCreateInfo context(4,      //major;
                               3,      //minor;
                               false,  //core;
                               1,      //MSAA;
                               24,     //depth bits
                               8,      //stencil bits
                               false,  //debug;
                               false,  //robust;
                               false,  //forward;
                               false,  //stereo
                               NULL    //share;
  );

  // -------------------------------
  // Create the window
  //
  if(!myWindow.open(0, 0, 1280, 720, "gl_vk_bk3dthreaded", context))
  {
    LOGE("Failed to initialize the sample\n");
    return EXIT_FAILURE;
  }

  // -------------------------------
  // Parse arguments/options
  //
  for(int i = 1; i < argc; i++)
  {
    if(argv[i][0] != '-')
    {
      const char* name = argv[i];
      LOGI("Load Model set to %s\n", name);
      g_bk3dModels.push_back(new Bk3dModel(name));
      continue;
    }
    if(strlen(argv[i]) <= 1)
      continue;
    switch(argv[i][1])
    {
      case 'm':
        if(i == argc - 1)
          return EXIT_FAILURE;
        {
          const char* name = argv[++i];
          LOGI("Load Model set to %s\n", name);
          g_bk3dModels.push_back(new Bk3dModel(name));
        }
        break;
      case 'o':
        g_bDisplayObject = atoi(argv[++i]) ? true : false;
        LOGI("g_bDisplayObject set to %s\n", g_bDisplayObject ? "true" : "false");
        break;
      case 'g':
        g_bDisplayGrid = atoi(argv[++i]) ? true : false;
        LOGI("g_bDisplayGrid set to %s\n", g_bDisplayGrid ? "true" : "false");
        break;
      case 's':
        s_bStats = atoi(argv[++i]) ? true : false;
        LOGI("s_bStats set to %s\n", s_bStats ? "true" : "false");
        break;
      case 'a':
        s_bCameraAnim = atoi(argv[++i]) ? true : false;
        LOGI("s_bCameraAnim set to %s\n", s_bCameraAnim ? "true" : "false");
        break;
      case 'q':
        g_MSAA = atoi(argv[++i]);
        LOGI("g_MSAA set to %d\n", g_MSAA);
        break;
      case 'i':
      {
        const char* name = argv[++i];
        LOGI("Load Model set to %s\n", name);
        readConfigFile(name);
      }
      break;
      case 'd':
        break;
      default:
        LOGE("Wrong command-line\n");
      case 'h':
        LOGI(s_sampleHelpCmdLine);
        break;
    }
  }
  Renderer* renderer = g_renderers[s_curRenderer];
  if(renderer->initGraphics(myWindow.getWidth(), myWindow.getHeight(), g_MSAA) == false)
    return 1;

  s_pCurRenderer = renderer;

// -------------------------------
// Model init
//
#ifdef NOGZLIB
#define MODELNAME "SubMarine_134.bk3d"  //"Smobby_134.bk3d"
#else
//#define MODELNAME "Smobby_134.bk3d.gz"
#define MODELNAME "SubMarine_134.bk3d.gz"  //"Smobby_134.bk3d.gz"
//#   define MODELNAME "Driveline_v134.bk3d.gz"
#endif
  if(g_bk3dModels.empty())
  {
    LOGI("Load default Model" MODELNAME "\n");
    g_bk3dModels.push_back(new Bk3dModel(MODELNAME));
  }
  else
  {
    // if the model is NOT the submarine, let's cancel the dedicated animation
    s_bCameraAnim = false;
  }
  for(int m = 0; m < g_bk3dModels.size(); m++)
  {
    if(g_bk3dModels[m]->loadModel() == false)
      return 1;
    s_pCurRenderer->attachModel(g_bk3dModels[m]);
    // TODO: break-down what is inside and issue Task-workers
    s_pCurRenderer->initResourcesModel(g_bk3dModels[m]);
  }

// -------------------------------
// Initialize what is needed for Multithreading
//
#ifdef USEWORKERS
  initThreads();
  // the current renderer will store things local to each thread (in TLS):
  initThreadLocalVars();
#endif
  myWindow.m_contextWindowGL.makeContextCurrent();
  myWindow.m_contextWindowGL.swapInterval(0);
  //
  // reshape will setup the first windows size and related stuff: main command-buffer, for example
  //
  myWindow.onWindowResize();
  // -------------------------------
  // Message pump loop
  //
  while (myWindow.pollEvents())
  {
#ifdef USEWORKERS
    // manage possible tasks, queued for this main thread
    while(g_mainThreadQueue->pollTask())
    {
      //LOGI("More than 1 task...\n");
    }
#endif

    if(myWindow.idle())
      myWindow.onWindowRefresh();

    if(myWindow.m_guiRegistry.checkValueChange(SCALAR_NCMDBUF))
    {
      destroyCommandBuffers(true);
      //g_numCmdBuffers
      g_bRefreshCmdBuffersCounter = 2;
    }
#ifdef USEWORKERS
    if(myWindow.m_guiRegistry.checkValueChange(CHECK_WORKERS))
    {
      // changing the method: let's first destroy command buffers (via threads or not)
      destroyCommandBuffers(true);
      //g_useWorkers
      g_bRefreshCmdBuffersCounter = 2;
    }
#endif
    if(myWindow.m_guiRegistry.checkValueChange(COMBO_RENDERER))
    {
      s_pCurRenderer->waitForGPUIdle();
      releaseThreadLocalVars();
      s_pCurRenderer->terminateGraphics();
      g_profiler.reset(1);
      s_pCurRenderer = g_renderers[s_curRenderer];  // s_curRenderer setup by ImGui
      s_pCurRenderer->initGraphics(myWindow.getWidth(), myWindow.getHeight(), g_MSAA);
      initThreadLocalVars();
      myWindow.onWindowResize(myWindow.getWidth(), myWindow.getHeight());
      for(int m = 0; m < g_bk3dModels.size(); m++)
      {
        s_pCurRenderer->attachModel(g_bk3dModels[m]);
        s_pCurRenderer->initResourcesModel(g_bk3dModels[m]);
      }
      g_bRefreshCmdBuffersCounter = 2;
    }
    if(myWindow.m_guiRegistry.checkValueChange(COMBO_MSAA))
    {
      //g_MSAA setup by ImGui
      // involves some changes at the source of initialization... re-create all
      s_pCurRenderer->waitForGPUIdle();
      releaseThreadLocalVars();
      s_pCurRenderer->terminateGraphics();
      g_profiler.reset(1);
      s_pCurRenderer->initGraphics(myWindow.getWidth(), myWindow.getHeight(), g_MSAA);
      initThreadLocalVars();
      s_pCurRenderer->buildPrimaryCmdBuffer();
      for(int m = 0; m < g_bk3dModels.size(); m++)
      {
        s_pCurRenderer->attachModel(g_bk3dModels[m]);
        s_pCurRenderer->initResourcesModel(g_bk3dModels[m]);
      }
      g_bRefreshCmdBuffersCounter = 2;
    }
    if(0)  //myWindow.m_guiRegistry.checkValueChange(BUTTON_PRINT))
    {
      if(s_curObject < g_bk3dModels.size())
        g_bk3dModels[s_curObject]->printPosition();
    }
    if(0)  //myWindow.m_guiRegistry.checkValueChange(SCALAR_OBJ))
    {
      //if (g_bk3dModels.size() > ...)
      {
        //LOGI("Object %d %s now current\n", (int)v, g_bk3dModels[(int)v]->m_name.c_str());
        //Get("CURX")...
      }
    }
  }  // while(MyWindow::sysPollEvents(false) )
// -------------------------------
// Terminate
//

  releaseThreadLocalVars();
  s_pCurRenderer->terminateGraphics();


  for(int i = 0; i < g_bk3dModels.size(); i++)
  {
    delete g_bk3dModels[i];
  }
  g_bk3dModels.clear();


#ifdef USEWORKERS
  //
  // terminate the threads at the very end (terminateGraphics() could still need it... for TLS)
  //
  terminateThreads();
#endif
  
  myWindow.m_contextWindowGL.deinit();
  return EXIT_SUCCESS;
}
