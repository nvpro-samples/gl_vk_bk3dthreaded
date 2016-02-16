# Vulkan rendering using Thread-workers

This example shows how to write some Vulkan code and how to implement a multi-threading approach based of thread-workers.

Thread-workers are spawned to build parts of the scene (artificially) divided in smaller chunks.
Each thread-worker will get the responsibility to write secondary command buffers.

When all are done, the main thread will gather these command-buffers to finally integrate them to the primary command-buffer.

In this example - because it is a sample - the scene is static: we didn't want to add too much complexity. However you can imagine that the scene could be more dynamic due to higher complexity, therefore justifying the worker-threads.

This sample also allows you to compare various ways to render 3D models

* Vulkan
* regular OpenGL
* OpenGL NVIDIA command-list

You might notice that Vulkan really shines in its ability to re-construct command-buffers each frame in parallel.

The OpenGL implementation doesn't take advantage of multi-threading: this API is known to not be such a good API for multi-threading. Therefore the CPU will spend more cycles for the same scene-update.

NVIDIA Command-lists extension is good at executing token-buffers (equivalent of command-buffers but made of simple token andstructures); Command-lists are also fine with updating such token-bufers. However this extension relies on OpenGL and therefore is not the best candidate for multi-threaded rendering design.

![Example](https://github.com/nvpro-samples/gl_vk_bk3dthreaded/blob/master/doc/sample.jpg)

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

