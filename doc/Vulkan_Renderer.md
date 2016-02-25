# vulkan renderer
Vulkan renderer is located in `bk3d_vk.cpp` file:

- `RendererVk` is the class for the renderer
- `Bk3dModelVk` is the class for the model being rendered

## Initialization of resources

`RendererVk::initGraphics` will setup most of the Vulkan objects and related memory

Vulkan requires you to manage memory as much as possible. Of course you can rely on driver memory allocation ( `vkAllocateMemory` ), but better practice is to allocate memory with `vkAllocateMemory` in larger chunks and later take care about partitioning what is inside.

few possibilities to reach the right resources:

1. bind many VkBuffers or images at various offsets of the device memory chunk (`vkBindBufferMemory`...)
2. or use the binding offsets available in `vkCmdBindVertexBuffers` or `vkCmdBindIndexBuffer` or `vkCmdBindDescriptorSets` to reach the right section in the current buffer
3. Or a mix of both!

Note that in a real situation, more chunks of memory would be allocated: when previous ones are full, the engine might create a new one; and in a real situation, the application should have a more clever heap management from what gets allocated to what gets freed in chunks of memory.  

Ideally, the memory areas are a mix of buffers mapped to various offsets, while drawcalls do also use offsets withing buffers that are active:

![memory chunks](https://github.com/nvpro-samples/gl_vk_bk3dthreaded/blob/master/doc/Memory_chunks.JPG)

This sample doesn't implement this general case, but implements both of the 'extreme' cases:

the default one (see `#define USE_VKCMDBINDVERTEXBUFFERS_OFFSET`) will allocate *one VkBuffer for one chunk of Device Memory*; then offsets will be maintained for the 3D parts to find back their vertices/indices
![offsets](https://github.com/nvpro-samples/gl_vk_bk3dthreaded/blob/master/doc/offsets.JPG)

Another approach will 'forget' about offsets in buffers and naively create a VkBuffer for each required VBO/IBO. A basic allocator will bind these buffers to the right offsets in the device memory chunk: 
![offsets](https://github.com/nvpro-samples/gl_vk_bk3dthreaded/blob/master/doc/vkbuffers.JPG)

It turns-out that, out of demonstrating how to bind buffers at different areas of a device memory, **this latter approach could rapidly reach the limits of available objects** (here VkBuffer). This is precisely what happened to me once, with a big model from a CAD application...

In other words, it is **not a good idea to blindly use object handles only**: there are good reasons for why the *offset* parameter in command-binding exist. The renderer should be clever enough aggregate small buffers together thanks to the offset-binding in the command-buffer creation. The best solution would be *to mix both, depending on the requirements of the engine*.

## Initialization of Vulkan components
This section should be self-explanatory. Essentially the idea is to prepare things up-front, as much as possible (in fact: everything, except command-buffers) so that the *rendering loop doesn't involve any sort of expensive validation*. 

Validation is what made OpenGL so tricky: the state-machine of OpenGL tends to transform the driver into a *paranoid state* where it can never really know or guess what exactly is the application doing: everything is possible at any time and the driver must be ready for this!

Vulkan on the other hand expects the opposite: the developer must exactly know what he'll use. He must prepare things so that Vulkan driver doesn't have to worry anymore on un-expected situations.

This section will setup the following components:

- **Spir-V shaders** (*.spv)
- **semaphores** for glDrawVkImageNV synchronization
- a combination of various **Graphics-Pipelines**: One for 'lines' primitives; one for triangle-fans; another for triangle lists...
- **Sampler(s) and Texture(s)** (Note that I do load a Noise DDS texture but the latest shaders don't use it, finally...) 
- **general Uniform buffer**: needed for Projection/view dependent matrices, for example
- **descriptor-set layouts**: how the descriptor-sets are put together for various situations. You can see the Descriptor-Set layout as a way to reduce the scope of resource addressing: a layout that allows the driver to identify the scope of which memory pointers need to be set for a given situation.
- **pipeline-layout**: created from the *descriptor-set layouts*
- a list of states we want to **keep dynamic** (meaning they can be modified from withing a command-buffer): Viewport and scissors etc.
-  a **Descriptor-Pool** and some **Descriptor-Sets**: we will associate some resources to some descriptor-sets
-  **Fences** for command-buffer update (later below)
-  **Render-Pass** and its sub-pass(es)
-  **Frame-buffer** to associate with the Render-Pass 
-  Vulkan **timer** initialization

## Initialization of Command-Buffer Pools
*Command-buffer Pools must be created per thread*: the allocation/deallocation of command-buffers can only be performed in a concurrent manner if each thread owns its own allocation pool. In our sample, we will use the **TLS** (Thread Local Storage) for each thread to refer to his own pool.

The main initialization function will issue a series of calls to each thread in order to have them store their command-buffer pool in their own TLS (see `initThreadLocalVars`)

## Command-Buffers
Vulkan introduced the concept of **primary** and **secondary** command-buffers. The idea behind is to allow a more generic primary command-buffer to call secondary ones that would contain more details about the scene. Note that Vulkan restricted the hierarchy to only 2 levels.

Command-buffer usage is rather flexible. In our case, we will use various command-buffers with the idea that:

- for every frame, we will re-create the *primary command-buffer*
- *secondary command-buffers* might be created every frame or recycled: it is optional (see 'c' option in the UI)
- secondary command-buffers are used for specific purposes: 
	- one for *memory barrier* and *viewport setup*: this buffer will be created/updated *only when the viewport size changed*
	- another one for the grid of the floor: this command-buffer can also be very static and can be created once and for all...
	- finally other secondary command-buffers are used to render the geometry of the scene

![cmd-buffers](https://github.com/nvpro-samples/gl_vk_bk3dthreaded/blob/master/doc/cmd-buffers.JPG)
 
I mentioned earlier in the "initialization section" the creation of **Fences**.

As a reminder, the GPU is a co-processor that we want to fill with tasks in parallel with what a CPU would do on its side. Because we really want both of them to work in parallel, it is bad to 'serialize' the CPU with the GPU. Neverthelessm, it is still necessary to synchronize them at various critical steps.

The update of command-buffers is one of them: After we generated a bunch of command-buffers, we will en-queue them for the GPU to consume them. But it is possible that the CPU already looped back to the next frame for command-buffer creation, and this *before* the GPU was finished with the previous batch of command-buffers.

One naive solution is to wait for the GPU to be done and finally recycle the command-buffers for the next iteration. But waiting for the GPU would a be waste...

The *ideal solution* would be to allocate new command-buffers for the next frame so that we don't wait for GPU completion. Later, the consumed command-buffers should be identified and put back to the pool (garbage collection).

This sample is doing a bit of the latter: using a 2 caches of command-buffers and doing a **"Ping-pong"** transaction: 

- while GPU is dealing with cache #1; we will check the completion of cache #2 thanks to the Fence #2;
- most of the time it might be ready; worst case: a bit of wait.
- When ready, we will Free the command-buffer from cache #2
- then we would Allocate new command-buffers in this cache #2, while GPU finishes to consume what is in cache #1
- Then we will enqueue the new cmd-buffers from cache #2 to the GPU, tagged with Fence #2
- Next frame, the CPU will check the Fence #1, to see if the GPU was done with it (normally it should... GPU would have already started to consume cmd-buffers from cache #2)
- etc.

![Fences](https://github.com/nvpro-samples/gl_vk_bk3dthreaded/blob/master/doc/Fences.JPG)

This approach cannot allow building more than 1 frame ahead (in fact, many games artificially limit the # of frames ahead: to prevent lagging in game controls. Frame-ahead are Good for Benchmarks... not so good for gaming experience ;-). 

I suppose that a more generic approach would be to use a **ring-buffer** or a **command-buffer 'garbage' collector**, rather than limiting ourselves to 2 slots (ping-pong). Next revision of the sample might have a better approach...

## Blit to OpenGL back-buffer: `GL_NV_draw_vulkan_image`

The driver team introduced a convenient way to mix Vulkan rendering with an existing OpenGL context associated with the window.

Normally, **WSI** should be the way to work with Vulkan: dealing with a swapchain; associating it with the windows surface etc.

The interesting part of GL_NV_draw_vulkan_image is that it can spare you the work of setting up WSI; but more importantly is allows you to **mix Vulkan with openGL**. As an example: most of our samples are currently running Vulkan with an overlay in OpenGL: **AntTweakBar** or any other UI overlay is are still in OpenGL. If we didn't have this feature, No overlay would have worked right away and would have required quite some time to port...

GL_NV_draw_vulkan_image requires 2 **semaphores**:

- one that will be signaled as soon as the blit of the Vulkan image to the backbuffer is done (`m_semOpenGLReadDone` in the sample)
- the other one to be signaled by the Queue (`vkQueueSubmit`) when the GPU finally finished the rendering (`m_semVKRenderingDone` in the sample)

    	nvk.vkQueueSubmit( NVK::VkSubmitInfo(
    	  1, &m_semOpenGLReadDone,			// <== might make the queue wait to be signaled
    	  1, &m_cmdScene[m_cmdSceneIdx],
    	  1, &m_semVKRenderingDone),		// <== might make the copy to OpenGL to wait
    	  m_sceneFence[m_cmdSceneIdx] );`

The sample will call `RendererVk::blitToBackbuffer()` at the end for the final copy to the OpenGL backbuffer:

    glWaitVkSemaphoreNV((GLuint64)m_semVKRenderingDone);
    glDrawVkImageNV((GLuint64)m_colorImage.img, 0, 0,0,w,h, 0, 0,1,1,0);
    glSignalVkSemaphoreNV((GLuint64)m_semOpenGLReadDone);
