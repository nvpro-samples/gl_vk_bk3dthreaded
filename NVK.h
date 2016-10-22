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

    Note: this section of the code is showing a basic implementation of
    Command-lists using a binary format called bk3d.
    This format has no value for command-list. However you will see that
    it allows to use pre-baked art asset without too parsing: all is
    available from structures in the file (after pointer resolution)

*/ //--------------------------------------------------------------------
#ifdef _DEBUG
#define CHECK(a) assert(a == VK_SUCCESS)
#else
#define CHECK(a) a
#endif

//------------------------------------------------------------------------------
// VULKAN
//------------------------------------------------------------------------------
#include "vulkan/vulkan.h"

//------------------------------------------------------------------------------
// Vulkan functions in a more "friendly" way
//------------------------------------------------------------------------------
class NVK
{
public:
    class MemoryChunk;
    class VkBufferImageCopy;
    class VkBufferCreateInfo;
    class VkFramebufferCreateInfo;
    class VkRenderPassCreateInfo;
    class VkSamplerCreateInfo;
    class VkDescriptorSetAllocateInfo;
    class VkSubmitInfo;
    class VkImageViewCreateInfo;
    class VkCommandBufferInheritanceInfo;

    ::VkDevice        m_device;
    ::VkInstance      m_instance;
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
        ::VkPhysicalDevice                    device;
        ::VkPhysicalDeviceMemoryProperties    memoryProperties;
        ::VkPhysicalDeviceProperties          properties;
        ::VkPhysicalDeviceFeatures            features;
        std::vector<::VkQueueFamilyProperties>  queueProperties;
        void clear() {
            memset(&device, 0, sizeof(::VkPhysicalDevice));
            memset(&memoryProperties, 0, sizeof(::VkPhysicalDeviceMemoryProperties));
            memset(&properties, 0, sizeof(::VkPhysicalDeviceProperties));
            memset(&features, 0, sizeof(::VkPhysicalDeviceFeatures));
            queueProperties.clear();
        }
    };
    GPU             m_gpu;
    ::VkQueue       m_queue;

    bool CreateDevice();
    bool DestroyDevice();
    //::VkDeviceMemory          allocMemAndBindObject(::VkObject obj, ::VkObjectType type, ::VkFlags memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    ::VkDeviceMemory          allocMemAndBindObject(VkBuffer obj, ::VkFlags memProps=VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    ::VkDeviceMemory          allocMemAndBindObject(::VkImage obj, ::VkFlags memProps=VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    MemoryChunk               allocateMemory(size_t size, ::VkFlags usage, ::VkFlags memProps=VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    ::VkResult                fillBuffer(::VkCommandPool cmdPool,  size_t size, ::VkResult result, const void* data, VkBuffer buffer, VkDeviceSize offset = 0);
    void                      fillImage(::VkCommandPool cmdPool, VkBufferImageCopy &bufferImageCopy, const void* data, VkDeviceSize dataSz, ::VkImage image);
    VkBuffer                  createAndFillBuffer(::VkCommandPool cmdPool, size_t size, const void* data, ::VkFlags usage, ::VkDeviceMemory &bufferMem, ::VkFlags memProps=VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    ::VkImage                 createImage1D(int width, ::VkDeviceMemory &colorMemory, ::VkFormat format, VkSampleCountFlagBits depthSamples=VK_SAMPLE_COUNT_1_BIT, VkSampleCountFlagBits colorSamples=VK_SAMPLE_COUNT_1_BIT, int mipLevels = 1);
    ::VkImage                 createImage2D(int width, int height, ::VkDeviceMemory &colorMemory, ::VkFormat format, VkSampleCountFlagBits depthSamples=VK_SAMPLE_COUNT_1_BIT, VkSampleCountFlagBits colorSamples=VK_SAMPLE_COUNT_1_BIT, int mipLevels = 1);
    ::VkImage                 createImage3D(int width, int height, int depth, ::VkDeviceMemory &colorMemory, ::VkFormat format, VkSampleCountFlagBits depthSamples=VK_SAMPLE_COUNT_1_BIT, VkSampleCountFlagBits colorSamples=VK_SAMPLE_COUNT_1_BIT, int mipLevels = 1);
    //
    // VULKAN function Overrides
    //
    //- vkInitFramebuffer()
    VkBuffer                  vkCreateBuffer(VkBufferCreateInfo &bci);
    VkResult                  vkBindBufferMemory(VkBuffer &buffer, VkDeviceMemory mem, VkDeviceSize offset);
    ::VkShaderModule          vkCreateShaderModule( const char *shaderCode, size_t size);
    ::VkBufferView            vkCreateBufferView( VkBuffer buffer, ::VkFormat format, ::VkDeviceSize size );
    ::VkFramebuffer           vkCreateFramebuffer(VkFramebufferCreateInfo &fbinfo);
    ::VkCommandBuffer         vkAllocateCommandBuffer(::VkCommandPool commandPool, bool primary) const;
    void                      vkFreeCommandBuffer(::VkCommandPool commandPool, ::VkCommandBuffer cmd);
    void                      vkResetCommandBuffer(VkCommandBuffer cmdBuffer, VkCommandBufferResetFlags flags);
    void                      vkEndCommandBuffer(::VkCommandBuffer cmd);
    ::VkCommandBuffer         vkBeginCommandBuffer(::VkCommandBuffer cmd, bool singleshot, NVK::VkCommandBufferInheritanceInfo &cmdIInfo) const;
    ::VkCommandBuffer         vkBeginCommandBuffer(::VkCommandBuffer cmd, bool singleshot, const ::VkCommandBufferInheritanceInfo* pInheritanceInfo = NULL) const;
    void                    vkCmdCopyBuffer(::VkCommandBuffer cmd, VkBuffer srcBuffer, VkBuffer dstBuffer, ::VkDeviceSize size, ::VkDeviceSize destOffset=0, ::VkDeviceSize srcOffset=0);
    void                    vkResetCommandPool(VkCommandPool cmdPool, VkCommandPoolResetFlags flags);
    void                    vkDestroyCommandPool(VkCommandPool cmdPool);
    void                    vkFreeMemory(VkDeviceMemory mem);
    void*                   vkMapMemory(VkDeviceMemory mem, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags);
    void                    vkUnmapMemory(VkDeviceMemory mem);
    void                    vkMemcpy(::VkDeviceMemory dstMem, const void * srcData, ::VkDeviceSize size);

    void                    vkDestroyBufferView(VkBufferView bufferView);
    void                    vkDestroyBuffer(VkBuffer buffer);

    ::VkRenderPass          vkCreateRenderPass(VkRenderPassCreateInfo &rpinfo);

    VkSemaphore             vkCreateSemaphore(VkSemaphoreCreateFlags flags=0);
    void                    vkDestroySemaphore(VkSemaphore s);
    VkFence                 vkCreateFence(VkFenceCreateFlags flags=0);
    void                    vkDestroyFence(VkFence fence);
    bool                    vkWaitForFences(uint32_t fenceCount, const VkFence* pFences, VkBool32 waitAll, uint64_t timeout);
    void                    vkResetFences(uint32_t fenceCount, const VkFence* pFences);

    void                    vkAllocateDescriptorSets(const NVK::VkDescriptorSetAllocateInfo& allocateInfo, VkDescriptorSet* pDescriptorSets);
    VkSampler               vkCreateSampler(const NVK::VkSamplerCreateInfo &createInfo);
    void                    vkDestroySampler(const VkSampler s);

    VkImageView             vkCreateImageView(const NVK::VkImageViewCreateInfo &createInfo);
    void                    vkDestroyImageView(const VkImageView s);
    void                    vkDestroyImage(const VkImage s);

    void                    vkQueueSubmit(const NVK::VkSubmitInfo& submits, VkFence fence);

    struct VkOffset2D : ::VkOffset2D {
        VkOffset2D( int32_t _x, int32_t _y) { x = _x; y = _y;}
    };
    struct VkOffset3D : ::VkOffset3D {
        VkOffset3D( int32_t _x, int32_t _y, int32_t _z) { x = _x; y = _y; z = _z; }
    };
    struct VkExtent2D : ::VkExtent2D{
        VkExtent2D(int32_t _width, int32_t _height) { width = _width; height = _height; }
    };
    struct VkRect2D : ::VkRect2D {
        VkRect2D() {}
        VkRect2D(const ::VkRect2D& r) { offset = r.offset; extent = r.extent; }
        VkRect2D(const VkOffset2D &_offset, const VkExtent2D &_extent) { offset = _offset; extent = _extent; }
        VkRect2D(float originX, float originY, float width, float height) {
            offset.x = originX; offset.y = originY;
            extent.width = width; extent.height = height;
        }
        operator ::VkRect2D* () { return this; }
        VkRect2D& operator=(const ::VkRect2D& r) { offset = r.offset; extent = r.extent; return *this; }
    };
    struct VkExtent3D : ::VkExtent3D {
        VkExtent3D() {}
        VkExtent3D(int32_t _width, int32_t _height, int32_t _depth) { width = _width; height = _height; depth = _depth; }
    };
    struct Float4 {
        Float4() { memset(v, 0, sizeof(float)*4); }
        Float4(float x, float y, float z, float w) { v[0]=x; v[1]=y; v[2]=z; v[3]=w; }
        operator float*() { return v; }
        float v[4];
    };
    //---------------------------------
    struct VkImageSubresource : ::VkImageSubresource {
        VkImageSubresource(
            VkImageAspectFlags _aspectMask,
            uint32_t      _mipLevel,
            uint32_t      _arrayLayer
            ) { aspectMask = _aspectMask; mipLevel = _mipLevel; arrayLayer = _arrayLayer; }
        VkImageSubresource() {}
    };
    //---------------------------------
    struct VkImageSubresourceLayers : ::VkImageSubresourceLayers {
        VkImageSubresourceLayers(
            VkImageAspectFlags _aspectMask,
            uint32_t      _mipLevel,
            uint32_t      _baseArrayLayer,
            uint32_t      _layerCount
            ) { aspectMask = _aspectMask; mipLevel = _mipLevel; baseArrayLayer = _baseArrayLayer; layerCount = _layerCount; }
        VkImageSubresourceLayers() {}
    };
    //---------------------------------
    class VkBufferImageCopy {
    public:
        VkBufferImageCopy() {}
        VkBufferImageCopy(const VkBufferImageCopy &src) { s = src.s; }
        VkBufferImageCopy(            
            VkDeviceSize                                bufferOffset,
            uint32_t                                    bufferRowLength,
            uint32_t                                    bufferImageHeight,
            const VkImageSubresourceLayers              &imageSubresource,
            const VkOffset3D                            &imageOffset,
            const VkExtent3D                            &imageExtent)
        {
            operator()(bufferOffset, bufferRowLength, bufferImageHeight, imageSubresource, imageOffset, imageExtent);
        }
        inline VkBufferImageCopy& operator () (
            VkDeviceSize                                bufferOffset,
            uint32_t                                    bufferRowLength,
            uint32_t                                    bufferImageHeight,
            const VkImageSubresourceLayers              &imageSubresource,
            const VkOffset3D                            &imageOffset,
            const VkExtent3D                            &imageExtent)
        {
            ::VkBufferImageCopy b = {bufferOffset, bufferRowLength, bufferImageHeight, imageSubresource, imageOffset, imageExtent};
            s.push_back(b);
            return *this;
        }
        void add(
            VkDeviceSize                                bufferOffset,
            uint32_t                                    bufferRowLength,
            uint32_t                                    bufferImageHeight,
            const VkImageSubresourceLayers              &imageSubresource,
            const VkOffset3D                            &imageOffset,
            const VkExtent3D                            &imageExtent)
        {
            operator()(bufferOffset, bufferRowLength, bufferImageHeight, imageSubresource, imageOffset, imageExtent);
        }
        inline VkBufferImageCopy &operator=(const VkBufferImageCopy &src) { s = src.s; }
        inline int size() { return s.size(); }
        inline ::VkBufferImageCopy* getItem(int i=0) { return &(s[i]); }
    private:
        std::vector<::VkBufferImageCopy> s;
    };
    //---------------------------------
    class VkImageCreateInfo {
    public:
        VkImageCreateInfo(
            VkFormat                                    format,
            VkExtent3D                                  extent,
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
        ::VkImageCreateInfo s;
    };
    //---------------------------------
    class VkSamplerCreateInfo {
    public:
        VkSamplerCreateInfo() {}
        VkSamplerCreateInfo(
			VkFilter                                    magFilter,
			VkFilter                                    minFilter,
			VkSamplerMipmapMode                         mipmapMode,
			VkSamplerAddressMode                        addressModeU,
			VkSamplerAddressMode                        addressModeV,
			VkSamplerAddressMode                        addressModeW,
			float                                       mipLodBias,
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
            s.maxAnisotropy = maxAnisotropy;
            s.compareEnable = compareEnable;
            s.compareOp = compareOp;
            s.minLod = minLod;
            s.maxLod = maxLod;
            s.borderColor = borderColor;
            s.unnormalizedCoordinates = unnormalizedCoordinates;
        }
        inline ::VkSamplerCreateInfo* getItem() { return &s; }
    private:
        ::VkSamplerCreateInfo   s;
    };
    //---------------------------------
    class VkComponentMapping {
    public:
        VkComponentMapping() 
        {
            s.r = VK_COMPONENT_SWIZZLE_R; s.g = VK_COMPONENT_SWIZZLE_G; s.b = VK_COMPONENT_SWIZZLE_B; s.a =VK_COMPONENT_SWIZZLE_A;
        }
        VkComponentMapping(VkComponentSwizzle r, VkComponentSwizzle g, VkComponentSwizzle b, VkComponentSwizzle a)
        {
            s.r = r; s.g = g; s.b = b; s.a =a;
        }
        inline ::VkComponentMapping* getItem() { return &s; }
        ::VkComponentMapping s;
    };
    //---------------------------------
    class VkBufferCreateInfo : public ::VkBufferCreateInfo {
    public:
        VkBufferCreateInfo(
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
        inline ::VkBufferCreateInfo* getItem() { return this; }
        operator ::VkBufferCreateInfo* () { return this; }
    };
    //---------------------------------
    class VkImageSubresourceRange {
    public:
        VkImageSubresourceRange(
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
        inline ::VkImageSubresourceRange* getItem() { return &s; }
        operator ::VkImageSubresourceRange* () { return &s; }
        ::VkImageSubresourceRange s;
    };
    //---------------------------------
    class VkImageViewCreateInfo {
    public:
        VkImageViewCreateInfo() {}
        VkImageViewCreateInfo(
            VkImage                                     image,
            VkImageViewType                             viewType,
            VkFormat                                    format,
            const VkComponentMapping                    &components,
            const VkImageSubresourceRange               &subresourceRange
        ) {
            s.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            s.pNext = NULL;
            s.image = image;
            s.viewType = viewType;
            s.format = format;
			s.components = components.s;
            s.subresourceRange = subresourceRange.s;
        }
        inline ::VkImageViewCreateInfo* getItem() { return &s; }
    private:
        ::VkImageViewCreateInfo s;
    };
    //---------------------------------
    class VkDescriptorSetAllocateInfo
    {
    public:
        VkDescriptorSetAllocateInfo(
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
        inline ::VkDescriptorSetAllocateInfo* getItem() { return &s; }
        inline operator ::VkDescriptorSetAllocateInfo* () { return &s; }
    private:
        ::VkDescriptorSetAllocateInfo s;
    };
    //---------------------------------
    class VkDescriptorSetLayoutBinding
    {
    public:
        VkDescriptorSetLayoutBinding() {}
        VkDescriptorSetLayoutBinding(
				uint32_t                                    binding,
				VkDescriptorType                            descriptorType,
				uint32_t                                    descriptorCount,
				VkShaderStageFlags                          stageFlags)
        {
            operator()(binding,descriptorType,descriptorCount,stageFlags);
        }
        inline VkDescriptorSetLayoutBinding& operator () (
				uint32_t                                    binding,
				VkDescriptorType                            descriptorType,
				uint32_t                                    descriptorCount,
				VkShaderStageFlags                          stageFlags)
        {
            ::VkDescriptorSetLayoutBinding b = {
				binding,
				descriptorType,
				descriptorCount,
				stageFlags,
				NULL /*pImmutableSamplers*/ };
            bindings.push_back(b);
            return *this;
        }
        inline int size() { return bindings.size(); }
        inline ::VkDescriptorSetLayoutBinding* getBindings() { return &(bindings[0]); }
    private:
        std::vector<::VkDescriptorSetLayoutBinding> bindings;
    };
    //---------------------------------
    class VkDescriptorSetLayoutCreateInfo
    {
    public:
        VkDescriptorSetLayoutCreateInfo(const VkDescriptorSetLayoutBinding &dslb_, VkDescriptorSetLayoutCreateFlags flags = 0)
        {
            dslb = dslb_;
            descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorSetLayoutCreateInfo.pNext = NULL;
            descriptorSetLayoutCreateInfo.flags = flags;
            descriptorSetLayoutCreateInfo.bindingCount =    dslb.size();// NUM_UBOS;//sizeof(bindings)/sizeof(bindings[0]);
            descriptorSetLayoutCreateInfo.pBindings =       dslb.getBindings();
        }
        ::VkDescriptorSetLayoutCreateInfo* getItem() { return &descriptorSetLayoutCreateInfo; }
    private:
        ::VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
        VkDescriptorSetLayoutBinding dslb;
    };
    //----------------------------------
    ::VkDescriptorSetLayout   vkCreateDescriptorSetLayout(const VkDescriptorSetLayoutCreateInfo &descriptorSetLayoutCreateInfo);
    ::VkPipelineLayout        vkCreatePipelineLayout(VkDescriptorSetLayout* dsls, uint32_t count);
    //---------------------------------
    class VkDescriptorPoolSize
    {
    public:
        VkDescriptorPoolSize() {}
        VkDescriptorPoolSize(::VkDescriptorType t, uint32_t cnt=1)
        {
            ::VkDescriptorPoolSize b = { t, cnt };
            typecount.push_back(b);
        }
        inline VkDescriptorPoolSize& operator () (::VkDescriptorType t, uint32_t cnt=1)
        {
            ::VkDescriptorPoolSize b = { t, cnt };
            typecount.push_back(b);
            return *this;
        }
        inline int size() { return typecount.size(); }
        inline ::VkDescriptorPoolSize* getItem(int n=0) { return &(typecount[n]); }
    private:
        std::vector<::VkDescriptorPoolSize> typecount;
    };
    //---------------------------------
    class VkDescriptorPoolCreateInfo
    {
    public:
        VkDescriptorPoolCreateInfo(
            uint32_t                                    maxSets,
            const VkDescriptorPoolSize                 &typeCount,
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
        inline ::VkDescriptorPoolCreateInfo* getItem() { return &s; }
    private:
        ::VkDescriptorPoolCreateInfo    s;
        VkDescriptorPoolSize            t;
    };
    //----------------------------------
    ::VkDescriptorPool    vkCreateDescriptorPool(const VkDescriptorPoolCreateInfo &descriptorPoolCreateInfo);
    //---------------------------------
    class VkDescriptorImageInfo
    {
    public:
        VkDescriptorImageInfo() {}
        VkDescriptorImageInfo(VkSampler                                   sampler,
							  VkImageView                                 imageView,
							  VkImageLayout                               imageLayout)
        {
            operator()(sampler, imageView, imageLayout);
        }
        inline VkDescriptorImageInfo& operator () (
							  VkSampler                                   sampler,
							  VkImageView                                 imageView,
							  VkImageLayout                               imageLayout)
        {
            ::VkDescriptorImageInfo b;
            b.sampler           = sampler;
            b.imageView         = imageView;
            b.imageLayout       = imageLayout;
            attachInfos.push_back(b);
            return *this;
        }
        inline int size() { return attachInfos.size(); }
        inline ::VkDescriptorImageInfo* getItem(int n=0) { return &(attachInfos[n]); }
    private:
        std::vector<::VkDescriptorImageInfo> attachInfos;
    };
    class VkDescriptorBufferInfo
    {
    public:
        VkDescriptorBufferInfo() {}
        //VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
        VkDescriptorBufferInfo(	VkBuffer                                    buffer,
								VkDeviceSize                                offset,
								VkDeviceSize                                range)
        {
            operator()(buffer, offset, range);
        }
        inline VkDescriptorBufferInfo& operator () (
								VkBuffer                                    buffer,
								VkDeviceSize                                offset,
								VkDeviceSize                                range)
        {
            ::VkDescriptorBufferInfo b;
            b.buffer = buffer;
            b.offset = offset;
            b.range  = range;
            attachInfos.push_back(b);
            return *this;
        }
        inline int size() { return attachInfos.size(); }
        inline ::VkDescriptorBufferInfo* getItem(int n=0) { return &(attachInfos[n]); }
    private:
        std::vector<::VkDescriptorBufferInfo> attachInfos;
    };
    //----------------------------------
    class VkWriteDescriptorSet
    {
    public:
        VkWriteDescriptorSet() {}
        VkWriteDescriptorSet(::VkDescriptorSet descSetDest, uint32_t binding, uint32_t arrayIndex, VkDescriptorBufferInfo& bufferInfos, ::VkDescriptorType descriptorType)
        {
			operator()(descSetDest, binding, arrayIndex, bufferInfos, descriptorType);
        }
        inline VkWriteDescriptorSet& operator () (::VkDescriptorSet descSetDest, uint32_t binding, uint32_t arrayIndex, VkDescriptorBufferInfo& bufferInfos, ::VkDescriptorType descriptorType)
        {
			::VkWriteDescriptorSet ub = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			ub.pNext = NULL;
            ub.dstSet = descSetDest;
            ub.dstBinding = binding;
            ub.dstArrayElement = arrayIndex;
			ub.descriptorCount = bufferInfos.size();
			ub.descriptorType =	descriptorType;
			ub.pImageInfo =	NULL;
			ub.pBufferInfo = bufferInfos.getItem(0);
			ub.pTexelBufferView = NULL;
            updateBuffers.push_back(ub);
            return *this;
        }
        VkWriteDescriptorSet(::VkDescriptorSet descSetDest, uint32_t binding, uint32_t arrayIndex, VkDescriptorImageInfo& imageInfos, ::VkDescriptorType descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
        {
			operator()(descSetDest, binding, arrayIndex, imageInfos, descriptorType);
        }
        inline VkWriteDescriptorSet& operator () (::VkDescriptorSet descSetDest, uint32_t binding, uint32_t arrayIndex, NVK::VkDescriptorImageInfo& imageInfos, ::VkDescriptorType descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
        {
			::VkWriteDescriptorSet ub = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			ub.pNext = NULL;
            ub.dstSet = descSetDest;
            ub.dstBinding = binding;
            ub.dstArrayElement = arrayIndex;
			ub.descriptorCount = imageInfos.size();
			ub.descriptorType =	descriptorType;//VK_DESCRIPTOR_TYPE_STORAGE_IMAGE or VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER or VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
			ub.pImageInfo =	imageInfos.getItem(0); 
			ub.pBufferInfo = NULL;
			ub.pTexelBufferView = NULL;
            updateBuffers.push_back(ub);
            return *this;
        }
        VkWriteDescriptorSet(::VkDescriptorSet descSetDest, uint32_t binding, uint32_t arrayIndex, uint32_t count, const ::VkBufferView* pTexelBufferViews, ::VkDescriptorType descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER)
        {
			operator()(descSetDest, binding, arrayIndex, count, pTexelBufferViews, descriptorType);
        }
        inline VkWriteDescriptorSet& operator () (::VkDescriptorSet descSetDest, uint32_t binding, uint32_t arrayIndex, uint32_t count, const ::VkBufferView* pTexelBufferViews, ::VkDescriptorType descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER)
        {
			::VkWriteDescriptorSet ub = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			ub.pNext = NULL;
            ub.dstSet = descSetDest;
            ub.dstBinding = binding;
            ub.dstArrayElement = arrayIndex;
			ub.descriptorCount = count;
			ub.descriptorType =	descriptorType;//VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER or VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER
			ub.pImageInfo =	NULL;
			ub.pBufferInfo = NULL;
			ub.pTexelBufferView = pTexelBufferViews;
            updateBuffers.push_back(ub);
            return *this;
        }
        inline int size() { return updateBuffers.size(); }
        inline ::VkWriteDescriptorSet* getItem(int n) { return &(updateBuffers[n]); }
    private:
        std::vector<::VkWriteDescriptorSet> updateBuffers;
    };
    //----------------------------------
    void vkUpdateDescriptorSets(VkWriteDescriptorSet &wds);
    //----------------------------------
    class VkVertexInputBindingDescription
    {
    public:
        VkVertexInputBindingDescription() {}
        VkVertexInputBindingDescription(uint32_t binding, uint32_t stride, ::VkVertexInputRate inputRate)
        {
            ::VkVertexInputBindingDescription ub = {binding, stride, inputRate };
            bindings.push_back(ub);
        }
        inline VkVertexInputBindingDescription& operator () (uint32_t binding, uint32_t stride, ::VkVertexInputRate inputRate)
        {
            ::VkVertexInputBindingDescription ub = {binding, stride, inputRate };
            bindings.push_back(ub);
            return *this;
        }
        inline int size() { return bindings.size(); }
        inline ::VkVertexInputBindingDescription* getItem(int n=0) { return &(bindings[n]); }
    private:
        std::vector<::VkVertexInputBindingDescription> bindings;
    };
    //----------------------------------
    class VkVertexInputAttributeDescription
    {
    public:
        VkVertexInputAttributeDescription() {}
        VkVertexInputAttributeDescription(uint32_t location, uint32_t binding, ::VkFormat format, uint32_t offsetInBytes)
        {
            ::VkVertexInputAttributeDescription iad = { location, binding, format, offsetInBytes };
            ia.push_back(iad);
        }
        inline VkVertexInputAttributeDescription& operator () (uint32_t location, uint32_t binding, ::VkFormat format, uint32_t offsetInBytes)
        {
            ::VkVertexInputAttributeDescription iad = { location, binding, format, offsetInBytes };
            ia.push_back(iad);
            return *this;
        }
        inline int size() { return ia.size(); }
        inline ::VkVertexInputAttributeDescription* getItem(int n=0) { return &(ia[n]); }
    private:
        std::vector<::VkVertexInputAttributeDescription> ia;
    };
    //----------------------------------
    class VkPipelineBaseCreateInfo
    {
    public:
        virtual void setNext(VkPipelineBaseCreateInfo& p) = 0;
        virtual const void* getPtr() = 0;
        virtual const ::VkStructureType getType() = 0;
    };
#define NVKPIPELINEBASECREATEINFOIMPLE \
        virtual void setNext(VkPipelineBaseCreateInfo& p) { s.pNext = p.getPtr(); }\
        virtual const void* getPtr() { return &s; }\
        virtual const ::VkStructureType getType() { return s.sType; }
    //----------------------------------
    class VkPipelineVertexInputStateCreateInfo : public VkPipelineBaseCreateInfo
    {
    public:
        VkPipelineVertexInputStateCreateInfo() {}
        VkPipelineVertexInputStateCreateInfo(const VkVertexInputBindingDescription& ibd, const VkVertexInputAttributeDescription& iad, VkPipelineVertexInputStateCreateFlags flags = 0)
        {
            _ibd = ibd;
            _iad = iad;
            ::VkPipelineVertexInputStateCreateInfo viinfo = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, NULL,
				flags,
                _ibd.size(),
                _ibd.getItem(),
                _iad.size(),
                _iad.getItem()
            };
            s = viinfo;
        }
        inline ::VkPipelineVertexInputStateCreateInfo* getItem() { return &s; }
        VkPipelineVertexInputStateCreateInfo& operator=(const VkPipelineVertexInputStateCreateInfo& src) { assert(!"TODO!"); return *this; }
        NVKPIPELINEBASECREATEINFOIMPLE
    private:
        ::VkPipelineVertexInputStateCreateInfo     s;
        VkVertexInputBindingDescription    _ibd;
        VkVertexInputAttributeDescription  _iad;
    };
    //----------------------------------
    class VkPipelineInputAssemblyStateCreateInfo : public VkPipelineBaseCreateInfo
    {
    public:
        VkPipelineInputAssemblyStateCreateInfo(::VkPrimitiveTopology topology, ::VkBool32 primitiveRestartEnable, VkPipelineInputAssemblyStateCreateFlags flags = 0)
        {
            s.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            s.pNext = NULL;
			s.flags = flags;
            s.topology = topology;
            s.primitiveRestartEnable = primitiveRestartEnable;
        }
        ::VkPipelineInputAssemblyStateCreateInfo* getItem() { return &s; }
        NVKPIPELINEBASECREATEINFOIMPLE
    private:
        ::VkPipelineInputAssemblyStateCreateInfo s;
    };
    //----------------------------------
    class VkPipelineShaderStageCreateInfo : public VkPipelineBaseCreateInfo
    {
    public:
        VkPipelineShaderStageCreateInfo(    VkShaderStageFlagBits                       stage,
											VkShaderModule                              module,
											const char*                                 pName,
                                            VkPipelineShaderStageCreateFlags            flags = 0 )
        {
            s.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            s.pNext = NULL;
			s.flags = flags;
			s.stage = stage;
			s.module = module;
			s.pName = pName;
            s.pSpecializationInfo = NULL;
        }
        ::VkPipelineShaderStageCreateInfo* getItem() { return &s; }
        NVKPIPELINEBASECREATEINFOIMPLE
    private:
        ::VkPipelineShaderStageCreateInfo s;
    };
    //----------------------------------
    class  VkViewport
    {
    public:
        VkViewport(float x, float y, float width, float height, float minDepth, float maxDepth)
        {
            s.resize(s.size()+1);
            ::VkViewport &vp = s.back();
            vp.x = x;
            vp.y = y;
            vp.width = width;
            vp.height = height;
            vp.minDepth = minDepth;
            vp.maxDepth = maxDepth;
        }
        inline ::VkViewport* getItem(int n) { return &s[n]; }
        inline int size() { return s.size(); }
        operator ::VkViewport* () { return &s[0]; }
    private:
            std::vector<::VkViewport> s;
    };
    //----------------------------------
    class  VkRect2DArray
    {
    public:
        VkRect2DArray(VkRect2D &rec)
        {
            s.resize(s.size()+1);
            ::VkRect2D &r = s.back();
            r = rec;
        }
        VkRect2DArray(float x, float y, float w, float h)
        {
            s.resize(s.size()+1);
            VkRect2D &r = s.back();
            r = VkRect2D(VkOffset2D(x,y), VkExtent2D(w,h));
        }
        inline ::VkRect2D* getItem(int n) { return &s[n]; }
        inline int size() { return s.size(); }
        operator ::VkRect2D* () { return &s[0]; }
    private:
            std::vector<VkRect2D> s;
    };
    //----------------------------------
    class VkPipelineViewportStateCreateInfo : public VkPipelineBaseCreateInfo
    {
    public:
        VkPipelineViewportStateCreateInfo(
            const VkViewport    viewports,
            const VkRect2DArray scissors,
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
        ::VkPipelineViewportStateCreateInfo* getItem() { return &s; }
        NVKPIPELINEBASECREATEINFOIMPLE
    private:
        ::VkPipelineViewportStateCreateInfo s;
        VkViewport t;
        VkRect2DArray u;
    };
    //----------------------------------
    class VkPipelineRasterizationStateCreateInfo : public VkPipelineBaseCreateInfo
    {
    public:
        VkPipelineRasterizationStateCreateInfo(
			VkBool32                                    depthClampEnable,
			VkBool32                                    rasterizerDiscardEnable,
			VkPolygonMode                               polygonMode,
			VkCullModeFlags                             cullMode,
			VkFrontFace                                 frontFace,
			VkBool32                                    depthBiasEnable,
			float                                       depthBiasConstantFactor,
			float                                       depthBiasClamp,
			float                                       depthBiasSlopeFactor,
			float                                       lineWidth,
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
        ::VkPipelineRasterizationStateCreateInfo* getItem() { return &s; }
        NVKPIPELINEBASECREATEINFOIMPLE
    private:
        ::VkPipelineRasterizationStateCreateInfo s;
    };
    //----------------------------------
    class VkPipelineColorBlendAttachmentState
    {
    public:
        VkPipelineColorBlendAttachmentState(const VkPipelineColorBlendAttachmentState &a)
        {
            s = a.s;
        }
        VkPipelineColorBlendAttachmentState(
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
        inline VkPipelineColorBlendAttachmentState& operator () (
			VkBool32                                    blendEnable,
			VkBlendFactor                               srcColorBlendFactor,
			VkBlendFactor                               dstColorBlendFactor,
			VkBlendOp                                   colorBlendOp,
			VkBlendFactor                               srcAlphaBlendFactor,
			VkBlendFactor                               dstAlphaBlendFactor,
			VkBlendOp                                   alphaBlendOp,
			VkColorComponentFlags                       colorWriteMask)
        {
            ::VkPipelineColorBlendAttachmentState ss;
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
        int size() {return s.size(); }
        ::VkPipelineColorBlendAttachmentState* getItem(int n=0) { return &s[n]; }
    private:
        std::vector<::VkPipelineColorBlendAttachmentState> s;
    };
    //----------------------------------
    class VkPipelineColorBlendStateCreateInfo : public VkPipelineBaseCreateInfo
    {
    public:
        VkPipelineColorBlendStateCreateInfo(
            ::VkBool32 logicOpEnable, 
            ::VkLogicOp logicOp,
            const VkPipelineColorBlendAttachmentState &attachments,
            float blendConstants[4],
			::VkPipelineColorBlendStateCreateFlags        flags = 0
            ) : t(attachments)
        {
            s.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            s.pNext = NULL;
			s.flags = flags;
            s.logicOpEnable = logicOpEnable;
            s.logicOp = logicOp;
            s.attachmentCount = attachments.size();
            s.pAttachments = t.getItem(0);
            memcpy(s.blendConstants, blendConstants, sizeof(float)*4);
        }
        ::VkPipelineColorBlendStateCreateInfo* getItem() { return &s; }
        VkPipelineColorBlendStateCreateInfo& operator=(const VkPipelineColorBlendStateCreateInfo& src) { assert(!"TODO!"); return *this; }
        NVKPIPELINEBASECREATEINFOIMPLE
    private:
        ::VkPipelineColorBlendStateCreateInfo s;
        VkPipelineColorBlendAttachmentState t; // need to keep the copies of attachments
    };
    //----------------------------------
    class VkDynamicState
    {
    public:
        VkDynamicState(const VkDynamicState &a)
        {
            s = a.s;
        }
        VkDynamicState(::VkDynamicState ds)
        {
            s.push_back(ds);
        }
        inline VkDynamicState& operator ()(::VkDynamicState ds)
        {
            s.push_back(ds);
            return *this;
        }
        int size() {return s.size(); }
        ::VkDynamicState* getItem(int n=0) { return &s[n]; }
    private:
        std::vector<::VkDynamicState> s;
    };
    //----------------------------------
    class VkPipelineDynamicStateCreateInfo : public VkPipelineBaseCreateInfo
    {
    public:
        VkPipelineDynamicStateCreateInfo(VkDynamicState &attachments, VkPipelineDynamicStateCreateFlags flags = 0) :
            t(attachments)
        {
            s.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            s.pNext = NULL;
			s.flags = flags;
            s.pDynamicStates = t.getItem(0);
            s.dynamicStateCount = attachments.size();
        }
        ::VkPipelineDynamicStateCreateInfo* getItem() { return &s; }
        VkPipelineDynamicStateCreateInfo& operator=(const VkPipelineDynamicStateCreateInfo& src) { assert(!"TODO!"); return *this; }
        NVKPIPELINEBASECREATEINFOIMPLE
    private:
        ::VkPipelineDynamicStateCreateInfo s;
        VkDynamicState t; // need to keep the copies of attachments
    };
    //----------------------------------
    class VkStencilOpState
    {
    public:
        VkStencilOpState(::VkStencilOp failOp = VK_STENCIL_OP_KEEP, 
						::VkStencilOp passOp = VK_STENCIL_OP_KEEP, 
                        ::VkStencilOp depthFailOp = VK_STENCIL_OP_KEEP, 
						::VkCompareOp compareOp = VK_COMPARE_OP_ALWAYS,
						uint32_t compareMask = 0,
						uint32_t writeMask = 0,
						uint32_t reference = 0)
        {
        failOp = failOp;
		passOp = passOp;
        depthFailOp = depthFailOp;
		compareOp = compareOp;
		compareMask = compareMask;
		writeMask = writeMask;
		reference = reference;
        }
        operator ::VkStencilOpState& () { return s; }
        ::VkStencilOpState* getItem() { return &s; }
    private:
        ::VkStencilOpState s;
    };
    //----------------------------------
    class VkPipelineDepthStencilStateCreateInfo : public VkPipelineBaseCreateInfo
    {
    public:
        VkPipelineDepthStencilStateCreateInfo(
            ::VkBool32 depthTestEnable, ::VkBool32 depthWriteEnable, ::VkCompareOp depthCompareOp,
            ::VkBool32 depthBoundsTestEnable, ::VkBool32 stencilTestEnable, const VkStencilOpState &front, const VkStencilOpState &back,
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
        ::VkPipelineDepthStencilStateCreateInfo* getItem() { return &s; }
        NVKPIPELINEBASECREATEINFOIMPLE
    private:
        ::VkPipelineDepthStencilStateCreateInfo s;
    };
    //----------------------------------
    class VkPipelineMultisampleStateCreateInfo : public VkPipelineBaseCreateInfo
    {
    public:
        VkPipelineMultisampleStateCreateInfo(
			::VkSampleCountFlagBits rasterizationSamples, ::VkBool32 sampleShadingEnable, 
			float minSampleShading, 
			::VkSampleMask *pSampleMask,
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
            s.pSampleMask = pSampleMask;
			s.alphaToCoverageEnable = alphaToCoverageEnable;
			s.alphaToOneEnable = alphaToOneEnable;
        }
        VkPipelineMultisampleStateCreateInfo(
			::VkSampleCountFlagBits rasterizationSamples, ::VkBool32 sampleShadingEnable, 
			float minSampleShading, 
			::VkSampleMask _sampleMask,
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
        ::VkPipelineMultisampleStateCreateInfo* getItem() { return &s; }
        NVKPIPELINEBASECREATEINFOIMPLE
    private:
        ::VkPipelineMultisampleStateCreateInfo s;
        ::VkSampleMask sampleMask;
    };
    //---------------------------------
    class VkGraphicsPipelineCreateInfo
    {
    public:
        VkGraphicsPipelineCreateInfo( 
            ::VkPipelineLayout    layout, 
            ::VkRenderPass        renderPass,
            uint32_t            subpass = 0,
            ::VkPipeline          basePipelineHandle = 0,
            int32_t             basePipelineIndex = 0,
            ::VkPipelineCreateFlags flags = 0)
        {
            s.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            s.pNext = NULL;
            s.stageCount = NULL;
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
        inline VkGraphicsPipelineCreateInfo& operator ()(const VkPipelineBaseCreateInfo& state) { return add (state); }
        inline VkGraphicsPipelineCreateInfo& add(const VkPipelineBaseCreateInfo& state)
        {
            switch(state.getType())
            {
            case VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO:
                pssci.push_back(((::VkPipelineShaderStageCreateInfo*)state.getPtr())[0]);
                s.stageCount = pssci.size();
                s.pStages = &pssci[0];
                break;
            case VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO:
                s.pVertexInputState = (::VkPipelineVertexInputStateCreateInfo*)state.getPtr();
                break;
            case VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO:
                s.pInputAssemblyState = (::VkPipelineInputAssemblyStateCreateInfo*)state.getPtr();
                break;
            case VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO:
                s.pTessellationState = (::VkPipelineTessellationStateCreateInfo*)state.getPtr();
                break;
            case VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO:
                s.pViewportState = (::VkPipelineViewportStateCreateInfo*)state.getPtr();
                break;
            case VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO:
                s.pRasterizationState = (::VkPipelineRasterizationStateCreateInfo*)state.getPtr();
                break;
            case VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO:
                s.pMultisampleState = (::VkPipelineMultisampleStateCreateInfo*)state.getPtr();
                break;
            case VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO:
                s.pDepthStencilState = (::VkPipelineDepthStencilStateCreateInfo*)state.getPtr();
                break;
            case VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO:
                s.pColorBlendState = (::VkPipelineColorBlendStateCreateInfo*)state.getPtr();
                break;
            case VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO:
                s.pDynamicState = (::VkPipelineDynamicStateCreateInfo*)state.getPtr();
                break;
            default: assert(!"can't happen");
            }
            return *this;
        }
        inline ::VkGraphicsPipelineCreateInfo* getItem() { return &s; }
        operator ::VkGraphicsPipelineCreateInfo* () { return &s; }
        VkGraphicsPipelineCreateInfo& operator=(const VkGraphicsPipelineCreateInfo& src) { assert(!"TODO!"); return *this; }
    private:
        struct StructHeader
        {
            ::VkStructureType                             sType;      // Must be VK_STRUCTURE_TYPE_PIPELINE_DS_STATE_CREATE_INFO
            const void*                                 pNext;      // Pointer to next structure
        };
        ::VkGraphicsPipelineCreateInfo s;
        std::vector<::VkPipelineShaderStageCreateInfo> pssci;
        friend class NVK::VkGraphicsPipelineCreateInfo& operator<<(NVK::VkGraphicsPipelineCreateInfo& os, NVK::VkPipelineBaseCreateInfo& dt);
    };
    //----------------------------------------------------------------------------
    inline ::VkPipeline vkCreateGraphicsPipeline(VkGraphicsPipelineCreateInfo &gp)
    {
        ::VkPipeline p;
        CHECK(::vkCreateGraphicsPipelines(m_device, NULL, 1, gp, NULL, &p) );
        return p;
    }
    //----------------------------------------------------------------------------
    class VkImageMemoryBarrier
    {
    public:
        VkImageMemoryBarrier& operator()(::VkAccessFlags srcAccessMask, ::VkAccessFlags dstAccessMask, 
			::VkImageLayout oldLayout, ::VkImageLayout newLayout, 
			uint32_t srcQueueFamilyIndex,
			uint32_t dstQueueFamilyIndex,
			::VkImage image, const VkImageSubresourceRange& subresourceRange)
        {
            ::VkImageMemoryBarrier ss;
            ss.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            ss.pNext = NULL;
            ss.srcAccessMask = srcAccessMask;
            ss.dstAccessMask = dstAccessMask;
            ss.oldLayout = oldLayout;
            ss.newLayout = newLayout;
			ss.srcQueueFamilyIndex = srcQueueFamilyIndex;
			ss.dstQueueFamilyIndex = dstQueueFamilyIndex;
            ss.image = image;
            ss.subresourceRange = subresourceRange.s;
            s.push_back(ss);
            ps.push_back(&s.back()); // CHECK: pointer may because wrong if vector reallocates... ?
            return *this;
        }
        VkImageMemoryBarrier(::VkAccessFlags srcAccessMask, ::VkAccessFlags dstAccessMask, 
			::VkImageLayout oldLayout, ::VkImageLayout newLayout, 
			uint32_t srcQueueFamilyIndex,
			uint32_t dstQueueFamilyIndex,
			::VkImage image, const VkImageSubresourceRange& subresourceRange)

        {
            operator()(srcAccessMask, dstAccessMask, oldLayout, newLayout, srcQueueFamilyIndex, dstQueueFamilyIndex, image, subresourceRange);
        }
        inline ::VkImageMemoryBarrier* getItem(int n) { return &s[n]; }
        inline int size() { return s.size(); }
        operator ::VkImageMemoryBarrier* () { return &s[0]; }
        VkImageMemoryBarrier& operator=(const VkImageMemoryBarrier& src) { assert(!"TODO!"); return *this; }
    private:
        std::vector<::VkImageMemoryBarrier> s;
        std::vector<::VkImageMemoryBarrier*> ps;
    };
    //----------------------------------------------------------------------------
    class VkBufferMemoryBarrier
    {
    public:
        VkBufferMemoryBarrier& operator()(::VkAccessFlags srcAccessMask, ::VkAccessFlags dstAccessMask, 
			uint32_t                                    srcQueueFamilyIndex,
			uint32_t                                    dstQueueFamilyIndex,
			::VkBuffer buffer, ::VkDeviceSize offset, ::VkDeviceSize size)
        {
            ::VkBufferMemoryBarrier ss;
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
        };
        VkBufferMemoryBarrier(::VkAccessFlags srcAccessMask, ::VkAccessFlags dstAccessMask, 
			uint32_t                                    srcQueueFamilyIndex,
			uint32_t                                    dstQueueFamilyIndex,
			::VkBuffer buffer, ::VkDeviceSize offset, ::VkDeviceSize size)
        {
            operator()(srcAccessMask, dstAccessMask, srcQueueFamilyIndex, dstQueueFamilyIndex, buffer, offset, size);
        }
        inline ::VkBufferMemoryBarrier* getItem(int n) { return &s[n]; }
        inline int size(int n) { return s.size(); }
        operator ::VkBufferMemoryBarrier* () { return &s[0]; }
    private:
        std::vector<::VkBufferMemoryBarrier> s;
    };
    //----------------------------------------------------------------------------
    struct VkClearColorValue
    {
        VkClearColorValue(float *v)
        { memcpy(s.float32, v, sizeof(float)*4); }
        VkClearColorValue(uint32_t *v)
        { memcpy(s.uint32, v, sizeof(uint32_t)*4); }
        VkClearColorValue(int32_t *v)
        { memcpy(s.int32, v, sizeof(int32_t)*4); }
        VkClearColorValue(float r, float g, float b, float a)
        { s.float32[0] = r; s.float32[1] = g; s.float32[2] = b; s.float32[3] = a; }
        VkClearColorValue(uint32_t r, uint32_t g, uint32_t b, uint32_t a)
        { s.uint32[0] = r; s.uint32[1] = g; s.uint32[2] = b; s.uint32[3] = a; }
        VkClearColorValue(int32_t r, int32_t g, int32_t b, int32_t a)
        { s.int32[0] = r; s.int32[1] = g; s.int32[2] = b; s.int32[3] = a; }
        ::VkClearColorValue s;
    };
    struct VkClearDepthStencilValue : ::VkClearDepthStencilValue
    {
        VkClearDepthStencilValue(float _depth, uint32_t _stencil)
        { depth = _depth; stencil = _stencil; }
    };
    //----------------------------------------------------------------------------
    class VkClearValue
    {
    public:
        VkClearValue(const VkClearColorValue &color)
        {
            ::VkClearValue cv;
            cv.color = color.s;
            s.push_back(cv);
        }
        VkClearValue(const VkClearDepthStencilValue &ds)
        {
            ::VkClearValue cv;
            cv.depthStencil = ds;
            s.push_back(cv);
        }
        inline VkClearValue& operator ()(const VkClearColorValue &color)
        {
            ::VkClearValue cv;
            cv.color = color.s;
            s.push_back(cv);
        }
        inline VkClearValue& operator ()(const VkClearDepthStencilValue &ds)
        {
            ::VkClearValue cv;
            cv.depthStencil = ds;
            s.push_back(cv);
            return *this;
        }
        ::VkClearValue* getItem(int n) { return &s[n]; }
        inline int size() { return s.size(); }
    private:
        std::vector<::VkClearValue> s;
    };
    //----------------------------------------------------------------------------
    class VkRenderPassBeginInfo
    {
    public:
        VkRenderPassBeginInfo(const ::VkRenderPass &renderPass, const ::VkFramebuffer &framebuffer, 
                               const ::VkRect2D &renderArea, const VkClearValue &clearValues ) :
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
        ::VkRenderPassBeginInfo* getItem() { return &s; }
        operator ::VkRenderPassBeginInfo* () { return &s; }
        VkRenderPassBeginInfo& operator=(const VkRenderPassBeginInfo& src) { assert(!"TODO!"); }
    private:
        ::VkRenderPassBeginInfo s;
        VkClearValue t;
    };
    //----------------------------------------------------------------------------
    class VkCommandBufferInheritanceInfo
    {
    public:
        VkCommandBufferInheritanceInfo(
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
        ::VkCommandBufferInheritanceInfo* getItem() { return &s; }
        operator ::VkCommandBufferInheritanceInfo* () { return &s; }
    private:
        ::VkCommandBufferInheritanceInfo s;
    };

    //----------------------------------------------------------------------------
    class VkFramebufferCreateInfo
    {
    public:
        VkFramebufferCreateInfo(
            ::VkRenderPass renderPass, 
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
        VkFramebufferCreateInfo(
            ::VkRenderPass renderPass, 
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
        ::VkFramebufferCreateInfo* getItem() { return &s; }
        operator ::VkFramebufferCreateInfo* () { return &s; }
        VkFramebufferCreateInfo& operator()(VkImageView imageView) // a way to easily append more imageViews
        {
            imageViews.push_back(imageView);
            s.attachmentCount = imageViews.size();
            s.pAttachments = &imageViews[0];
            return *this;
        }
    private:
        ::VkFramebufferCreateInfo s;
        std::vector<VkImageView> imageViews;
    };
    //----------------------------------------------------------------------------
    class VkAttachmentDescription
    {
    public:
        VkAttachmentDescription& operator()(::VkFormat format,VkSampleCountFlagBits samples,::VkAttachmentLoadOp loadOp,::VkAttachmentStoreOp storeOp,
            ::VkAttachmentLoadOp stencilLoadOp,::VkAttachmentStoreOp stencilStoreOp,::VkImageLayout initialLayout,::VkImageLayout finalLayout)
        {
            ::VkAttachmentDescription ss;
            ss.format= format;
            ss.samples = samples;
            ss.loadOp = loadOp;
            ss.storeOp = storeOp;
            ss.stencilLoadOp = stencilLoadOp;
            ss.stencilStoreOp = stencilStoreOp;
            ss.initialLayout = initialLayout;
            ss.finalLayout = finalLayout;
            s.push_back(ss);
            return *this;
        }
        VkAttachmentDescription(::VkFormat format,VkSampleCountFlagBits samples,::VkAttachmentLoadOp loadOp,::VkAttachmentStoreOp storeOp,
            ::VkAttachmentLoadOp stencilLoadOp,::VkAttachmentStoreOp stencilStoreOp,::VkImageLayout initialLayout,::VkImageLayout finalLayout)
        {
            operator()(format, samples, loadOp, storeOp, stencilLoadOp, stencilStoreOp, initialLayout, finalLayout);
        }
        VkAttachmentDescription(VkAttachmentDescription &a) { s = a.s; }
        VkAttachmentDescription() {}
        ::VkAttachmentDescription* getItem(int n) { return &s[n]; }
        inline int size() { return s.size(); }
        operator ::VkAttachmentDescription* () { return &s[0]; }
    private:
        std::vector<::VkAttachmentDescription> s;
    };
    //----------------------------------------------------------------------------
    class VkAttachmentReference
    {
    public:
        VkAttachmentReference& operator()(uint32_t attachment, ::VkImageLayout layout)
        {
            ::VkAttachmentReference ss;
            ss.attachment = attachment; // TODO: set it to s.size()
            ss.layout = layout;
            s.push_back(ss);
            return *this;
        }
        VkAttachmentReference() { }
        VkAttachmentReference(uint32_t attachment, ::VkImageLayout layout)
        {
            operator()(attachment, layout);
        }
        ::VkAttachmentReference* getItem(int n) { return s.size()>0 ? &s[n] : NULL; }
        inline int size() { return s.size(); }
        operator ::VkAttachmentReference* () { return s.size()>0 ? &s[0] : NULL; }
    private:
        std::vector<::VkAttachmentReference> s;
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
        ::uint32_t* getItem(int n) { return s.size()>0 ? &s[n] : NULL; }
        inline int size() { return s.size(); }
        operator ::uint32_t* () { return s.size()>0 ? &s[0] : NULL; }
    private:
        std::vector<::uint32_t> s;
    };
    //----------------------------------------------------------------------------
    // this class takes pointers: the copy of fields is more complicated than the
    // other cases. passing VkAttachmentReference as reference would be overkill
    // And NULL allows to avoid un-necessary ones
    //----------------------------------------------------------------------------
    class VkSubpassDescription
    {
    private:
        struct References {
            ::VkSubpassDescription s;
            bool bUsing_local_references; // when not from outside and from below:
            NVK::VkAttachmentReference inputAttachments;
            NVK::VkAttachmentReference colorAttachments;
            NVK::VkAttachmentReference resolveAttachments;
            NVK::VkAttachmentReference depthStencilAttachment;
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
                ::VkSubpassDescription &ss = r.s;

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
        VkSubpassDescription& operator()(::VkPipelineBindPoint pipelineBindPoint,
            const NVK::VkAttachmentReference *inputAttachments=NULL, const NVK::VkAttachmentReference *colorAttachments=NULL,
            const NVK::VkAttachmentReference *resolveAttachments=NULL, const NVK::VkAttachmentReference *depthStencilAttachment=NULL,
            const NVK::Uint32Array *preserveAttachments=NULL, ::VkSubpassDescriptionFlags flags=0)
        {
            // FIXME: const are annoying... for for now I wildly cast things below... sorry.
            s.resize(s.size()+1);
            References &r = s.back();
            r.bUsing_local_references = false;
            ::VkSubpassDescription &ss = r.s;
            ss.pipelineBindPoint = pipelineBindPoint;
            ss.flags = flags;
            if(inputAttachments) {
                ss.inputAttachmentCount = ((NVK::VkAttachmentReference *)inputAttachments)->size();
                ss.pInputAttachments = *((NVK::VkAttachmentReference *)inputAttachments);
            } else {
                ss.inputAttachmentCount = 0;
                ss.pInputAttachments = NULL;
            }
            if(colorAttachments) {
                ss.colorAttachmentCount = ((NVK::VkAttachmentReference *)colorAttachments)->size();
                ss.pColorAttachments = *((NVK::VkAttachmentReference *)colorAttachments);
            } else {
                ss.colorAttachmentCount = 0;
                ss.pColorAttachments = NULL;
            }
            if(resolveAttachments) {
                ss.pResolveAttachments = *((NVK::VkAttachmentReference *)resolveAttachments);
            } else {
                ss.pResolveAttachments = NULL;
            }
            // Hacky... sorry. Need to do better
            if(depthStencilAttachment 
                && (((NVK::VkAttachmentReference *)depthStencilAttachment)->getItem(0)->attachment != VK_ATTACHMENT_UNUSED)) {
                assert(((NVK::VkAttachmentReference *)depthStencilAttachment)->size() == 1);
                ss.pDepthStencilAttachment = ((NVK::VkAttachmentReference *)depthStencilAttachment)[0];
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
        VkSubpassDescription(::VkPipelineBindPoint pipelineBindPoint,
            NVK::VkAttachmentReference *inputAttachments=NULL,   NVK::VkAttachmentReference *colorAttachments=NULL,
            NVK::VkAttachmentReference *resolveAttachments=NULL, NVK::VkAttachmentReference *depthStencilAttachment=NULL,
            NVK::Uint32Array *preserveAttachments=NULL, ::VkSubpassDescriptionFlags flags=0) :
            unused(VK_ATTACHMENT_UNUSED/*attachment*/, VK_IMAGE_LAYOUT_UNDEFINED/*layout*/)
        {
            operator()(pipelineBindPoint, inputAttachments, colorAttachments, resolveAttachments, depthStencilAttachment, preserveAttachments, flags);
        }
        VkSubpassDescription& operator()(const ::VkPipelineBindPoint pipelineBindPoint,
            const NVK::VkAttachmentReference &inputAttachments_,   const NVK::VkAttachmentReference &colorAttachments_,
            const NVK::VkAttachmentReference &resolveAttachments_, const NVK::VkAttachmentReference &depthStencilAttachment_,
            const NVK::Uint32Array &preserveAttachments_, ::VkSubpassDescriptionFlags flags=0)
        {
            s.resize(s.size()+1);
            References &r = s.back();
            r.bUsing_local_references = true;
            ::VkSubpassDescription &ss = r.s;
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
        VkSubpassDescription(::VkPipelineBindPoint pipelineBindPoint,
            const NVK::VkAttachmentReference &inputAttachments,   const NVK::VkAttachmentReference &colorAttachments,
            const NVK::VkAttachmentReference &resolveAttachments, const NVK::VkAttachmentReference &depthStencilAttachment,
            const NVK::Uint32Array &preserveAttachments, ::VkSubpassDescriptionFlags flags=0) :
            unused(VK_ATTACHMENT_UNUSED/*attachment*/, VK_IMAGE_LAYOUT_UNDEFINED/*layout*/)
        {
            operator()(pipelineBindPoint, inputAttachments, colorAttachments, resolveAttachments, depthStencilAttachment, preserveAttachments, flags);
        }
        void copyFrom(const VkSubpassDescription &a)
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
        VkSubpassDescription(const VkSubpassDescription &a) 
        { 
            copyFrom(a);
        }
        VkSubpassDescription& operator=(const VkSubpassDescription &a) 
        { 
            copyFrom(a);
            return *this;
        }
        //VkSubpassDescription& operator=(const VkSubpassDescription &a) 
        //{ 
        //    copyFrom(a);
        //    return *this;
        //}
        VkSubpassDescription() { }
        ::VkSubpassDescription* getItem(int n) { return &s[n].s; }
        inline int size() { return s.size(); }
        operator ::VkSubpassDescription* () { return &s[0].s; }
    private:
        std::vector<References> s;
        // Unused attachment for when needed
        NVK::VkAttachmentReference unused;
    };
    //----------------------------------------------------------------------------
    class VkSubpassDependency
    {
    public:
        VkSubpassDependency& operator()(
			uint32_t                                    srcSubpass,
			uint32_t                                    dstSubpass,
			VkPipelineStageFlags                        srcStageMask,
			VkPipelineStageFlags                        dstStageMask,
			VkAccessFlags                               srcAccessMask,
			VkAccessFlags                               dstAccessMask,
			VkDependencyFlags                           dependencyFlags
			)
        {
            ::VkSubpassDependency ss;
			ss.srcSubpass = srcSubpass;
			ss.dstSubpass = dstSubpass;
			ss.srcStageMask = srcStageMask;
			ss.dstStageMask = dstStageMask;
			ss.srcAccessMask = srcAccessMask;
			ss.dstAccessMask = dstAccessMask;
			ss.dependencyFlags = dependencyFlags;
            s.push_back(ss);
        }
        VkSubpassDependency() {} // allows to reference None of it
        VkSubpassDependency(VkSubpassDependency &a) { s = a.s; } // allows to reference None of it
        VkSubpassDependency(
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
        ::VkSubpassDependency* getItem(int n) { return s.size()>0 ? &s[n] : NULL; }
        inline int size() { return s.size(); }
        operator ::VkSubpassDependency* () { return s.size()>0 ? &s[0] : NULL; }
    private:
        std::vector<::VkSubpassDependency> s;
    };
    //----------------------------------------------------------------------------
    class VkRenderPassCreateInfo
    {
    public:
        VkRenderPassCreateInfo() {
            s.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            s.pNext = NULL;
        }
        VkRenderPassCreateInfo(const VkRenderPassCreateInfo& src) {
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
        }
        VkRenderPassCreateInfo& operator=(const VkRenderPassCreateInfo& src) {
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
        VkRenderPassCreateInfo(
            const VkAttachmentDescription            &_attachments,
            const VkSubpassDescription               &_subpasses,
            const VkSubpassDependency                &_dependencies)
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
        }
        ::VkRenderPassCreateInfo* getItem() { return &s; }
        operator ::VkRenderPassCreateInfo* () { return &s; }
    private:
        ::VkRenderPassCreateInfo      s;
        VkAttachmentDescription    attachments;
        VkSubpassDescription       subpasses;
        VkSubpassDependency        dependencies;
    };

    class MemoryChunk
    {
    public:
        MemoryChunk();
        void     free();
        bool isValid() { return bValid; }
        VkBuffer createBufferAlloc(VkBufferCreateInfo &bufferInfo, ::VkFlags memProps=VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ::VkDeviceMemory *bufferMem=NULL);
        VkBuffer createBufferAlloc(size_t size, ::VkFlags bufferUsage, ::VkFlags memProps=VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ::VkDeviceMemory *bufferMem=NULL);
        VkBuffer createBufferAllocFill(::VkCommandPool cmdPool, VkBufferCreateInfo &bufferInfo, const void* data, ::VkFlags memProps=VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ::VkDeviceMemory *bufferMem=NULL);
        VkBuffer createBufferAllocFill(::VkCommandPool cmdPool, size_t size, const void* data, ::VkFlags usage, ::VkFlags memProps=VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ::VkDeviceMemory *bufferMem=NULL);
        void    destroyBuffers();
    private:
        bool                    bValid;
        NVK*                    nvk;
        ::VkDeviceSize          size;
        ::VkBufferUsageFlags    usage;
        ::VkFlags               memProps;
        ::VkDeviceMemory        deviceMem;
        ::VkBuffer              defaultBuffer;
        unsigned int            alignMask;
        size_t                  sizeUsed;
        std::vector<VkBuffer>   buffers;
        friend class NVK;
    };

    class VkSubmitInfo {
    public:
        VkSubmitInfo(
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
        VkSubmitInfo& operator()(
            uint32_t                                    waitSemaphoreCount,
            const VkSemaphore*                          pWaitSemaphores,
            const VkPipelineStageFlags*                 pWaitDstStageMask,
            uint32_t                                    commandBufferCount,
            const VkCommandBuffer*                      pCommandBuffers,
            uint32_t                                    signalSemaphoreCount,
            const VkSemaphore*                          pSignalSemaphores)
    {
        ::VkSubmitInfo ss;
        ss.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        ss.pNext = NULL;
        ss.waitSemaphoreCount = waitSemaphoreCount;
        ss.pWaitSemaphores = pWaitSemaphores;
        ss.pWaitDstStageMask = pWaitDstStageMask;
        ss.commandBufferCount = commandBufferCount;
        ss.pCommandBuffers = pCommandBuffers;
        ss.signalSemaphoreCount = signalSemaphoreCount;
        ss.pSignalSemaphores = pSignalSemaphores;
        s.push_back(ss);
        return *this;
    }
        operator ::VkSubmitInfo* () { return &s[0]; }
        size_t size() { return s.size(); }
        ::VkSubmitInfo* getItem(int n=0) { return &s[n]; }
    private:
        std::vector<::VkSubmitInfo> s;
    };

}; // NVK
