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

     --------------------------------------------------------------------

     This header is a wrapper over vulkan.h : it allows to take advantage
     of constructors and functors in order to 'nest' structure declaration
     so that the code is more compact.

     NOTE: only working for 1 single Vulkan Device, for now

     Example on RenderPass Info declaration:

    NVK::RenderPassCreateInfo rpinfo = NVK::RenderPassCreateInfo(
        NVK::AttachmentDescription
            (   VK_FORMAT_R8G8B8A8_UNORM, (VkSampleCountFlagBits)MSAA,                                        //format, samples
                VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,          //loadOp, storeOp
                VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,  //stencilLoadOp, stencilStoreOp
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL //initialLayout, finalLayout
            )
            (   VK_FORMAT_D24_UNORM_S8_UINT, (VkSampleCountFlagBits)MSAA,
                VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
                VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
            )
            (   VK_FORMAT_R8G8B8A8_UNORM, (VkSampleCountFlagBits)1,                 //format, samples
                VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,          //loadOp, storeOp
                VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,  //stencilLoadOp, stencilStoreOp
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL //initialLayout, finalLayout
            ),
        NVK::SubpassDescription
        (   VK_PIPELINE_BIND_POINT_GRAPHICS,                                                //pipelineBindPoint
            NVK::AttachmentReference(),                                                   //inputAttachments
            NVK::AttachmentReference(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),        //colorAttachments
            NVK::AttachmentReference(2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),        //resolveAttachments
            NVK::AttachmentReference(1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL),//depthStencilAttachment
            NVK::Uint32Array(),                                                             //preserveAttachments
            0                                                                               //flags
        ),
        NVK::SubpassDependency()
    );

*/ //--------------------------------------------------------------------
#ifndef _NVK_H_
#define _NVK_H_
#ifdef _DEBUG
#define CHECK(a) assert(a == VK_SUCCESS)
#else
#define CHECK(a) a
#endif

#include <vector>
#include <map>
#include <set>
#include <string>
#include <vulkan/vulkan.h>

#include <nvvk/swapchain_vk.hpp>
#include <nvvk/context_vk.hpp>
#include "window_surface_vk.hpp"
//------------------------------------------------------------------------------
// Forward declarations
//------------------------------------------------------------------------------
struct NVPWindowInternal;
namespace nvvk
{
  class ContextWindowVK;
  class SwapChain;
}

//------------------------------------------------------------------------------
// Vulkan functions in a more "friendly" way
// NOTE: vulkan-like functions don't start with vk<Capital Letter> 
// to avoid confusion...
// If a method is too far away from what Vulkan would expose, the methor will
// start with 'ut', as 'utility'
// Types that mimic Vulkan types don't have either VkXXX and start right away with XXX
// in other words, consider the NVK class as a replacement for the prefix vk in functions
// and as a replacemenet of 'Vk' in types
//------------------------------------------------------------------------------
class NVK
{
private:
    std::map<unsigned long, VkShaderModule> m_shaderModules;
public:
    PFN_vkDebugMarkerSetObjectTagEXT    pfnDebugMarkerSetObjectTagEXT;
    PFN_vkDebugMarkerSetObjectNameEXT   pfnDebugMarkerSetObjectNameEXT;
    PFN_vkCmdDebugMarkerBeginEXT        pfnCmdDebugMarkerBeginEXT;
    PFN_vkCmdDebugMarkerEndEXT          pfnCmdDebugMarkerEndEXT;
    PFN_vkCmdDebugMarkerInsertEXT       pfnCmdDebugMarkerInsertEXT;

    class MemoryChunk;
    class BufferImageCopy;
    class BufferCreateInfo;
    class FramebufferCreateInfo;
    class RenderPassCreateInfo;
    class SamplerCreateInfo;
    class DescriptorSetAllocateInfo;
    class SubmitInfo;
    class ImageViewCreateInfo;
    class CommandBufferInheritanceInfo;
    //
    // class for Command Buffer
    //
    class CommandBuffer
    {
    public:
        CommandBuffer(VkCommandBuffer _cmdbuffer = VK_NULL_HANDLE) : m_cmdbuffer(_cmdbuffer) {}
        void resetCommandBuffer(VkCommandBufferResetFlags flags);
        void endCommandBuffer();
        void beginCommandBuffer(bool singleshot, bool bRenderpassContinue, NVK::CommandBufferInheritanceInfo &cmdIInfo) const;
        void beginCommandBuffer(bool singleshot, bool bRenderpassContinue, const VkCommandBufferInheritanceInfo* pInheritanceInfo = NULL) const;

        void cmdBindPipeline(VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline);
        void cmdSetViewport(uint32_t firstViewport, uint32_t viewportCount, const VkViewport* pViewports);
        void cmdSetScissor(uint32_t firstScissor, uint32_t scissorCount, const VkRect2D* pScissors);
        void cmdSetLineWidth(float lineWidth);
        void cmdSetDepthBias(float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor);
        void cmdSetBlendConstants(const float blendConstants[4]);
        void cmdSetDepthBounds(float minDepthBounds, float maxDepthBounds);
        void cmdSetStencilCompareMask(VkStencilFaceFlags faceMask, uint32_t compareMask);
        void cmdSetStencilWriteMask(VkStencilFaceFlags faceMask, uint32_t writeMask);
        void cmdSetStencilReference(VkStencilFaceFlags faceMask, uint32_t reference);
        void cmdBindDescriptorSets(VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets);
        void cmdBindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType);
        void cmdBindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* pBuffers, const VkDeviceSize* pOffsets);
        void cmdDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
        void cmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance);
        void cmdDrawIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride);
        void cmdDrawIndexedIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride);
        void cmdDispatch(uint32_t x, uint32_t y, uint32_t z);
        void cmdDispatchIndirect(VkBuffer buffer, VkDeviceSize offset);
        void cmdCopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkDeviceSize destOffset=0, VkDeviceSize srcOffset=0);
        void cmdCopyImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageCopy* pRegions);
        void cmdBlitImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit* pRegions, VkFilter filter);
        void cmdCopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy* pRegions);
        void cmdCopyImageToBuffer(VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferImageCopy* pRegions);
        void cmdUpdateBuffer(VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const uint32_t* pData);
        void cmdFillBuffer(VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data);
        void cmdClearColorImage(VkImage image, VkImageLayout imageLayout, const VkClearColorValue* pColor, uint32_t rangeCount, const VkImageSubresourceRange* pRanges);
        void cmdClearDepthStencilImage(VkImage image, VkImageLayout imageLayout, const VkClearDepthStencilValue* pDepthStencil, uint32_t rangeCount, const VkImageSubresourceRange* pRanges);
        void cmdClearAttachments(uint32_t attachmentCount, const VkClearAttachment* pAttachments, uint32_t rectCount, const VkClearRect* pRects);
        void cmdResolveImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageResolve* pRegions);
        void cmdSetEvent(VkEvent event, VkPipelineStageFlags stageMask);
        void cmdResetEvent(VkEvent event, VkPipelineStageFlags stageMask);
        void cmdWaitEvents(uint32_t eventCount, const VkEvent* pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers);
        void cmdPipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers);
        void cmdBeginQuery(VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags);
        void cmdEndQuery(VkQueryPool queryPool, uint32_t query);
        void cmdResetQueryPool(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount);
        void cmdWriteTimestamp(VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool, uint32_t query);
        void cmdCopyQueryPoolResults(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize stride, VkQueryResultFlags flags);
        void cmdPushConstants(VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* pValues);
        void cmdBeginRenderPass(const VkRenderPassBeginInfo* pRenderPassBegin, VkSubpassContents contents);
        void cmdNextSubpass(VkSubpassContents contents);
        void cmdEndRenderPass();
        void cmdExecuteCommands(uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers);
        // ut... : methods that don't really correspond to VK API
        void utCmdSetImageLayout( VkImage image, VkImageAspectFlags aspectMask, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkImageSubresourceRange subresourceRange);

        // Debug Markers. See https://github.com/KhronosGroup/Vulkan-Docs/blob/1.0-VK_EXT_debug_marker/doc/specs/vulkan/appendices/VK_EXT_debug_marker.txt
        void debugMarkerBeginEXT(NVK &nvk, const char* pMarkerName, float r, float g, float b)
        {
            if (nvk.pfnCmdDebugMarkerBeginEXT)
            {
                VkDebugMarkerMarkerInfoEXT markerInfo = {};
                float color[4] = {r,g,b,0.0f};
                markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
                memcpy(markerInfo.color, color, sizeof(float) * 4);
                markerInfo.pMarkerName = pMarkerName;
                nvk.pfnCmdDebugMarkerBeginEXT(m_cmdbuffer, &markerInfo);
            }
        }
        void debugMarkerInsertEXT(NVK &nvk, std::string markerName, float r, float g, float b)
        {
            if (nvk.pfnCmdDebugMarkerInsertEXT)
            {
                VkDebugMarkerMarkerInfoEXT markerInfo = {};
                float color[4] = {r,g,b,0.0f};
                markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
                memcpy(markerInfo.color, color, sizeof(float) * 4);
                markerInfo.pMarkerName = markerName.c_str();
                nvk.pfnCmdDebugMarkerInsertEXT(m_cmdbuffer, &markerInfo);
            }
        }
        void debugMarkerEndEXT(NVK &nvk)
        {
            if (nvk.pfnCmdDebugMarkerEndEXT)
            {
                nvk.pfnCmdDebugMarkerEndEXT(m_cmdbuffer);
            }
        }

        operator VkCommandBuffer () { return m_cmdbuffer; }
        operator const VkCommandBuffer () const { return m_cmdbuffer; }
        operator VkCommandBuffer*  () { return &m_cmdbuffer; }
        operator const VkCommandBuffer* () const { return &m_cmdbuffer; }
        operator uint64_t () { return (uint64_t)m_cmdbuffer; }
    //private:
        VkCommandBuffer m_cmdbuffer;
    };
    //
    // class for Command Pool
    //
    class CommandPool
    {
    public:
        CommandPool(VkDevice device = VK_NULL_HANDLE, VkCommandPool cmdPool = VK_NULL_HANDLE);
        CommandPool(const CommandPool &srcPool);
        // remember: resetCommandPool() will reset every command-buffer related to it
        void                  resetCommandPool(VkCommandPoolResetFlags flags);
        void                  destroyCommandPool();
        VkCommandBuffer       allocateCommandBuffers(bool primary, uint32_t nCmdBuffers, VkCommandBuffer *cmdBuffers);
        VkCommandBuffer       allocateCommandBuffer(bool primary);
        void                  freeCommandBuffers(uint32_t nCmdBuffers, VkCommandBuffer *cmdBuffers);
        void                  freeCommandBuffer(VkCommandBuffer cmdBuffer);

        operator VkCommandPool  () { return m_cmdPool; }
        operator const VkCommandPool () const { return m_cmdPool; }
        operator VkCommandPool*  () { return &m_cmdPool; }
        operator const VkCommandPool* () const { return &m_cmdPool; }
        operator uint64_t () { return (uint64_t)m_cmdPool; }
    //private:
        VkCommandPool m_cmdPool;
        VkDevice      m_device;
        int           m_currentCmdBufferSlot[2];
        #define BUFFERPRIMARY 0
        #define BUFFERSECONDARY 1
        std::vector<VkCommandBuffer> m_allocatedCmdBuffers[2];
    };

    bool              m_deviceExternal;
    nvvk::SwapChain*  m_swapChain;
    VkDevice          m_device;
    VkInstance        m_instance;
    //vk_ext_debug_report.h:
    PFN_vkCreateDebugReportCallbackEXT  m_CreateDebugReportCallback;
    PFN_vkDestroyDebugReportCallbackEXT m_DestroyDebugReportCallback;
    VkDebugReportCallbackEXT            m_msg_callback;
    PFN_vkDebugReportMessageEXT         m_DebugReportMessage;
    //
    // Physical device stuff
    //
    struct GPU
    {
        VkPhysicalDevice                    device;
        VkPhysicalDeviceMemoryProperties    memoryProperties;
        VkPhysicalDeviceProperties          properties;
        VkPhysicalDeviceFeatures2            features2;
        std::vector<VkQueueFamilyProperties>  queueProperties;
        void clear() {
            memset(&device, 0, sizeof(VkPhysicalDevice));
            memset(&memoryProperties, 0, sizeof(VkPhysicalDeviceMemoryProperties));
            memset(&properties, 0, sizeof(VkPhysicalDeviceProperties));
            memset(&features2, 0, sizeof(VkPhysicalDeviceFeatures2));
            queueProperties.clear();
        }
    };
    GPU             m_gpu;
    VkQueue       m_queue;
    //
    // Initialization utilities
    //
    bool utInitialize(bool bValidationLayer, WindowSurface* pWindowSurface = NULL);
    bool utDestroy();
    //
    // ut... : methods that don't really correspond to VK API
    //
    //VkDeviceMemory        utAllocMemAndBindObject(VkObject obj, VkObjectType type, VkFlags memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VkDeviceMemory        utAllocMemAndBindBuffer(VkBuffer obj, VkFlags memProps=VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VkDeviceMemory        utAllocMemAndBindImage(VkImage obj, VkFlags memProps=VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    MemoryChunk           utAllocateMemory(size_t size, VkFlags usage, VkFlags memProps=VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VkResult              utFillBuffer(CommandPool *cmdPool,  size_t size, VkResult result, const void* data, VkBuffer buffer, VkDeviceSize offset = 0);
    void                  utFillImage(CommandPool *cmdPool, BufferImageCopy &bufferImageCopy, const void* data, VkDeviceSize dataSz, VkImage image);
    VkBuffer              utCreateAndFillBuffer(CommandPool *cmdPool, size_t size, const void* data, VkFlags usage, VkDeviceMemory &bufferMem, VkFlags memProps=VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VkImage               utCreateImage1D(int width, VkDeviceMemory &colorMemory, VkFormat format, VkSampleCountFlagBits depthSamples=VK_SAMPLE_COUNT_1_BIT, VkSampleCountFlagBits colorSamples=VK_SAMPLE_COUNT_1_BIT, int mipLevels = 1, bool asAttachment=false);
    VkImage               utCreateImage2D(int width, int height, VkDeviceMemory &colorMemory, VkFormat format, VkSampleCountFlagBits depthSamples=VK_SAMPLE_COUNT_1_BIT, VkSampleCountFlagBits colorSamples=VK_SAMPLE_COUNT_1_BIT, int mipLevels = 1, bool asAttachment=false);
    VkImage               utCreateImage3D(int width, int height, int depth, VkDeviceMemory &colorMemory, VkFormat format, VkSampleCountFlagBits depthSamples=VK_SAMPLE_COUNT_1_BIT, VkSampleCountFlagBits colorSamples=VK_SAMPLE_COUNT_1_BIT, int mipLevels = 1, bool asAttachment=false);
    VkImage               utCreateImageCube(int width, VkDeviceMemory &colorMemory, VkFormat format, VkSampleCountFlagBits depthSamples=VK_SAMPLE_COUNT_1_BIT, VkSampleCountFlagBits colorSamples=VK_SAMPLE_COUNT_1_BIT, int mipLevels = 1, bool asAttachment=false);
    void                  utMemcpy(VkDeviceMemory dstMem, const void * srcData, VkDeviceSize size);
    //
    // VULKAN function Overrides: similar to Vulkan API but simplified arguments (using structs...)
    // NON-EXHAUSTIVE LIST: need to add missing ones when needed
    //
    VkBuffer              createBuffer(BufferCreateInfo &bci);
    VkResult              bindBufferMemory(VkBuffer &buffer, VkDeviceMemory mem, VkDeviceSize offset);
    VkShaderModule        createShaderModule( const char *shaderCode, size_t size);
    VkBufferView          createBufferView( VkBuffer buffer, VkFormat format, VkDeviceSize size );
    VkFramebuffer         createFramebuffer(FramebufferCreateInfo &fbinfo);
    VkResult              createCommandPool(const VkCommandPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, CommandPool *commandPool);
    void                  freeMemory(VkDeviceMemory mem);
    void*                 mapMemory(VkDeviceMemory mem, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags);
    void                  unmapMemory(VkDeviceMemory mem);

    void                  destroyBufferView(VkBufferView bufferView);
    void                  destroyBuffer(VkBuffer buffer);
    void                  destroyRenderPass(VkRenderPass renderPass, const VkAllocationCallbacks* pAllocator);
    void                  destroyFramebuffer(VkFramebuffer framebuffer, const VkAllocationCallbacks* pAllocator);
    void                  destroyDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout, const VkAllocationCallbacks* pAllocator);
    void                  destroyDescriptorPool(VkDescriptorPool descriptorPool, const VkAllocationCallbacks* pAllocator);
    void                  destroyPipelineLayout(VkPipelineLayout pipelineLayout, const VkAllocationCallbacks* pAllocator);
    void                  destroyPipeline(VkPipeline pipeline, const VkAllocationCallbacks* pAllocator);

    VkRenderPass          createRenderPass(RenderPassCreateInfo &rpinfo);

    VkSemaphore           createSemaphore(VkSemaphoreCreateFlags flags=0);
    void                  destroySemaphore(VkSemaphore s);
    VkFence               createFence(VkFenceCreateFlags flags=0);
    void                  destroyFence(VkFence fence);
    bool                  waitForFences(uint32_t fenceCount, const VkFence* pFences, VkBool32 waitAll, uint64_t timeout);
    void                  resetFences(uint32_t fenceCount, const VkFence* pFences);

    void                  allocateDescriptorSets(const NVK::DescriptorSetAllocateInfo& allocateInfo, VkDescriptorSet* pDescriptorSets);
    VkResult              freeDescriptorSets(VkDescriptorPool descriptorPool,uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets);
    VkSampler             createSampler(const NVK::SamplerCreateInfo &createInfo);
    void                  destroySampler(VkSampler s);

    VkImageView           createImageView(const NVK::ImageViewCreateInfo &createInfo);
    void                  destroyImageView(VkImageView s);
    void                  destroyImage(VkImage s);

    void                  queueSubmit(const NVK::SubmitInfo& submits, VkFence fence);

    VkResult              queueWaitIdle();
    VkResult              deviceWaitIdle();

    VkResult              createQueryPool(const VkQueryPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkQueryPool* pQueryPool);
    void                  destroyQueryPool(VkQueryPool queryPool, const VkAllocationCallbacks* pAllocator);
    VkResult              getQueryPoolResults(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, size_t dataSize, void* pData, VkDeviceSize stride, VkQueryResultFlags flags);
    //
    // Debug Markers
    // Extension spec can be found at https://github.com/KhronosGroup/Vulkan-Docs/blob/1.0-VK_EXT_debug_marker/doc/specs/vulkan/appendices/VK_EXT_debug_marker.txt
    //
    void debugMarkerSetObjectNameEXT(uint64_t object, VkDebugReportObjectTypeEXT objectType, const char *name)
    {
        // Check for valid function pointer (may not be present if not running in a debugging application)
        if (pfnDebugMarkerSetObjectNameEXT)
        {
            VkDebugMarkerObjectNameInfoEXT nameInfo = {};
            nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
            nameInfo.objectType = objectType;
            nameInfo.object = object;
            nameInfo.pObjectName = name;
            pfnDebugMarkerSetObjectNameEXT(m_device, &nameInfo);
        }
    }

    // Set the tag for an object
    void debugMarkerSetObjectTagEXT(uint64_t object, VkDebugReportObjectTypeEXT objectType, uint64_t name, size_t tagSize, const void* tag)
    {
        // Check for valid function pointer (may not be present if not running in a debugging application)
        if (pfnDebugMarkerSetObjectTagEXT)
        {
            VkDebugMarkerObjectTagInfoEXT tagInfo = {};
            tagInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_TAG_INFO_EXT;
            tagInfo.objectType = objectType;
            tagInfo.object = object;
            tagInfo.tagName = name;
            tagInfo.tagSize = tagSize;
            tagInfo.pTag = tag;
            pfnDebugMarkerSetObjectTagEXT(m_device, &tagInfo);
        }
    }
    //
    // overloadind strcuts for better constructors
    //
    struct Offset2D : VkOffset2D {
        Offset2D( int32_t _x, int32_t _y) { x = _x; y = _y;}
    };
    struct Offset3D : VkOffset3D {
        Offset3D( int32_t _x, int32_t _y, int32_t _z) { x = _x; y = _y; z = _z; }
    };
    struct Extent2D : VkExtent2D{
        Extent2D(int32_t _width, int32_t _height) { width = _width; height = _height; }
    };
    struct Rect2D : VkRect2D {
        Rect2D() {}
        Rect2D(const VkRect2D& r) { offset = r.offset; extent = r.extent; }
        Rect2D(const Offset2D &_offset, const Extent2D &_extent) { offset = _offset; extent = _extent; }
        Rect2D(int32_t originX, int32_t originY, uint32_t width, uint32_t height) {
            offset.x = originX; offset.y = originY;
            extent.width = width; extent.height = height;
        }
        operator VkRect2D* () { return this; }
        Rect2D& operator=(const VkRect2D& r) { offset = r.offset; extent = r.extent; return *this; }
    };
    struct Extent3D : VkExtent3D {
        Extent3D() {}
        Extent3D(int32_t _width, int32_t _height, int32_t _depth) { width = _width; height = _height; depth = _depth; }
    };
    struct Float4 {
        Float4() { memset(v, 0, sizeof(float)*4); }
        Float4(float x, float y, float z, float w) { v[0]=x; v[1]=y; v[2]=z; v[3]=w; }
        operator float*() { return v; }
        float v[4];
    };
    //---------------------------------
    struct ImageSubresource : VkImageSubresource {
        ImageSubresource(
            VkImageAspectFlags _aspectMask,
            uint32_t      _mipLevel,
            uint32_t      _arrayLayer
            ) { aspectMask = _aspectMask; mipLevel = _mipLevel; arrayLayer = _arrayLayer; }
        ImageSubresource() {}
    };
    //---------------------------------
    struct ImageSubresourceLayers : VkImageSubresourceLayers {
        ImageSubresourceLayers(
            VkImageAspectFlags _aspectMask,
            uint32_t      _mipLevel,
            uint32_t      _baseArrayLayer,
            uint32_t      _layerCount
            ) { aspectMask = _aspectMask; mipLevel = _mipLevel; baseArrayLayer = _baseArrayLayer; layerCount = _layerCount; }
        ImageSubresourceLayers() {}
    };
    //---------------------------------
    class BufferImageCopy {
    public:
        BufferImageCopy() {
            mipLevels = 0;
            layerCount = 0;
        }
        BufferImageCopy(const BufferImageCopy &src) { s = src.s; }
        BufferImageCopy(            
            VkDeviceSize                                bufferOffset,
            uint32_t                                    bufferRowLength,
            uint32_t                                    bufferImageHeight,
            ImageSubresourceLayers const              &imageSubresource,
            Offset3D const                            &imageOffset,
            Extent3D const                            &imageExtent,
            int miplevel = 0, int layer = 0)
        {
            operator()(bufferOffset, bufferRowLength, bufferImageHeight, imageSubresource, imageOffset, imageExtent, miplevel, layer);
        }
        inline BufferImageCopy& operator () (
            VkDeviceSize                                bufferOffset,
            uint32_t                                    bufferRowLength,
            uint32_t                                    bufferImageHeight,
            ImageSubresourceLayers const              &imageSubresource,
            Offset3D const                            &imageOffset,
            Extent3D const                            &imageExtent,
            int miplevel = 0, int layer = 0)
        {
            VkBufferImageCopy b = {bufferOffset, bufferRowLength, bufferImageHeight, imageSubresource, imageOffset, imageExtent};
            miplevel++;
            layer++;
            if(mipLevels < miplevel)
                mipLevels =  miplevel;
            if(layerCount < layer)
                layerCount =  layer;
            s.push_back(b);
            return *this;
        }
        void add(
            VkDeviceSize                                bufferOffset,
            uint32_t                                    bufferRowLength,
            uint32_t                                    bufferImageHeight,
            ImageSubresourceLayers const              &imageSubresource,
            Offset3D const                            &imageOffset,
            Extent3D const                            &imageExtent,
            int miplevel = 0, int layer = 0)
        {
            operator()(bufferOffset, bufferRowLength, bufferImageHeight, imageSubresource, imageOffset, imageExtent, miplevel, layer);
        }
        inline BufferImageCopy& operator=(const BufferImageCopy& src)
        {
          s = src.s;
          return *this;
        }
        inline size_t size() { return s.size(); }
        inline VkBufferImageCopy* getItem(int i=0) { return &(s[i]); }
        inline const VkBufferImageCopy* getItemCst(int i=0) const { return &(s[i]); }
        int mipLevels;
        int layerCount;
    private:
        std::vector<VkBufferImageCopy> s;
    };
    //---------------------------------
    class ImageCreateInfo {
    public:
        ImageCreateInfo(
            VkFormat                                    format,
            Extent3D                                  extent,
            VkImageType                                 imageType=VK_IMAGE_TYPE_2D,
            uint32_t                                    mipLevels=1,
            uint32_t                                    arrayLayers=1,
            VkSampleCountFlagBits                       samples=VK_SAMPLE_COUNT_1_BIT,
            VkImageTiling                               tiling=VK_IMAGE_TILING_OPTIMAL,
            VkImageUsageFlags                           usage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VkImageCreateFlags                          flags = 0,
            VkSharingMode                               sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            uint32_t                                    queueFamilyIndexCount = 0,
            const uint32_t*                             pQueueFamilyIndices = NULL)
    {
        s.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        s.pNext = NULL;
        s.imageType = imageType;
        s.format = format;
        s.extent = extent;
        s.mipLevels = mipLevels;
        s.arrayLayers = arrayLayers;
        s.samples = samples;
        s.tiling = tiling;
        s.usage = usage;
        s.flags = flags;
        s.sharingMode = sharingMode;
        s.queueFamilyIndexCount = queueFamilyIndexCount;
        s.pQueueFamilyIndices = pQueueFamilyIndices;
    }

    private:
        VkImageCreateInfo s;
    };
    //---------------------------------
    class SamplerCreateInfo {
    public:
        SamplerCreateInfo() {}
        SamplerCreateInfo(
            VkFilter                                    magFilter,
            VkFilter                                    minFilter,
            VkSamplerMipmapMode                         mipmapMode,
            VkSamplerAddressMode                        addressModeU,
            VkSamplerAddressMode                        addressModeV,
            VkSamplerAddressMode                        addressModeW,
            float                                       mipLodBias,
            VkBool32                                    anisotropyEnable,
            float                                       maxAnisotropy,
            VkBool32                                    compareEnable,
            VkCompareOp                                 compareOp,
            float                                       minLod,
            float                                       maxLod,
            VkBorderColor                               borderColor,
            VkBool32                                    unnormalizedCoordinates
        )
        {
            s.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            s.pNext = NULL;
            s.magFilter = magFilter;
            s.minFilter = minFilter;
            s.mipmapMode = mipmapMode;
            s.addressModeU = addressModeU;
            s.addressModeV = addressModeV;
            s.addressModeW = addressModeW;
            s.mipLodBias = mipLodBias;
            s.anisotropyEnable = anisotropyEnable;
            s.maxAnisotropy = maxAnisotropy;
            s.compareEnable = compareEnable;
            s.compareOp = compareOp;
            s.minLod = minLod;
            s.maxLod = maxLod;
            s.borderColor = borderColor;
            s.unnormalizedCoordinates = unnormalizedCoordinates;
            s.flags = 0;
        }
        inline VkSamplerCreateInfo* getItem() { return &s; }
        inline const VkSamplerCreateInfo * getItemCst() const { return &s; }
    private:
        VkSamplerCreateInfo   s;
    };
    //---------------------------------
    class ComponentMapping {
    public:
        ComponentMapping() 
        {
            s.r = VK_COMPONENT_SWIZZLE_R; s.g = VK_COMPONENT_SWIZZLE_G; s.b = VK_COMPONENT_SWIZZLE_B; s.a =VK_COMPONENT_SWIZZLE_A;
        }
        ComponentMapping(VkComponentSwizzle r, VkComponentSwizzle g, VkComponentSwizzle b, VkComponentSwizzle a)
        {
            s.r = r; s.g = g; s.b = b; s.a =a;
        }
        inline VkComponentMapping* getItem() { return &s; }
        inline const VkComponentMapping* getItemCst() const { return &s; }
        VkComponentMapping s;
    };
    //---------------------------------
    class BufferCreateInfo : public VkBufferCreateInfo {
    public:
        BufferCreateInfo(
            VkDeviceSize                                size_,
            VkBufferUsageFlags                          usage_,
            VkBufferCreateFlags                         flags_ = 0,
            VkSharingMode                               sharingMode_ = VK_SHARING_MODE_EXCLUSIVE,
            uint32_t                                    queueFamilyIndexCount_ = 0,
            const uint32_t*                             pQueueFamilyIndices_ = NULL)
        {
            sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            pNext = NULL;
            size = size_;
            usage = usage_;
            flags = flags_;
            sharingMode = sharingMode_;
            queueFamilyIndexCount = queueFamilyIndexCount_;
            pQueueFamilyIndices = pQueueFamilyIndices_;
        }
        inline VkBufferCreateInfo* getItem() { return this; }
        inline const VkBufferCreateInfo* getItemCst() const { return this; }
        operator VkBufferCreateInfo* () { return this; }
        operator const VkBufferCreateInfo* () const { return this; }
    };
    //---------------------------------
    class ImageSubresourceRange {
    public:
        ImageSubresourceRange(
            VkImageAspectFlags                          aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT,
            uint32_t                                    baseMipLevel    = 0,
            uint32_t                                    levelCount      = 1,
            uint32_t                                    baseArrayLayer  = 0,
            uint32_t                                    layerCount      = 1
        ) {
            s.aspectMask = aspectMask;
            s.baseMipLevel = baseMipLevel;
            s.levelCount = levelCount;
            s.baseArrayLayer = baseArrayLayer;
            s.layerCount = layerCount;
        }
        inline VkImageSubresourceRange* getItem() { return &s; }
        inline const VkImageSubresourceRange* getItemCst() const { return &s; }
        operator VkImageSubresourceRange* () { return &s; }
        operator const VkImageSubresourceRange* () const { return &s; }
        VkImageSubresourceRange s;
    };
    //---------------------------------
    class ImageViewCreateInfo {
    public:
        ImageViewCreateInfo() {}
        ImageViewCreateInfo(
            VkImage                                     image,
            VkImageViewType                             viewType,
            VkFormat                                    format,
            const ComponentMapping                          &components,
            const ImageSubresourceRange                     &subresourceRange
        ) {
            s.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            s.pNext = NULL;
            s.image = image;
            s.viewType = viewType;
            s.format = format;
            s.components = components.s;
            s.subresourceRange = subresourceRange.s;
            s.flags = 0;
        }
        inline VkImageViewCreateInfo* getItem() { return &s; }
        inline const VkImageViewCreateInfo* getItemCst() const { return &s; }
    private:
        VkImageViewCreateInfo s;
    };
    //---------------------------------
    class DescriptorSetAllocateInfo
    {
    public:
        DescriptorSetAllocateInfo(
            VkDescriptorPool                            descriptorPool,
            uint32_t                                    descriptorSetCount,
            const VkDescriptorSetLayout*                pSetLayouts )
        {
            s.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            s.pNext = NULL;
            s.descriptorPool = descriptorPool;
            s.descriptorSetCount = descriptorSetCount;
            s.pSetLayouts = pSetLayouts;
        }
        inline VkDescriptorSetAllocateInfo* getItem() { return &s; }
        inline const VkDescriptorSetAllocateInfo* getItemCst() const { return &s; }
        inline operator VkDescriptorSetAllocateInfo* () { return &s; }
        inline operator const VkDescriptorSetAllocateInfo* () const { return &s; }
    private:
        VkDescriptorSetAllocateInfo s;
    };
    //---------------------------------
    class DescriptorSetLayoutBinding
    {
    public:
        DescriptorSetLayoutBinding() {}
        DescriptorSetLayoutBinding(
                uint32_t                                    binding,
                VkDescriptorType                            descriptorType,
                uint32_t                                    descriptorCount,
                VkShaderStageFlags                          stageFlags)
        {
            operator()(binding,descriptorType,descriptorCount,stageFlags);
        }
        inline DescriptorSetLayoutBinding& operator () (
                uint32_t                                    binding,
                VkDescriptorType                            descriptorType,
                uint32_t                                    descriptorCount,
                VkShaderStageFlags                          stageFlags)
        {
            VkDescriptorSetLayoutBinding b = {
                binding,
                descriptorType,
                descriptorCount,
                stageFlags,
                NULL /*pImmutableSamplers*/ };
            bindings.push_back(b);
            return *this;
        }
        inline int size() const { return (int)bindings.size(); }
        inline VkDescriptorSetLayoutBinding* getBindings() { return &(bindings[0]); }
    private:
        std::vector<VkDescriptorSetLayoutBinding> bindings;
    };
    //---------------------------------
    class DescriptorSetLayoutCreateInfo
    {
    public:
        DescriptorSetLayoutCreateInfo(const DescriptorSetLayoutBinding &dslb_, VkDescriptorSetLayoutCreateFlags flags = 0)
        {
            dslb = dslb_;
            descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorSetLayoutCreateInfo.pNext = NULL;
            descriptorSetLayoutCreateInfo.flags = flags;
            descriptorSetLayoutCreateInfo.bindingCount =    dslb.size();// NUM_UBOS;//sizeof(bindings)/sizeof(bindings[0]);
            descriptorSetLayoutCreateInfo.pBindings =       dslb.getBindings();
        }
        VkDescriptorSetLayoutCreateInfo* getItem() { return &descriptorSetLayoutCreateInfo; }
        const VkDescriptorSetLayoutCreateInfo* getItemCst() const { return &descriptorSetLayoutCreateInfo; }
    private:
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
        DescriptorSetLayoutBinding dslb;
    };
    //----------------------------------
    VkDescriptorSetLayout   createDescriptorSetLayout(const DescriptorSetLayoutCreateInfo &descriptorSetLayoutCreateInfo);
    VkPipelineLayout        createPipelineLayout(VkDescriptorSetLayout* dsls, uint32_t count);
    //---------------------------------
    class DescriptorPoolSize
    {
    public:
        DescriptorPoolSize() {}
        DescriptorPoolSize(VkDescriptorType t, uint32_t cnt=1)
        {
            VkDescriptorPoolSize b = { t, cnt };
            typecount.push_back(b);
        }
        inline DescriptorPoolSize& operator () (VkDescriptorType t, uint32_t cnt=1)
        {
            VkDescriptorPoolSize b = { t, cnt };
            typecount.push_back(b);
            return *this;
        }
        inline int size() const { return (int)typecount.size(); }
        inline VkDescriptorPoolSize* getItem(int n=0) { return &(typecount[n]); }
        inline const VkDescriptorPoolSize* getItemCst(int n=0) const { return &(typecount[n]); }
    private:
        std::vector<VkDescriptorPoolSize> typecount;
    };
    //---------------------------------
    class DescriptorPoolCreateInfo
    {
    public:
        DescriptorPoolCreateInfo(
            uint32_t                                    maxSets,
            const DescriptorPoolSize                 &typeCount,
            VkDescriptorPoolCreateFlags                 flags = 0)
        {
            s.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            s.pNext = NULL;
            s.flags = flags;
            s.maxSets = maxSets;
            t = typeCount;
            s.poolSizeCount = t.size();
            s.pPoolSizes = t.getItem(0);
        }
        inline VkDescriptorPoolCreateInfo* getItem() { return &s; }
        inline const VkDescriptorPoolCreateInfo* getItemCst() const { return &s; }
    private:
        VkDescriptorPoolCreateInfo    s;
        DescriptorPoolSize            t;
    };
    //----------------------------------
    VkDescriptorPool    createDescriptorPool(const DescriptorPoolCreateInfo &descriptorPoolCreateInfo);
    //---------------------------------
    class DescriptorImageInfo
    {
    public:
        DescriptorImageInfo() {}
        DescriptorImageInfo(VkSampler                                   sampler,
                              VkImageView                                 imageView,
                              VkImageLayout                               imageLayout)
        {
            operator()(sampler, imageView, imageLayout);
        }
        inline DescriptorImageInfo& operator () (
                              VkSampler                                   sampler,
                              VkImageView                                 imageView,
                              VkImageLayout                               imageLayout)
        {
            VkDescriptorImageInfo b;
            b.sampler           = sampler;
            b.imageView         = imageView;
            b.imageLayout       = imageLayout;
            attachInfos.push_back(b);
            return *this;
        }
        inline int size() const { return (int)attachInfos.size(); }
        inline VkDescriptorImageInfo* getItem(int n=0) { return &(attachInfos[n]); }
    private:
        std::vector<VkDescriptorImageInfo> attachInfos;
    };
    class DescriptorBufferInfo
    {
    public:
        DescriptorBufferInfo() {}
        //VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
        DescriptorBufferInfo(    VkBuffer                                    buffer,
                                VkDeviceSize                                offset,
                                VkDeviceSize                                range)
        {
            operator()(buffer, offset, range);
        }
        inline DescriptorBufferInfo& operator () (
                                VkBuffer                                    buffer,
                                VkDeviceSize                                offset,
                                VkDeviceSize                                range)
        {
            VkDescriptorBufferInfo b;
            b.buffer = buffer;
            b.offset = offset;
            b.range  = range;
            attachInfos.push_back(b);
            return *this;
        }
        inline int size() const { return (int)attachInfos.size(); }
        inline VkDescriptorBufferInfo* getItem(int n=0) { return &(attachInfos[n]); }
        inline const VkDescriptorBufferInfo* getItemCst(int n=0) const { return &(attachInfos[n]); }
    private:
        std::vector<VkDescriptorBufferInfo> attachInfos;
    };
    //----------------------------------
    class WriteDescriptorSet
    {
    public:
        WriteDescriptorSet() {}
        WriteDescriptorSet(VkDescriptorSet descSetDest, uint32_t binding, uint32_t arrayIndex, DescriptorBufferInfo& bufferInfos, VkDescriptorType descriptorType)
        {
            operator()(descSetDest, binding, arrayIndex, bufferInfos, descriptorType);
        }
        inline WriteDescriptorSet& operator () (VkDescriptorSet descSetDest, uint32_t binding, uint32_t arrayIndex, DescriptorBufferInfo& bufferInfos, VkDescriptorType descriptorType)
        {
            VkWriteDescriptorSet ub = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
            ub.pNext = NULL;
            ub.dstSet = descSetDest;
            ub.dstBinding = binding;
            ub.dstArrayElement = arrayIndex;
            ub.descriptorCount = bufferInfos.size();
            ub.descriptorType =    descriptorType;
            ub.pImageInfo =    NULL;
            ub.pBufferInfo = bufferInfos.getItem(0);
            ub.pTexelBufferView = NULL;
            updateBuffers.push_back(ub);
            return *this;
        }
        WriteDescriptorSet(VkDescriptorSet descSetDest, uint32_t binding, uint32_t arrayIndex, DescriptorImageInfo& imageInfos, VkDescriptorType descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
        {
            operator()(descSetDest, binding, arrayIndex, imageInfos, descriptorType);
        }
        inline WriteDescriptorSet& operator () (VkDescriptorSet descSetDest, uint32_t binding, uint32_t arrayIndex, NVK::DescriptorImageInfo& imageInfos, VkDescriptorType descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
        {
            VkWriteDescriptorSet ub = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
            ub.pNext = NULL;
            ub.dstSet = descSetDest;
            ub.dstBinding = binding;
            ub.dstArrayElement = arrayIndex;
            ub.descriptorCount = imageInfos.size();
            ub.descriptorType =    descriptorType;//VK_DESCRIPTOR_TYPE_STORAGE_IMAGE or VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER or VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
            ub.pImageInfo =    imageInfos.getItem(0); 
            ub.pBufferInfo = NULL;
            ub.pTexelBufferView = NULL;
            updateBuffers.push_back(ub);
            return *this;
        }
        WriteDescriptorSet(VkDescriptorSet descSetDest, uint32_t binding, uint32_t arrayIndex, uint32_t count, const VkBufferView* pTexelBufferViews, VkDescriptorType descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER)
        {
            operator()(descSetDest, binding, arrayIndex, count, pTexelBufferViews, descriptorType);
        }
        inline WriteDescriptorSet& operator () (VkDescriptorSet descSetDest, uint32_t binding, uint32_t arrayIndex, uint32_t count, const VkBufferView* pTexelBufferViews, VkDescriptorType descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER)
        {
            VkWriteDescriptorSet ub = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
            ub.pNext = NULL;
            ub.dstSet = descSetDest;
            ub.dstBinding = binding;
            ub.dstArrayElement = arrayIndex;
            ub.descriptorCount = count;
            ub.descriptorType =    descriptorType;//VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER or VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER
            ub.pImageInfo =    NULL;
            ub.pBufferInfo = NULL;
            ub.pTexelBufferView = pTexelBufferViews;
            updateBuffers.push_back(ub);
            return *this;
        }
        inline int size() const { return (int)updateBuffers.size(); }
        inline VkWriteDescriptorSet* getItem(int n) { return &(updateBuffers[n]); }
        inline const VkWriteDescriptorSet* getItemCst(int n) const { return &(updateBuffers[n]); }
    private:
        std::vector<VkWriteDescriptorSet> updateBuffers;
    };
    //----------------------------------
    void updateDescriptorSets(const WriteDescriptorSet &wds);
    //----------------------------------
    class VertexInputBindingDescription
    {
    public:
        VertexInputBindingDescription() {}
        VertexInputBindingDescription(uint32_t binding, uint32_t stride, VkVertexInputRate inputRate)
        {
            VkVertexInputBindingDescription ub = {binding, stride, inputRate };
            bindings.push_back(ub);
        }
        inline VertexInputBindingDescription& operator () (uint32_t binding, uint32_t stride, VkVertexInputRate inputRate)
        {
            VkVertexInputBindingDescription ub = {binding, stride, inputRate };
            bindings.push_back(ub);
            return *this;
        }
        inline int size() const { return (int)bindings.size(); }
        inline VkVertexInputBindingDescription* getItem(int n=0) { return &(bindings[n]); }
        inline const VkVertexInputBindingDescription* getItemCst(int n=0) const { return &(bindings[n]); }
    private:
        std::vector<VkVertexInputBindingDescription> bindings;
    };
    //----------------------------------
    class VertexInputAttributeDescription
    {
    public:
        VertexInputAttributeDescription() {}
        VertexInputAttributeDescription(uint32_t location, uint32_t binding, VkFormat format, uint32_t offsetInBytes)
        {
            VkVertexInputAttributeDescription iad = { location, binding, format, offsetInBytes };
            ia.push_back(iad);
        }
        inline VertexInputAttributeDescription& operator () (uint32_t location, uint32_t binding, VkFormat format, uint32_t offsetInBytes)
        {
            VkVertexInputAttributeDescription iad = { location, binding, format, offsetInBytes };
            ia.push_back(iad);
            return *this;
        }
        inline int size() const { return (int)ia.size(); }
        inline VkVertexInputAttributeDescription* getItem(int n=0) { return &(ia[n]); }
        inline const VkVertexInputAttributeDescription* getItemCst(int n=0) const { return &(ia[n]); }
    private:
        std::vector<VkVertexInputAttributeDescription> ia;
    };
    //----------------------------------
    class PipelineBaseCreateInfo
    {
    public:
        virtual void setNext(PipelineBaseCreateInfo& p) = 0;
        virtual const void* getPtr() const = 0;
        virtual const VkStructureType getType() const = 0;
    };
#define NVKPIPELINEBASECREATEINFOIMPLE \
        virtual void setNext(PipelineBaseCreateInfo& p) { s.pNext = p.getPtr(); }\
        virtual const void* getPtr() const { return &s; }\
        virtual const VkStructureType getType() const { return s.sType; }
    //----------------------------------
    class PipelineVertexInputStateCreateInfo : public PipelineBaseCreateInfo
    {
    public:
        PipelineVertexInputStateCreateInfo() {}
        PipelineVertexInputStateCreateInfo(const VertexInputBindingDescription& ibd, const VertexInputAttributeDescription& iad, VkPipelineVertexInputStateCreateFlags flags = 0)
        {
            _ibd = ibd;
            _iad = iad;
            VkPipelineVertexInputStateCreateInfo viinfo = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, NULL,
                flags,
                (uint32_t)_ibd.size(),
                _ibd.getItem(),
                (uint32_t)_iad.size(),
                _iad.getItem()
            };
            s = viinfo;
        }
        inline VkPipelineVertexInputStateCreateInfo* getItem() { return &s; }
        inline const VkPipelineVertexInputStateCreateInfo* getItemCst() const { return &s; }
        PipelineVertexInputStateCreateInfo& operator=(const PipelineVertexInputStateCreateInfo& src) { assert(!"TODO!"); return *this; }
        NVKPIPELINEBASECREATEINFOIMPLE
    private:
        VkPipelineVertexInputStateCreateInfo     s;
        VertexInputBindingDescription    _ibd;
        VertexInputAttributeDescription  _iad;
    };
    //----------------------------------
    class PipelineInputAssemblyStateCreateInfo : public PipelineBaseCreateInfo
    {
    public:
        PipelineInputAssemblyStateCreateInfo(VkPrimitiveTopology topology, VkBool32 primitiveRestartEnable, VkPipelineInputAssemblyStateCreateFlags flags = 0)
        {
            s.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            s.pNext = NULL;
            s.flags = flags;
            s.topology = topology;
            s.primitiveRestartEnable = primitiveRestartEnable;
        }
        inline VkPipelineInputAssemblyStateCreateInfo* getItem() { return &s; }
        inline const VkPipelineInputAssemblyStateCreateInfo* getItemCst() const { return &s; }
        NVKPIPELINEBASECREATEINFOIMPLE
    private:
        VkPipelineInputAssemblyStateCreateInfo s;
    };
    //----------------------------------
    class PipelineShaderStageCreateInfo : public PipelineBaseCreateInfo
    {
    public:
        PipelineShaderStageCreateInfo(const PipelineShaderStageCreateInfo &src) {
          s = src.s;
          name = src.name;
          s.pName = name.c_str();
        }
        PipelineShaderStageCreateInfo& operator=(const PipelineShaderStageCreateInfo &src) {
          s = src.s;
          name = src.name;
          s.pName = name.c_str();
          return *this;
        }
        PipelineShaderStageCreateInfo(    VkShaderStageFlagBits                       stage,
                                            VkShaderModule                              module,
                                            const char*                                 pName,
                                            VkPipelineShaderStageCreateFlags            flags = 0 )
        {
            s.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            s.pNext = NULL;
            s.flags = flags;
            s.stage = stage;
            s.module = module;
            name = std::string(pName);
            s.pName = name.c_str();
            s.pSpecializationInfo = NULL;
        }
        inline VkPipelineShaderStageCreateInfo* getItem() { return &s; }
        inline const VkPipelineShaderStageCreateInfo* getItemCst() const { return &s; }
        NVKPIPELINEBASECREATEINFOIMPLE
    private:
        VkPipelineShaderStageCreateInfo s;
        std::string name;
    };
    //----------------------------------
    class  Viewport
    {
    public:
        Viewport(float x, float y, float width, float height, float minDepth, float maxDepth)
        {
            s.resize(s.size()+1);
            VkViewport &vp = s.back();
            vp.x = x;
            vp.y = y;
            vp.width = width;
            vp.height = height;
            vp.minDepth = minDepth;
            vp.maxDepth = maxDepth;
        }
        inline VkViewport* getItem(int n) { return &s[n]; }
        inline const VkViewport* getItemCst(int n) const { return &s[n]; }
        inline int size() const { return (int)s.size(); }
        operator VkViewport* () { return &s[0]; }
        operator const VkViewport* () const { return &s[0]; }
    private:
            std::vector<VkViewport> s;
    };
    //----------------------------------
    class  Rect2DArray
    {
    public:
        Rect2DArray(VkRect2D &rec)
        {
            s.resize(s.size()+1);
            VkRect2D &r = s.back();
            r = rec;
        }
        Rect2DArray(int32_t x, int32_t y, uint32_t w, uint32_t h)
        {
            s.resize(s.size()+1);
            Rect2D &r = s.back();
            r = Rect2D(Offset2D(x,y), Extent2D(w,h));
        }
        inline VkRect2D* getItem(int n) { return &s[n]; }
        inline const VkRect2D* getItemCst(int n) const { return &s[n]; }
        inline int size() const { return (int)s.size(); }
        operator VkRect2D* () { return &s[0]; }
        operator const VkRect2D* () const { return &s[0]; }
    private:
            std::vector<Rect2D> s;
    };
    //----------------------------------
    class PipelineViewportStateCreateInfo : public PipelineBaseCreateInfo
    {
    public:
        PipelineViewportStateCreateInfo(const PipelineViewportStateCreateInfo& src) : t(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f), u(0.0f, 0.0f, 0.0f, 0.0f)
        {
          s = src.s;
          t = src.t;
          u = src.u;
          s.viewportCount = t.size();
          s.pViewports = t.getItem(0);
          s.scissorCount = u.size();
          s.pScissors = u.getItem(0);
        }
        PipelineViewportStateCreateInfo& operator=(const PipelineViewportStateCreateInfo& src) {
          s = src.s;
          t = src.t;
          u = src.u;
          s.viewportCount = t.size();
          s.pViewports = t.getItem(0);
          s.scissorCount = u.size();
          s.pScissors = u.getItem(0);
          return *this;
        }
        PipelineViewportStateCreateInfo() : t(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f), u(0.0f, 0.0f, 0.0f, 0.0f)
        {
          s.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
          s.pNext = NULL;
          s.flags = 0;
          s.viewportCount = 0;
          s.pViewports = NULL;
          s.scissorCount = 0;
          s.pScissors = NULL;
        }
        PipelineViewportStateCreateInfo(
            const Viewport    viewports,
            const Rect2DArray scissors,
            VkPipelineViewportStateCreateFlags flags = 0) :
            t(viewports), u(scissors)
        {
            s.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO ;
            s.pNext = NULL;
            s.flags = flags;
            s.viewportCount = t.size();
            s.pViewports    = t.getItem(0);
            s.scissorCount  = u.size();
            s.pScissors     = u.getItem(0);
        }
        inline VkPipelineViewportStateCreateInfo* getItem() { return &s; }
        inline const VkPipelineViewportStateCreateInfo* getItemCst() const { return &s; }
        NVKPIPELINEBASECREATEINFOIMPLE
    private:
        VkPipelineViewportStateCreateInfo s;
        Viewport t;
        Rect2DArray u;
    };
    //----------------------------------
    class PipelineRasterizationStateCreateInfo : public PipelineBaseCreateInfo
    {
    public:
        PipelineRasterizationStateCreateInfo(const PipelineRasterizationStateCreateInfo& src) {
          s = src.s;
        }
        PipelineRasterizationStateCreateInfo& operator=(const PipelineRasterizationStateCreateInfo& src) {
          s = src.s;
          return *this;
        }
        PipelineRasterizationStateCreateInfo(
            VkBool32                                    depthClampEnable = VK_FALSE,
            VkBool32                                    rasterizerDiscardEnable = VK_FALSE,
            VkPolygonMode                               polygonMode = VK_POLYGON_MODE_FILL,
            VkCullModeFlags                             cullMode = VK_CULL_MODE_NONE,
            VkFrontFace                                 frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            VkBool32                                    depthBiasEnable = VK_FALSE,
            float                                       depthBiasConstantFactor = 0.0f,
            float                                       depthBiasClamp = 0.0f,
            float                                       depthBiasSlopeFactor = 0.0f,
            float                                       lineWidth = 1.0f,
            VkPipelineRasterizationStateCreateFlags     flags = 0)
        {
            s.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            s.pNext = NULL;
            s.flags = flags;
            s.depthClampEnable = depthClampEnable;
            s.rasterizerDiscardEnable = rasterizerDiscardEnable;
            s.polygonMode = polygonMode;
            s.cullMode = cullMode;
            s.frontFace = frontFace;
            s.depthBiasEnable = depthBiasEnable;
            s.depthBiasConstantFactor = depthBiasConstantFactor;
            s.depthBiasClamp = depthBiasClamp;
            s.depthBiasSlopeFactor = depthBiasSlopeFactor;
            s.lineWidth = lineWidth;
        }
        inline VkPipelineRasterizationStateCreateInfo* getItem() { return &s; }
        inline const VkPipelineRasterizationStateCreateInfo* getItem() const { return &s; }
        NVKPIPELINEBASECREATEINFOIMPLE
    private:
        VkPipelineRasterizationStateCreateInfo s;
    };
    //----------------------------------
    class PipelineColorBlendAttachmentState
    {
    public:
        PipelineColorBlendAttachmentState(const PipelineColorBlendAttachmentState &a)
        {
            s = a.s;
        }
        PipelineColorBlendAttachmentState& operator=(const PipelineColorBlendAttachmentState& src)
        {
          s = src.s;
          return *this;
        }
        PipelineColorBlendAttachmentState()
        {
        }
        PipelineColorBlendAttachmentState(
            VkBool32                                    blendEnable,
            VkBlendFactor                               srcColorBlendFactor,
            VkBlendFactor                               dstColorBlendFactor,
            VkBlendOp                                   colorBlendOp,
            VkBlendFactor                               srcAlphaBlendFactor,
            VkBlendFactor                               dstAlphaBlendFactor,
            VkBlendOp                                   alphaBlendOp,
            VkColorComponentFlags                       colorWriteMask)
        {
            operator () (
            blendEnable,
            srcColorBlendFactor,
            dstColorBlendFactor,
            colorBlendOp,
            srcAlphaBlendFactor,
            dstAlphaBlendFactor,
            alphaBlendOp,
            colorWriteMask);
        }
        inline PipelineColorBlendAttachmentState& operator () (
            VkBool32                                    blendEnable,
            VkBlendFactor                               srcColorBlendFactor,
            VkBlendFactor                               dstColorBlendFactor,
            VkBlendOp                                   colorBlendOp,
            VkBlendFactor                               srcAlphaBlendFactor,
            VkBlendFactor                               dstAlphaBlendFactor,
            VkBlendOp                                   alphaBlendOp,
            VkColorComponentFlags                       colorWriteMask)
        {
            VkPipelineColorBlendAttachmentState ss;
            ss.blendEnable = blendEnable;
            ss.srcColorBlendFactor = srcColorBlendFactor;
            ss.dstColorBlendFactor = dstColorBlendFactor;
            ss.colorBlendOp = colorBlendOp;
            ss.srcAlphaBlendFactor = srcAlphaBlendFactor;
            ss.dstAlphaBlendFactor = dstAlphaBlendFactor;
            ss.alphaBlendOp = alphaBlendOp;
            ss.colorWriteMask = colorWriteMask;
            s.push_back(ss);
            return *this;
        }
        int size() const {return (int)s.size(); }
        VkPipelineColorBlendAttachmentState* getItem(int n=0) { return &s[n]; }
        const VkPipelineColorBlendAttachmentState* getItemCst(int n=0) const { return &s[n]; }
    private:
        std::vector<VkPipelineColorBlendAttachmentState> s;
    };
    //----------------------------------
    class PipelineColorBlendStateCreateInfo : public PipelineBaseCreateInfo
    {
    public:
      PipelineColorBlendStateCreateInfo(const PipelineColorBlendStateCreateInfo& src) { s = src.s; }
      PipelineColorBlendStateCreateInfo& operator=(const PipelineColorBlendStateCreateInfo& src) { 
        s = src.s;
        t = src.t;
        s.pAttachments = t.getItem(0);
        return *this;
      }
      PipelineColorBlendStateCreateInfo()
      {
        s.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        s.pNext = NULL;
        s.flags = 0;
        s.logicOpEnable = VK_FALSE;
        s.logicOp = VK_LOGIC_OP_CLEAR;
        s.attachmentCount = 0;
        s.pAttachments = NULL;
        s.blendConstants[0] = 0.0f;
        s.blendConstants[1] = 0.0f;
        s.blendConstants[2] = 0.0f;
        s.blendConstants[3] = 0.0f;
      }
      PipelineColorBlendStateCreateInfo(
            VkBool32 logicOpEnable, 
            VkLogicOp logicOp,
            const PipelineColorBlendAttachmentState &attachments,
            float blendConstants[4],
            VkPipelineColorBlendStateCreateFlags        flags = 0
            ) : t(attachments)
        {
            s.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            s.pNext = NULL;
            s.flags = flags;
            s.logicOpEnable = logicOpEnable;
            s.logicOp = logicOp;
            s.attachmentCount = attachments.size();
            s.pAttachments = t.getItem(0);
            ::memcpy(s.blendConstants, blendConstants, sizeof(float)*4);
        }
        inline VkPipelineColorBlendStateCreateInfo* getItem() { return &s; }
        inline const VkPipelineColorBlendStateCreateInfo* getItemCst() const { return &s; }
        NVKPIPELINEBASECREATEINFOIMPLE
    private:
        VkPipelineColorBlendStateCreateInfo s;
        PipelineColorBlendAttachmentState t; // need to keep the copies of attachments
    };
    //----------------------------------
    class DynamicState
    {
    public:
      DynamicState()
      {}
      DynamicState(const DynamicState &a)
        {
            s = a.s;
        }
        DynamicState(VkDynamicState ds)
        {
            s.push_back(ds);
        }
        inline DynamicState& operator=(const DynamicState &src) {
          s = src.s;
          return *this;
        }
        inline DynamicState& operator ()(VkDynamicState ds)
        {
            s.push_back(ds);
            return *this;
        }
        int size() const {return (int)s.size(); }
        inline VkDynamicState* getItem(int n=0) { return &s[n]; }
        inline const VkDynamicState* getItemCst(int n=0) const { return &s[n]; }
    private:
        std::vector<VkDynamicState> s;
    };
    //----------------------------------
    class PipelineDynamicStateCreateInfo : public PipelineBaseCreateInfo
    {
    public:
        PipelineDynamicStateCreateInfo()
        {
          s.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
          s.pNext = NULL;
          s.flags = 0;
          s.pDynamicStates = NULL;
          s.dynamicStateCount = 0;
        }
        PipelineDynamicStateCreateInfo(DynamicState &attachments, VkPipelineDynamicStateCreateFlags flags = 0) :
            t(attachments)
        {
            s.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            s.pNext = NULL;
            s.flags = flags;
            s.pDynamicStates = t.getItem(0);
            s.dynamicStateCount = attachments.size();
        }
        inline PipelineDynamicStateCreateInfo& operator=(const PipelineDynamicStateCreateInfo &src) {
          s = src.s;
          t = src.t;
          s.pDynamicStates = t.getItem(0);
          return *this;
        }
        inline VkPipelineDynamicStateCreateInfo* getItem() { return &s; }
        inline const VkPipelineDynamicStateCreateInfo* getItemCst() const { return &s; }
        NVKPIPELINEBASECREATEINFOIMPLE
    private:
        VkPipelineDynamicStateCreateInfo s;
        DynamicState t; // need to keep the copies of attachments
    };
    //----------------------------------
    class StencilOpState
    {
    public:
        StencilOpState(VkStencilOp failOp = VK_STENCIL_OP_KEEP, 
                        VkStencilOp passOp = VK_STENCIL_OP_KEEP, 
                        VkStencilOp depthFailOp = VK_STENCIL_OP_KEEP, 
                        VkCompareOp compareOp = VK_COMPARE_OP_ALWAYS,
                        uint32_t compareMask = 0,
                        uint32_t writeMask = 0,
                        uint32_t reference = 0)
        {
        s.failOp = failOp;
        s.passOp = passOp;
        s.depthFailOp = depthFailOp;
        s.compareOp = compareOp;
        s.compareMask = compareMask;
        s.writeMask = writeMask;
        s.reference = reference;
        }
        operator VkStencilOpState& () { return s; }
        operator const VkStencilOpState& () const { return s; }
        inline VkStencilOpState* getItem() { return &s; }
        inline const VkStencilOpState* getItemCst() const { return &s; }
    private:
        VkStencilOpState s;
    };
    //----------------------------------
    class PipelineDepthStencilStateCreateInfo : public PipelineBaseCreateInfo
    {
    public:
      PipelineDepthStencilStateCreateInfo(const PipelineDepthStencilStateCreateInfo& src) {
        s = src.s;
      }
      PipelineDepthStencilStateCreateInfo& operator=(const PipelineDepthStencilStateCreateInfo& src) {
        s = src.s;
        return *this;
      }
      PipelineDepthStencilStateCreateInfo()
      {
        s.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        s.pNext = NULL;
        s.flags = 0;
        s.depthTestEnable = VK_FALSE;
        s.depthWriteEnable = VK_FALSE;
        s.depthCompareOp = VK_COMPARE_OP_NEVER;
        s.depthBoundsTestEnable = VK_FALSE;
        s.stencilTestEnable = VK_FALSE;
        s.front.compareMask = 0;
        s.front.failOp = VK_STENCIL_OP_KEEP;
        s.front.passOp = VK_STENCIL_OP_KEEP;
        s.front.depthFailOp = VK_STENCIL_OP_KEEP;
        s.front.compareOp = VK_COMPARE_OP_NEVER;
        s.front.compareMask = 0;
        s.front.writeMask = 0;
        s.front.reference = 0;
        s.back.compareMask = 0;
        s.back.failOp = VK_STENCIL_OP_KEEP;
        s.back.passOp = VK_STENCIL_OP_KEEP;
        s.back.depthFailOp = VK_STENCIL_OP_KEEP;
        s.back.compareOp = VK_COMPARE_OP_NEVER;
        s.back.compareMask = 0;
        s.back.writeMask = 0;
        s.back.reference = 0;
        s.minDepthBounds = 0.0f;
        s.maxDepthBounds = 0.0f;
      }
      PipelineDepthStencilStateCreateInfo(
            VkBool32 depthTestEnable, VkBool32 depthWriteEnable, VkCompareOp depthCompareOp,
            VkBool32 depthBoundsTestEnable, VkBool32 stencilTestEnable, const StencilOpState &front, const StencilOpState &back,
            float minDepthBounds, float maxDepthBounds,
            VkPipelineDepthStencilStateCreateFlags      flags = 0)
        {
            s.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            s.pNext = NULL;
            s.flags = flags;
            s.depthTestEnable = depthTestEnable;
            s.depthWriteEnable = depthWriteEnable;
            s.depthCompareOp = depthCompareOp;
            s.depthBoundsTestEnable = depthBoundsTestEnable;
            s.stencilTestEnable = stencilTestEnable;
            s.front = front;
            s.back = back;
            s.minDepthBounds = minDepthBounds;
            s.maxDepthBounds = maxDepthBounds;
        }
        inline VkPipelineDepthStencilStateCreateInfo* getItem() { return &s; }
        inline const VkPipelineDepthStencilStateCreateInfo* getItemCst() const { return &s; }
        NVKPIPELINEBASECREATEINFOIMPLE
    private:
        VkPipelineDepthStencilStateCreateInfo s;
    };
    //----------------------------------
    class PipelineMultisampleStateCreateInfo : public PipelineBaseCreateInfo
    {
    public:
        PipelineMultisampleStateCreateInfo(const PipelineMultisampleStateCreateInfo& src) {
          s = src.s;
          sampleMask = src.sampleMask;
          s.pSampleMask = &sampleMask;
        }
        PipelineMultisampleStateCreateInfo& operator=(const PipelineMultisampleStateCreateInfo& src) {
          s = src.s;
          sampleMask = src.sampleMask;
          s.pSampleMask = &sampleMask;
          return *this;
        }
        PipelineMultisampleStateCreateInfo(
            VkSampleCountFlagBits rasterizationSamples, VkBool32 sampleShadingEnable, 
            float minSampleShading, 
            VkSampleMask *pSampleMask,
            VkBool32       alphaToCoverageEnable,
            VkBool32       alphaToOneEnable,
            VkPipelineMultisampleStateCreateFlags flags = 0)
        {
            s.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            s.pNext = NULL;
            s.flags = flags;
            s.rasterizationSamples = rasterizationSamples;
            s.sampleShadingEnable = sampleShadingEnable;
            s.minSampleShading = minSampleShading;
            sampleMask = pSampleMask ? *pSampleMask : 0xFFFF; // NOTE: this is unclear how relevant it is here. To Fix
            s.pSampleMask = &sampleMask;
            s.alphaToCoverageEnable = alphaToCoverageEnable;
            s.alphaToOneEnable = alphaToOneEnable;
        }
        PipelineMultisampleStateCreateInfo(
            VkSampleCountFlagBits rasterizationSamples, VkBool32 sampleShadingEnable, 
            float minSampleShading, 
            VkSampleMask _sampleMask,
            VkBool32       alphaToCoverageEnable,
            VkBool32       alphaToOneEnable,
            VkPipelineMultisampleStateCreateFlags flags = 0 )
        {
            s.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            s.pNext = NULL;
            s.flags = flags;
            s.rasterizationSamples = rasterizationSamples;
            s.sampleShadingEnable = sampleShadingEnable;
            s.minSampleShading = minSampleShading;
            sampleMask = _sampleMask;
            s.pSampleMask = &sampleMask;
            s.alphaToCoverageEnable = alphaToCoverageEnable;
            s.alphaToOneEnable = alphaToOneEnable;
        }
        PipelineMultisampleStateCreateInfo()
        {
            s.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            s.pNext = NULL;
            s.flags = 0;
            s.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
            s.sampleShadingEnable = VK_FALSE;
            s.minSampleShading = 0.0f;
            s.pSampleMask = NULL;
            s.alphaToCoverageEnable = VK_FALSE;
            s.alphaToOneEnable = VK_FALSE;
        }
        inline VkPipelineMultisampleStateCreateInfo* getItem() { return &s; }
        inline const VkPipelineMultisampleStateCreateInfo* getItemCst() const { return &s; }
        NVKPIPELINEBASECREATEINFOIMPLE
    private:
        VkPipelineMultisampleStateCreateInfo s;
        VkSampleMask sampleMask;
    };
    //---------------------------------
    class GraphicsPipelineCreateInfo
    {
    public:
        GraphicsPipelineCreateInfo( 
            VkPipelineLayout    layout, 
            VkRenderPass        renderPass,
            uint32_t            subpass = 0,
            VkPipeline          basePipelineHandle = 0,
            int32_t             basePipelineIndex = 0,
            VkPipelineCreateFlags flags = 0)
        {
            s.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            s.pNext = NULL;
            s.stageCount = VK_NULL_HANDLE;
            s.pStages = NULL;
            s.pVertexInputState = NULL;
            s.pInputAssemblyState = NULL;
            s.pTessellationState = NULL;
            s.pViewportState = NULL;
            s.pRasterizationState = NULL;
            s.pMultisampleState = NULL;
            s.pDepthStencilState = NULL;
            s.pColorBlendState = NULL;
            s.pDynamicState = NULL;
            s.flags = flags;
            s.layout = layout;
            s.renderPass = renderPass;
            s.subpass = subpass;
            s.basePipelineHandle = basePipelineHandle;
            s.basePipelineIndex = basePipelineIndex;
        }
        inline GraphicsPipelineCreateInfo& operator ()(const PipelineBaseCreateInfo& state) { return add (state); }
        inline GraphicsPipelineCreateInfo& add(const PipelineBaseCreateInfo& state)
        {
            switch(state.getType())
            {
            case VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO:
                pssci.push_back(((VkPipelineShaderStageCreateInfo*)state.getPtr())[0]);
                s.stageCount = (uint32_t)pssci.size();
                s.pStages = &pssci[0];
                break;
            case VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO:
                s.pVertexInputState = (VkPipelineVertexInputStateCreateInfo*)state.getPtr();
                break;
            case VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO:
                s.pInputAssemblyState = (VkPipelineInputAssemblyStateCreateInfo*)state.getPtr();
                break;
            case VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO:
                s.pTessellationState = (VkPipelineTessellationStateCreateInfo*)state.getPtr();
                break;
            case VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO:
                s.pViewportState = (VkPipelineViewportStateCreateInfo*)state.getPtr();
                break;
            case VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO:
                s.pRasterizationState = (VkPipelineRasterizationStateCreateInfo*)state.getPtr();
                break;
            case VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO:
                s.pMultisampleState = (VkPipelineMultisampleStateCreateInfo*)state.getPtr();
                break;
            case VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO:
                s.pDepthStencilState = (VkPipelineDepthStencilStateCreateInfo*)state.getPtr();
                break;
            case VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO:
                s.pColorBlendState = (VkPipelineColorBlendStateCreateInfo*)state.getPtr();
                break;
            case VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO:
                s.pDynamicState = (VkPipelineDynamicStateCreateInfo*)state.getPtr();
                break;
            default: assert(!"can't happen");
            }
            return *this;
        }
        inline VkGraphicsPipelineCreateInfo* getItem() { return &s; }
        inline const VkGraphicsPipelineCreateInfo* getItemCst() const { return &s; }
        operator VkGraphicsPipelineCreateInfo* () { return &s; }
        operator const VkGraphicsPipelineCreateInfo* () const { return &s; }
        GraphicsPipelineCreateInfo& operator=(const GraphicsPipelineCreateInfo& src) { assert(!"TODO!"); return *this; }
    private:
        struct StructHeader
        {
            VkStructureType                             sType;      // Must be VK_STRUCTURE_TYPE_PIPELINE_DS_STATE_CREATE_INFO
            const void*                                 pNext;      // Pointer to next structure
        };
        VkGraphicsPipelineCreateInfo s;
        std::vector<VkPipelineShaderStageCreateInfo> pssci;
        friend class NVK::GraphicsPipelineCreateInfo& operator<<(NVK::GraphicsPipelineCreateInfo& os, NVK::PipelineBaseCreateInfo& dt);
    };
    //----------------------------------------------------------------------------
    inline VkPipeline createGraphicsPipeline(GraphicsPipelineCreateInfo &gp)
    {
        VkPipeline p;
        CHECK(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, gp, NULL, &p) );
        return p;
    }
    //----------------------------------------------------------------------------
    class ImageMemoryBarrier
    {
    public:
        ImageMemoryBarrier& operator()(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, 
            VkImageLayout oldLayout, VkImageLayout newLayout, 
            uint32_t srcQueueFamilyIndex,
            uint32_t dstQueueFamilyIndex,
            VkImage image, const ImageSubresourceRange& subresourceRange)
        {
            VkImageMemoryBarrier ss;
            ss.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            ss.pNext = NULL;
            ss.srcAccessMask = srcAccessMask;
            ss.dstAccessMask = dstAccessMask;
            ss.oldLayout = oldLayout;
            ss.newLayout = newLayout;
            ss.srcQueueFamilyIndex = srcQueueFamilyIndex;
            ss.dstQueueFamilyIndex = dstQueueFamilyIndex;
            ss.image = image;
            ss.subresourceRange = *subresourceRange;
            s.push_back(ss);
            ps.push_back(&s.back()); // CHECK: pointer may because wrong if vector reallocates... ?
            return *this;
        }
        ImageMemoryBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, 
            VkImageLayout oldLayout, VkImageLayout newLayout, 
            uint32_t srcQueueFamilyIndex,
            uint32_t dstQueueFamilyIndex,
            VkImage image, const ImageSubresourceRange& subresourceRange)

        {
            operator()(srcAccessMask, dstAccessMask, oldLayout, newLayout, srcQueueFamilyIndex, dstQueueFamilyIndex, image, subresourceRange);
        }
        inline VkImageMemoryBarrier* getItem(int n) { return &s[n]; }
        inline const VkImageMemoryBarrier* getItemCst(int n) const { return &s[n]; }
        inline int size() const { return (int)s.size(); }
        operator VkImageMemoryBarrier* () { return &s[0]; }
        operator const VkImageMemoryBarrier* () const { return &s[0]; }
        ImageMemoryBarrier& operator=(const ImageMemoryBarrier& src) { assert(!"TODO!"); return *this; }
    private:
        std::vector<VkImageMemoryBarrier> s;
        std::vector<VkImageMemoryBarrier*> ps;
    };
    //----------------------------------------------------------------------------
    class BufferMemoryBarrier
    {
    public:
        BufferMemoryBarrier& operator()(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, 
            uint32_t                                    srcQueueFamilyIndex,
            uint32_t                                    dstQueueFamilyIndex,
            VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size)
        {
            VkBufferMemoryBarrier ss;
            ss.sType     = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            ss.pNext     = NULL;
            ss.srcAccessMask = srcAccessMask;
            ss.dstAccessMask = dstAccessMask;
            ss.srcQueueFamilyIndex = srcQueueFamilyIndex;
            ss.dstQueueFamilyIndex = dstQueueFamilyIndex;
            ss.buffer    = buffer;
            ss.offset    = offset;
            ss.size      = size;
            s.push_back(ss);
            return *this;
        };
        BufferMemoryBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, 
            uint32_t                                    srcQueueFamilyIndex,
            uint32_t                                    dstQueueFamilyIndex,
            VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size)
        {
            operator()(srcAccessMask, dstAccessMask, srcQueueFamilyIndex, dstQueueFamilyIndex, buffer, offset, size);
        }
        inline VkBufferMemoryBarrier* getItem(int n) { return &s[n]; }
        inline const VkBufferMemoryBarrier* getItemCst(int n) const { return &s[n]; }
        inline int size(int n) const { return (int)s.size(); }
        operator VkBufferMemoryBarrier* () { return &s[0]; }
        operator const VkBufferMemoryBarrier* () const { return &s[0]; }
    private:
        std::vector<VkBufferMemoryBarrier> s;
    };
    //----------------------------------------------------------------------------
    struct ClearColorValue
    {
        ClearColorValue(float *v)
        { ::memcpy(s.float32, v, sizeof(float)*4); }
        ClearColorValue(uint32_t *v)
        { ::memcpy(s.uint32, v, sizeof(uint32_t)*4); }
        ClearColorValue(int32_t *v)
        { ::memcpy(s.int32, v, sizeof(int32_t)*4); }
        ClearColorValue(float r, float g, float b, float a)
        { s.float32[0] = r; s.float32[1] = g; s.float32[2] = b; s.float32[3] = a; }
        ClearColorValue(uint32_t r, uint32_t g, uint32_t b, uint32_t a)
        { s.uint32[0] = r; s.uint32[1] = g; s.uint32[2] = b; s.uint32[3] = a; }
        ClearColorValue(int32_t r, int32_t g, int32_t b, int32_t a)
        { s.int32[0] = r; s.int32[1] = g; s.int32[2] = b; s.int32[3] = a; }
        VkClearColorValue s;
    };
    struct ClearDepthStencilValue : VkClearDepthStencilValue
    {
        ClearDepthStencilValue(float _depth, uint32_t _stencil)
        { depth = _depth; stencil = _stencil; }
    };
    //----------------------------------------------------------------------------
    class ClearValue
    {
    public:
        ClearValue(const ClearColorValue &color)
        {
            VkClearValue cv;
            cv.color = color.s;
            s.push_back(cv);
        }
        ClearValue(const ClearDepthStencilValue &ds)
        {
            VkClearValue cv;
            cv.depthStencil = ds;
            s.push_back(cv);
        }
        inline ClearValue& operator ()(const ClearColorValue &color)
        {
            VkClearValue cv;
            cv.color = color.s;
            s.push_back(cv);
            return *this;
        }
        inline ClearValue& operator ()(const ClearDepthStencilValue &ds)
        {
            VkClearValue cv;
            cv.depthStencil = ds;
            s.push_back(cv);
            return *this;
        }
        inline VkClearValue* getItem(int n) { return &s[n]; }
        inline const VkClearValue* getItemCst(int n) const { return &s[n]; }
        inline int size() const { return (int)s.size(); }
    private:
        std::vector<VkClearValue> s;
    };
    //----------------------------------------------------------------------------
    class RenderPassBeginInfo
    {
    public:
        RenderPassBeginInfo(VkRenderPass &renderPass, VkFramebuffer &framebuffer, 
                               VkRect2D &renderArea, ClearValue &clearValues ) :
            t(clearValues)
        {
            s.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            s.pNext = NULL;
            s.renderPass = renderPass;
            s.framebuffer = framebuffer;
            s.renderArea = renderArea;
            s.clearValueCount = t.size();
            s.pClearValues = t.getItem(0);
        }
        inline VkRenderPassBeginInfo* getItem() { return &s; }
        inline const VkRenderPassBeginInfo* getItemCst() const { return &s; }
        operator VkRenderPassBeginInfo* () { return &s; }
        operator const VkRenderPassBeginInfo* () const { return &s; }
        RenderPassBeginInfo&                operator=(const RenderPassBeginInfo& src)
        {
          assert(!"TODO!");
          return *this;
        }
    private:
        VkRenderPassBeginInfo s;
        ClearValue t;
    };
    //----------------------------------------------------------------------------
    class CommandBufferInheritanceInfo
    {
    public:
        CommandBufferInheritanceInfo(
            VkRenderPass                     renderPass,
            uint32_t                         subpass,
            VkFramebuffer                    framebuffer,
            VkBool32                         occlusionQueryEnable,
            VkQueryControlFlags              queryFlags,
            VkQueryPipelineStatisticFlags    pipelineStatistics )
        {
            s.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
            s.pNext = NULL;
            s.renderPass = renderPass;
            s.subpass = subpass;
            s.framebuffer = framebuffer;
            s.occlusionQueryEnable = occlusionQueryEnable;
            s.queryFlags = queryFlags;
            s.pipelineStatistics = pipelineStatistics;
        }
        inline VkCommandBufferInheritanceInfo* getItem() { return &s; }
        inline const VkCommandBufferInheritanceInfo* getItemCst() const { return &s; }
        operator VkCommandBufferInheritanceInfo* () { return &s; }
        operator const VkCommandBufferInheritanceInfo* () const { return &s; }
    private:
        VkCommandBufferInheritanceInfo s;
    };

    //----------------------------------------------------------------------------
    class FramebufferCreateInfo
    {
    public:
        FramebufferCreateInfo(
            VkRenderPass renderPass, 
            uint32_t attachmentCount, VkImageView *pAttachments, 
            uint32_t width, uint32_t height, uint32_t layers, VkFramebufferCreateFlags flags = 0 )
        {
            imageViews.clear();
            for(uint32_t i=0; i<attachmentCount; i++)        // copy them so the functor can add more later
                imageViews.push_back(pAttachments[i]);
            s.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            s.pNext = NULL;
            s.flags = flags;
            s.renderPass = renderPass;
            s.attachmentCount = attachmentCount;
            s.pAttachments = &imageViews[0];
            s.width = width;
            s.height = height;
            s.layers = layers;
        }
        FramebufferCreateInfo(
            VkRenderPass renderPass, 
            uint32_t width, uint32_t height, uint32_t layers,
            VkImageView &attachment, VkFramebufferCreateFlags flags = 0 )
        {
            imageViews.clear();
            imageViews.push_back(attachment);
            s.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            s.pNext = NULL;
            s.flags = flags;
            s.renderPass = renderPass;
            s.attachmentCount = 1;
            s.pAttachments = &imageViews[0];
            s.width = width;
            s.height = height;
            s.layers = layers;
        }
        inline VkFramebufferCreateInfo* getItem() { return &s; }
        inline const VkFramebufferCreateInfo* getItemCst() const { return &s; }
        operator VkFramebufferCreateInfo* () { return &s; }
        operator const VkFramebufferCreateInfo* () const { return &s; }
        FramebufferCreateInfo& operator()(VkImageView imageView) // a way to easily append more imageViews
        {
            imageViews.push_back(imageView);
            s.attachmentCount = (uint32_t)imageViews.size();
            s.pAttachments = &imageViews[0];
            return *this;
        }
    private:
        VkFramebufferCreateInfo s;
        std::vector<VkImageView> imageViews;
    };
    //----------------------------------------------------------------------------
    class AttachmentDescription
    {
    public:
        AttachmentDescription& operator()(VkFormat format,VkSampleCountFlagBits samples,VkAttachmentLoadOp loadOp,VkAttachmentStoreOp storeOp,
            VkAttachmentLoadOp stencilLoadOp,VkAttachmentStoreOp stencilStoreOp,VkImageLayout initialLayout,VkImageLayout finalLayout)
        {
            VkAttachmentDescription ss;
            ss.format= format;
            ss.samples = samples;
            ss.loadOp = loadOp;
            ss.storeOp = storeOp;
            ss.stencilLoadOp = stencilLoadOp;
            ss.stencilStoreOp = stencilStoreOp;
            ss.initialLayout = initialLayout;
            ss.finalLayout = finalLayout;
            ss.flags = 0;
            s.push_back(ss);
            return *this;
        }
        AttachmentDescription(VkFormat format,VkSampleCountFlagBits samples,VkAttachmentLoadOp loadOp,VkAttachmentStoreOp storeOp,
            VkAttachmentLoadOp stencilLoadOp,VkAttachmentStoreOp stencilStoreOp,VkImageLayout initialLayout,VkImageLayout finalLayout)
        {
            operator()(format, samples, loadOp, storeOp, stencilLoadOp, stencilStoreOp, initialLayout, finalLayout);
        }
        AttachmentDescription(AttachmentDescription &a) { s = a.s; }
        AttachmentDescription() {}
        inline VkAttachmentDescription* getItem(int n) { return &s[n]; }
        inline const VkAttachmentDescription* getItemCst(int n) const { return &s[n]; }
        inline int size() const { return (int)s.size(); }
        operator VkAttachmentDescription* () { return &s[0]; }
        operator const VkAttachmentDescription* () const { return &s[0]; }
    private:
        std::vector<VkAttachmentDescription> s;
    };
    //----------------------------------------------------------------------------
    class AttachmentReference
    {
    public:
        AttachmentReference& operator()(uint32_t attachment, VkImageLayout layout)
        {
            VkAttachmentReference ss;
            ss.attachment = attachment; // TODO: set it to s.size()
            ss.layout = layout;
            s.push_back(ss);
            return *this;
        }
        AttachmentReference() { }
        AttachmentReference(uint32_t attachment, VkImageLayout layout)
        {
            operator()(attachment, layout);
        }
        inline VkAttachmentReference* getItem(int n) { return s.size()>0 ? &s[n] : NULL; }
        inline const VkAttachmentReference* getItemCst(int n) const { return s.size()>0 ? &s[n] : NULL; }
        inline int size() const { return (int)s.size(); }
        operator VkAttachmentReference* () { return s.size()>0 ? &s[0] : NULL; }
        operator const VkAttachmentReference* () const { return s.size()>0 ? &s[0] : NULL; }
    private:
        std::vector<VkAttachmentReference> s;
    };

    //----------------------------------------------------------------------------
    class Uint32Array
    {
    public:
        Uint32Array& operator()(::uint32_t attachment)
        {
            s.push_back(attachment);
            return *this;
        }
        Uint32Array() { }
        Uint32Array(::uint32_t attachment)
        {
            operator()(attachment);
        }
        inline ::uint32_t* getItem(int n) { return s.size()>0 ? &s[n] : NULL; }
        inline const ::uint32_t* getItemCst(int n) const { return s.size()>0 ? &s[n] : NULL; }
        inline int size() const { return (int)s.size(); }
        operator ::uint32_t* () { return s.size()>0 ? &s[0] : NULL; }
        operator const ::uint32_t* () const { return s.size()>0 ? &s[0] : NULL; }
    private:
        std::vector<uint32_t> s;
    };
    //----------------------------------------------------------------------------
    // this class takes pointers: the copy of fields is more complicated than the
    // other cases. passing AttachmentReference as reference would be overkill
    // And NULL allows to avoid un-necessary ones
    //----------------------------------------------------------------------------
    class SubpassDescription
    {
    private:
        struct References {
            VkSubpassDescription s;
            bool bUsing_local_references; // when not from outside and from below:
            NVK::AttachmentReference inputAttachments;
            NVK::AttachmentReference colorAttachments;
            NVK::AttachmentReference resolveAttachments;
            NVK::AttachmentReference depthStencilAttachment;
            NVK::Uint32Array           preserveAttachments;
        };
        // to re-adjust pointers in the vector table of 'References'
        // essentially because of a realloc (resize())
        void assignPointersToVKStructure()
        {
            for(int i=0; i<s.size(); i++)
            {
                References &r = s[i];
                if(!r.bUsing_local_references)
                    continue;
                r.bUsing_local_references = true;
                VkSubpassDescription &ss = r.s;

                if(r.inputAttachments.size() > 0) {
                    ss.inputAttachmentCount = r.inputAttachments.size();
                    ss.pInputAttachments = r.inputAttachments;
                } else {
                    ss.inputAttachmentCount = 0;
                    ss.pInputAttachments = NULL;
                }
                if(r.colorAttachments.size() > 0) {
                    ss.colorAttachmentCount = r.colorAttachments.size();
                    ss.pColorAttachments = r.colorAttachments;
                } else {
                    ss.colorAttachmentCount = 0;
                    ss.pColorAttachments = NULL;
                }
                if(r.resolveAttachments.size() > 0) {
                    ss.pResolveAttachments = r.resolveAttachments;
                } else {
                    ss.pResolveAttachments = NULL;
                }
                if(r.depthStencilAttachment.size() > 0) {
                    assert(r.depthStencilAttachment.size() == 1);
                    ss.pDepthStencilAttachment = r.depthStencilAttachment;
                } else {
                    ss.pDepthStencilAttachment = unused.getItem(0);
                }
                if(r.preserveAttachments.size() > 0) {
                    ss.preserveAttachmentCount = r.preserveAttachments.size();
                    ss.pPreserveAttachments = r.preserveAttachments;
                } else {
                    ss.preserveAttachmentCount = 0;
                    ss.pPreserveAttachments = NULL;
                }
            }
        }
    public:
        SubpassDescription& operator()(VkPipelineBindPoint pipelineBindPoint,
            const NVK::AttachmentReference *inputAttachments=NULL, const NVK::AttachmentReference *colorAttachments=NULL,
            const NVK::AttachmentReference *resolveAttachments=NULL, const NVK::AttachmentReference *depthStencilAttachment=NULL,
            const NVK::Uint32Array *preserveAttachments=NULL, VkSubpassDescriptionFlags flags=0)
        {
            // FIXME: const are annoying... for for now I wildly cast things below... sorry.
            s.resize(s.size()+1);
            References &r = s.back();
            r.bUsing_local_references = false;
            VkSubpassDescription &ss = r.s;
            ss.pipelineBindPoint = pipelineBindPoint;
            ss.flags = flags;
            if(inputAttachments) {
                ss.inputAttachmentCount = ((NVK::AttachmentReference *)inputAttachments)->size();
                ss.pInputAttachments = *((NVK::AttachmentReference *)inputAttachments);
            } else {
                ss.inputAttachmentCount = 0;
                ss.pInputAttachments = NULL;
            }
            if(colorAttachments) {
                ss.colorAttachmentCount = ((NVK::AttachmentReference *)colorAttachments)->size();
                ss.pColorAttachments = *((NVK::AttachmentReference *)colorAttachments);
            } else {
                ss.colorAttachmentCount = 0;
                ss.pColorAttachments = NULL;
            }
            if(resolveAttachments) {
                ss.pResolveAttachments = *((NVK::AttachmentReference *)resolveAttachments);
            } else {
                ss.pResolveAttachments = NULL;
            }
            // Hacky... sorry. Need to do better
            if(depthStencilAttachment 
                && (((NVK::AttachmentReference *)depthStencilAttachment)->getItem(0)->attachment != VK_ATTACHMENT_UNUSED)) {
                assert(((NVK::AttachmentReference *)depthStencilAttachment)->size() == 1);
                ss.pDepthStencilAttachment = ((NVK::AttachmentReference *)depthStencilAttachment)[0];
            } else {
                ss.pDepthStencilAttachment = unused.getItem(0);
            }
            if(preserveAttachments) {
                ss.preserveAttachmentCount = ((NVK::Uint32Array *)preserveAttachments)->size();
                ss.pPreserveAttachments = *((NVK::Uint32Array *)preserveAttachments);
            } else {
                ss.preserveAttachmentCount = 0;
                ss.pPreserveAttachments = NULL;
            }
            return *this;
        }
        SubpassDescription(VkPipelineBindPoint pipelineBindPoint,
            NVK::AttachmentReference *inputAttachments=NULL,   NVK::AttachmentReference *colorAttachments=NULL,
            NVK::AttachmentReference *resolveAttachments=NULL, NVK::AttachmentReference *depthStencilAttachment=NULL,
            NVK::Uint32Array *preserveAttachments=NULL, VkSubpassDescriptionFlags flags=0) :
            unused(VK_ATTACHMENT_UNUSED/*attachment*/, VK_IMAGE_LAYOUT_UNDEFINED/*layout*/)
        {
            operator()(pipelineBindPoint, inputAttachments, colorAttachments, resolveAttachments, depthStencilAttachment, preserveAttachments, flags);
        }
        SubpassDescription& operator()(const VkPipelineBindPoint pipelineBindPoint,
            const NVK::AttachmentReference &inputAttachments_,   const NVK::AttachmentReference &colorAttachments_,
            const NVK::AttachmentReference &resolveAttachments_, const NVK::AttachmentReference &depthStencilAttachment_,
            const NVK::Uint32Array &preserveAttachments_, VkSubpassDescriptionFlags flags=0)
        {
            s.resize(s.size()+1);
            References &r = s.back();
            r.bUsing_local_references = true;
            VkSubpassDescription &ss = r.s;
            r.inputAttachments = inputAttachments_;
            r.colorAttachments = colorAttachments_;
            r.resolveAttachments = resolveAttachments_;
            r.depthStencilAttachment = depthStencilAttachment_;
            r.preserveAttachments = preserveAttachments_;
            ss.pipelineBindPoint = pipelineBindPoint;
            ss.flags = flags;
            assignPointersToVKStructure();
            return *this;
        }
        SubpassDescription(VkPipelineBindPoint pipelineBindPoint,
            const NVK::AttachmentReference &inputAttachments,   const NVK::AttachmentReference &colorAttachments,
            const NVK::AttachmentReference &resolveAttachments, const NVK::AttachmentReference &depthStencilAttachment,
            const NVK::Uint32Array &preserveAttachments, VkSubpassDescriptionFlags flags=0) :
            unused(VK_ATTACHMENT_UNUSED/*attachment*/, VK_IMAGE_LAYOUT_UNDEFINED/*layout*/)
        {
            operator()(pipelineBindPoint, inputAttachments, colorAttachments, resolveAttachments, depthStencilAttachment, preserveAttachments, flags);
        }
        void copyFrom(const SubpassDescription &a)
        {
            s.clear();
            for(int i=0; i<a.s.size(); i++)
            {
                const References &r = a.s[i];
                if(r.bUsing_local_references)
                {
                    operator()(r.s.pipelineBindPoint, r.inputAttachments, r.colorAttachments, r.resolveAttachments, r.depthStencilAttachment, r.preserveAttachments, r.s.flags);
                } else {
                    s.resize(s.size()+1);
                    s[i].bUsing_local_references = r.bUsing_local_references;
                    s[i].s = r.s;
                    // Special case: if the attachment is referencing an unused one, 
                    // we want to forget it and use the local one here (to avoid messing with invalid pointers later)
                    if((r.s.pDepthStencilAttachment == NULL) || (r.s.pDepthStencilAttachment->attachment == VK_ATTACHMENT_UNUSED))
                    {
                        // because... pDepthStencilAttachment CANNOT be NULL
                        s[i].s.pDepthStencilAttachment = unused.getItem(0);
                    }
                }
            }
        }
        SubpassDescription(const SubpassDescription &a) 
        { 
            copyFrom(a);
        }
        SubpassDescription& operator=(const SubpassDescription &a) 
        { 
            copyFrom(a);
            return *this;
        }
        //VkSubpassDescription& operator=(const SubpassDescription &a) 
        //{ 
        //    copyFrom(a);
        //    return *this;
        //}
        SubpassDescription() { }
        inline VkSubpassDescription* getItem(int n) { return &s[n].s; }
        inline const VkSubpassDescription* getItemCst(int n) const { return &s[n].s; }
        inline int size() const { return (int)s.size(); }
        operator VkSubpassDescription* () { return &s[0].s; }
        operator const VkSubpassDescription* () const { return &s[0].s; }
    private:
        std::vector<References> s;
        // Unused attachment for when needed
        NVK::AttachmentReference unused;
    };
    //----------------------------------------------------------------------------
    class SubpassDependency
    {
    public:
        SubpassDependency& operator()(
            uint32_t                                    srcSubpass,
            uint32_t                                    dstSubpass,
            VkPipelineStageFlags                        srcStageMask,
            VkPipelineStageFlags                        dstStageMask,
            VkAccessFlags                               srcAccessMask,
            VkAccessFlags                               dstAccessMask,
            VkDependencyFlags                           dependencyFlags
            )
        {
            VkSubpassDependency ss;
            ss.srcSubpass = srcSubpass;
            ss.dstSubpass = dstSubpass;
            ss.srcStageMask = srcStageMask;
            ss.dstStageMask = dstStageMask;
            ss.srcAccessMask = srcAccessMask;
            ss.dstAccessMask = dstAccessMask;
            ss.dependencyFlags = dependencyFlags;
            s.push_back(ss);
            return *this;
        }
        SubpassDependency() {} // allows to reference None of it
        SubpassDependency(SubpassDependency &a) { s = a.s; } // allows to reference None of it
        SubpassDependency(
            uint32_t                                    srcSubpass,
            uint32_t                                    dstSubpass,
            VkPipelineStageFlags                        srcStageMask,
            VkPipelineStageFlags                        dstStageMask,
            VkAccessFlags                               srcAccessMask,
            VkAccessFlags                               dstAccessMask,
            VkDependencyFlags                           dependencyFlags
        )
        {
            operator()(srcSubpass, dstSubpass, srcStageMask, dstStageMask, srcAccessMask, dstAccessMask, dependencyFlags);
        }
        inline VkSubpassDependency* getItem(int n) { return s.size()>0 ? &s[n] : NULL; }
        inline const VkSubpassDependency* getItemCst(int n) const { return s.size()>0 ? &s[n] : NULL; }
        inline int size() const { return (int)s.size(); }
        operator VkSubpassDependency* () { return s.size()>0 ? &s[0] : NULL; }
        operator const VkSubpassDependency* () const { return s.size()>0 ? &s[0] : NULL; }
    private:
        std::vector<VkSubpassDependency> s;
    };
    //----------------------------------------------------------------------------
    class RenderPassCreateInfo
    {
    public:
        RenderPassCreateInfo() {
            s.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            s.pNext = NULL;
        }
        RenderPassCreateInfo(const RenderPassCreateInfo& src) {
            operator=(src);
        }
        RenderPassCreateInfo& operator=(const RenderPassCreateInfo& src) {
            s.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            s.pNext = NULL;
            attachments = src.attachments;
            s.attachmentCount = attachments.size();
            s.pAttachments = attachments;
            subpasses = src.subpasses;
            s.subpassCount = subpasses.size();
            dependencies = src.dependencies;
            s.pSubpasses = subpasses;
            s.dependencyCount = dependencies.size();
            s.pDependencies = dependencies;
            return *this;
        }
        RenderPassCreateInfo(
            const AttachmentDescription            &_attachments,
            const SubpassDescription               &_subpasses,
            const SubpassDependency                &_dependencies)
        {
            s.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            s.pNext = NULL;
            attachments = _attachments;
            s.attachmentCount = attachments.size();
            s.pAttachments = attachments;
            subpasses = _subpasses;
            s.subpassCount = subpasses.size();
            dependencies = _dependencies;
            s.pSubpasses = subpasses;
            s.dependencyCount = dependencies.size();
            s.pDependencies = dependencies;
            s.flags = 0;
        }
        inline VkRenderPassCreateInfo* getItem() { return &s; }
        inline const VkRenderPassCreateInfo* getItemCst() const { return &s; }
        operator VkRenderPassCreateInfo* () { return &s; }
        operator const VkRenderPassCreateInfo* () const { return &s; }
    private:
        VkRenderPassCreateInfo      s;
        AttachmentDescription    attachments;
        SubpassDescription       subpasses;
        SubpassDependency        dependencies;
    };

    class MemoryChunk
    {
    public:
        MemoryChunk();
        void     free();
        bool isValid() { return bValid; }
        VkBuffer createBufferAlloc(NVK::BufferCreateInfo bufferInfo, VkFlags memProps=VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VkDeviceMemory *bufferMem=NULL);
        VkBuffer createBufferAlloc(size_t size, VkFlags bufferUsage, VkFlags memProps=VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VkDeviceMemory *bufferMem=NULL);
        VkBuffer createBufferAllocFill(CommandPool *cmdPool, BufferCreateInfo &bufferInfo, const void* data, VkFlags memProps=VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VkDeviceMemory *bufferMem=NULL);
        VkBuffer createBufferAllocFill(CommandPool *cmdPool, size_t size, const void* data, VkFlags usage, VkFlags memProps=VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VkDeviceMemory *bufferMem=NULL);
        void    destroyBuffers();
    private:
        bool                    bValid;
        NVK*                    nvk;
        VkDeviceSize          size;
        VkBufferUsageFlags    usage;
        VkFlags               memProps;
        VkDeviceMemory        deviceMem;
        VkBuffer              defaultBuffer;
        VkDeviceSize          alignMask;
        size_t                  sizeUsed;
        std::vector<VkBuffer>   buffers;
        friend class NVK;
    };

    class SubmitInfo {
    public:
        SubmitInfo(
            uint32_t                                    waitSemaphoreCount,
            const VkSemaphore*                          pWaitSemaphores,
            const VkPipelineStageFlags*                 pWaitDstStageMask,
            uint32_t                                    commandBufferCount,
            const VkCommandBuffer*                      pCommandBuffers,
            uint32_t                                    signalSemaphoreCount,
            const VkSemaphore*                          pSignalSemaphores)
        {
            operator()(waitSemaphoreCount, pWaitSemaphores, pWaitDstStageMask, commandBufferCount, pCommandBuffers, signalSemaphoreCount, pSignalSemaphores);
        }
        SubmitInfo& operator()(
            uint32_t                                    waitSemaphoreCount,
            const VkSemaphore*                          pWaitSemaphores,
            const VkPipelineStageFlags*                 pWaitDstStageMask,
            uint32_t                                    commandBufferCount,
            const VkCommandBuffer*                      pCommandBuffers,
            uint32_t                                    signalSemaphoreCount,
            const VkSemaphore*                          pSignalSemaphores)
    {
        VkSubmitInfo ss;
        ss.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        ss.pNext = NULL;
        ss.waitSemaphoreCount = 0;
        ss.pWaitSemaphores = pWaitSemaphores;
        ss.pWaitDstStageMask = pWaitDstStageMask;
        ss.commandBufferCount = commandBufferCount;
        ss.pCommandBuffers = pCommandBuffers;
        ss.signalSemaphoreCount = signalSemaphoreCount;
        ss.pSignalSemaphores = pSignalSemaphores;
        s.push_back(ss);
        return *this;
    }
        operator VkSubmitInfo* () { return &s[0]; }
        operator const VkSubmitInfo* () const { return &s[0]; }
        size_t size() const { return s.size(); }
        VkSubmitInfo* getItem(int n=0) { return &s[n]; }
        const VkSubmitInfo* getItemCst(int n=0) const { return &s[n]; }
    private:
        std::vector<VkSubmitInfo> s;
    };
}; // NVK
#endif //_NVK_H_
