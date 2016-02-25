# Vulkan code style
This previous source code snippet reveals a weird syntax that is not native to Vulkan...

`NVK.h` and `NVK.cpp` contain an experimental overlay that turns many (ideally, all) structures of Vulkan to simple Classes made of **constructors** and occasionally **functors**.

My purpose was to find a way to lower the amount of C code required to fill all these Vulkan structures: To be honest I was quite scared the first time I saw Vulkan include file!

>
**Note**: I don't claim this is an ideal solution. Not even sure that it makes the code more readable. But I wanted to try it through few samples and stress the idea. Feedback or better ideas are most welcome.

The best examples are in the source code of `bk3d_vk.cpp`. But here is a simple example:

When creating a **Vertex Input State**, there are a bunch of nested structures to put together in order to finalize the description.

constructors and functors are interesting because they can turn C/C++ code into some sort of *functional* programming, where declarations are nested into one another and don't require *explicit temporary storage*.

Besides, they need less space in the code and can even have default argument values.

    NVK::VkPipelineVertexInputStateCreateInfo vkPipelineVertexInputStateCreateInfo(
        NVK::VkVertexInputBindingDescription    (0/*binding*/, 2*sizeof(vec3f)/*stride*/, VK_VERTEX_INPUT_RATE_VERTEX),
        NVK::VkVertexInputAttributeDescription  (0/*location*/, 0/*binding*/, VK_FORMAT_R32G32B32_SFLOAT, 0            /*offset*/) // pos
                                                (1/*location*/, 0/*binding*/, VK_FORMAT_R32G32B32_SFLOAT, sizeof(vec3f)/*offset*/) // normal
    );

> **Notes**: `VkVertexInputBindingDescription` pretty much corresponds to **D3D10 Slots** : a way to group interleaved attributes together in one buffer. You can have many of these 'Slots'
> `VkVertexInputAttributeDescription` corresponds to the attribute that lives in one of these slots, Hence the reference to the binding

In this example, the structure `VkPipelineVertexInputStateCreateInfo` is filled with parameters without the need to declare any temporary intermediate structure, to then pass its pointer:

- `NVK::VkVertexInputBindingDescription` constructor directly creates a local instance of the structure; which obviously will be destroyed with `vkPipelineVertexInputStateCreateInfo`
- if there were more than one Input-binding, a functor with the same arguments as the constructor would be added right afterward. This is what happens with the next class:
- `NVK::VkVertexInputAttributeDescription` is needed for more than one attribute: position and normal
	- the first tuple `(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0)` is its **constructor**
	- the second tuple `(1, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(vec3f))` is its **functor**
	- if there was more than 2 attributes, *another functor* would follow, etc.  

>**Note**: I tried to *avoid 'shortcuts'* and keep the *original names* and structures so there is less confusion when translating Vulkan structures to this kind of writing.

Another example I find particularly nicer to read is for the **RenderPass** creation:

    NVK::VkRenderPassCreateInfo rpinfo = NVK::VkRenderPassCreateInfo(
        NVK::VkAttachmentDescription
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
            (   VK_FORMAT_R8G8B8A8_UNORM, (VkSampleCountFlagBits)1,                                        //format, samples
                VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,          //loadOp, storeOp
                VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,  //stencilLoadOp, stencilStoreOp
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL //initialLayout, finalLayout
            ),
        // many sub-passes could be put after one another
        NVK::VkSubpassDescription
        (   VK_PIPELINE_BIND_POINT_GRAPHICS,                                                                        //pipelineBindPoint
            NVK::VkAttachmentReference(),                                                                           //inputAttachments
            NVK::VkAttachmentReference(0/*attachment*/, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL/*layout*/),        //colorAttachments
            NVK::VkAttachmentReference(2/*attachment*/, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL/*layout*/),        //resolveAttachments
            NVK::VkAttachmentReference(1/*attachment*/, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL/*layout*/),//depthStencilAttachment
            NVK::Uint32Array(),                                                                           //preserveAttachments
            0                                                                                                       //flags
        ),


Of course there is not magic and what you don't do yourself is done behind the scene (check `class VkPipelineVertexInputStateCreateInfo` for example). One could argue that it would be even more expensive than using regular Vulkan structures... But let's not forget that this part of the code is happening at **initialization time**... so does it really matter ?

Now, more attention should be brought when dealing with the *main rendering loop*...
