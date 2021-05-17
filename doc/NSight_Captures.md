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
   Copyright (c) 2016-2021, NVIDIA CORPORATION.  All rights reserved.
  
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
  
       http://www.apache.org/licenses/LICENSE-2.0
  
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
  
   SPDX-FileCopyrightText: Copyright (c) 2016-2021 NVIDIA CORPORATION
   SPDX-License-Identifier: Apache-2.0
````


