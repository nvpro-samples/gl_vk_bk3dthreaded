/*-----------------------------------------------------------------------
    Copyright (c) 2016, NVIDIA. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
     * Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
     * Neither the name of its contributors may be used to endorse 
       or promote products derived from this software without specific
       prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
    PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
    CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
    EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
    PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
    PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
    OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    feedback to tlorach@nvidia.com (Tristan Lorach)
*/ //--------------------------------------------------------------------
#ifdef _DEBUG
#define CHECK(a) assert(a == VK_SUCCESS)
#else
#define CHECK(a) a
#endif

#ifdef WIN32
#if defined(_MSC_VER)
#  include <windows.h>
#  include <direct.h>
#endif
#endif
#include <assert.h>
#ifdef WIN32
#include <float.h>
#endif
#include <math.h>

#include <string.h>
#include <vector>

//------------------------------------------------------------------------------
// VULKAN: NVK.h > fnptrinline.h > vulkannv.h > vulkan.h
//------------------------------------------------------------------------------
#include "NVK.h"
#include "nvpwindow.hpp"

template <typename T, size_t sz> inline size_t getArraySize(T(&t)[sz]) { return sz; }

//--------------------------------------------------------------------------------
NVK::GraphicsPipelineCreateInfo& operator<<(NVK::GraphicsPipelineCreateInfo& os, const NVK::PipelineBaseCreateInfo& dt)
{
    os.add(dt);
    return os;
}

void NVK::updateDescriptorSets(const WriteDescriptorSet &wds)
{
    std::vector<VkWriteDescriptorSet> updateArray;
    int N = wds.size();
    for(int i = 0; i<N; i++)
    {
        updateArray.push_back(*(wds.getItemCst(i)));
    }
    vkUpdateDescriptorSets(m_device, N, &updateArray[0], 0, 0);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VkDescriptorPool    NVK::createDescriptorPool(const DescriptorPoolCreateInfo &descriptorPoolCreateInfo)
{
    VkDescriptorPool descPool;
    vkCreateDescriptorPool(m_device, descriptorPoolCreateInfo.getItemCst(), NULL, &descPool);
    return descPool;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VkDescriptorSetLayout NVK::createDescriptorSetLayout(const DescriptorSetLayoutCreateInfo &descriptorSetLayoutCreateInfo)
{
    VkDescriptorSetLayout dsl;
    (vkCreateDescriptorSetLayout(
    m_device,
    descriptorSetLayoutCreateInfo.getItemCst(),
    NULL,
    &dsl
    ) );
    return dsl;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VkPipelineLayout NVK::createPipelineLayout(VkDescriptorSetLayout* dsls, uint32_t count)
{
    VkPipelineLayout pipelineLayout;
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    pipelineLayoutCreateInfo.setLayoutCount = count;
    pipelineLayoutCreateInfo.pSetLayouts = dsls;
    pipelineLayoutCreateInfo.pNext = NULL;
    pipelineLayoutCreateInfo.flags = 0;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = NULL;

    CHECK(vkCreatePipelineLayout(
        m_device,
        &pipelineLayoutCreateInfo,
        NULL,
        &pipelineLayout
        ) );
    return pipelineLayout;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::freeMemory(VkDeviceMemory mem)
{
    vkFreeMemory(m_device, mem, NULL);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void* NVK::mapMemory(VkDeviceMemory mem, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags)
{
    void* bufferPtr = NULL;
    CHECK(vkMapMemory(m_device, mem, 0, size, 0, &bufferPtr) );
    return bufferPtr;
}
void NVK::unmapMemory(VkDeviceMemory mem)
{
    vkUnmapMemory(m_device, mem);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::utMemcpy(VkDeviceMemory dstMem, const void * srcData, VkDeviceSize size)
{
    void* bufferPtr = NULL;
    CHECK(vkMapMemory(m_device, dstMem, 0, size, 0, &bufferPtr) );
    {
        ::memcpy(bufferPtr, srcData, size);
    }
    vkUnmapMemory(m_device, dstMem);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::destroyBuffer(VkBuffer buffer)
{
    vkDestroyBuffer(m_device, buffer, NULL);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::destroyBufferView(VkBufferView bufferView)
{
    vkDestroyBufferView(m_device, bufferView, NULL);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::destroyRenderPass(VkRenderPass renderPass, const VkAllocationCallbacks* pAllocator)
{
    vkDestroyRenderPass(m_device, renderPass, pAllocator);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::destroyFramebuffer(VkFramebuffer framebuffer, const VkAllocationCallbacks* pAllocator)
{
    vkDestroyFramebuffer(m_device, framebuffer, pAllocator);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::destroyDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout, const VkAllocationCallbacks* pAllocator)
{
    vkDestroyDescriptorSetLayout(m_device, descriptorSetLayout, pAllocator);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::destroyDescriptorPool(VkDescriptorPool descriptorPool, const VkAllocationCallbacks* pAllocator)
{
    vkDestroyDescriptorPool(m_device, descriptorPool, pAllocator);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::destroyPipelineLayout(VkPipelineLayout pipelineLayout, const VkAllocationCallbacks* pAllocator)
{
    vkDestroyPipelineLayout(m_device, pipelineLayout, pAllocator);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::destroyPipeline(VkPipeline pipeline, const VkAllocationCallbacks* pAllocator)
{
    vkDestroyPipeline(m_device, pipeline, pAllocator);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::CommandBuffer::endCommandBuffer()
{
    CHECK(vkEndCommandBuffer(m_cmdbuffer) );
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::CommandBuffer::cmdCopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkDeviceSize dstOffset, VkDeviceSize srcOffset)
{
    VkBufferCopy copy;
    copy.size   = size;
    copy.dstOffset = dstOffset;
    copy.srcOffset  = srcOffset;
    vkCmdCopyBuffer(m_cmdbuffer, srcBuffer, dstBuffer, 1, &copy);
}
//------------------------------------------------------------------------------
// other commands provided as the original one, minus the cmd buffer handle
//------------------------------------------------------------------------------
void NVK::CommandBuffer::cmdBindPipeline(VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline)
{
    vkCmdBindPipeline(m_cmdbuffer, pipelineBindPoint, pipeline);
}
void NVK::CommandBuffer::cmdSetViewport(uint32_t firstViewport, uint32_t viewportCount, const VkViewport* pViewports)
{
    vkCmdSetViewport(m_cmdbuffer, firstViewport, viewportCount, pViewports);
}
void NVK::CommandBuffer::cmdSetScissor(uint32_t firstScissor, uint32_t scissorCount, const VkRect2D* pScissors)
{
    vkCmdSetScissor(m_cmdbuffer, firstScissor, scissorCount, pScissors);
}
void NVK::CommandBuffer::cmdSetLineWidth(float lineWidth)
{
    vkCmdSetLineWidth(m_cmdbuffer, lineWidth);
}
void NVK::CommandBuffer::cmdSetDepthBias(float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor)
{
    vkCmdSetDepthBias(m_cmdbuffer, depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
}
void NVK::CommandBuffer::cmdSetBlendConstants(const float blendConstants[4])
{
    vkCmdSetBlendConstants(m_cmdbuffer, blendConstants);
}
void NVK::CommandBuffer::cmdSetDepthBounds(float minDepthBounds, float maxDepthBounds)
{
    vkCmdSetDepthBounds(m_cmdbuffer, minDepthBounds, maxDepthBounds);
}
void NVK::CommandBuffer::cmdSetStencilCompareMask(VkStencilFaceFlags faceMask, uint32_t compareMask)
{
    vkCmdSetStencilCompareMask(m_cmdbuffer, faceMask, compareMask);
}
void NVK::CommandBuffer::cmdSetStencilWriteMask(VkStencilFaceFlags faceMask, uint32_t writeMask)
{
    vkCmdSetStencilWriteMask(m_cmdbuffer, faceMask, writeMask);
}
void NVK::CommandBuffer::cmdSetStencilReference(VkStencilFaceFlags faceMask, uint32_t reference)
{
    vkCmdSetStencilReference(m_cmdbuffer, faceMask, reference);
}
void NVK::CommandBuffer::cmdBindDescriptorSets(VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets)
{
    vkCmdBindDescriptorSets(m_cmdbuffer, pipelineBindPoint, layout, firstSet, descriptorSetCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
}
void NVK::CommandBuffer::cmdBindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType)
{
    vkCmdBindIndexBuffer(m_cmdbuffer, buffer, offset, indexType);
}
void NVK::CommandBuffer::cmdBindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* pBuffers, const VkDeviceSize* pOffsets)
{
    vkCmdBindVertexBuffers(m_cmdbuffer, firstBinding, bindingCount, pBuffers, pOffsets);
}
void NVK::CommandBuffer::cmdDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    vkCmdDraw(m_cmdbuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}
void NVK::CommandBuffer::cmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
{
    vkCmdDrawIndexed(m_cmdbuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}
void NVK::CommandBuffer::cmdDrawIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
{
    vkCmdDrawIndirect(m_cmdbuffer, buffer, offset, drawCount, stride);
}
void NVK::CommandBuffer::cmdDrawIndexedIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
{
    vkCmdDrawIndexedIndirect(m_cmdbuffer, buffer, offset, drawCount, stride);
}
void NVK::CommandBuffer::cmdDispatch(uint32_t x, uint32_t y, uint32_t z)
{
    vkCmdDispatch(m_cmdbuffer, x, y, z);
}
void NVK::CommandBuffer::cmdDispatchIndirect(VkBuffer buffer, VkDeviceSize offset)
{
    vkCmdDispatchIndirect(m_cmdbuffer, buffer, offset);
}
void NVK::CommandBuffer::cmdCopyImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageCopy* pRegions)
{
    vkCmdCopyImage(m_cmdbuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
}
void NVK::CommandBuffer::cmdBlitImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit* pRegions, VkFilter filter)
{
    vkCmdBlitImage(m_cmdbuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions, filter);
}
void NVK::CommandBuffer::cmdCopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy* pRegions)
{
    vkCmdCopyBufferToImage(m_cmdbuffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions);
}
void NVK::CommandBuffer::cmdCopyImageToBuffer(VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferImageCopy* pRegions)
{
    vkCmdCopyImageToBuffer(m_cmdbuffer, srcImage, srcImageLayout, dstBuffer, regionCount, pRegions);
}
void NVK::CommandBuffer::cmdUpdateBuffer(VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const uint32_t* pData)
{
    vkCmdUpdateBuffer(m_cmdbuffer, dstBuffer, dstOffset, dataSize, pData);
}
void NVK::CommandBuffer::cmdFillBuffer(VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data)
{
    vkCmdFillBuffer(m_cmdbuffer, dstBuffer, dstOffset, size, data);
}
void NVK::CommandBuffer::cmdClearColorImage(VkImage image, VkImageLayout imageLayout, const VkClearColorValue* pColor, uint32_t rangeCount, const VkImageSubresourceRange* pRanges)
{
    vkCmdClearColorImage(m_cmdbuffer, image, imageLayout, pColor, rangeCount, pRanges);
}
void NVK::CommandBuffer::cmdClearDepthStencilImage(VkImage image, VkImageLayout imageLayout, const VkClearDepthStencilValue* pDepthStencil, uint32_t rangeCount, const VkImageSubresourceRange* pRanges)
{
    vkCmdClearDepthStencilImage(m_cmdbuffer, image, imageLayout, pDepthStencil, rangeCount, pRanges);
}
void NVK::CommandBuffer::cmdClearAttachments(uint32_t attachmentCount, const VkClearAttachment* pAttachments, uint32_t rectCount, const VkClearRect* pRects)
{
    vkCmdClearAttachments(m_cmdbuffer, attachmentCount, pAttachments, rectCount, pRects);
}
void NVK::CommandBuffer::cmdResolveImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageResolve* pRegions)
{
    vkCmdResolveImage(m_cmdbuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
}
void NVK::CommandBuffer::cmdSetEvent(VkEvent event, VkPipelineStageFlags stageMask)
{
    vkCmdSetEvent(m_cmdbuffer, event, stageMask);
}
void NVK::CommandBuffer::cmdResetEvent(VkEvent event, VkPipelineStageFlags stageMask)
{
    vkCmdResetEvent(m_cmdbuffer, event, stageMask);
}
void NVK::CommandBuffer::cmdWaitEvents(uint32_t eventCount, const VkEvent* pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers)
{
    vkCmdWaitEvents(m_cmdbuffer, eventCount, pEvents, srcStageMask, dstStageMask, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
}
void NVK::CommandBuffer::cmdPipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers)
{
    vkCmdPipelineBarrier(m_cmdbuffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
}
void NVK::CommandBuffer::cmdBeginQuery(VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags)
{
    vkCmdBeginQuery(m_cmdbuffer, queryPool, query, flags);
}
void NVK::CommandBuffer::cmdEndQuery(VkQueryPool queryPool, uint32_t query)
{
    vkCmdEndQuery(m_cmdbuffer, queryPool, query);
}
void NVK::CommandBuffer::cmdResetQueryPool(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount)
{
    vkCmdResetQueryPool(m_cmdbuffer, queryPool, firstQuery, queryCount);
}
void NVK::CommandBuffer::cmdWriteTimestamp(VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool, uint32_t query)
{
    vkCmdWriteTimestamp(m_cmdbuffer, pipelineStage, queryPool, query);
}
void NVK::CommandBuffer::cmdCopyQueryPoolResults(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize stride, VkQueryResultFlags flags)
{
    vkCmdCopyQueryPoolResults(m_cmdbuffer, queryPool, firstQuery, queryCount, dstBuffer, dstOffset, stride, flags);
}
void NVK::CommandBuffer::cmdPushConstants(VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* pValues)
{
    vkCmdPushConstants(m_cmdbuffer, layout, stageFlags, offset, size, pValues);
}
void NVK::CommandBuffer::cmdBeginRenderPass(const VkRenderPassBeginInfo* pRenderPassBegin, VkSubpassContents contents)
{
    vkCmdBeginRenderPass(m_cmdbuffer, pRenderPassBegin, contents);
}
void NVK::CommandBuffer::cmdNextSubpass(VkSubpassContents contents)
{
    vkCmdNextSubpass(m_cmdbuffer, contents);
}
void NVK::CommandBuffer::cmdEndRenderPass()
{
    vkCmdEndRenderPass(m_cmdbuffer);
}
void NVK::CommandBuffer::cmdExecuteCommands(uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers)
{
    vkCmdExecuteCommands(m_cmdbuffer, commandBufferCount, pCommandBuffers);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VkResult NVK::createCommandPool(const VkCommandPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, CommandPool *commandPool)
{
    commandPool->m_device = m_device;
    return vkCreateCommandPool(m_device, pCreateInfo, pAllocator, &commandPool->m_cmdPool);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
NVK::CommandPool::CommandPool(VkDevice device, VkCommandPool cmdPool) : 
    m_device(device), 
    m_cmdPool(cmdPool)
{
    m_currentCmdBufferSlot[0] = 0;
    m_currentCmdBufferSlot[1] = 0;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
NVK::CommandPool::CommandPool(const CommandPool &srcPool)
{
    assert(!"You should avoid copying it");
    m_cmdPool = srcPool.m_cmdPool;
    m_device = srcPool.m_device;
    m_currentCmdBufferSlot[0] = srcPool.m_currentCmdBufferSlot[0];
    m_currentCmdBufferSlot[1] = srcPool.m_currentCmdBufferSlot[1];
    m_allocatedCmdBuffers[0] = srcPool.m_allocatedCmdBuffers[0];
    m_allocatedCmdBuffers[1] = srcPool.m_allocatedCmdBuffers[1];
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VkCommandBuffer NVK::CommandPool::allocateCommandBuffers(bool primary, uint32_t nCmdBuffers, VkCommandBuffer *cmdBuffers)
{
  int l = primary ? BUFFERPRIMARY : BUFFERSECONDARY;
  std::vector<VkCommandBuffer> &allocatedCmdBuffers = m_allocatedCmdBuffers[l];
  int &currentCmdBufferSlot = m_currentCmdBufferSlot[l];
  if(currentCmdBufferSlot < allocatedCmdBuffers.size())
  {
    return allocatedCmdBuffers[currentCmdBufferSlot++];
  } else {
    VkCommandBufferAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocInfo.pNext = NULL;
    allocInfo.commandPool = m_cmdPool;
    allocInfo.level = primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    allocInfo.commandBufferCount = nCmdBuffers;
    CHECK(vkAllocateCommandBuffers(m_device, &allocInfo, cmdBuffers));
    // keep track of allocated handles
    for (uint32_t i = 0; i< nCmdBuffers; i++)
      allocatedCmdBuffers.push_back(cmdBuffers[i]);
    currentCmdBufferSlot = allocatedCmdBuffers.size();
    return cmdBuffers[0]; // return the first one...
  }
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VkCommandBuffer NVK::CommandPool::allocateCommandBuffer(bool primary)
{
    int l = primary ? BUFFERPRIMARY : BUFFERSECONDARY;
  std::vector<VkCommandBuffer> &allocatedCmdBuffers = m_allocatedCmdBuffers[l];
  int &currentCmdBufferSlot = m_currentCmdBufferSlot[l];
  if(currentCmdBufferSlot < allocatedCmdBuffers.size())
  {
    return allocatedCmdBuffers[currentCmdBufferSlot++];
  } else {
    VkCommandBufferAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocInfo.pNext = NULL;
    allocInfo.commandPool = m_cmdPool;
    allocInfo.level = primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    allocInfo.commandBufferCount = 1;
    VkCommandBuffer cmd[1];
    CHECK(vkAllocateCommandBuffers(m_device, &allocInfo, cmd) );
    allocatedCmdBuffers.push_back(cmd[0]);
    currentCmdBufferSlot = allocatedCmdBuffers.size();
    return cmd[0];
  }
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::CommandPool::freeCommandBuffers(uint32_t nCmdBuffers, VkCommandBuffer *cmdBuffers)
{
  vkFreeCommandBuffers( m_device, m_cmdPool, nCmdBuffers, cmdBuffers);
  // slow :-(
  // but we don't intent to use it all the time within a frame
  for (uint32_t i = 0; i<nCmdBuffers; i++)
  {
    for (int j = 0; j<2; j++)
    {
      std::vector<VkCommandBuffer>::iterator it = m_allocatedCmdBuffers[j].begin();
      for(; it != m_allocatedCmdBuffers[j].end(); ++it)
      {
        if(*it == cmdBuffers[i])
        {
          m_allocatedCmdBuffers[j].erase(it);
          m_currentCmdBufferSlot[j] = m_allocatedCmdBuffers[j].size();
          break;
        }
      }
    }
  }
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::CommandPool::freeCommandBuffer(VkCommandBuffer cmdBuffer)
{
  vkFreeCommandBuffers(m_device, m_cmdPool, 1, &cmdBuffer);
  for (int j = 0; j<2; j++)
  {
    std::vector<VkCommandBuffer>::iterator it = m_allocatedCmdBuffers[j].begin();
    for (; it != m_allocatedCmdBuffers[j].end(); ++it)
    {
      if (*it == cmdBuffer)
      {
        m_allocatedCmdBuffers[j].erase(it);
        m_currentCmdBufferSlot[j] = m_allocatedCmdBuffers[j].size();
        break;
      }
    }
  }
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::CommandPool::resetCommandPool(VkCommandPoolResetFlags flags)
{
  // keep command buffer handles
  for (int j = 0; j<2; j++)
  {
    m_currentCmdBufferSlot[j] = 0;
  }
  CHECK(vkResetCommandPool(m_device, m_cmdPool, flags) );
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::CommandPool::destroyCommandPool()
{
  for (int j = 0; j<2; j++)
  {
    if(!m_allocatedCmdBuffers[j].empty())
      vkFreeCommandBuffers(m_device, m_cmdPool, m_allocatedCmdBuffers[j].size(), &m_allocatedCmdBuffers[j][0]);
    m_allocatedCmdBuffers[j].clear();
    m_currentCmdBufferSlot[j] = 0;
  }
  vkDestroyCommandPool(m_device, m_cmdPool, NULL);
    m_cmdPool = VK_NULL_HANDLE;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VkBufferView NVK::createBufferView( VkBuffer buffer, VkFormat format, VkDeviceSize size )
{
    VkBufferView view;
    VkBufferViewCreateInfo viewInfo = {VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO };
    viewInfo.pNext = NULL;
    viewInfo.flags = 0;
    viewInfo.buffer = buffer;
    viewInfo.format = format;
    viewInfo.offset = 0;
    viewInfo.range  = size;
    CHECK(vkCreateBufferView(m_device, &viewInfo, NULL, &view) );
    return view;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::CommandBuffer::resetCommandBuffer(VkCommandBufferResetFlags flags)
{
    CHECK(vkResetCommandBuffer(m_cmdbuffer, flags));
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::CommandBuffer::beginCommandBuffer(bool singleshot, bool bRenderpassContinue, NVK::CommandBufferInheritanceInfo &cmdIInfo) const
{
    // Record the commands.
    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.pNext = NULL;
    beginInfo.flags = (singleshot ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT  :  VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
    if(bRenderpassContinue)
      beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    beginInfo.pInheritanceInfo = cmdIInfo.getItem();
    CHECK(vkBeginCommandBuffer(m_cmdbuffer, &beginInfo) );
} 
void NVK::CommandBuffer::beginCommandBuffer(bool singleshot, bool bRenderpassContinue, const VkCommandBufferInheritanceInfo* pInheritanceInfo) const
{
    // Record the commands.
    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.pNext = NULL;
    beginInfo.flags = singleshot ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT :  VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    if (bRenderpassContinue)
      beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    beginInfo.pInheritanceInfo = pInheritanceInfo;
    CHECK(vkBeginCommandBuffer(m_cmdbuffer, &beginInfo) );
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VkRenderPass NVK::createRenderPass(NVK::RenderPassCreateInfo &rpinfo)
{
    VkRenderPass rp;
    CHECK(vkCreateRenderPass(m_device, rpinfo, NULL, &rp) );
    return rp;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VkImage NVK::utCreateImage1D(
    int width,
    VkDeviceMemory &colorMemory, 
    VkFormat format, 
    VkSampleCountFlagBits depthSamples, 
    VkSampleCountFlagBits colorSamples,
    int mipLevels, bool asAttachment)
{
    VkImage                     colorImage;
    // color texture & view
    VkImageCreateInfo cbImageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    cbImageInfo.flags = 0;
    cbImageInfo.imageType = VK_IMAGE_TYPE_1D;
    cbImageInfo.format = format;
    cbImageInfo.extent.width = width;
    cbImageInfo.extent.height = 1;
    cbImageInfo.extent.depth = 1;
    cbImageInfo.mipLevels = mipLevels;
    cbImageInfo.arrayLayers = 1;
    cbImageInfo.samples = depthSamples;
    cbImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    cbImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    switch(format)
    {
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_D32_SFLOAT:
    case VK_FORMAT_S8_UINT:
    case VK_FORMAT_D16_UNORM_S8_UINT:
    case VK_FORMAT_D24_UNORM_S8_UINT:
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        cbImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        break;
    default:
        // THIS FLAG VK_IMAGE_USAGE_SAMPLED_BIT wasted me almost a DAY to understand why nothing was working !!!
        cbImageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT |VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        if(asAttachment) cbImageInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    cbImageInfo.flags = 0;

    CHECK(vkCreateImage(m_device, &cbImageInfo, NULL, &colorImage) );
    colorMemory = utAllocMemAndBindImage(colorImage, VK_MEMORY_HEAP_DEVICE_LOCAL_BIT);
    return colorImage;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VkImage NVK::utCreateImage2D(
    int width, int height, 
    VkDeviceMemory &colorMemory, 
    VkFormat format, 
    VkSampleCountFlagBits depthSamples, 
    VkSampleCountFlagBits colorSamples,
    int mipLevels, bool asAttachment)
{
    VkImage                     colorImage;
    // color texture & view
    VkImageCreateInfo cbImageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    cbImageInfo.flags = 0;
    cbImageInfo.imageType = VK_IMAGE_TYPE_2D;
    cbImageInfo.format = format;
    cbImageInfo.extent.width = width;
    cbImageInfo.extent.height = height;
    cbImageInfo.extent.depth = 1;
    cbImageInfo.mipLevels = mipLevels;
    cbImageInfo.arrayLayers = 1;
    cbImageInfo.samples = depthSamples;
    cbImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    cbImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    switch(format)
    {
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_D32_SFLOAT:
    case VK_FORMAT_S8_UINT:
    case VK_FORMAT_D16_UNORM_S8_UINT:
    case VK_FORMAT_D24_UNORM_S8_UINT:
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        cbImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        break;
    default:
        // THIS FLAG VK_IMAGE_USAGE_SAMPLED_BIT wasted me almost a DAY to understand why nothing was working !!!
        cbImageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT |VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        if(asAttachment) cbImageInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    cbImageInfo.flags = 0;

    CHECK(vkCreateImage(m_device, &cbImageInfo, NULL, &colorImage) );
    colorMemory = utAllocMemAndBindImage(colorImage, VK_MEMORY_HEAP_DEVICE_LOCAL_BIT);
    return colorImage;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VkImage NVK::utCreateImage3D(
    int width, int height, int depth,
    VkDeviceMemory &colorMemory, 
    VkFormat format, 
    VkSampleCountFlagBits depthSamples, 
    VkSampleCountFlagBits colorSamples,
    int mipLevels, bool asAttachment)
{
    VkImage                     colorImage;
    // color texture & view
    VkImageCreateInfo cbImageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    cbImageInfo.flags = 0;
    cbImageInfo.imageType = VK_IMAGE_TYPE_3D;
    cbImageInfo.format = format;
    cbImageInfo.extent.width = width;
    cbImageInfo.extent.height = height;
    cbImageInfo.extent.depth = depth;
    cbImageInfo.mipLevels = mipLevels;
    cbImageInfo.arrayLayers = 1;
    cbImageInfo.samples = depthSamples;
    cbImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    cbImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    switch(format)
    {
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_D32_SFLOAT:
    case VK_FORMAT_S8_UINT:
    case VK_FORMAT_D16_UNORM_S8_UINT:
    case VK_FORMAT_D24_UNORM_S8_UINT:
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        cbImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        break;
    default:
        // THIS FLAG VK_IMAGE_USAGE_SAMPLED_BIT wasted me almost a DAY to understand why nothing was working !!!
        cbImageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        if(asAttachment) cbImageInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    cbImageInfo.flags = 0;

    CHECK(vkCreateImage(m_device, &cbImageInfo, NULL, &colorImage) );
    colorMemory = utAllocMemAndBindImage(colorImage, VK_MEMORY_HEAP_DEVICE_LOCAL_BIT);
    return colorImage;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VkImage NVK::utCreateImageCube(
    int width,
    VkDeviceMemory &colorMemory, 
    VkFormat format, 
    VkSampleCountFlagBits depthSamples, 
    VkSampleCountFlagBits colorSamples,
    int mipLevels, bool asAttachment)
{
    VkImage                     colorImage;
    // color texture & view
    VkImageCreateInfo cbImageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    cbImageInfo.flags = 0;
    cbImageInfo.imageType = VK_IMAGE_TYPE_2D;
    cbImageInfo.format = format;
    cbImageInfo.extent.width = width;
    cbImageInfo.extent.height = width;
    cbImageInfo.extent.depth = 1;
    cbImageInfo.mipLevels = mipLevels;
    cbImageInfo.arrayLayers = 6;
    cbImageInfo.samples = depthSamples;
    cbImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    cbImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    switch(format)
    {
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_D32_SFLOAT:
    case VK_FORMAT_S8_UINT:
    case VK_FORMAT_D16_UNORM_S8_UINT:
    case VK_FORMAT_D24_UNORM_S8_UINT:
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        cbImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        break;
    default:
        // THIS FLAG VK_IMAGE_USAGE_SAMPLED_BIT wasted me almost a DAY to understand why nothing was working !!!
       cbImageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT |VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    cbImageInfo.flags = 0;

    CHECK(vkCreateImage(m_device, &cbImageInfo, NULL, &colorImage) );
    colorMemory = utAllocMemAndBindImage(colorImage, VK_MEMORY_HEAP_DEVICE_LOCAL_BIT);
    return colorImage;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VkFramebuffer NVK::createFramebuffer(FramebufferCreateInfo &fbinfo)
{
    VkFramebuffer framebuffer;
    CHECK(vkCreateFramebuffer(m_device, fbinfo, NULL, &framebuffer) );
    return framebuffer;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VkShaderModule NVK::createShaderModule( const char *shaderCode, size_t size)
{
    VkResult result;
    VkShaderModuleCreateInfo shaderModuleInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    VkShaderModule shaderModule;
    unsigned long checksum = 0;
    for (int i = 0; i<size; i++)
      checksum += (unsigned long)shaderCode[i];
    std::map<unsigned long, VkShaderModule>::const_iterator it = m_shaderModules.find(checksum);
    if(it != m_shaderModules.end())
      return it->second;
#if 0
    if (spirvSupported) TODO
    {
        // SPIR-V path
        int spirvSourceSize = 0;

        // Execute the glslang and get the compiled SPIR-V binary
        spirvSource = nvogSpirVExecuteGlslang(code, 
            m_testName?m_testName:"vulkan", 
            m_spirvShaderCount++, 
            shaderStage, 
            spirvKeepTempFile,
            &spirvSourceSize,
            VK_TRUE, 
            VK_TRUE);

        // If SPIR-V blob is generated
        if (spirvSource) {
            shaderModuleInfo.codeSize = spirvSourceSize;
            shaderModuleInfo.pCode = (const uint32_t *)spirvSource;
        } else {
            return NULL;
        }
    } else
#endif
    {
        // GLSL path
        shaderModuleInfo.codeSize = size;
        shaderModuleInfo.pCode = (const uint32_t*)shaderCode;
    }

    result = vkCreateShaderModule(m_device, &shaderModuleInfo, NULL, &shaderModule);

    if (result != VK_SUCCESS)
        return VK_NULL_HANDLE;
    m_shaderModules[checksum] = shaderModule;
    return shaderModule;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VkBool32 dbgFunc(
    VkDebugReportFlagsEXT                       flags,
    VkDebugReportObjectTypeEXT                  objectType,
    uint64_t                                    object,
    size_t                                      location,
    int32_t                                     messageCode,
    const char*                                 pLayerPrefix,
    const char*                                 pMessage,
    void*                                       pUserData)
{
    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
    {
        if (strstr(pMessage, "is signaling semaphore"))
        {
            return false;
        }
        LOGE("ERROR: [%s] Code %d : %s\n", pLayerPrefix, messageCode, pMessage);
        return false;
    } else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
    {
        LOGW("WARNING: [%s] Code %d : %s\n", pLayerPrefix, messageCode, pMessage);
        return false;
    }
    else if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
    {
      LOGI("INFO: [%s] Code %d : %s\n", pLayerPrefix, messageCode, pMessage);
      return false;
    } else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
    {
      LOGI("PERF WARNING: [%s] Code %d : %s\n", pLayerPrefix, messageCode, pMessage);
      return false;
    }
    else if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
    {
      LOGI("DEBUG: [%s] Code %d : %s\n", pLayerPrefix, messageCode, pMessage);
      return false;
    } 
    LOGI("[%s] Code %d : %s\n", pLayerPrefix, messageCode, pMessage);
    return false;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool NVK::utInitialize(bool bValidationLayer, WindowSurface* pWindowSurface)
{
    m_swapChain = NULL;
    if(pWindowSurface)
    {
        nvvk::Context* pContext = pWindowSurface->getContext();
        m_deviceExternal = true;
        // No need to initialize anything: done by the framework
        // update nvk with existing device from WINinternalVK
        // keep track of the swapchain
        m_swapChain = &pWindowSurface->m_swapChain;

        m_device = pContext->m_device;
        m_instance = pContext->m_instance;
        m_CreateDebugReportCallback = NULL;//pwinInternalVK->m_CreateDebugReportCallback;
        m_DestroyDebugReportCallback = NULL;//pwinInternalVK->m_DestroyDebugReportCallback;
        m_msg_callback = NULL;//pwinInternalVK->m_msg_callback;
        m_DebugReportMessage = NULL;//pwinInternalVK->m_DebugReportMessage;
        m_gpu.device = pContext->m_physicalDevice;
        m_gpu.memoryProperties = pContext->m_physicalInfo.memoryProperties;
        m_gpu.properties = pContext->m_physicalInfo.properties10;
        m_gpu.features2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
        m_gpu.features2.features = pContext->m_physicalInfo.features10;
        m_gpu.queueProperties = pContext->m_physicalInfo.queueProperties;
        //m_gpu.graphics_queue_family_index = pwinInternalVK->m_gpu.graphics_queue_family_index;
        m_queue = pContext->m_queueGCT;
        //m_surface = pwinInternalVK->m_surface;
        //m_surfFormat = pwinInternalVK->m_surfFormat;
        //m_swap_chain = pwinInternalVK->m_swap_chain;
        //m_swapchainImageCount = pwinInternalVK->m_swapchainImageCount;
        //m_swapchaineBuffers = pwinInternalVK->m_swapchaineBuffers;
       // pfnDebugMarkerSetObjectTagEXT = pwinInternalVK->pfnDebugMarkerSetObjectTagEXT;
       // pfnDebugMarkerSetObjectNameEXT = pwinInternalVK->pfnDebugMarkerSetObjectNameEXT;
       // pfnCmdDebugMarkerBeginEXT = pwinInternalVK->pfnCmdDebugMarkerBeginEXT;
       // pfnCmdDebugMarkerEndEXT = pwinInternalVK->pfnCmdDebugMarkerEndEXT;
        //pfnCmdDebugMarkerInsertEXT = pwinInternalVK->pfnCmdDebugMarkerInsertEXT;
        return true;
    }
    m_deviceExternal = false;
    VkResult result;
    int                         queueFamilyIndex= 0;
    VkApplicationInfo         appInfo         = { VK_STRUCTURE_TYPE_APPLICATION_INFO, NULL };
    VkInstanceCreateInfo      instanceInfo    = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, NULL };
    VkDeviceQueueCreateInfo   queueInfo       = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, NULL };
    VkDeviceCreateInfo        devInfo         = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, NULL };
    std::vector<char*> extension_names;
    std::vector<VkLayerProperties>      instance_layers;
    std::vector<VkPhysicalDevice>       physical_devices;
    std::vector<VkExtensionProperties>  device_extensions;
    std::vector<VkLayerProperties>      device_layers;
    std::vector<VkExtensionProperties>  instance_extensions;
    uint32_t count = 0;

    //
    // Instance layers
    //
    result = vkEnumerateInstanceLayerProperties(&count, NULL);
    assert(result == VK_SUCCESS);
    if(count > 0)
    {
        instance_layers.resize(count);
        result = vkEnumerateInstanceLayerProperties(&count, &instance_layers[0]);
        for(int i=0; i<instance_layers.size(); i++)
        {
            LOGI("%d: Layer %s: %s\n", i, instance_layers[i].layerName, instance_layers[i].description);
        }
    }
    //
    // Instance Extensions
    //
    result = vkEnumerateInstanceExtensionProperties(NULL, &count, NULL);
    assert(result == VK_SUCCESS);
    instance_extensions.resize(count);
    extension_names.resize(count);
    result = vkEnumerateInstanceExtensionProperties(NULL, &count, &instance_extensions[0]);
    for(int i=0; i<instance_extensions.size(); i++)
    {
        LOGI("%d: Extension %s\n", i, instance_extensions[i].extensionName);
        extension_names[i] = instance_extensions[i].extensionName;
    }
    // TODO: enumerate the extensions we want to include...
    // set enabled_extension_count
    // fill extension_names

    static const char *instance_validation_layers[] = {
      "VK_LAYER_LUNARG_core_validation",
      //"VK_LAYER_LUNARG_image",
      "VK_LAYER_LUNARG_object_tracker",
      //"VK_LAYER_LUNARG_api_dump",
    };
    static int instance_validation_layers_sz = 
#ifdef _DEBUG
      bValidationLayer ? getArraySize(instance_validation_layers) : 0;
#else
      0;
#endif
    //
    // Instance creation
    //
    appInfo.pApplicationName = "...";
    appInfo.applicationVersion = 1;
    appInfo.pEngineName = "...";
    appInfo.engineVersion = 1;
    appInfo.apiVersion = VK_API_VERSION_1_0;
    instanceInfo.flags = 0;
    instanceInfo.pApplicationInfo = &appInfo;
    // add some layers here ?
    instanceInfo.enabledLayerCount = instance_validation_layers_sz;
    instanceInfo.ppEnabledLayerNames = instance_validation_layers;
    instanceInfo.enabledExtensionCount = (uint32_t)extension_names.size();
    instanceInfo.ppEnabledExtensionNames = &extension_names[0];

    result = vkCreateInstance(&instanceInfo, NULL, &m_instance);
    if(result == VK_ERROR_INCOMPATIBLE_DRIVER) {
        LOGE("Cannot find a compatible Vulkan installable client driver");
    } else if (result == VK_ERROR_EXTENSION_NOT_PRESENT) {
        LOGE("Cannot find a specified extension library");
    }
    if (result != VK_SUCCESS)
        return false;
    //
    // Callbacks
    //
    //if (1)
    {
        m_CreateDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)      vkGetInstanceProcAddr(m_instance, "vkCreateDebugReportCallbackEXT");
        m_DestroyDebugReportCallback = (PFN_vkDestroyDebugReportCallbackEXT)    vkGetInstanceProcAddr(m_instance, "vkDestroyDebugReportCallbackEXT");
        if (!m_CreateDebugReportCallback) {
            LOGE("GetProcAddr: Unable to find CreateDebugReportCallbackEXT\n");
        }
        if (!m_DestroyDebugReportCallback) {
            LOGE("GetProcAddr: Unable to find DestroyDebugReportCallbackEXT\n");
        }
        m_DebugReportMessage = (PFN_vkDebugReportMessageEXT) vkGetInstanceProcAddr(m_instance, "vkDebugReportMessageEXT");
        if (!m_DebugReportMessage) {
            LOGE("GetProcAddr: Unable to find DebugReportMessageEXT\n");
        }
        VkDebugReportCallbackCreateInfoEXT dbgCreateInfo;
        dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        dbgCreateInfo.pNext = NULL;
        dbgCreateInfo.pfnCallback = dbgFunc;
        dbgCreateInfo.pUserData = NULL;
        dbgCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
        result = m_CreateDebugReportCallback(
                  m_instance,
                  &dbgCreateInfo,
                  NULL,
                  &m_msg_callback);
        switch (result) {
        case VK_SUCCESS:
            break;
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            LOGE("CreateDebugReportCallback: out of host memory");
            return false;
            break;
        default:
            LOGE("CreateDebugReportCallback: unknown failure\n");
            return false;
            break;
        }
    }

    //
    // Physical device infos
    //
    result = vkEnumeratePhysicalDevices(m_instance, &count, NULL);
    physical_devices.resize(count);
    result = vkEnumeratePhysicalDevices(m_instance, &count, &physical_devices[0]);
    if (result != VK_SUCCESS) {
        return false;
    }
    LOGI("found %d Physical Devices (using device 0)\n", physical_devices.size());
    for(int j=0; j<physical_devices.size(); j++)
    {
        // redundant for displaying infos
        vkGetPhysicalDeviceProperties(physical_devices[j], &m_gpu.properties);
        LOGI("API ver. %x; driver ver. %x; VendorID %x; DeviceID %x; deviceType %x; Device Name: %s\n",
            m_gpu.properties.apiVersion, 
            m_gpu.properties.driverVersion, 
            m_gpu.properties.vendorID, 
            m_gpu.properties.deviceID, 
            m_gpu.properties.deviceType, 
            m_gpu.properties.deviceName);
        vkGetPhysicalDeviceMemoryProperties(physical_devices[j], &m_gpu.memoryProperties);
        for(unsigned int i=0; i<m_gpu.memoryProperties.memoryTypeCount; i++)
        {
            LOGI("Memory type %d: heapindex:%d %s|%s|%s|%s|%s\n", i,
                m_gpu.memoryProperties.memoryTypes[i].heapIndex,
                m_gpu.memoryProperties.memoryTypes[i].propertyFlags&VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT?"VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT":"",
                m_gpu.memoryProperties.memoryTypes[i].propertyFlags&VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT?"VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT":"",
                m_gpu.memoryProperties.memoryTypes[i].propertyFlags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT?"VK_MEMORY_PROPERTY_HOST_COHERENT_BIT":"",
                m_gpu.memoryProperties.memoryTypes[i].propertyFlags&VK_MEMORY_PROPERTY_HOST_CACHED_BIT?"VK_MEMORY_PROPERTY_HOST_CACHED_BIT":"",
                m_gpu.memoryProperties.memoryTypes[i].propertyFlags&VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT?"VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT":""
            );
        }
        for(unsigned int i=0; i<m_gpu.memoryProperties.memoryHeapCount; i++)
        {
            LOGI("Memory heap %d: size:%uMb %s\n", i,
                m_gpu.memoryProperties.memoryHeaps[i].size/(1024*1024),
                m_gpu.memoryProperties.memoryHeaps[i].flags&VK_MEMORY_HEAP_DEVICE_LOCAL_BIT?"VK_MEMORY_HEAP_DEVICE_LOCAL_BIT":""
            );
        }
        vkGetPhysicalDeviceFeatures2(physical_devices[j], &m_gpu.features2); // too many stuff to display
        vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[j], &count, NULL);
        m_gpu.queueProperties.resize(count);
        vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[j], &count, &m_gpu.queueProperties[0]);
        for(int i=0; i<m_gpu.queueProperties.size(); i++)
        {
            LOGI("%d: Queue count %d; flags: %s|%s|%s|%s\n", i, m_gpu.queueProperties[i].queueCount,
                (m_gpu.queueProperties[i].queueFlags&VK_QUEUE_GRAPHICS_BIT ? "VK_QUEUE_GRAPHICS_BIT":""),
                (m_gpu.queueProperties[i].queueFlags&VK_QUEUE_COMPUTE_BIT?"VK_QUEUE_COMPUTE_BIT":""),
                (m_gpu.queueProperties[i].queueFlags&VK_QUEUE_TRANSFER_BIT?"VK_QUEUE_TRANSFER_BIT":""),
                (m_gpu.queueProperties[i].queueFlags&VK_QUEUE_SPARSE_BINDING_BIT?"VK_QUEUE_SPARSE_BINDING_BIT":"")
                );
        }
        //
        // Device Layer Properties
        //
        result = vkEnumerateDeviceLayerProperties(physical_devices[j], &count, NULL);
        assert(!result);
        if(count > 0)
        {
            device_layers.resize(count);
            result = vkEnumerateDeviceLayerProperties(physical_devices[j], &count, &device_layers[0]);
            for(int i=0; i<device_layers.size(); i++)
            {
                LOGI("%d: Device layer %s: %s\n", i,device_layers[i].layerName, device_layers[i].description);
            }
        }
        result = vkEnumerateDeviceExtensionProperties(physical_devices[j], NULL, &count, NULL);
        assert(!result);
        device_extensions.resize(count);
        result = vkEnumerateDeviceExtensionProperties(physical_devices[j], NULL, &count, &device_extensions[0]);
        extension_names.resize(device_extensions.size() );
        for(int i=0; i<device_extensions.size(); i++)
        {
            LOGI("%d: HW Device Extension: %s\n", i,device_extensions[i].extensionName);
            extension_names[i] = device_extensions[i].extensionName;
        }
    }
    //
    // Take one physical device
    //
    m_gpu.device = physical_devices[0];
    vkGetPhysicalDeviceProperties(m_gpu.device, &m_gpu.properties);
    vkGetPhysicalDeviceMemoryProperties(m_gpu.device, &m_gpu.memoryProperties);
    vkGetPhysicalDeviceFeatures2(m_gpu.device, &m_gpu.features2);
    vkGetPhysicalDeviceQueueFamilyProperties(m_gpu.device, &count, NULL);
    m_gpu.queueProperties.resize(count);
    vkGetPhysicalDeviceQueueFamilyProperties(m_gpu.device, &count, &m_gpu.queueProperties[0]);
    //
    // retain the queue type that can do graphics
    //
    for(unsigned int i=0; i<count; i++)
    {
        if(m_gpu.queueProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            queueFamilyIndex = i;
            break;
        }
    }
    //
    // DeviceLayer Props
    //
    result = vkEnumerateDeviceLayerProperties(m_gpu.device, &count, NULL);
    assert(!result);
    if(count > 0)
    {
        device_layers.resize(count);
        result = vkEnumerateDeviceLayerProperties(m_gpu.device, &count, &device_layers[0]);
    }
    result = vkEnumerateDeviceExtensionProperties(m_gpu.device, NULL, &count, NULL);
    assert(!result);
    //
    // Device extensions
    //
    device_extensions.resize(count);
    result = vkEnumerateDeviceExtensionProperties(m_gpu.device, NULL, &count, &device_extensions[0]);
    //
    // Create the device
    //
    queueInfo.queueFamilyIndex = queueFamilyIndex;
    queueInfo.queueCount =  m_gpu.queueProperties[queueFamilyIndex].queueCount;
    devInfo.queueCreateInfoCount = 1;
    devInfo.pQueueCreateInfos = &queueInfo;
    devInfo.enabledLayerCount = instance_validation_layers_sz;
    devInfo.ppEnabledLayerNames = instance_validation_layers;
    devInfo.enabledExtensionCount = (uint32_t)extension_names.size();
    devInfo.ppEnabledExtensionNames = &(extension_names[0]);
    result = vkCreateDevice(m_gpu.device, &devInfo, NULL, &m_device);
    if (result != VK_SUCCESS) {
        return false;
    }
    vkGetDeviceQueue(m_device, 0, 0, &m_queue);
    //
    // Debug Markers, eventually
    //
    //pfnDebugMarkerSetObjectTagEXT = (PFN_vkDebugMarkerSetObjectTagEXT)vkGetDeviceProcAddr(m_device, "vkDebugMarkerSetObjectTagEXT");
    //pfnDebugMarkerSetObjectNameEXT = (PFN_vkDebugMarkerSetObjectNameEXT)vkGetDeviceProcAddr(m_device, "vkDebugMarkerSetObjectNameEXT");
    //pfnCmdDebugMarkerBeginEXT = (PFN_vkCmdDebugMarkerBeginEXT)vkGetDeviceProcAddr(m_device, "vkCmdDebugMarkerBeginEXT");
    //pfnCmdDebugMarkerEndEXT = (PFN_vkCmdDebugMarkerEndEXT)vkGetDeviceProcAddr(m_device, "vkCmdDebugMarkerEndEXT");
    //pfnCmdDebugMarkerInsertEXT = (PFN_vkCmdDebugMarkerInsertEXT)vkGetDeviceProcAddr(m_device, "vkCmdDebugMarkerInsertEXT");
    return true;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool NVK::utDestroy()
{
  std::map<unsigned long, VkShaderModule>::const_iterator it = m_shaderModules.begin();
  while(it != m_shaderModules.end()) {
    vkDestroyShaderModule(m_device, it->second, NULL);
    ++it;
  }
  m_shaderModules.clear();

  if(!m_deviceExternal)
        vkDestroyDevice(m_device, NULL);
    m_device = NULL;
    m_DestroyDebugReportCallback(m_instance, m_msg_callback, NULL);
    if(!m_deviceExternal)
        vkDestroyInstance(m_instance, NULL);
    m_instance = NULL;
    m_queue = NULL;
    m_gpu.clear();
    return true;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VkDeviceMemory NVK::utAllocMemAndBindBuffer(VkBuffer obj, VkFlags memProps)
{
    VkResult result;
    VkDeviceMemory deviceMem;
    VkMemoryRequirements  memReqs;
    vkGetBufferMemoryRequirements(m_device, obj, &memReqs);

    if (!memReqs.size){
      return VK_NULL_HANDLE;
    }
    //
    // Find an available memory type that satifies the requested properties.
    //
    uint32_t memoryTypeIndex;
    for (memoryTypeIndex = 0; memoryTypeIndex < m_gpu.memoryProperties.memoryTypeCount; ++memoryTypeIndex) {
      if (( memReqs.memoryTypeBits & (1<<memoryTypeIndex)) &&
        ( m_gpu.memoryProperties.memoryTypes[memoryTypeIndex].propertyFlags & memProps) == memProps) 
      {
        break;
      }
    }
    if (memoryTypeIndex >= m_gpu.memoryProperties.memoryTypeCount) {
      assert(0 && "memoryTypeIndex not found");
      return VK_NULL_HANDLE;
    }

    VkMemoryAllocateInfo memInfo = {
      VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,             // sType
      0,                                               // pNext
      memReqs.size,                                    // allocationSize
      memoryTypeIndex,                                 // memoryTypeIndex
    };

    //LOGW("allocating memory: memReqs.memoryTypeBits=%d memProps=%d memoryTypeIndex=%d Size=%d\n", memReqs.memoryTypeBits, memProps, memoryTypeIndex, memReqs.size);

    result = vkAllocateMemory(m_device, &memInfo, NULL, &deviceMem);
    if (result != VK_SUCCESS) {
      return VK_NULL_HANDLE;
    }

    result = bindBufferMemory(obj, deviceMem, 0);
    if (result != VK_SUCCESS) {
      return VK_NULL_HANDLE;
    }

    return deviceMem;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VkDeviceMemory NVK::utAllocMemAndBindImage(VkImage obj, VkFlags memProps)
{
    VkResult result;
    VkDeviceMemory deviceMem;
    VkMemoryRequirements  memReqs;
    vkGetImageMemoryRequirements(m_device, obj, &memReqs);

    if (!memReqs.size){
      return VK_NULL_HANDLE;
    }

    //
    // Find an available memory type that satifies the requested properties.
    //
    uint32_t memoryTypeIndex;
    for (memoryTypeIndex = 0; memoryTypeIndex < m_gpu.memoryProperties.memoryTypeCount; ++memoryTypeIndex) {
      if (( memReqs.memoryTypeBits & (1<<memoryTypeIndex)) &&
        ( m_gpu.memoryProperties.memoryTypes[memoryTypeIndex].propertyFlags & memProps) == memProps) 
      {
        break;
      }
    }
    if (memoryTypeIndex >= m_gpu.memoryProperties.memoryTypeCount) {
      assert(0 && "memoryTypeIndex not found");
      return VK_NULL_HANDLE;
    }

    VkMemoryAllocateInfo memInfo = {
      VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,             // sType
      0,                                               // pNext
      memReqs.size,                                    // allocationSize
      memoryTypeIndex,                                 // memoryTypeIndex
    };

    //LOGW("allocating memory: memReqs.memoryTypeBits=%d memProps=%d memoryTypeIndex=%d Size=%d\n", memReqs.memoryTypeBits, memProps, memoryTypeIndex, memReqs.size);

    result = vkAllocateMemory(m_device, &memInfo, NULL, &deviceMem);
    if (result != VK_SUCCESS) {
      return VK_NULL_HANDLE;
    }

    result = vkBindImageMemory(m_device, obj, deviceMem, 0);
    if (result != VK_SUCCESS) {
      return VK_NULL_HANDLE;
    }

    return deviceMem;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VkResult NVK::utFillBuffer(NVK::CommandPool *cmdPool, size_t size, VkResult result, const void* data, VkBuffer buffer, VkDeviceSize offset)
{
    if(!data)
        return VK_SUCCESS;
    //
    // Create staging buffer
    //
    VkBuffer bufferStage;
    BufferCreateInfo bufferStageInfo(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    CHECK(vkCreateBuffer(m_device, &bufferStageInfo, NULL, &bufferStage) );
    //
    // Allocate and bind to the buffer
    //
    VkDeviceMemory bufferStageMem;
    bufferStageMem = utAllocMemAndBindBuffer(bufferStage, (VkFlags)VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    utMemcpy(bufferStageMem, data, size);

    CommandBuffer cmd(cmdPool->allocateCommandBuffer(true));
    cmd.beginCommandBuffer(true, false);
    {
        cmd.cmdCopyBuffer(bufferStage, buffer, size, offset);
    }
    cmd.endCommandBuffer();

    VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO, NULL };
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = NULL;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = cmd;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = NULL;

    CHECK(vkQueueSubmit(m_queue, 1, &submitInfo, VK_NULL_HANDLE) );
    vkDeviceWaitIdle(m_device);
    //
    // release stuff
    //
    cmdPool->freeCommandBuffer(cmd);
    destroyBuffer(bufferStage);
    vkFreeMemory(m_device, bufferStageMem, NULL);
    //obsolete: QueueRemoveMemReferences(m_queue, 1, &bufferStageMem);
    return result;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::utFillImage(NVK::CommandPool *cmdPool, BufferImageCopy &bufferImageCopy, const void* data, VkDeviceSize dataSz, VkImage image)
{
    if(!data)
        return;
    //
    // Create staging buffer
    //
    VkBuffer bufferStage;
    BufferCreateInfo bufferStageInfo(dataSz, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    CHECK(vkCreateBuffer(m_device, &bufferStageInfo, NULL, &bufferStage) );
    //
    // Allocate and bind to the buffer
    //
    VkDeviceMemory bufferStageMem;
    bufferStageMem = utAllocMemAndBindBuffer(bufferStage, (VkFlags)VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    utMemcpy(bufferStageMem, data, dataSz);

    CommandBuffer cmd(cmdPool->allocateCommandBuffer(true));
    cmd.beginCommandBuffer(true, false);
    {
        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = bufferImageCopy.mipLevels;
        subresourceRange.layerCount = bufferImageCopy.layerCount;
        // change the image layout to copy on it
        cmd.utCmdSetImageLayout(image, VK_IMAGE_ASPECT_COLOR_BIT, 
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            subresourceRange);
        //obsolete: QueueAddMemReferences(m_queue, 1, &bufferStageMem);
        cmd.cmdCopyBufferToImage(bufferStage, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, bufferImageCopy.size(), bufferImageCopy.getItem());
        // back to proper layout for use in shaders
        //cmd.utCmdSetImageLayout(image, VK_IMAGE_ASPECT_COLOR_BIT, 
        //    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        //    subresourceRange);
    }
    cmd.endCommandBuffer();

    VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO, NULL };
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = NULL;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = cmd;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = NULL;

    CHECK(vkQueueSubmit(m_queue, 1, &submitInfo, VK_NULL_HANDLE) );
    vkDeviceWaitIdle(m_device);
    //
    // release stuff
    //
    cmdPool->freeCommandBuffer(cmd);
    destroyBuffer(bufferStage);
    vkFreeMemory(m_device, bufferStageMem, NULL);
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VkBuffer NVK::utCreateAndFillBuffer(NVK::CommandPool *cmdPool, size_t size, const void* data, VkFlags usage, VkDeviceMemory &bufferMem, VkFlags memProps)
{
    VkResult result = VK_SUCCESS;
    VkBuffer buffer;
    BufferCreateInfo bufferInfo(size, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

    CHECK(vkCreateBuffer(m_device, &bufferInfo, NULL, &buffer) );
    
    bufferMem = utAllocMemAndBindBuffer(buffer, memProps);

    if (data)
    {
      result = utFillBuffer(cmdPool, size, result, data, buffer);
    }

    return buffer;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VkSemaphore NVK::createSemaphore(VkSemaphoreCreateFlags flags)
{
    VkSemaphoreCreateInfo semCreateInfo = { 
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        NULL,
        flags
    };
    VkSemaphore sem;
    vkCreateSemaphore(m_device, &semCreateInfo, NULL, &sem);
    return sem;
}
void NVK::destroySemaphore(VkSemaphore s)
{
    vkDestroySemaphore(m_device, s, NULL);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VkFence NVK::createFence(VkFenceCreateFlags flags)
{
    VkFenceCreateInfo finfo = {
        VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        NULL,
        flags
    };
    VkFence fence;
    CHECK(vkCreateFence(m_device, &finfo, NULL, &fence) );
    return fence;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::destroyFence(VkFence fence)
{
    vkDestroyFence(m_device, fence, NULL);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool NVK::waitForFences(uint32_t fenceCount, const VkFence* pFences, VkBool32 waitAll, uint64_t timeout)
{
    if(vkWaitForFences(m_device, fenceCount, pFences, waitAll, timeout) == VK_TIMEOUT)
        return false;
    return true;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::resetFences(uint32_t fenceCount, const VkFence* pFences)
{
    CHECK(vkResetFences(m_device, fenceCount, pFences) );
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::allocateDescriptorSets( const NVK::DescriptorSetAllocateInfo& allocateInfo,
                                    VkDescriptorSet*                            pDescriptorSets)
{
    CHECK(vkAllocateDescriptorSets( m_device, allocateInfo, pDescriptorSets) );
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VkResult NVK::freeDescriptorSets(VkDescriptorPool descriptorPool,uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets)
{
    return vkFreeDescriptorSets(m_device, descriptorPool, descriptorSetCount, pDescriptorSets);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VkSampler NVK::createSampler(const NVK::SamplerCreateInfo &createInfo)
{
    VkSampler s;
    CHECK(vkCreateSampler(m_device, createInfo.getItemCst(), NULL, &s) );
    return s;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::destroySampler(VkSampler s)
{
    vkDestroySampler(m_device, s, NULL);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VkImageView NVK::createImageView(const NVK::ImageViewCreateInfo &createInfo)
{
    VkImageView s;
    CHECK(vkCreateImageView(m_device, createInfo.getItemCst(), NULL, &s) );
    return s;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::destroyImageView(VkImageView s)
{
    vkDestroyImageView(m_device, s, NULL);
}

void NVK::destroyImage(VkImage s)
{
    vkDestroyImage(m_device, s, NULL);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VkBuffer NVK::createBuffer(BufferCreateInfo &bci)
{
    VkBuffer buf;
    vkCreateBuffer(m_device, bci, NULL, &buf) ;
    return buf;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VkResult NVK::bindBufferMemory(VkBuffer &buffer, VkDeviceMemory mem, VkDeviceSize offset)
{
    return vkBindBufferMemory(m_device, buffer, mem, offset);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::queueSubmit(const NVK::SubmitInfo& submits, VkFence fence)
{
    const VkSubmitInfo* ps = submits.getItemCst(0);
    vkQueueSubmit(m_queue, (uint32_t)submits.size(), submits.getItemCst(0), fence);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VkResult NVK::queueWaitIdle()
{
    return vkQueueWaitIdle(m_queue);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VkResult NVK::deviceWaitIdle()
{
    return vkDeviceWaitIdle(m_device);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VkResult NVK::createQueryPool(const VkQueryPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkQueryPool* pQueryPool)
{
    return vkCreateQueryPool(m_device, pCreateInfo, pAllocator, pQueryPool);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::destroyQueryPool(VkQueryPool queryPool, const VkAllocationCallbacks* pAllocator)
{
    vkDestroyQueryPool(m_device, queryPool, pAllocator);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VkResult NVK::getQueryPoolResults(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, size_t dataSize, void* pData, VkDeviceSize stride, VkQueryResultFlags flags)
{
    return vkGetQueryPoolResults(m_device, queryPool, firstQuery, queryCount, dataSize, pData, stride, flags);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
NVK::MemoryChunk NVK::utAllocateMemory(size_t size, VkFlags usage, VkFlags memProps)
{
    VkResult              result;
    VkMemoryRequirements  memReqs;
    NVK::MemoryChunk        memoryChunk;

    memoryChunk.nvk         = this;
    memoryChunk.size        = size;
    memoryChunk.usage       = usage;
    memoryChunk.bValid      = false;
    memoryChunk.deviceMem   = VK_NULL_HANDLE;
    memoryChunk.memProps    = memProps;
    memoryChunk.sizeUsed    = 0;

    //
    // Create staging buffer
    //
    BufferCreateInfo bufferStageInfo(size, usage);
    CHECK(vkCreateBuffer(m_device, &bufferStageInfo, NULL, &memoryChunk.defaultBuffer) );
    //
    // Allocate and bind to the buffer
    //
    vkGetBufferMemoryRequirements(m_device, memoryChunk.defaultBuffer, &memReqs);

    if (!memReqs.size){
        return memoryChunk;
    }
    memoryChunk.alignMask   = memReqs.alignment-1;
    //
    // Find an available memory type that satifies the requested properties.
    //
    uint32_t memoryTypeIndex;
    for (memoryTypeIndex = 0; memoryTypeIndex < m_gpu.memoryProperties.memoryTypeCount; ++memoryTypeIndex) {
      if (( memReqs.memoryTypeBits & (1<<memoryTypeIndex)) &&
        ( m_gpu.memoryProperties.memoryTypes[memoryTypeIndex].propertyFlags & memProps) == memProps) 
      {
        break;
      }
    }
    if (memoryTypeIndex >= m_gpu.memoryProperties.memoryTypeCount) {
      assert(0 && "memoryTypeIndex not found");
      return memoryChunk;
    }

    VkMemoryAllocateInfo memInfo = {
      VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,             // sType
      0,                                               // pNext
      memReqs.size,                                    // allocationSize
      memoryTypeIndex,                                 // memoryTypeIndex
    };

    //LOGW("allocating memory: memReqs.memoryTypeBits=%d memProps=%d memoryTypeIndex=%d Size=%d\n", memReqs.memoryTypeBits, memProps, memoryTypeIndex, memReqs.size);

    result = vkAllocateMemory(m_device, &memInfo, NULL, &memoryChunk.deviceMem);
    if (result != VK_SUCCESS) {
      return memoryChunk;
    }

    //result = BindBufferMemory(memoryChunk.defaultBuffer, memoryChunk.deviceMem, 0);
    //if (result != VK_SUCCESS) {
    //    FreeMemory(memoryChunk.deviceMem);
    //    memoryChunk.deviceMem = NULL;
    //    return memoryChunk;
    //}

    memoryChunk.bValid = true;
    return memoryChunk;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
NVK::MemoryChunk::MemoryChunk()
{
    bValid = false;
    nvk = NULL;
    size = 0;
    usage = 0;
    memProps = 0;
    deviceMem = 0;
    defaultBuffer = 0;
    sizeUsed = 0;
    alignMask = 0;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::MemoryChunk::free()
{
    if(bValid)
    {
        bValid = false;
        if(nvk) {
            nvk->destroyBuffer(defaultBuffer);
            nvk->freeMemory(deviceMem);
        }
    }
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VkBuffer NVK::MemoryChunk::createBufferAlloc(NVK::BufferCreateInfo bufferInfo, VkFlags memProps, VkDeviceMemory *bufferMem)
{
    VkBuffer buffer;
    buffer = nvk->createBuffer(bufferInfo);
    if(buffer == VK_NULL_HANDLE)
        return buffer;
    VkResult result;
    assert((sizeUsed + bufferInfo.size) <= size);
    result = nvk->bindBufferMemory(buffer, deviceMem, sizeUsed);
    if (result != VK_SUCCESS) {
        //nvk->FreeMemory(deviceMem);
        nvk->destroyBuffer(buffer);
        deviceMem = VK_NULL_HANDLE;
        return buffer;
    }
    size_t alignedSz = ((bufferInfo.size + alignMask) & ~alignMask);
    sizeUsed += alignedSz;
    buffers.push_back(buffer); // keep track
    return buffer;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VkBuffer NVK::MemoryChunk::createBufferAlloc(size_t size, VkFlags bufferUsage, VkFlags memProps, VkDeviceMemory *bufferMem)
{
    return createBufferAlloc(BufferCreateInfo(
            size,
            bufferUsage,
            0,
            VK_SHARING_MODE_EXCLUSIVE /*VkSharingMode sharingMode*/,
            0/*uint32_t queueFamilyCount*/,
            NULL/*const uint32_t* pQueueFamilyIndices*/),
        memProps, bufferMem);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VkBuffer NVK::MemoryChunk::createBufferAllocFill(NVK::CommandPool *cmdPool, size_t size, const void* bufferData, VkFlags bufferUsage, VkFlags memProps, VkDeviceMemory *bufferMem)
{
    VkResult result = VK_SUCCESS;
    VkBuffer buffer = createBufferAlloc(BufferCreateInfo(
            size,
            bufferUsage,
            0,
            VK_SHARING_MODE_EXCLUSIVE /*VkSharingMode sharingMode*/,
            0/*uint32_t queueFamilyCount*/,
            NULL/*const uint32_t* pQueueFamilyIndices*/),
        memProps, bufferMem);
    if (bufferData)
    {
      result = nvk->utFillBuffer(cmdPool, size, result, bufferData, buffer);
    }
    return buffer;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::MemoryChunk::destroyBuffers()
{
    for(int i=0; i<buffers.size(); i++)
    {
        nvk->destroyBuffer(buffers[i]);
    }
    buffers.clear();
    sizeUsed = 0;
}


//-------------------------------------------------------------------------------------------
// from https://github.com/SaschaWillems/Vulkan/blob/master/base/vulkantools.cpp#L113
// Create an image memory barrier for changing the layout of
// an image and put it into an active command buffer
// See chapter 11.4 "Image Layout" for details
//-------------------------------------------------------------------------------------------
void NVK::CommandBuffer::utCmdSetImageLayout(
    VkImage image, 
    VkImageAspectFlags aspectMask, 
    VkImageLayout oldImageLayout, 
    VkImageLayout newImageLayout,
    VkImageSubresourceRange subresourceRange)
{
    // Some default values
    VkImageMemoryBarrier imageMemoryBarrier = {};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.pNext = NULL;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.oldLayout = oldImageLayout;
    imageMemoryBarrier.newLayout = newImageLayout;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.subresourceRange = subresourceRange;

    // Source layouts (old)
    // Source access mask controls actions that have to be finished on the old layout
    // before it will be transitioned to the new layout
    switch (oldImageLayout)
    {
    case VK_IMAGE_LAYOUT_UNDEFINED:
            // Image layout is undefined (or does not matter)
            // Only valid as initial layout
            // No flags required, listed only for completeness
            imageMemoryBarrier.srcAccessMask = 0;
            break;

    case VK_IMAGE_LAYOUT_PREINITIALIZED:
            // Image is preinitialized
            // Only valid as initial layout for linear images, preserves memory contents
            // Make sure host writes have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            // Image is a color attachment
            // Make sure any writes to the color buffer have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            // Image is a depth/stencil attachment
            // Make sure any writes to the depth/stencil buffer have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            // Image is a transfer source 
            // Make sure any reads from the image have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;

    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            // Image is a transfer destination
            // Make sure any writes to the image have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            // Image is read by a shader
            // Make sure any shader reads from the image have been finished
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;
    }

    // Target layouts (new)
    // Destination access mask controls the dependency for the new image layout
    switch (newImageLayout)
    {
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        // Image will be used as a transfer destination
        // Make sure any writes to the image have been finished
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        // Image will be used as a transfer source
        // Make sure any reads from and writes to the image have been finished
        imageMemoryBarrier.srcAccessMask = imageMemoryBarrier.srcAccessMask | VK_ACCESS_TRANSFER_READ_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        // Image will be used as a color attachment
        // Make sure any writes to the color buffer have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        // Image layout will be used as a depth/stencil attachment
        // Make sure any writes to depth/stencil buffer have been finished
        imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        // Image will be read in a shader (sampler, input attachment)
        // Make sure any writes to the image have been finished
        if (imageMemoryBarrier.srcAccessMask == 0)
        {
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        }
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    }

    // Put barrier on top
    VkPipelineStageFlags srcStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
    VkPipelineStageFlags destStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;

    // Put barrier inside setup command buffer
    vkCmdPipelineBarrier(
        m_cmdbuffer, 
        srcStageFlags, 
        destStageFlags, 
        0, 
        0, NULL,
        0, NULL,
        1, &imageMemoryBarrier);
}

