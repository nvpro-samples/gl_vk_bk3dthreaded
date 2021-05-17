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


#include "dedicated_image.h"
#include "nvvk/commands_vk.hpp"
#include "nvvk/images_vk.hpp"
#include <assert.h>

namespace nvvk {

//////////////////////////////////////////////////////////////////////////

void DedicatedImage::init(VkDevice                 device,
                          VkPhysicalDevice         physical,
                          const VkImageCreateInfo& imageInfo,
                          VkMemoryPropertyFlags    memoryPropertyFlags,
                          const void*              pNextMemory /*= nullptr*/)
{

  m_device = device;

  if(vkCreateImage(device, &imageInfo, nullptr, &m_image) != VK_SUCCESS)
  {
    assert(0 && "image create failed");
  }

  VkMemoryRequirements2          memReqs       = {VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
  VkMemoryDedicatedRequirements  dedicatedRegs = {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS};
  VkImageMemoryRequirementsInfo2 imageReqs     = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2};

  imageReqs.image = m_image;
  memReqs.pNext   = &dedicatedRegs;
  vkGetImageMemoryRequirements2(device, &imageReqs, &memReqs);

  VkMemoryDedicatedAllocateInfo dedicatedInfo = {VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_MEMORY_ALLOCATE_INFO_NV};
  dedicatedInfo.image                         = m_image;
  dedicatedInfo.pNext                         = pNextMemory;

  VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  allocInfo.pNext          = &dedicatedInfo;
  allocInfo.allocationSize = memReqs.memoryRequirements.size;

  VkPhysicalDeviceMemoryProperties memoryProperties;
  vkGetPhysicalDeviceMemoryProperties(physical, &memoryProperties);

  // Find an available memory type that satisfies the requested properties.
  for(uint32_t memoryTypeIndex = 0; memoryTypeIndex < memoryProperties.memoryTypeCount; ++memoryTypeIndex)
  {
    if((memReqs.memoryRequirements.memoryTypeBits & (1 << memoryTypeIndex))
       && (memoryProperties.memoryTypes[memoryTypeIndex].propertyFlags & memoryPropertyFlags) == memoryPropertyFlags)
    {
      allocInfo.memoryTypeIndex = memoryTypeIndex;
      break;
    }
  }
  assert(allocInfo.memoryTypeIndex != ~0);

  if(vkAllocateMemory(device, &allocInfo, nullptr, &m_memory) != VK_SUCCESS)
  {
    assert(0 && "failed to allocate image memory!");
  }

  vkBindImageMemory(device, m_image, m_memory, 0);
}

void DedicatedImage::initWithView(VkDevice              device,
                                  VkPhysicalDevice      physical,
                                  uint32_t              width,
                                  uint32_t              height,
                                  uint32_t              layers,
                                  VkFormat              format,
                                  VkImageUsageFlags     usage /*= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT*/,
                                  VkImageTiling         tiling /*= VK_IMAGE_TILING_OPTIMAL*/,
                                  VkMemoryPropertyFlags memoryPropertyFlags /*= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT*/,
                                  VkSampleCountFlagBits samples /*= VK_SAMPLE_COUNT_1_BIT*/,
                                  VkImageAspectFlags    aspect /*= VK_IMAGE_ASPECT_COLOR_BIT*/,
                                  const void*           pNextImage /*= nullptr*/,
                                  const void*           pNextMemory /*= nullptr*/,
                                  const void*           pNextImageView /*= nullptr*/)
{
  VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  imageInfo.pNext         = pNextImage;
  imageInfo.imageType     = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width  = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth  = 1;
  imageInfo.mipLevels     = 1;
  imageInfo.arrayLayers   = layers;
  imageInfo.format        = format;
  imageInfo.tiling        = tiling;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage         = usage;
  imageInfo.samples       = samples;
  imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

  init(device, physical, imageInfo, memoryPropertyFlags, pNextMemory);
  initView(imageInfo, aspect, layers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D, pNextImageView);
}

void DedicatedImage::initView(const VkImageCreateInfo& imageInfo, VkImageAspectFlags aspect, VkImageViewType viewType, const void* pNextImageView /*= nullptr*/)
{
  VkImageViewCreateInfo createInfo           = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  createInfo.pNext                           = pNextImageView;
  createInfo.components.r                    = VK_COMPONENT_SWIZZLE_R;
  createInfo.components.g                    = VK_COMPONENT_SWIZZLE_G;
  createInfo.components.b                    = VK_COMPONENT_SWIZZLE_B;
  createInfo.components.a                    = VK_COMPONENT_SWIZZLE_A;
  createInfo.subresourceRange.aspectMask     = aspect;
  createInfo.subresourceRange.baseArrayLayer = 0;
  createInfo.subresourceRange.baseMipLevel   = 0;
  createInfo.subresourceRange.layerCount     = imageInfo.arrayLayers;
  createInfo.subresourceRange.levelCount     = imageInfo.mipLevels;
  createInfo.format                          = imageInfo.format;
  createInfo.viewType                        = viewType;
  createInfo.image                           = m_image;

  VkResult result = vkCreateImageView(m_device, &createInfo, nullptr, &m_imageView);
  assert(result == VK_SUCCESS);
}

void DedicatedImage::deinit()
{
  if(m_image != nullptr)
    vkDestroyImage(m_device, m_image, nullptr);
  if(m_imageView != nullptr)
    vkDestroyImageView(m_device, m_imageView, nullptr);
  if(m_memory != nullptr)
    vkFreeMemory(m_device, m_memory, nullptr);
  *this = {};
}

void DedicatedImage::cmdInitialTransition(VkCommandBuffer cmd, VkImageLayout layout, VkAccessFlags access)
{
  VkPipelineStageFlags srcPipe = nvvk::makeAccessMaskPipelineStageFlags(0);
  VkPipelineStageFlags dstPipe = nvvk::makeAccessMaskPipelineStageFlags(access);

  VkImageMemoryBarrier memBarrier = nvvk::makeImageMemoryBarrier(m_image, 0, access, VK_IMAGE_LAYOUT_UNDEFINED, layout);

  vkCmdPipelineBarrier(cmd, srcPipe, dstPipe, VK_FALSE, 0, NULL, 0, NULL, 1, &memBarrier);
}

}  // namespace nvvk
