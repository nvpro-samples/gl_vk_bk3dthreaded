/* Copyright (c) 2014-2018, NVIDIA CORPORATION. All rights reserved.
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

#include <array>
#include <assert.h>
#include <cstdio>   // printf, fprintf
#include <cstdlib>  // abort
#include <nvh/nvprint.hpp>
#include <set>
#include <vector>

#include <vulkan/vulkan.h>

#include "window_surface_vk.hpp"
#include <nvvk/error_vk.hpp>
#include <nvvk/images_vk.hpp>
#include <nvvk/renderpasses_vk.hpp>
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#else
#define GLFW_EXPOSE_NATIVE_X11
#endif
#include <GLFW/glfw3native.h>

#include "dedicated_image.h"
#include <nvvk/structs_vk.hpp>

bool WindowSurface::init(nvvk::Context* pContext, NVPWindow* pWin, int MSAA)
{
  switch(MSAA)
  {
    case 0:
    case 1:
      m_samples = VK_SAMPLE_COUNT_1_BIT;
      break;
    case 2:
      m_samples = VK_SAMPLE_COUNT_2_BIT;
      break;
    case 4:
      m_samples = VK_SAMPLE_COUNT_4_BIT;
      break;
    case 8:
      m_samples = VK_SAMPLE_COUNT_8_BIT;
      break;
    case 16:
      m_samples = VK_SAMPLE_COUNT_16_BIT;
      break;
    case 32:
      m_samples = VK_SAMPLE_COUNT_32_BIT;
      break;
    case 64:
      m_samples = VK_SAMPLE_COUNT_64_BIT;
      break;
    default:
      return false;
  }
  fb_width   = pWin->getWidth();
  fb_height  = pWin->getHeight();
  m_pContext = pContext;

  // Construct the surface description:
  VkResult result;
#ifdef _WIN32
  VkWin32SurfaceCreateInfoKHR createInfo = {};
  createInfo.sType                       = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
  createInfo.pNext                       = NULL;
  HINSTANCE hInstance                    = GetModuleHandle(NULL);
  createInfo.hinstance                   = hInstance;
  createInfo.hwnd                        = glfwGetWin32Window(pWin->m_internal);
  result = vkCreateWin32SurfaceKHR(pContext->m_instance, &createInfo, nullptr, &m_surface);
#else   // _WIN32
  result = glfwCreateWindowSurface(pContext->m_instance, pWin->m_internal, NULL, &m_surface);
#endif  // _WIN32
  assert(result == VK_SUCCESS);

  pContext->setGCTQueueWithPresent(m_surface);

  m_swapChain.init(pContext->m_device, pContext->m_physicalDevice, pContext->m_queueGCT, pContext->m_queueGCT.familyIndex, m_surface);

  resize(fb_width, fb_height);
  VkDevice device = m_pContext->m_device;
  VkResult err;
  assert(m_swapChain.getImageCount() <= VK_MAX_QUEUED_FRAMES);
  for(uint32_t i = 0; i < m_swapChain.getImageCount(); i++)
  {
    {
      VkCommandPoolCreateInfo info = {};
      info.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
      info.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
      info.queueFamilyIndex        = m_pContext->m_queueGCT;
      err                          = vkCreateCommandPool(device, &info, m_allocator, &m_commandPool[i]);
      nvvk::checkResult(err);
    }
    {
      VkCommandBufferAllocateInfo info = {};
      info.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      info.commandPool                 = m_commandPool[i];
      info.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      info.commandBufferCount          = 1;
      err                              = vkAllocateCommandBuffers(device, &info, &m_commandBuffer[i]);
      nvvk::checkResult(err);
    }
    {
      VkFenceCreateInfo info = {};
      info.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
      info.flags             = VK_FENCE_CREATE_SIGNALED_BIT;
      err                    = vkCreateFence(device, &info, m_allocator, &m_fence[i]);
      nvvk::checkResult(err);
    }
    //{
    //  VkSemaphoreCreateInfo info = {};
    //  info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    //  err = vkCreateSemaphore(device, &info, m_allocator, &m_presentCompleteSemaphore[i]);
    //  check_vk_result(err);
    //  err = vkCreateSemaphore(m_device, &info, m_allocator, &m_renderCompleteSemaphore[i]);
    //  check_vk_result(err);
    //}
  }
  return true;
}
void WindowSurface::deinit()
{
  //////////////////////////////////////////////
#if 1
  vkDeviceWaitIdle(m_pContext->m_device);
//    m_pDeviceInstance->deinit();
#endif
  //////////////////////////////////////////////
  VkDevice device = m_pContext->m_device;
  if(device == VK_NULL_HANDLE)
    return;
  for(uint32_t i = 0; i < m_swapChain.getImageCount(); i++)
  {
    if(m_framebuffer[i])
    {
      vkDestroyFramebuffer(device, m_framebuffer[i], m_allocator);
    }
  }
  for(uint32_t i = 0; i < m_swapChain.getImageCount(); i++)
  {
    vkDestroyFence(device, m_fence[i], m_allocator);
    m_fence[i] = VK_NULL_HANDLE;
    //vkFreeCommandBuffers(device, m_commandPool[i], 1, &m_commandBuffer[i]);
    vkDestroyCommandPool(device, m_commandPool[i], m_allocator);
    m_commandPool[i] = VK_NULL_HANDLE;
    //vkDestroySemaphore(device, m_presentCompleteSemaphore[i], m_allocator);
    //vkDestroySemaphore(device, m_renderCompleteSemaphore[i], m_allocator);
  }
  vkDestroyRenderPass(device, m_renderPass, m_allocator);
  m_renderPass = VK_NULL_HANDLE;

  if(m_depthImageView)
    vkDestroyImageView(device, m_depthImageView, nullptr);
  if(m_depthImage)
    vkDestroyImage(device, m_depthImage, nullptr);
  if(m_depthImageMemory)
    vkFreeMemory(device, m_depthImageMemory, nullptr);
  if(m_msaaColorImageView)
    vkDestroyImageView(device, m_msaaColorImageView, nullptr);
  if(m_msaaColorImage)
    vkDestroyImage(device, m_msaaColorImage, nullptr);
  if(m_msaaColorImageMemory)
    vkFreeMemory(device, m_msaaColorImageMemory, nullptr);
  m_depthImageView       = VK_NULL_HANDLE;
  m_depthImage           = VK_NULL_HANDLE;
  m_depthImageMemory     = VK_NULL_HANDLE;
  m_msaaColorImageView   = VK_NULL_HANDLE;
  m_msaaColorImage       = VK_NULL_HANDLE;
  m_msaaColorImageMemory = VK_NULL_HANDLE;

  m_swapChain.deinit();
  vkDestroySurfaceKHR(m_pContext->m_instance, m_surface, m_allocator);
}

//--------------------------------------------------------------------------------------------------
//
//
bool WindowSurface::resize(int w, int h)
{
  VkResult err;
  assert(m_pContext->m_device != NULL);
  if(m_pContext->m_device == NULL)
    return false;
  VkDevice device = m_pContext->m_device;

  err = vkDeviceWaitIdle(device);
  nvvk::checkResult(err);

  m_swapChain.update(w, h, m_swapVsync);

  for(uint32_t i = 0; i < m_swapChain.getImageCount(); i++)
  {
    if(m_framebuffer[i])
    {
      vkDestroyFramebuffer(device, m_framebuffer[i], m_allocator);
    }
  }

  if(m_renderPass)
  {
    vkDestroyRenderPass(device, m_renderPass, m_allocator);
  }

  fb_width  = w;
  fb_height = h;

  if(m_depthImageView)
    vkDestroyImageView(device, m_depthImageView, nullptr);
  if(m_depthImage)
    vkDestroyImage(device, m_depthImage, nullptr);
  if(m_depthImageMemory)
    vkFreeMemory(device, m_depthImageMemory, nullptr);
  if(m_msaaColorImageView)
    vkDestroyImageView(device, m_msaaColorImageView, nullptr);
  if(m_msaaColorImage)
    vkDestroyImage(device, m_msaaColorImage, nullptr);
  if(m_msaaColorImageMemory)
    vkFreeMemory(device, m_msaaColorImageMemory, nullptr);
  m_depthImageView       = VK_NULL_HANDLE;
  m_depthImage           = VK_NULL_HANDLE;
  m_depthImageMemory     = VK_NULL_HANDLE;
  m_msaaColorImageView   = VK_NULL_HANDLE;
  m_msaaColorImage       = VK_NULL_HANDLE;
  m_msaaColorImageMemory = VK_NULL_HANDLE;

  createDepthResources();
  createMSAAColorResources();

  // Create the Render Pass:
  createRenderPass();
  // Create Framebuffer:
  createFrameBuffer();

  return true;
}

//--------------------------------------------------------------------------------------------------
//
//
void WindowSurface::acquire()
{
  if(!m_swapChain.acquire())
  {
    LOGE("error: vulkan swapchain acquire failed\n");
    assert(!"error: vulkan swapchain acquire failed");
    //exit(-1);
  }
}
//--------------------------------------------------------------------------------------------------
//
//
VkCommandBuffer WindowSurface::beginCommandBuffer(VkCommandBufferUsageFlags flags)
{
  VkResult err;
  VkDevice device     = m_pContext->m_device;
  int      frameIndex = m_swapChain.getActiveImageIndex();
  m_curCommandBuffer  = m_commandBuffer[frameIndex];
  m_curFence          = m_fence[frameIndex];
  for(;;)
  {
    err = vkWaitForFences(device, 1, &m_curFence, VK_TRUE, 100);
    if(err == VK_SUCCESS)
    {
      break;
    }
    if(err == VK_TIMEOUT)
    {
      continue;
    }
    nvvk::checkResult(err);
  }
  {
    VkCommandBufferBeginInfo info = {};
    info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.flags                    = flags;
    info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    err = vkBeginCommandBuffer(m_curCommandBuffer, &info);
    nvvk::checkResult(err);
  }
  return m_curCommandBuffer;
}
//--------------------------------------------------------------------------------------------------
//
//
void WindowSurface::beginRenderPass(VkSubpassContents contents)
{
  VkRenderPassBeginInfo info       = {};
  int                   frameIndex = m_swapChain.getActiveImageIndex();
  info.sType                       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  info.renderPass                  = m_renderPass;
  info.framebuffer                 = m_framebuffer[frameIndex];
  info.renderArea.extent.width     = fb_width;
  info.renderArea.extent.height    = fb_height;
  std::vector<VkClearValue> clearValues;
  VkClearValue              c;
  c.color = m_clearColor;
  clearValues.push_back(c);
  c.depthStencil = m_clearDST;
  clearValues.push_back(c);
  if(m_samples != VK_SAMPLE_COUNT_1_BIT)
  {
    clearValues.push_back(c);
  }

  info.clearValueCount = static_cast<uint32_t>(clearValues.size());
  info.pClearValues    = clearValues.data();
  vkCmdBeginRenderPass(m_curCommandBuffer, &info, contents);
}
//--------------------------------------------------------------------------------------------------
//
//
void WindowSurface::nextSubPassForOverlay(VkSubpassContents contents)
{
  vkCmdNextSubpass(m_curCommandBuffer, contents);
}
//--------------------------------------------------------------------------------------------------
//
//
void WindowSurface::endRenderPass()
{
  vkCmdEndRenderPass(m_curCommandBuffer);
}
//--------------------------------------------------------------------------------------------------
//
//
void WindowSurface::endCommandBuffer()
{
  VkResult err;
  err = vkEndCommandBuffer(m_curCommandBuffer);
  nvvk::checkResult(err);
}
//--------------------------------------------------------------------------------------------------
//
//
void WindowSurface::submit()
{
  VkResult err;
  VkDevice device = m_pContext->m_device;
  {
    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo         info       = {};
    info.sType                      = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    info.waitSemaphoreCount         = 1;
    VkSemaphore waitSemaphores[1]   = {m_swapChain.getActiveReadSemaphore()};
    info.pWaitSemaphores            = waitSemaphores;  // &m_presentCompleteSemaphore[frameIndex];
    info.pWaitDstStageMask          = &wait_stage;
    info.commandBufferCount         = 1;
    info.pCommandBuffers            = &m_curCommandBuffer;
    info.signalSemaphoreCount       = 1;
    VkSemaphore signalSemaphores[1] = {m_swapChain.getActiveWrittenSemaphore()};
    info.pSignalSemaphores          = signalSemaphores;  //&m_renderCompleteSemaphore[frameIndex];

    err = vkResetFences(device, 1, &m_curFence);
    nvvk::checkResult(err);
    err = vkQueueSubmit(m_pContext->m_queueGCT, 1, &info, m_curFence);
    nvvk::checkResult(err);
  }
  m_curCommandBuffer = VK_NULL_HANDLE;
  m_curFence         = VK_NULL_HANDLE;
}
//--------------------------------------------------------------------------------------------------
//
//
//void WindowSurface::endRPassCBufferSubmitAndPresent()
//{
//VkResult err;
//VkDevice device = m_pDeviceInstance->device;
//vkCmdEndRenderPass(m_curCommandBuffer);
//{
//  VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
//  VkSubmitInfo info = {};
//  info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
//  info.waitSemaphoreCount = 1;
//  VkSemaphore waitSemaphores[1] = { m_swapChain.getActiveReadSemaphore() };
//  info.pWaitSemaphores = waitSemaphores;// &m_presentCompleteSemaphore[frameIndex];
//  info.pWaitDstStageMask = &wait_stage;
//  info.commandBufferCount = 1;
//  info.pCommandBuffers = &m_curCommandBuffer;
//  info.signalSemaphoreCount = 1;
//  VkSemaphore signalSemaphores[1] = { m_swapChain.getActiveWrittenSemaphore() };
//  info.pSignalSemaphores = signalSemaphores; //&m_renderCompleteSemaphore[frameIndex];

//  err = vkEndCommandBuffer(m_curCommandBuffer);
//  check_vk_result(err);
//  err = vkResetFences(device, 1, &m_curFence);
//  check_vk_result(err);
//  err = vkQueueSubmit(m_pDeviceInstance->queue_GCTB, 1, &info, m_curFence);
//  check_vk_result(err);
//  m_swapChain.present(m_pDeviceInstance->queue_GCTB);
//  m_curCommandBuffer = VK_NULL_HANDLE;
//  m_curFence = VK_NULL_HANDLE;
//}
//}

//--------------------------------------------------------------------------------------------------
//
//
void WindowSurface::createFrameBuffer()
{
  VkResult err;
  VkDevice device = m_pContext->m_device;

  int         numAttachments  = 2;
  int         backbufferIndex = 0;
  VkImageView attachments[3]  = {m_swapChain.getImageView(0), m_depthImageView, VK_NULL_HANDLE};
  if(m_samples != VK_SAMPLE_COUNT_1_BIT)
  {
    numAttachments  = 3;
    backbufferIndex = 2;
    attachments[0]  = m_msaaColorImageView;
    attachments[2]  = m_swapChain.getImageView(0);
  }
  ///  VkImageView attachment[1];
  VkFramebufferCreateInfo info = {};
  info.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  info.renderPass              = m_renderPass;
  info.attachmentCount         = numAttachments;
  info.pAttachments            = attachments;
  info.width                   = fb_width;
  info.height                  = fb_height;
  info.layers                  = 1;
  for(uint32_t i = 0; i < m_swapChain.getImageCount(); i++)
  {
    attachments[backbufferIndex] = m_swapChain.getImageView(i);
    err                          = vkCreateFramebuffer(device, &info, m_allocator, &m_framebuffer[i]);
    nvvk::checkResult(err);
  }
}

//--------------------------------------------------------------------------------------------------
//
//
void WindowSurface::createRenderPass()
{
  VkResult err;
  VkDevice device = m_pContext->m_device;

  int numAttachments = 2;

  VkAttachmentDescription colorAttachmentDesc = {};
  colorAttachmentDesc.format                  = m_swapChain.getFormat();
  colorAttachmentDesc.samples                 = m_samples;
  colorAttachmentDesc.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachmentDesc.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachmentDesc.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachmentDesc.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachmentDesc.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachmentDesc.finalLayout             = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef = {};
  colorAttachmentRef.attachment            = 0;
  colorAttachmentRef.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentDescription depthAttachmentDesc = {};
  depthAttachmentDesc.format                  = nvvk::findDepthFormat(this->m_pContext->m_physicalDevice);
  depthAttachmentDesc.samples                 = m_samples;
  depthAttachmentDesc.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachmentDesc.storeOp                 = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachmentDesc.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depthAttachmentDesc.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachmentDesc.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAttachmentDesc.finalLayout             = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depthAttachmentRef = {};
  depthAttachmentRef.attachment            = 1;
  depthAttachmentRef.layout                = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpasses[2]    = {};
  subpasses[0].pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpasses[0].colorAttachmentCount    = 1;
  subpasses[0].pColorAttachments       = &colorAttachmentRef;
  subpasses[0].pDepthStencilAttachment = &depthAttachmentRef;

  subpasses[1].pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpasses[1].colorAttachmentCount    = 1;
  subpasses[1].pColorAttachments       = &colorAttachmentRef;
  subpasses[1].pDepthStencilAttachment = NULL;

  // Possible case of MSAA
  VkAttachmentDescription noMSAAcolorAttachmentDesc = {};
  noMSAAcolorAttachmentDesc.format                  = m_swapChain.getFormat();
  noMSAAcolorAttachmentDesc.samples                 = VK_SAMPLE_COUNT_1_BIT;
  noMSAAcolorAttachmentDesc.loadOp                  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;  // VK_ATTACHMENT_LOAD_OP_LOAD
  noMSAAcolorAttachmentDesc.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
  noMSAAcolorAttachmentDesc.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  noMSAAcolorAttachmentDesc.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  noMSAAcolorAttachmentDesc.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
  noMSAAcolorAttachmentDesc.finalLayout             = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorResolveAttachmentRef = {};
  colorResolveAttachmentRef.attachment            = 2;
  colorResolveAttachmentRef.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  subpasses[0].pResolveAttachments = NULL;
  if(m_samples != VK_SAMPLE_COUNT_1_BIT)
  {
    numAttachments                   = 3;
    subpasses[0].pResolveAttachments = &colorResolveAttachmentRef;
    subpasses[1].pColorAttachments   = &colorResolveAttachmentRef;
  }

  int                 dependencyCount = 1;
  VkSubpassDependency dependencies[2] = {};
  dependencies[0].srcSubpass          = VK_SUBPASS_EXTERNAL;
  dependencies[0].dstSubpass          = 0;
  dependencies[0].srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[0].dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[0].srcAccessMask       = 0;
  dependencies[0].dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  if(m_samples == VK_SAMPLE_COUNT_1_BIT)
  {
    dependencyCount               = 2;
    dependencies[1].srcSubpass    = 0;
    dependencies[1].dstSubpass    = 1;
    dependencies[1].srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  }

  VkAttachmentDescription attachments[3] = {colorAttachmentDesc, depthAttachmentDesc, noMSAAcolorAttachmentDesc};
  VkRenderPassCreateInfo  renderPassInfo = {};
  renderPassInfo.sType                   = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount         = numAttachments;
  renderPassInfo.pAttachments            = attachments;
  renderPassInfo.subpassCount            = 2;  // (m_samples != VK_SAMPLE_COUNT_1_BIT) ? 2 : 1;commented because if the previous sub-pass is VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS, then we must have a dedicated one for VK_SUBPASS_CONTENTS_INLINE
  renderPassInfo.pSubpasses              = subpasses;
  renderPassInfo.dependencyCount         = dependencyCount;
  renderPassInfo.pDependencies           = dependencies;
  err                                    = vkCreateRenderPass(device, &renderPassInfo, m_allocator, &m_renderPass);
  nvvk::checkResult(err);
}

//--------------------------------------------------------------------------------------------------
//
//
bool WindowSurface::hasStencilComponent(VkFormat format)
{
  return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

//--------------------------------------------------------------------------------------------------
//
//
void WindowSurface::createDepthResources()
{
  VkFormat          depthFormat = nvvk::findDepthFormat(m_pContext->m_physicalDevice);
  VkImageCreateInfo dsImageInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  dsImageInfo.imageType         = VK_IMAGE_TYPE_2D;
  dsImageInfo.format            = depthFormat;
  dsImageInfo.extent.width      = fb_width;
  dsImageInfo.extent.height     = fb_height;
  dsImageInfo.extent.depth      = 1;
  dsImageInfo.mipLevels         = 1;
  dsImageInfo.arrayLayers       = 1;
  dsImageInfo.samples           = m_samples;
  dsImageInfo.tiling            = VK_IMAGE_TILING_OPTIMAL;
  dsImageInfo.usage             = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  dsImageInfo.flags             = 0;
  dsImageInfo.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;

  nvvk::DedicatedImage image;
  image.init(m_pContext->m_device, m_pContext->m_physicalDevice, dsImageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  image.initView(dsImageInfo, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D, nullptr);
  m_depthImage       = image.m_image;
  m_depthImageMemory = image.m_memory;
  m_depthImageView   = image.m_imageView;
}
//--------------------------------------------------------------------------------------------------
//
//
void WindowSurface::createMSAAColorResources()
{
  if(m_samples == VK_SAMPLE_COUNT_1_BIT)
    return;
  VkImageCreateInfo cbImageInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  cbImageInfo.imageType         = VK_IMAGE_TYPE_2D;
  cbImageInfo.format            = m_swapChain.getFormat();
  cbImageInfo.extent.width      = fb_width;
  cbImageInfo.extent.height     = fb_height;
  cbImageInfo.extent.depth      = 1;
  cbImageInfo.mipLevels         = 1;
  cbImageInfo.arrayLayers       = 1;
  cbImageInfo.samples           = m_samples;
  cbImageInfo.tiling            = VK_IMAGE_TILING_OPTIMAL;
  cbImageInfo.usage             = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  cbImageInfo.flags             = 0;
  cbImageInfo.initialLayout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  nvvk::DedicatedImage image;
  image.init(m_pContext->m_device, m_pContext->m_physicalDevice, cbImageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  image.initView(cbImageInfo, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, nullptr);
  m_msaaColorImage       = image.m_image;
  m_msaaColorImageMemory = image.m_memory;
  m_msaaColorImageView   = image.m_imageView;
}
