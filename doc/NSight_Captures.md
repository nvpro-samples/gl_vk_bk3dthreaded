### NSight captures

Here is an image of NSight Custom-markers when using OpenGL

![OpenGL](https://github.com/nvpro-samples/gl_vk_bk3dthreaded/blob/master/doc/OpenGL.JPG)

You can see the expected "display" function where 

- in the bottom: the cascade of GPU commands pipelined through the GPU
- on the top, the brown line: a very dense series of OpenGL commands for state changes, buffer-binding and drawcalls. It shows how busy is the display() function in issuing commands to OpenGL. It shows how much the CPU is involved in this task (including the driver)
 

----------

Here is an image of NSight Custom-markers when Vulkan is using thread-workers.

![VulkanMT](https://github.com/nvpro-samples/gl_vk_bk3dthreaded/blob/master/doc/Vulkan_MT.JPG)

You can see how much rooms is available for anything else on the CPU: the 8 thread finished very quickly the command-buffer update. Not only the main thread could do something while waiting for other threads to build command-buffers, but more thread workers could be allocated onto the 8 threads available.

----------

And here is an NSight capture when Vulkan is only using the main-thread. 

![VulkanMT](https://github.com/nvpro-samples/gl_vk_bk3dthreaded/blob/master/doc/Vulkan.JPG)

Despite the fact that the framerate is the same, there is not much room for more CPU processing. Now, one could argue that 8 additional threads could be used in parallel for other tasks, too. 

This is true: multi-threading can offer a wide range of possibilities. It all depends on what kind of design is needed...

And what is really exciting with Vulkan is exactly this kind of flexibility that many engineers have patiently been waiting for some time. Now the challenge is to make good use of this strength... it may not be always easy.

````
    Copyright (c) 2016, NVIDIA CORPORATION. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
     * Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
     * Neither the name of NVIDIA CORPORATION nor the names of its
       contributors may be used to endorse or promote products derived
       from this software without specific prior written permission.

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

````

