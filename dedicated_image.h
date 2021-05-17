/*
 * Copyright (c) 2018-2021, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2018-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
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
