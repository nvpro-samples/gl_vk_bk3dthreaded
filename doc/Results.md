# Results

- **Vulkan static**: means we render with Vulkan but command-buffers for geometry is never updated. The scene is made of **static meshes**
- **Vulkan dynamic 16 workers**: command-buffers are all built during each frame, like if it required constant update or change. Typic for dynamic scenes. 16 workers involved
- **Vulkan dynamic 1  worker**: same above but like if no multi-threading involved
- **OpenGL**: regular OpenGL. It pretty much corresponds to a dynamic scene, because OpenGL *requires you to update render-states and drawcalls each frames* (except for Display-Lists)
- Cmd-Lists static: assuming we created once all the **token-buffers**. Scene is static
- Cmd-Lists dynamic 16 workers: re-building the token buffers each frame in multithread
- Cmd-Lists dynamic 1 worker:re-building the token buffers each frame with one thread


rendering mode              | GPU time | CPU time [ms]| 
--------------------------- | -------- | -------- |
Vulkan static               |      5.7 |    0.688 |
Vulkan dynamic 16 workers   |      5.7 |     3.0  |
Vulkan dynamic 1  worker    |      5.7 |     5.2  |
                            |          |          |
OpenGL                      |      9.9 |     9.4  |
                            |          |          |
Cmd-Lists static            |      5.0 |    0.097 |
Cmd-Lists dynamic 16 workers|     40.0 |     40.0 |
Cmd-Lists dynamic 1 worker  |     20.0 |     20.0 |

Vulkans shows as expected that it performs very well in multi-threaded mode. this model may not be the best use-case for multi-threading, but we can already see that workers allows parallel processing, almost dividing by 2 the amount of time required.

**OpenGL is driver limited**: the fact that a lot of state changes and drawcalls are required for each frame doesn't leave much room (none, in fact) for *more CPU processing*. So if the engine had to perform some *Physic simulation over the scene*, the performances would *drop even more*. On the other hand, Vulkan left some room for the CPU to process additional tasks: the frame-rate could stay the same with more processing !

Command-lists are the best for static scenes. It makes sense because the token-buffers are really very close to the GPU front-End. So the *driver has nearly nothing to do*.

On the other hand: as soon as we want to make command-lists dynamic, things get complicated and way less efficient. However:

- there must be a **bug** (sorry) in the multi-threaded command-list approach. Even though it may not be as efficient as Vulkan, it shouldn't be as bad... to be continued in upcoming github updates :-). But in a way, this bug shows one thing: it *shows that Command-lists in multithreaded mode is not as straightforward as Vulkan API*. Even if it might be possible to get good performances, the source code could become hard to maintain. 
- Command-lists are in OpenGL API. And OpenGL is really bad for multi-threading. In this sample, the token-buffer creation in thread-workers is absolutely not dealing with OpenGL. It means that it postponed the stitching of token-buffers to later, by the main thread.
