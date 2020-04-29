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

#pragma once
#include <vulkan/vulkan_core.h>

namespace nvvk {
//////////////////////////////////////////////////////////////////////////
/**
# class DedicatedImage

DedicatedImages have their own dedicated device memory allocation.
This can be beneficial for render pass attachments.

Also provides utility function setup the initial image layout.
*/
class DedicatedImage
{
public:
  VkDevice       m_device    = {};  // Logical device, help for many operations
  VkImage        m_image     = {};  // Vulkan image representation (handle)
  VkImageView    m_imageView = {};  // view of the image (optional)
  VkDeviceMemory m_memory    = {};  // Device allocation of the image
  VkFormat       m_format    = {};  // Format when created

  operator VkImage() const { return m_image; }
  operator VkImageView() const { return m_imageView; }

  void init(VkDevice                 device,
            VkPhysicalDevice         physical,
            const VkImageCreateInfo& createInfo,
            VkMemoryPropertyFlags    memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            const void*              pNextMemory         = nullptr);

  void initWithView(VkDevice              device,
                    VkPhysicalDevice      physical,
                    uint32_t              width,
                    uint32_t              height,
                    uint32_t              layers,
                    VkFormat              format,
                    VkImageUsageFlags     usage               = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                    VkImageTiling         tiling              = VK_IMAGE_TILING_OPTIMAL,
                    VkMemoryPropertyFlags memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    VkSampleCountFlagBits samples             = VK_SAMPLE_COUNT_1_BIT,
                    VkImageAspectFlags    aspect              = VK_IMAGE_ASPECT_COLOR_BIT,
                    const void*           pNextImage          = nullptr,
                    const void*           pNextMemory         = nullptr,
                    const void*           pNextImageView      = nullptr);

  void initView(const VkImageCreateInfo& imageInfo, VkImageAspectFlags aspect, VkImageViewType viewType, const void* pNextImageView = nullptr);
  void deinit();

  void cmdInitialTransition(VkCommandBuffer cmd, VkImageLayout layout, VkAccessFlags access);
};

}  // namespace nvvk
