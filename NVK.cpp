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

#if defined(_MSC_VER)
#  include <windows.h>
#  include <direct.h>
#endif
#include <assert.h>
#include <float.h>
#include <math.h>

#include <string.h>
#include <vector>

#include <GL/glew.h>
//------------------------------------------------------------------------------
// VULKAN: NVK.h > vkfnptrinline.h > vulkannv.h > vulkan.h
//------------------------------------------------------------------------------
#include "NVK.h"
#include "main.h" // for LOGI/E...

template <typename T, size_t sz> inline size_t getArraySize(T(&t)[sz]) { return sz; }

//--------------------------------------------------------------------------------
NVK::VkGraphicsPipelineCreateInfo& operator<<(NVK::VkGraphicsPipelineCreateInfo& os, NVK::VkPipelineBaseCreateInfo& dt)
{
    os.add(dt);
    return os;
}

void NVK::vkUpdateDescriptorSets(const VkWriteDescriptorSet &wds)
{
    std::vector<::VkWriteDescriptorSet> updateArray;
    int N = wds.size();
    for(int i = 0; i<N; i++)
    {
        updateArray.push_back(*(wds.getItem(i)));
    }
    ::vkUpdateDescriptorSets(m_device, N, &updateArray[0], 0, 0);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
::VkDescriptorPool    NVK::vkCreateDescriptorPool(const VkDescriptorPoolCreateInfo &descriptorPoolCreateInfo)
{
    ::VkDescriptorPool descPool;
    ::vkCreateDescriptorPool(m_device, descriptorPoolCreateInfo.getItem(), NULL, &descPool);
    return descPool;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
::VkDescriptorSetLayout NVK::vkCreateDescriptorSetLayout(const VkDescriptorSetLayoutCreateInfo &descriptorSetLayoutCreateInfo)
{
    ::VkDescriptorSetLayout dsl;
    (::vkCreateDescriptorSetLayout(
    m_device,
    descriptorSetLayoutCreateInfo.getItem(),
	NULL,
    &dsl
    ) );
    return dsl;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
::VkPipelineLayout NVK::vkCreatePipelineLayout(::VkDescriptorSetLayout* dsls, uint32_t count)
{
    ::VkPipelineLayout pipelineLayout;
    ::VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    pipelineLayoutCreateInfo.setLayoutCount = count;
    pipelineLayoutCreateInfo.pSetLayouts = dsls;
    pipelineLayoutCreateInfo.pNext = NULL;
    pipelineLayoutCreateInfo.flags = 0;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = NULL;

    CHECK(::vkCreatePipelineLayout(
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
void NVK::vkFreeMemory(VkDeviceMemory mem)
{
    ::vkFreeMemory(m_device, mem, NULL);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void* NVK::vkMapMemory(VkDeviceMemory mem, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags)
{
    void* bufferPtr = NULL;
    CHECK(::vkMapMemory(m_device, mem, 0, size, 0, &bufferPtr) );
    return bufferPtr;
}
void NVK::vkUnmapMemory(VkDeviceMemory mem)
{
    ::vkUnmapMemory(m_device, mem);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::vkMemcpy(::VkDeviceMemory dstMem, const void * srcData, ::VkDeviceSize size)
{
    void* bufferPtr = NULL;
    CHECK(::vkMapMemory(m_device, dstMem, 0, size, 0, &bufferPtr) );
    {
        memcpy(bufferPtr, srcData, size);
    }
    ::vkUnmapMemory(m_device, dstMem);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::vkDestroyBuffer(VkBuffer buffer)
{
    ::vkDestroyBuffer(m_device, buffer, NULL);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::vkDestroyBufferView(VkBufferView bufferView)
{
    ::vkDestroyBufferView(m_device, bufferView, NULL);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::vkEndCommandBuffer(::VkCommandBuffer cmd)
{
    CHECK(::vkEndCommandBuffer(cmd) );
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::vkCmdCopyBuffer(::VkCommandBuffer cmd, VkBuffer srcBuffer, VkBuffer dstBuffer, ::VkDeviceSize size, ::VkDeviceSize dstOffset, ::VkDeviceSize srcOffset)
{
    ::VkBufferCopy copy;
    copy.size   = size;
    copy.dstOffset = dstOffset;
    copy.srcOffset  = srcOffset;
    ::vkCmdCopyBuffer(cmd, srcBuffer, dstBuffer, 1, &copy);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::vkResetCommandPool(VkCommandPool cmdPool, VkCommandPoolResetFlags flags)
{
    CHECK(::vkResetCommandPool(m_device, cmdPool, flags) );
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::vkDestroyCommandPool(VkCommandPool cmdPool)
{
    ::vkDestroyCommandPool(m_device, cmdPool, NULL);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
::VkBufferView NVK::vkCreateBufferView( VkBuffer buffer, ::VkFormat format, ::VkDeviceSize size )
{
    ::VkBufferView view;
    ::VkBufferViewCreateInfo viewInfo = {VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO };
    viewInfo.pNext = NULL;
    viewInfo.flags = 0;
    viewInfo.buffer = buffer;
    viewInfo.format = format;
    viewInfo.offset = 0;
    viewInfo.range  = size;
    CHECK(::vkCreateBufferView(m_device, &viewInfo, NULL, &view) );
    return view;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
::VkCommandBuffer NVK::vkAllocateCommandBuffer(::VkCommandPool pool, bool primary) const
{
    // Create the command buffer.
	::VkCommandBufferAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocInfo.pNext = NULL;
    allocInfo.commandPool = pool;
    allocInfo.level = primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    allocInfo.commandBufferCount = 1;
    ::VkCommandBuffer cmd;
	CHECK(::vkAllocateCommandBuffers(m_device, &allocInfo, &cmd) );
    return cmd;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void    NVK::vkFreeCommandBuffer(::VkCommandPool commandPool, ::VkCommandBuffer cmd)
{
    if(cmd == NULL)
        return;
	::vkFreeCommandBuffers(m_device, commandPool, 1, &cmd);
}
void NVK::vkResetCommandBuffer(VkCommandBuffer cmdBuffer, VkCommandBufferResetFlags flags)
{
    CHECK(::vkResetCommandBuffer(cmdBuffer, flags));
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
::VkCommandBuffer NVK::vkBeginCommandBuffer(::VkCommandBuffer cmd, bool singleshot, NVK::VkCommandBufferInheritanceInfo &cmdIInfo) const
{
    // Record the commands.
    ::VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.pNext = NULL;
    beginInfo.flags = singleshot ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : 0;
    beginInfo.pInheritanceInfo = cmdIInfo.getItem();
    CHECK(::vkBeginCommandBuffer(cmd, &beginInfo) );
    return cmd;
}
::VkCommandBuffer NVK::vkBeginCommandBuffer(::VkCommandBuffer cmd, bool singleshot, const ::VkCommandBufferInheritanceInfo* pInheritanceInfo) const
{
    // Record the commands.
    ::VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.pNext = NULL;
    beginInfo.flags = singleshot ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : 0;
    beginInfo.pInheritanceInfo = pInheritanceInfo;
    CHECK(::vkBeginCommandBuffer(cmd, &beginInfo) );
    return cmd;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
::VkRenderPass NVK::vkCreateRenderPass(NVK::VkRenderPassCreateInfo &rpinfo)
{
    ::VkRenderPass rp;
    CHECK(::vkCreateRenderPass(m_device, rpinfo, NULL, &rp) );
    return rp;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
::VkImage NVK::createImage1D(
    int width,
    ::VkDeviceMemory &colorMemory, 
    ::VkFormat format, 
    VkSampleCountFlagBits depthSamples, 
    VkSampleCountFlagBits colorSamples,
    int mipLevels)
{
    ::VkImage                     colorImage;
    // color texture & view
    ::VkImageCreateInfo cbImageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
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
    }
    cbImageInfo.flags = 0;

    CHECK(::vkCreateImage(m_device, &cbImageInfo, NULL, &colorImage) );
    colorMemory = allocMemAndBindObject(colorImage, VK_MEMORY_HEAP_DEVICE_LOCAL_BIT);
    return colorImage;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
::VkImage NVK::createImage2D(
    int width, int height, 
    ::VkDeviceMemory &colorMemory, 
    ::VkFormat format, 
    VkSampleCountFlagBits depthSamples, 
    VkSampleCountFlagBits colorSamples,
    int mipLevels)
{
    ::VkImage                     colorImage;
    // color texture & view
    ::VkImageCreateInfo cbImageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
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
    }
    cbImageInfo.flags = 0;

    CHECK(::vkCreateImage(m_device, &cbImageInfo, NULL, &colorImage) );
    colorMemory = allocMemAndBindObject(colorImage, VK_MEMORY_HEAP_DEVICE_LOCAL_BIT);
    return colorImage;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
::VkImage NVK::createImage3D(
    int width, int height, int depth,
    ::VkDeviceMemory &colorMemory, 
    ::VkFormat format, 
    VkSampleCountFlagBits depthSamples, 
    VkSampleCountFlagBits colorSamples,
    int mipLevels)
{
    ::VkImage                     colorImage;
    // color texture & view
    ::VkImageCreateInfo cbImageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
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
    }
    cbImageInfo.flags = 0;

    CHECK(::vkCreateImage(m_device, &cbImageInfo, NULL, &colorImage) );
    colorMemory = allocMemAndBindObject(colorImage, VK_MEMORY_HEAP_DEVICE_LOCAL_BIT);
    return colorImage;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
::VkFramebuffer NVK::vkCreateFramebuffer(VkFramebufferCreateInfo &fbinfo)
{
    ::VkFramebuffer framebuffer;
    CHECK(::vkCreateFramebuffer(m_device, fbinfo, NULL, &framebuffer) );
    return framebuffer;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
::VkShaderModule NVK::vkCreateShaderModule( const char *shaderCode, size_t size)
{
    ::VkResult result;
    ::VkShaderModuleCreateInfo shaderModuleInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    ::VkShaderModule shaderModule;
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

	result = ::vkCreateShaderModule(m_device, &shaderModuleInfo, NULL, &shaderModule);

    if (result != VK_SUCCESS)
        return NULL;
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
        if (strstr(pMessage, "Shader is not SPIR-V"))
        {
            return false;
        }
        //LOGE("ERROR: [%s] Code %d : %s\n", pLayerPrefix, msgCode, pMsg);
        return false;
    } else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
    {
        //if (strstr(pMsg, "vkQueueSubmit parameter, VkFence fence, is null pointer"))
        //{
        //    return false;
        //}
        LOGW("WARNING: [%s] Code %d : %s\n", pLayerPrefix, messageCode, pMessage);
        return false;
    } else {
        return false;
    }

    return false;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool NVK::CreateDevice()
{
    ::VkResult result;
    int                         queueFamilyIndex= 0;
    ::VkApplicationInfo         appInfo         = { VK_STRUCTURE_TYPE_APPLICATION_INFO, NULL };
    ::VkInstanceCreateInfo      instanceInfo    = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, NULL };
    ::VkDeviceQueueCreateInfo   queueInfo       = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, NULL };
    ::VkDeviceCreateInfo        devInfo         = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, NULL };
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

    static char *instance_validation_layers[] = {
        //"VK_LAYER_LUNARG_api_dump",
        "VK_LAYER_LUNARG_core_validation",
        "VK_LAYER_LUNARG_device_limits",
        //"VK_LAYER_LUNARG_image",
        //"VK_LAYER_LUNARG_object_tracker",
        "VK_LAYER_LUNARG_parameter_validation",
        //"VK_LAYER_LUNARG_screenshot",
        "VK_LAYER_LUNARG_swapchain",
        //"VK_LAYER_GOOGLE_threading",
        //"VK_LAYER_GOOGLE_unique_objects",
        //"VK_LAYER_LUNARG_vktrace",
        //"VK_LAYER_RENDERDOC_Capture",
        "VK_LAYER_LUNARG_standard_validation"
    };
    static int instance_validation_layers_sz = 0;//getArraySize(instance_validation_layers);
    //
    // Instance creation
    //
    appInfo.pApplicationName = "...";
    appInfo.applicationVersion = 1;
    appInfo.pEngineName = "...";
    appInfo.engineVersion = 1;
    appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);
    instanceInfo.flags = 0;
    instanceInfo.pApplicationInfo = &appInfo;
    // add some layers here ?
    instanceInfo.enabledLayerCount = instance_validation_layers_sz;
    instanceInfo.ppEnabledLayerNames = instance_validation_layers;
    instanceInfo.enabledExtensionCount = extension_names.size();
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
        m_CreateDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(m_instance, "vkCreateDebugReportCallbackEXT");
        m_DestroyDebugReportCallback = (PFN_vkDestroyDebugReportCallbackEXT) vkGetInstanceProcAddr(m_instance, "vkDestroyDebugReportCallbackEXT");
        if (!m_CreateDebugReportCallback) {
            LOGE("GetProcAddr: Unable to find vkCreateDebugReportCallbackEXT\n");
        }
        if (!m_DestroyDebugReportCallback) {
            LOGE("GetProcAddr: Unable to find vkDestroyDebugReportCallbackEXT\n");
        }
        m_DebugReportMessage = (PFN_vkDebugReportMessageEXT) vkGetInstanceProcAddr(m_instance, "vkDebugReportMessageEXT");
        if (!m_DebugReportMessage) {
            LOGE("GetProcAddr: Unable to find vkDebugReportMessageEXT\n");
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
        for(int i=0; i<m_gpu.memoryProperties.memoryTypeCount; i++)
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
        for(int i=0; i<m_gpu.memoryProperties.memoryHeapCount; i++)
        {
            LOGI("Memory heap %d: size:%uMb %s\n", i,
                m_gpu.memoryProperties.memoryHeaps[i].size/(1024*1024),
                m_gpu.memoryProperties.memoryHeaps[i].flags&VK_MEMORY_HEAP_DEVICE_LOCAL_BIT?"VK_MEMORY_HEAP_DEVICE_LOCAL_BIT":""
            );
        }
    m_gpu.memoryProperties.memoryHeaps[VK_MAX_MEMORY_HEAPS];
        vkGetPhysicalDeviceFeatures(physical_devices[j], &m_gpu.features); // too many stuff to display
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
    vkGetPhysicalDeviceFeatures(m_gpu.device, &m_gpu.features);
    vkGetPhysicalDeviceQueueFamilyProperties(m_gpu.device, &count, NULL);
    m_gpu.queueProperties.resize(count);
    vkGetPhysicalDeviceQueueFamilyProperties(m_gpu.device, &count, &m_gpu.queueProperties[0]);
    //
    // retain the queue type that can do graphics
    //
    for(int i=0; i<count; i++)
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
    devInfo.enabledExtensionCount = extension_names.size();
    devInfo.ppEnabledExtensionNames = &(extension_names[0]);
    result = ::vkCreateDevice(m_gpu.device, &devInfo, NULL, &m_device);
    if (result != VK_SUCCESS) {
        return false;
    }
    vkGetDeviceQueue(m_device, 0, 0, &m_queue);

    return true;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool NVK::DestroyDevice()
{
    ::vkDestroyDevice(m_device, NULL);
    m_device = NULL;
    ::vkDestroyInstance(m_instance, NULL);
    m_instance = NULL;
    m_queue = NULL;
    m_gpu.clear();
    return true;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
::VkDeviceMemory NVK::allocMemAndBindObject(VkBuffer obj, ::VkFlags memProps)
{
    ::VkResult result;
    ::VkDeviceMemory deviceMem;
    ::VkMemoryRequirements  memReqs;
    vkGetBufferMemoryRequirements(m_device, obj, &memReqs);

    if (!memReqs.size){
      return NULL;
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
      return NULL;
    }

    ::VkMemoryAllocateInfo memInfo = {
      VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,             // sType
      0,                                               // pNext
      memReqs.size,                                    // allocationSize
      memoryTypeIndex,                                 // memoryTypeIndex
    };

    //LOGW("allocating memory: memReqs.memoryTypeBits=%d memProps=%d memoryTypeIndex=%d Size=%d\n", memReqs.memoryTypeBits, memProps, memoryTypeIndex, memReqs.size);

    result = vkAllocateMemory(m_device, &memInfo, NULL, &deviceMem);
    if (result != VK_SUCCESS) {
      return NULL;
    }

    result = vkBindBufferMemory(obj, deviceMem, 0);
    if (result != VK_SUCCESS) {
      return NULL;
    }

    return deviceMem;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
::VkDeviceMemory NVK::allocMemAndBindObject(::VkImage obj, ::VkFlags memProps)
{
    ::VkResult result;
    ::VkDeviceMemory deviceMem;
    ::VkMemoryRequirements  memReqs;
    vkGetImageMemoryRequirements(m_device, obj, &memReqs);

    if (!memReqs.size){
      return NULL;
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
      return NULL;
    }

    ::VkMemoryAllocateInfo memInfo = {
      VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,             // sType
      0,                                               // pNext
      memReqs.size,                                    // allocationSize
      memoryTypeIndex,                                 // memoryTypeIndex
    };

    //LOGW("allocating memory: memReqs.memoryTypeBits=%d memProps=%d memoryTypeIndex=%d Size=%d\n", memReqs.memoryTypeBits, memProps, memoryTypeIndex, memReqs.size);

    result = vkAllocateMemory(m_device, &memInfo, NULL, &deviceMem);
    if (result != VK_SUCCESS) {
      return NULL;
    }

    result = vkBindImageMemory(m_device, obj, deviceMem, 0);
    if (result != VK_SUCCESS) {
      return NULL;
    }

    return deviceMem;
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
::VkResult NVK::fillBuffer(::VkCommandPool cmdPool, size_t size, ::VkResult result, const void* data, VkBuffer buffer, VkDeviceSize offset)
{
    if(!data)
        return VK_SUCCESS;
    //
    // Create staging buffer
    //
    VkBuffer bufferStage;
    VkBufferCreateInfo bufferStageInfo(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    CHECK(::vkCreateBuffer(m_device, &bufferStageInfo, NULL, &bufferStage) );
    //
    // Allocate and bind to the buffer
    //
    ::VkDeviceMemory bufferStageMem;
    bufferStageMem = allocMemAndBindObject(bufferStage, (::VkFlags)VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    vkMemcpy(bufferStageMem, data, size);

    ::VkCommandBuffer cmd;
    cmd = vkAllocateCommandBuffer(cmdPool, true);
    vkBeginCommandBuffer(cmd, true);
    {
        //obsolete: vkQueueAddMemReferences(m_queue, 1, &bufferStageMem);
        vkCmdCopyBuffer(cmd, bufferStage, buffer, size, offset);
    }
    vkEndCommandBuffer(cmd);

	::VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO, NULL };
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = NULL;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = NULL;

    CHECK(::vkQueueSubmit(m_queue, 1, &submitInfo, VK_NULL_HANDLE) );
    vkDeviceWaitIdle(m_device);
    //
    // release stuff
    //
    vkFreeCommandBuffer(cmdPool, cmd);
    vkDestroyBuffer(bufferStage);
    ::vkFreeMemory(m_device, bufferStageMem, NULL);
    //obsolete: vkQueueRemoveMemReferences(m_queue, 1, &bufferStageMem);
    return result;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::fillImage(::VkCommandPool cmdPool, VkBufferImageCopy &bufferImageCopy, const void* data, VkDeviceSize dataSz, ::VkImage image)
{
    if(!data)
        return;
    //
    // Create staging buffer
    //
    VkBuffer bufferStage;
    VkBufferCreateInfo bufferStageInfo(dataSz, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    CHECK(::vkCreateBuffer(m_device, &bufferStageInfo, NULL, &bufferStage) );
    //
    // Allocate and bind to the buffer
    //
    ::VkDeviceMemory bufferStageMem;
    bufferStageMem = allocMemAndBindObject(bufferStage, (::VkFlags)VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    vkMemcpy(bufferStageMem, data, dataSz);

    ::VkCommandBuffer cmd;
    cmd = vkAllocateCommandBuffer(cmdPool, true);
    vkBeginCommandBuffer(cmd, true);
    {
        //obsolete: vkQueueAddMemReferences(m_queue, 1, &bufferStageMem);
        vkCmdCopyBufferToImage(cmd, bufferStage, image, VK_IMAGE_LAYOUT_GENERAL, bufferImageCopy.size(), bufferImageCopy.getItem());
    }
    vkEndCommandBuffer(cmd);

	::VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO, NULL };
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = NULL;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = NULL;

    CHECK(::vkQueueSubmit(m_queue, 1, &submitInfo, VK_NULL_HANDLE) );
    vkDeviceWaitIdle(m_device);
    //
    // release stuff
    //
    vkFreeCommandBuffer(cmdPool, cmd);
    vkDestroyBuffer(bufferStage);
    ::vkFreeMemory(m_device, bufferStageMem, NULL);
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VkBuffer NVK::createAndFillBuffer(::VkCommandPool cmdPool, size_t size, const void* data, ::VkFlags usage, ::VkDeviceMemory &bufferMem, ::VkFlags memProps)
{
    ::VkResult result = VK_SUCCESS;
    VkBuffer buffer;
    VkBufferCreateInfo bufferInfo(size, usage | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

    CHECK(::vkCreateBuffer(m_device, &bufferInfo, NULL, &buffer) );
    
    bufferMem = allocMemAndBindObject(buffer, memProps);

    if (data)
    {
      result = fillBuffer(cmdPool, size, result, data, buffer);
    }

    return buffer;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VkSemaphore NVK::vkCreateSemaphore(::VkSemaphoreCreateFlags flags)
{
    ::VkSemaphoreCreateInfo semCreateInfo = { 
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        NULL,
        flags
    };
    ::VkSemaphore sem;
    ::vkCreateSemaphore(m_device, &semCreateInfo, NULL, &sem);
    return sem;
}
void NVK::vkDestroySemaphore(::VkSemaphore s)
{
    ::vkDestroySemaphore(m_device, s, NULL);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
::VkFence NVK::vkCreateFence(::VkFenceCreateFlags flags)
{
    ::VkFenceCreateInfo finfo = {
        VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        NULL,
        flags
    };
    ::VkFence fence;
    CHECK(::vkCreateFence(m_device, &finfo, NULL, &fence) );
    return fence;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::vkDestroyFence(VkFence fence)
{
    ::vkDestroyFence(m_device, fence, NULL);
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool NVK::vkWaitForFences(uint32_t fenceCount, const VkFence* pFences, VkBool32 waitAll, uint64_t timeout)
{
    if(::vkWaitForFences(m_device, fenceCount, pFences, waitAll, timeout) == VK_TIMEOUT)
        return false;
    return true;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::vkResetFences(uint32_t fenceCount, const VkFence* pFences)
{
    CHECK(::vkResetFences(m_device, fenceCount, pFences) );
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::vkAllocateDescriptorSets( const NVK::VkDescriptorSetAllocateInfo& allocateInfo,
								    VkDescriptorSet*                            pDescriptorSets)
{
    CHECK(::vkAllocateDescriptorSets( m_device, allocateInfo, pDescriptorSets) );
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VkSampler NVK::vkCreateSampler(const NVK::VkSamplerCreateInfo &createInfo)
{
    VkSampler s;
    CHECK(::vkCreateSampler(m_device, createInfo.getItem(), NULL, &s) );
    return s;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::vkDestroySampler(const VkSampler s)
{
    ::vkDestroySampler(m_device, s, NULL);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VkImageView NVK::vkCreateImageView(const NVK::VkImageViewCreateInfo &createInfo)
{
    VkImageView s;
    CHECK(::vkCreateImageView(m_device, createInfo.getItem(), NULL, &s) );
    return s;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::vkDestroyImageView(const VkImageView s)
{
    ::vkDestroyImageView(m_device, s, NULL);
}

void NVK::vkDestroyImage(const VkImage s)
{
    ::vkDestroyImage(m_device, s, NULL);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VkBuffer NVK::vkCreateBuffer(const VkBufferCreateInfo &bci)
{
    VkBuffer buf;
    ::vkCreateBuffer(m_device, bci, NULL, &buf) ;
    return buf;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VkResult NVK::vkBindBufferMemory(VkBuffer &buffer, VkDeviceMemory mem, VkDeviceSize offset)
{
    return ::vkBindBufferMemory(m_device, buffer, mem, offset);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void NVK::vkQueueSubmit(const NVK::VkSubmitInfo& submits, VkFence fence)
{
    ::vkQueueSubmit(m_queue, submits.size(), submits.getItem(0), fence);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
NVK::MemoryChunk NVK::allocateMemory(size_t size, ::VkFlags usage, ::VkFlags memProps)
{
    ::VkResult              result;
    ::VkMemoryRequirements  memReqs;
    NVK::MemoryChunk        memoryChunk;

    memoryChunk.nvk         = this;
    memoryChunk.size        = size;
    memoryChunk.usage       = usage;
    memoryChunk.bValid      = false;
    memoryChunk.deviceMem   = NULL;
    memoryChunk.memProps    = memProps;
    memoryChunk.sizeUsed    = 0;

    //
    // Create staging buffer
    //
    VkBufferCreateInfo bufferStageInfo(size, usage);
    CHECK(::vkCreateBuffer(m_device, &bufferStageInfo, NULL, &memoryChunk.defaultBuffer) );
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

    ::VkMemoryAllocateInfo memInfo = {
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

    //result = vkBindBufferMemory(memoryChunk.defaultBuffer, memoryChunk.deviceMem, 0);
    //if (result != VK_SUCCESS) {
    //    vkFreeMemory(memoryChunk.deviceMem);
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
            nvk->vkDestroyBuffer(defaultBuffer);
            nvk->vkFreeMemory(deviceMem);
        }
    }
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
VkBuffer NVK::MemoryChunk::createBufferAlloc(const VkBufferCreateInfo &bufferInfo, ::VkFlags memProps, ::VkDeviceMemory *bufferMem)
{
    VkBuffer buffer;
    buffer = nvk->vkCreateBuffer(bufferInfo);
    if(buffer == NULL)
        return buffer;
    VkResult result;
    assert((sizeUsed + bufferInfo.size) <= size);
    result = nvk->vkBindBufferMemory(buffer, deviceMem, sizeUsed);
    if (result != VK_SUCCESS) {
        //nvk->vkFreeMemory(deviceMem);
        nvk->vkDestroyBuffer(buffer);
        deviceMem = NULL;
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
VkBuffer NVK::MemoryChunk::createBufferAlloc(size_t size, ::VkFlags bufferUsage, ::VkFlags memProps, ::VkDeviceMemory *bufferMem)
{
    return createBufferAlloc(VkBufferCreateInfo(
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
VkBuffer NVK::MemoryChunk::createBufferAllocFill(::VkCommandPool cmdPool, size_t size, const void* bufferData, ::VkFlags bufferUsage, ::VkFlags memProps, ::VkDeviceMemory *bufferMem)
{
    VkResult result = VK_SUCCESS;
    VkBuffer buffer = createBufferAlloc(VkBufferCreateInfo(
            size,
            bufferUsage,
            0,
            VK_SHARING_MODE_EXCLUSIVE /*VkSharingMode sharingMode*/,
            0/*uint32_t queueFamilyCount*/,
            NULL/*const uint32_t* pQueueFamilyIndices*/),
        memProps, bufferMem);
    if (bufferData)
    {
      result = nvk->fillBuffer(cmdPool, size, result, bufferData, buffer);
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
        nvk->vkDestroyBuffer(buffers[i]);
    }
    buffers.clear();
    sizeUsed = 0;
}



