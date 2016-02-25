# Multithreading: Thread-Workers

The source code of the sample containing Vulkan and OpenGL source code is **not** especially designed to be multi-threaded.

To be more specific, the *only* declaration that suggests multi-threading is `NThreadLocalVar` template: to allow TLS (Thread Local Storage) to happen...

All the rest of the multi-threading happens in the main sample file `gl_vk_bk3dthreaded.cpp`.

In other words, some methods of OpenGL or Vulkan renderers become multithreaded because *they got wrapped by dedicated Class*, making them multi-threaded.

## Thread-Workers job assignment
To assign a job to a worker, you must declare a specific Class, where:

- the *constructor* will become the receiver for **the function arguments**
- the worker will start his job through a specific method: `Invoke()`

This approach allows to prepare function arguments so that they are ready for later use: when the thread will finally be kicked-off by the thread-worker manager.
Generic example:

    class TskXXX : public TaskBase
    {
    private:
        int arg1;
        int arg2;
    public:
        TskUpdateCommandBuffer(int _arg1, int _arg2)
        {
            arg1 = _arg1; arg2 = _arg2;
        }
        virtual void Invoke()
        {
            s_pCurRenderer->SomeMethod(arg1, arg2);
        }
    };
 
To execute this job, we can queue workers for TskXXX as follow:

    for(int n=0; n<100; n++)
    {
        // worker will be deleted by the default method Done()
        TskXXX *tskXXX = new TskXXX(10, 2);
        g_mainThreadPool->pushTask(tskXXX);
    }

`g_mainThreadPool` is the main thread Pool manager that god initialized as follow:

    g_mainThreadPool = new ThreadWorkerPool(NUMTHREADS, false, false, NWTPS_ROUND_ROBIN, std::string("Main Worker Pool"));

![ThreadWorkers](https://github.com/nvpro-samples/gl_vk_bk3dthreaded/blob/master/doc/Thread_workers.JPG)

## Workers for command-buffer creation

for more details, one of the most important part of multi-threading in this sample is in `refreshCmdBuffers()`

Here is what multi-threaded command-buffer updates do:

- walk through the 3D model and split it in equal parts (almost...)
- push a Worker for the command-buffer creation of this part ( `g_mainThreadPool->pushTask(tskUpdateCommandBuffer)` )
- workers will execute in specific thread: what the worker-manager (`g_mainThreadPool`) will chose for you
- each worker will signal an *event* object when it finished the command-buffer creation
- the main thread in the meantime will have to wait for all to be done: looping into all the *event objects*
- Once secondary command-buffers are ready, the main thread will put them together in the primary command-buffer: . This task is not supposed to take time
