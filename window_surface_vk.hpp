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


#ifndef NV_VK_DEFAULTWINDOWSURFACE_INCLUDED
#define NV_VK_DEFAULTWINDOWSURFACE_INCLUDED


#include <stdio.h>
#include <vector>
#include <string>
#include <vulkan/vulkan.h>
#include <nvvk/context_vk.hpp>
#include <nvvk/swapchain_vk.hpp>
#include <nvpwindow.hpp>

#define VK_MAX_QUEUED_FRAMES 4
#define MAX_POSSIBLE_BACK_BUFFERS 16


  /*
    WindowSurface is a basic implementation of whatever is required to have a regular color+Depthstencil setup realted to a window
    This class is *not mandatory* for a sample to run. It's just a convenient way to have something put together for quick
    rendering in a window
    - a render-pass associated with the framebuffer(s)
    - buffers/framebuffers associated with the views of the window
    - command-buffers to match the current swapchain index
    typical use :
    0)  ...
        m_WindowSurface.acquire()
        ...
    1)  m_WindowSurface.setClearValue();
        VkCommandBuffer command_buffer = m_windowSurface.beginCommandBuffer();
    2)  m_windowSurface.beginRenderPass();
        vkCmd...()
        ...
    3)  //for MSAA case: advances in the sub-pass to render *after* the resolve of AA
        m_windowSurface.nextSubPassForOverlay();
        ... draw some non MSAA stuff (UI...)
    4)  m_windowSurface.endRPassCBufferSubmitAndPresent();
  */
  class WindowSurface {
  public:
    nvvk::SwapChain       m_swapChain;
  private:
    nvvk::Context*        m_pContext;
    VkSurfaceKHR          m_surface;
    // framebuffer size and # of samples
    int                   fb_width = 0, fb_height = 0;
    VkSampleCountFlagBits m_samples = VK_SAMPLE_COUNT_1_BIT;
    bool                  m_swapVsync;

    VkClearColorValue           m_clearColor;
    VkClearDepthStencilValue    m_clearDST;

    VkRenderPass    m_renderPass = VK_NULL_HANDLE;

    VkCommandPool   m_commandPool[VK_MAX_QUEUED_FRAMES];
    VkCommandBuffer m_curCommandBuffer = VK_NULL_HANDLE;
    VkCommandBuffer m_commandBuffer[VK_MAX_QUEUED_FRAMES];
    VkFence         m_curFence = VK_NULL_HANDLE;
    VkFence         m_fence[VK_MAX_QUEUED_FRAMES];

    VkFramebuffer   m_framebuffer[MAX_POSSIBLE_BACK_BUFFERS] = {};

    VkImage         m_depthImage = {};
    VkImage         m_msaaColorImage = {};
    VkDeviceMemory  m_depthImageMemory = {};
    VkDeviceMemory  m_msaaColorImageMemory = {};
    VkImageView     m_depthImageView = {};
    VkImageView     m_msaaColorImageView = {};

    VkAllocationCallbacks   *m_allocator = VK_NULL_HANDLE;

    bool        hasStencilComponent(VkFormat format);

  public:
    bool init(nvvk::Context* pContext, NVPWindow* pWin, int MSAA);
    void deinit();
    bool resize(int w, int h);
    void createFrameBuffer();
    //void createImageViews();
    void createRenderPass();

    void acquire();
    VkCommandBuffer beginCommandBuffer(VkCommandBufferUsageFlags flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    void beginRenderPass(VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE); // could be VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS
    void nextSubPassForOverlay(VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE);
    void endRenderPass();
    void endCommandBuffer();
    void submit();
    void present()
    {
      m_swapChain.present(m_pContext->m_queueGCT);//m_presentQueue.queue);
    }
    void endCBufferSubmitAndPresent()  // does the 3 methods in 1 call
    {
      endCommandBuffer();
      submit();
      present();
    }
    void endRPassCBufferSubmitAndPresent()  // does the 4 methods in 1 call
    {
      endRenderPass();
      endCommandBuffer();
      submit();
      present();
    }
    void createDepthResources();
    void createMSAAColorResources();
    void swapVsync(bool state)
    {
      if (m_swapVsync != state)
      {
        m_swapChain.update(fb_width, fb_height, state);
        m_swapVsync = state;
      }
    }
    //
    // Setters
    //
    void setClearValue(VkClearColorValue clearColor, VkClearDepthStencilValue clearDST = { 1.0f, 0 })
    {
      m_clearColor = clearColor;
      m_clearDST = clearDST;
    }
    void setClearValue(VkClearValue clearColor, VkClearValue clearDST = { 1.0f, 0 })
    {
      m_clearColor = clearColor.color;
      m_clearDST = clearDST.depthStencil;
    }
    //
    // getters
    //
    uint32_t getHeight() { return fb_height; }
    uint32_t getWidth()  { return fb_width; }
    uint32_t getFrameIndex() { return m_swapChain.getActiveImageIndex(); }
    const VkRenderPass &getRenderPass() { return m_renderPass; }
    VkFormat getSurfaceFormat() const { return m_swapChain.getFormat(); }
    VkImage getCurrentBackBuffer() const {  return m_swapChain.getActiveImage(); }
    VkImageView getCurrentBackBufferView() const { return m_swapChain.getActiveImageView(); }
    VkCommandBuffer getCurrentCommandBuffer() { return m_curCommandBuffer; }
    VkFramebuffer   getCurrentFramebuffer() { return m_framebuffer[m_swapChain.getActiveImageIndex()]; }
    nvvk::Context* getContext() { return m_pContext; }
  };

#endif