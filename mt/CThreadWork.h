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
#pragma once
//#define NOWIN32BUILTIN
//#define DBGTHREAD
#ifdef DBGTHREAD
# define LOGDBG LOGI
#else
# define LOGDBG(...)
#endif

#include <string>
#include <assert.h>
#include "CThread.h"

#include "RingBuffer.h"

//#define CB_CALL_CONV
#ifdef WIN32
#define CB_CONV      CALLBACK
#define CALL_CONV    __stdcall
#else
#define CB_CONV
#define CALL_CONV
#endif



#if defined IOS || defined ANDROID
#endif

#ifdef WIN32
#endif

class TaskQueue;
class ThreadWorker;

//#pragma mark - Globals // MacOSX thing
#ifdef USEGLOBALS
/************************************************************************************/
/**
 ** \brief return the main thread
 **/
NThreadHandle           GetMainThread();
/**
 ** \brief 
 **/
void                    DeclareMainThread();
/**
 ** \brief 
 **/
bool                    IsMainThread();
/**
 ** \brief 
 **/
TaskQueue*              GetMainTaskQueue();
/**
 ** \brief 
 **/
uint                    GetNumCPUCores();
/**
 ** \brief 
 **/
NThreadID               GetCurrentThreadID();
/**
 ** \brief 
 **/
TaskQueue*    GetCurrentTaskQueue();
#endif //USEGLOBALS

//#pragma mark - Events, Mutex, Semaphores with altertable feature // MacOSX thing

//pthreads doesn't support real alertable waiting, so we need to wake up and poll for it
#define NV_FAKE_WAIT_ALERTABLE_SLICES_MS 5

class CEventAlertable : public CEvent
{
public:
    bool WaitOnEventAltertable();
    bool WaitOnEventAltertable(::uint msTimeOut);
};

//#pragma mark - TaskBase // MacOSX thing
/************************************************************************************/
/**
 ** \brief Base for a Task, with the common way to invoke the task through Invoke()
 **
 ** this is the very base for any task to be part of the game: derive a class from this
 ** one and create a dedicated constructor that will contain the function arguments
 ** (arguments that normally you would pass to the method for executing what you want
 ** then the thread pool will call "Invoke()": the overloaded implementation will allow
 ** you to find back the arguments that you passed in the constructor
 ** This allows to normalize the invokation of a task, while allowing you to pre-declare
 ** arguments via the constructor
 **/
class TaskBase
{
private:
    /// \brief pointer to TaskQueue::m_taskCount
    NInterlockedValue* m_queueCountRef; 
protected:
    TaskBase() /*: m_queueCountRef(NULL)*/ {}
    virtual ~TaskBase();
public:
    /// \brief the main entry point for Task execution : this method is the one called to exectute the Task
    virtual void Invoke() = 0;
    /// \brief when the Task got accomplished, Done() gets called.
    virtual void Done();
#ifdef DBGTHREAD
    virtual const char *getDbgString() { return "NONAME"; };
#endif
    friend class TaskQueue;
};

// ?? Shall we create a bas class for tasks that we want to be able to invoke other Tasks

//#pragma mark - Cross Thread // MacOSX thing
//************************************************************************************
//
// TaskSyncCall is a class for function call(s) that would require waiting for the result
// prior moving forward: method "Call()" is what needs to be called. The caller will wait
// for the method "Call" to be completed. This method might execute on another thread
// This privately inherits from TaskBase so you can't pass it to the task invoker directly
//
class TaskSyncCall : private TaskBase 
{
private:
  CEventAlertable m_doneEvent;
  virtual void Done();
protected:
  TaskSyncCall();

  // Implement this
  virtual void Invoke() = 0;
public:
  /// \brief performs a synchonous call : wait for the result
    void Call(TaskQueue* destThread = NULL, bool waitAltertable = false);//GetMainTaskQueue());
};

//#pragma mark - Task Queue // MacOSX thing


/************************************************************************************/
/**
 ** \brief thread-local variable of the current Queue list of task 
 **/
extern TaskQueue* getCurrentTaskQueue();
/**
 ** \brief sets thread-local variable of the current Queue list of task 
 **/
extern void setCurrentTaskQueue(TaskQueue * tb);
extern int getThreadNumber();
extern void setThreadNumber(int n);

/************************************************************************************/
/**
 ** \brief Queue list of task to execute
 **
 ** each thread-worker owns a TaskQueue in which tasks are put ( pushTask() )
 ** Thread workers will get a TaskQueue assigned by default.
 **
 ** Special and optional case: the MAIN thread might also have such a TaskQueue: you can instanciate
 ** it and do while(g_mainTB->pollTask()) {} to execute possibly queued tasks that other
 ** workers might have pushed to the main thread.
 **/
class TaskQueue
{
private:
    //CThread       *m_thread;
    NThreadHandle   m_thread;
#ifdef WIN32
    NThreadID       m_threadID;
#endif

    /// \brief prototype for the function that is invoked for the task execution
    /// In many cases this function is TaskQueue::taskThreadFunc() with a TaskBase as the argument
    typedef void (CB_CONV *ThreadFunc)(void* params);
#if !defined WIN32 || defined NOWIN32BUILTIN
    /// \name non Win32 queue implementation
    /// @{
    typedef NRingBuffer<std::pair<ThreadFunc, void*> > Ring;
    // Seems like we could ask Windows to do this work for us...
    CEvent*                 m_dataReadyEvent;
    CCriticalSection        m_taskQueueLock;
    Ring                    m_taskQueue;
    /// @}
#endif
    
    /// \brief entry point to execute the task
    static void CB_CONV taskThreadFunc(void* params);
    /// \name Constructors/Destructors
    /// @{
#if !defined WIN32 || defined NOWIN32BUILTIN
    TaskQueue(CEvent* dataReadyEvent);
#endif
    TaskQueue(const TaskQueue&); //these are purposely not implemented
    TaskQueue& operator= (const TaskQueue&);
public:
    TaskQueue();
    TaskQueue(/*CThread **/NThreadHandle thread, CEvent* dataReadyEvent=NULL);
#ifdef WIN32
    TaskQueue(NThreadID id, CEvent* dataReadyEvent=NULL);
#endif
    ~TaskQueue();
private:
    /// @}

    /// \brief maintains the total amount of tasks
    NInterlockedValue m_taskCount;
    
    /// \brief push a function in the list ring of functions to call
    void pushTaskFunc(ThreadFunc call, void* params);    
public:
    inline int GetQueuedTaskCount() { return (int)m_taskCount; }
    /// \brief push a task into the execution buffer. Using taskThreadFunc.
    void pushTask(TaskBase * task);
    /// \brief poll a Task's function from the execution buffer and execute it
    bool pollTask(int timeout=0);
    void FlushTasks(bool waitAlertable = false);
    
    inline NThreadHandle GetDestinationThread() { return m_thread; }
  #ifdef WIN32
    inline NThreadID GetDestinationThreadID() { return m_threadID; }
  #else
    /// \brief pthreads doesn't differentiate between handles and IDs
    inline NThreadID GetDestinationThreadID() { return GetDestinationThread(); }
  #endif

    friend class ThreadWorker;
};


//#pragma mark - Task Worker // MacOSX thing
/************************************************************************************/
/**
 ** \brief Pool of workers
 **/
class ThreadWorker
{
private:
    /// \name contructors/destructor
    /// @{
    ThreadWorker(const ThreadWorker&); //these are purposely not implemented
    ThreadWorker& operator= (const ThreadWorker&);
public:
    /// \brief this starts the thread
    ThreadWorker(const std::string& threadName = std::string(), bool discardQueuedOnExit = false, bool waitAleratableOnExit = false);
    // \brief this blocks until all queued work is done unless you had told it to discard
    ~ThreadWorker();
    /// @}
private:
    unsigned long        m_workerID;
    std::string         m_threadName;
    CCriticalSection    m_threadNameSec;
    CEvent              m_doneEvent;
    TaskQueue           m_invoker; 
    volatile bool       m_discardQueuedOnExit;
    //volatile bool       m_alertableOnExit;

#ifdef WIN32
    /// \brief the real function that the thread will invoke - Win32 version
    static ::uint CALL_CONV threadFunc(void* p);
#else
    /// \brief the real function that the thread will invoke - Unix version
    static void* CALL_CONV threadFunc(void* p);
#endif
    /// whenever the work is done, signals data are ready to pickup
#if !defined WIN32 || defined NOWIN32BUILTIN
    CEvent              m_dataReadyEvent;
#endif
public:
    /// \name getters/setters
    /// @{
    inline bool GetDiscardQueuedOnExit() const { return m_discardQueuedOnExit; }
    inline void SetDiscardQueuedOnExit(bool b) { m_discardQueuedOnExit = b; }

    //inline bool GetWaitAlertableOnExit() const { return m_alertableOnExit; }
    //inline void SetWaitAleratbleOnExit(bool b) { m_alertableOnExit = b; }

    inline TaskQueue& GetTaskQueue() { return m_invoker; }
    inline int GetWorkerID() { return m_workerID; }

    const std::string& GetThreadName()/* const*/;
    void SetThreadName(const std::string& n);
    /// @}
    void SetBackgroundMode(bool b);
};

//#pragma mark - Pool of workers // MacOSX thing
/************************************************************************************/
/**
 ** \brief various cases to schedule work
 **/
enum NWORKER_THREADPOOL_SCHEDULE
{
    //the task is assigned to whichever thread has the least total queued tasks
    NWTPS_LEAST_QUEUED_TASKS, 
    //the tasks are assigned sequentially to threads wrapping around
    NWTPS_ROUND_ROBIN, 
    //the threads read from a central queue of tasks. 
    //this one is higher overhead, but it might be worth if you have very variable task completion times
    NWTPS_SHARED_QUEUE, 
};

/************************************************************************************/
/**
 ** \brief Pool of thread workers
 ** 
 ** this class contains all the thread workers available for any jobs/tasks
 **
 **/
class ThreadWorkerPool
{
private:
    ThreadWorkerPool(const ThreadWorkerPool&); //these are purposely not implemented
    ThreadWorkerPool& operator= (const ThreadWorkerPool&);
    
    ::uint m_threadCount;
    ThreadWorker* m_threads;
    NWORKER_THREADPOOL_SCHEDULE m_schedule;
    ::uint m_invokedTaskCount;
    
    /// \brief this task is invoked on multiple threads at once
    struct QueuedWorkProcessorTask : public TaskBase
    {
        const bool  m_discardOnExit;
        CEvent      m_doneEvent;
        CSemaphore  m_dataReadySem,         // Threads (QueuedWorkProcessorTask::Invoke()) are stuck in this semaphore. Pushing a Task will Release one so one of them can wake-the-F@#K-up and work
                    m_dataProcessedSem;
        NRingBuffer<TaskBase*>  m_taskQueue;
        CCriticalSection        m_taskQueueLock;
        
        QueuedWorkProcessorTask(bool discardOnExit);
        virtual void Invoke();
        virtual void Done();
#ifdef DBGTHREAD
        const char *getDbgString() { return __FUNCTION__; };
#endif
    };
    //this is only non-null if you are using NWTPS_SHARED_QUEUE
    QueuedWorkProcessorTask* m_queueTask;
    
public:
    /// \brief constructor
    ThreadWorkerPool(::uint numThreads, bool discardQueuedOnExit = false, bool waitAleratableOnExit = false, NWORKER_THREADPOOL_SCHEDULE sched = NWTPS_ROUND_ROBIN,  const std::string& threadName = std::string());
    /// this will block until all the threads are done
    ~ThreadWorkerPool();
    
    ::uint getThreadCount() { return m_threadCount; }
    ThreadWorker * getThreadWorker(int n);
    /// this destroys the task when it's done
    void pushTask(TaskBase* task);
    
    void SetBackgroundMode(bool b);
    void FlushTasks(bool waitAlertable = false);
    void Terminate();
};


