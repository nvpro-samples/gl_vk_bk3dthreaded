## Rendering modes
This is a simple sample, so I took the liberty to make the shader-system extremely simple: *only 3 fragment shaders* are in involved: one for the grid; one for lines; the other for filled primitives. So I cannot claim showcasing complex use-case made of tons of shaders as it often happens. On the other hand, it might allow the sample to keep simple... 

In any renderer, we are trying to be efficient: the model contains lots of transformations as well as lots of materials. Rather than updating them on the flight (by updating uniforms, for example), we will generate *arrays of materials* and *arrays of transformations*. Then we will bind the right buffer offsets thanks to:

* `glBindBufferRange` for OpenGL
* *Bindless pointers* for Command-lists
* `vkCmdBindDescriptorSets` offset argument for Vulkan

In many cases, especially for OpenGL, 'bucketing' Primitives and/or grouping them according to their shaders allows greater performance than taking primitives as they come. Although Vulkan & Command-lists adds lots of tolerance over the amount of state transition in their command buffers, it is better practice to avoid overloading them too much.
