# Vulkan & OpenGL & Command-list Sample using "Thread-Workers"

With the official release of Vulkan, NVIDIA and the "Devtech-Proviz" Team released new samples on [professional graphics repository](https://github.com/nvpro-samples). 

The Purpose of this Blog post is to give more details on what is happening in the Sample called `gl_vk_bk3dthreaded` [(available here)](https://github.com/nvpro-samples/gl_vk_bk3dthreaded).

![Example](https://github.com/nvpro-samples/gl_vk_bk3dthreaded/blob/master/doc/sample.jpg)

## How to build the sample
For now, I am sorry to say that the sample might only run on Windows. I didn't consolidate it for Linux, yet.

This sample requires the following:

- LunarG SDK v1.0.3.1 : just install it from https://vulkan.lunarg.com : cmake should be able to locate it
- [shared sources](https://github.com/nvpro-samples/shared_sources) : this folder contains few helper files and additional cmake information needed to build the project
- [shared external](https://github.com/nvpro-samples/shared_external) : this folder a is a convenient way to gather all external tool that our samples rely on. Rather than trying to find back the right versions of zlib, AntTweakbar etc, this folder contains all the needed external projects that our samples need:
	- zlib: to read gz files (3D model(s) )
	- SvcMfCUI: a simple UI based on Windows MFC. No fancy but convenient
	- AntTweakBar (not used in this sample... yet. SvcMfCUI used instead)
	- Optionally: NSight nvTX custom markers
-  the submarine model: when you will configure the project with cmake, cmake script will perform a *wget* to get the model and store it locally: `MODEL_DOWNLOAD_SUBMARINE` Checked. The model is 32Mb and will be stored in a shared folder called `downloaded_resources`

Optionally, be aware that other *bk3d* models could be used in this sample. But to avoid heavy download, only the submarine will be taken by default. Check `MODEL_DOWNLOAD_MORE` On for more models...  

## How does the sample work

The sample will run by default with the *submarine* model *and some camera animation*. So if you want to freely move the camera, don't forget to stop the animation (UI or 'a' key) 

If you give as cmd-line argument another model (*.bk3d.gz or *.bk3d), the sample should be able to render it but the animation will be turned off; and it is possible that the camera won't focus exactly over the new model...

Vulkan renderer will be the default one at startup. You can switch between:

- **OpenGL & Command-lists**: an example on how to feed the token-buffers
- **OpenGL**: a basic implementation of how would you render 3D with OpenGL
- **Vulkan**: the default renderer

![toggles](https://github.com/nvpro-samples/gl_vk_bk3dthreaded/blob/master/doc/toggles.JPG)

> **Note**: toggles are preceded by a character between quotes: when the viewport has the focus, you can use the keyboard instead. 

- **Use Workers**: checked for multi-threading. Unchecked: only the main thread will update the draw commands (cmd-buffers)
- **command-buffer amount**: by default, 16 secondary command-buffers will be created to render everything. In the multi-threading case, thread-workers will get spawned and will work on building them: when **'c'** toggle is checked (*command-buffer continuous refresh*)
- **Cmd-buf-style**: this model came from a CAD application. It turns out that at the time this model was created, primitives were issued depending on their 'parts', rather than depending on their primitive type and/or materials (hence shaders). **"sort on primitive type"** would allow to first render triangles; then strips; then lines...
-  **MSAA**: Multispampling mode
- 'c', &g_bRefreshCmdBuffers, "c: toggles command buffer continuous refresh\n");
- <space-bar>, &m_realtime.bNonStopRendering, "space: toggles continuous rendering\n");
- toggles from 'o'to '5' are obvious options... just give a try

###cmd-line arguments

- -v (VBO max Size)
- -m (bk3d model)
- -c 0 or 1 : use command-lists
- -o 0 or 1 : display meshes
- -g 0 or 1 : display grid
- -s 0 or 1 : stats
- -a 0 or 1 : animate camera
- -d 0 or 1 : debug stuff (ui)
- -m (bk3d file) : load a specific model
- (bk3d file name)    : load a specific model
- -q (msaa) : MSAA

### mouse
special Key with the mouse allows few to move around the model. The camera is always targeting a focus point and is essentially working in "polar coordinates" (**TODO**: I need to display the focus point with a cross...)

- **mouse wheel**: zoom in/out from the focus point
- **left mouse button**: rotate around the focus point
- **right mouse button**: rotate around Ox axis and zoom in/out from focus point
- **right mouse button + Ctrl**: will push forward/backward the focus point 
- **middle mouse button**: pan left/right up/down the focus point along camera axis
- **arrows**: rotate around the focus point
- **Pg-up/Pg-down**: zoom in/out
- **Pg-up/Pg-down + Ctrl**: push forward/backward the focus point along camera axis

## 3D model(s)
the 3D model comes from a *pre-baked* format (see [here](https://github.com/tlorach/Bak3d) ). There is no value to understand how it is working: main interest is that it loads fast (baked format... saving us parsing time) and that I managed to 'capture' some models as they were issued by various applications.

The sample will load the model, then attach it to the renderers. The resource creation will thus depend on which Graphic API is being used. 

## More technical details

Here are more details in separate sub-sections :

* [Rendering Modes](https://github.com/nvpro-samples/gl_vk_bk3dthreaded/blob/master/doc/Rendering_Modes.md) : details on what is this sample rendering and how
* [Rendering with Vulkan](https://github.com/nvpro-samples/gl_vk_bk3dthreaded/blob/master/doc/Vulkan_Renderer.md)
: key steps in order to make Vulkan API work for this sample, including `GL_NV_draw_vulkan_image` extension
* [Vulkan Code Style](https://github.com/nvpro-samples/gl_vk_bk3dthreaded/blob/master/doc/Vulkan_Code_Style.md)
: helper file that allows to write Vulkan code in a more compact fashion 
* [Multithreading](https://github.com/nvpro-samples/gl_vk_bk3dthreaded/blob/master/doc/Multithreading.md "Multithreading")
: based on "Thread-workers", and how to use Vulkan in this case
* [Results / performances](https://github.com/nvpro-samples/gl_vk_bk3dthreaded/blob/master/doc/Results.md "Results")
* [NSight captures](https://github.com/nvpro-samples/gl_vk_bk3dthreaded/blob/master/doc/NSight_Captures.md "NSight captures") : some NSight captures showing what is happening

