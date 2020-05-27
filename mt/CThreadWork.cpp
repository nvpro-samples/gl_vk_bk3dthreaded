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
//#include "comms.h"
#if defined WIN32
#include <windows.h>
#include <process.h>
#else
#include <unistd.h>
#include <sys/types.h>
//#include <sys/sysctl.h>
#include <sys/time.h>
#include <pthread.h>
#endif

#include <stdio.h>
#include "CThreadWork.h"
#include "nvh/nvprint.hpp"

//#ifdef ANDROID
//#pragma mark - Android defs
//#   pragma message("-----------------------------------------------")
//#   pragma message("---- coreRenderer.h : ANDROID -----------------")
//#   pragma message("-----------------------------------------------")
//#   include <android/log.h>
//#   define  LOG_TAG    "nvFXTest1"
//#   define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
//#   define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
//#elif defined(IOS)
//#pragma mark - IOS defs
//#   define  LOGI(...)  { printf(__VA_ARGS__); printf("\n"); }
//#   define  LOGE(...)  { printf(__VA_ARGS__); printf("\n"); }
//#elif defined (ES2EMU)
//#pragma mark - ES2Emu
//#   pragma message("-----------------------------------------------")
//#   pragma message("---- coreRenderer.h : Windows ES2 Emulation ---")
//#   pragma message("-----------------------------------------------")
//#   include "logging.h"
//#   define  LOGI(...)  { PRINTF((__VA_ARGS__)); PRINTF(("\n")); }
//#   define  LOGE(...)  { PRINTF((__VA_ARGS__)); PRINTF(("\n")); }
//#else
//#pragma mark - Windows defs
//#   pragma message("-----------------------------------------------")
//#   pragma message("---- coreRenderer.h : Windows + Glew ----------")
//#   pragma message("-----------------------------------------------")
////#   include "logging.h"
//#   define  LOGI(...)  { PRINTF((__VA_ARGS__)); PRINTF(("\n")); }
//#   define  LOGE(...)  { PRINTF((__VA_ARGS__)); PRINTF(("\n")); }
//#endif


//#pragma mark - Task base

/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/**
 ** 
 **/
#ifdef USEGLOBALS

#ifdef WIN32
static NThreadID g_mainThreadID = 0;
static NThreadHandle g_mainThreadHandle = INVALID_HANDLE_VALUE;
#else
#define InvalidThreadHandle ((NThreadHandle)0xBEEFCAFE)
static NThreadHandle g_mainThreadHandle = InvalidThreadHandle;
#endif
static TaskQueue* g_mainTaskQueue = NULL;
static /* NThreadLocalVar< */TaskQueue* /* > */ g_tl_currentTaskQueue;

#ifdef WIN32
struct HandleCloser 
{
  HANDLE& handle;
  HandleCloser(HANDLE& h) : handle(h) {};
  ~HandleCloser() 
  {  
    if (handle != INVALID_HANDLE_VALUE)
    {
      CloseHandle(handle); 
      handle = INVALID_HANDLE_VALUE;
    }
  }
};
static HandleCloser hc(g_mainThreadHandle);
#endif

template <typename T>
struct Deleter 
{
  T*& ptr;
  Deleter(T*& p) : ptr(p) {};
  ~Deleter() 
  {  
    if (ptr)
    {
      delete ptr;
      ptr = NULL;
    }
  }
};
static Deleter<TaskQueue> d(g_mainTaskQueue);

/************************************************************************************/
/**
 ** 
 **/
NThreadHandle GetMainThread()
{
  return g_mainThreadHandle;
}

/************************************************************************************/
/**
 ** 
 **/
TaskQueue* GetMainTaskQueue()
{
  return g_mainTaskQueue;
}

/************************************************************************************/
/**
 ** 
 **/
TaskQueue* GetCurrentTaskQueue()
{
  return g_tl_currentTaskQueue;
}


/************************************************************************************/
/**
 ** gets back the current main thread, store it in the global variable
 ** and then create a TaskQueue related to this thread
 **/
void DeclareMainThread()
{
#ifdef WIN32
  //GetCurrentThread actually returns a fake handle that always means "the current thread" no matter what thread you are in
  //so we use GetCurrentThreadId + OpenThread instead
  NThreadID curID = GetCurrentThreadId();
  if (curID == g_mainThreadID)
    return;
  
  if (g_mainThreadHandle != INVALID_HANDLE_VALUE){
    LOGDBG("You already declared a main thread!\n");
    return;
  }

  g_mainThreadID = curID;
  // Opens an existing thread curID :
  g_mainThreadHandle = OpenThread(THREAD_ALL_ACCESS, FALSE, curID);
  g_mainTaskQueue = new TaskQueue(g_mainThreadID);
  g_tl_currentTaskQueue = g_mainTaskQueue;
#else
  if (g_mainThreadHandle != InvalidThreadHandle){
    LOGDBG("You already declared a main thread!\n");
    return;
  }

  g_mainThreadHandle = pthread_self();
  g_mainTaskQueue = new TaskQueue(g_mainThreadHandle);
  g_tl_currentTaskQueue = g_mainTaskQueue;
#endif
}

/************************************************************************************/
/**
 ** 
 **/
bool IsMainThread()
{
#ifdef WIN32
  return (g_mainThreadID && g_mainThreadID ==  GetCurrentThreadId());
#else
  return (g_mainThreadHandle != InvalidThreadHandle && pthread_equal(g_mainThreadHandle, pthread_self()));
#endif
}

/************************************************************************************/
/**
 ** 
 **/
NThreadID GetCurrentThreadID()
{
#ifdef WIN32
  return GetCurrentThreadId();
#else
  return pthread_self();
#endif
}

#endif //#ifdef USEGLOBALS

#ifdef ANDROID
extern "C"
{
//#include <cpu-features.h>
}
#endif
/************************************************************************************/
/**
 ** 
 **/
uint GetNumCPUCores()
{
#ifdef WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo( &sysinfo );
    return sysinfo.dwNumberOfProcessors;
#else
#ifdef ANDROID
    // hack for now
//    uint64_t features = android_getCpuFeatures();
//    if (features & ANDROID_CPU_ARM_FEATURE_NEON)
        return 4; //T30
//    else
//        return 2; //T20
#else
#   pragma warning CPU cores is hard coded to 2 for now...
    return 2;
#endif
#endif
}

/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/**
 ** Constructors
 **/
TaskBase::~TaskBase()
{
}
/**
    The default behavior is to delete the class.
    But in some case the class wasn't allocated as usual so you may want to override
    This method
 **/
void TaskBase::Done()
{
  delete this; // ok because ~NThreadTaskBase is virtual
  return;
}

//#pragma mark - Task list

/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/**
 ** Constructors
 **/
TaskQueue::TaskQueue() :
#if !defined WIN32 || defined NOWIN32BUILTIN
    m_taskQueue(16),
    m_dataReadyEvent(NULL),
#endif
    m_taskCount(0),
    m_thread(NULL)
    {}

TaskQueue::TaskQueue(/*CThread **/NThreadHandle thread, CEvent* dataReadyEvent) : 
#if !defined WIN32 || defined NOWIN32BUILTIN
    m_taskQueue(16),
    m_dataReadyEvent(dataReadyEvent),
#endif
    m_taskCount(0)
{
#ifdef WIN32
    if(thread)
        m_threadID = GetThreadId(thread);
    else
        m_threadID = GetCurrentThreadId();
    m_thread = OpenThread(THREAD_ALL_ACCESS, FALSE, m_threadID);
#else
    if(thread == 0)
        m_thread = pthread_self();
    else
        m_thread = thread;
#endif
}

#ifdef WIN32
TaskQueue::TaskQueue(NThreadID id, CEvent* dataReadyEvent) : 
#if defined NOWIN32BUILTIN
    m_taskQueue(16),
    m_dataReadyEvent(dataReadyEvent),
#endif
    m_taskCount(0)
{
    if(id == 0)
        m_threadID = GetCurrentThreadId();
    else
        m_threadID = id;
    m_thread = OpenThread(THREAD_ALL_ACCESS, FALSE, m_threadID);
}
#endif

TaskQueue::~TaskQueue()
{
#ifdef WIN32
    CloseHandle(m_thread);
#endif
}

//Either we are not using windows or we want to use the non-built-in task queue
#if !defined(WIN32) || defined(NOWIN32BUILTIN)

TaskQueue::TaskQueue(CEvent* dataReadyEvent) :
    m_taskCount(0),
    m_thread(0),
    m_dataReadyEvent(dataReadyEvent),
    m_taskQueue(16)
    {}
#endif
/************************************************************************************/
/**
 ** 
 **/
void TaskQueue::pushTaskFunc(ThreadFunc call, void* params)
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_CYAN);
#if !defined WIN32 || defined NOWIN32BUILTIN
    CCriticalSectionHolder h(m_taskQueueLock);
    m_taskQueue.WriteData(std::make_pair(call, params));
    
    if (m_dataReadyEvent)
        m_dataReadyEvent->Set(); //fire the data ready event to wake up our thread (if we have one)
#else
    // http://msdn.microsoft.com/en-us/library/windows/desktop/ms684954(v=VS.85).aspx
    DWORD d = QueueUserAPC((PAPCFUNC)call, m_thread/*->GetHandle()*/, (ULONG_PTR)params);
    //NASSERT(d, "Queueing a call failed!");
#endif
}
/************************************************************************************/
/**
 ** 
 **/
void TaskQueue::pushTask(TaskBase * task)
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_CYAN);
#ifdef DBGTHREAD
    LOGDBG("pushTaskFunc %s %p\n", task->getDbgString(), task );
    //if(((unsigned long)params) > 0x11000000)
    //{
    //    LOGI("Found\n");
    //}
#endif
    
#if !defined WIN32 || defined NOWIN32BUILTIN
    task->m_queueCountRef = &m_taskCount;
#ifdef WIN32
    InterlockedIncrement(&m_taskCount);
#else
    __sync_fetch_and_add(&m_taskCount, 1);
#endif
    pushTaskFunc(taskThreadFunc, task);
#else
    task->m_queueCountRef = &m_taskCount;
    InterlockedIncrement(&m_taskCount);
    pushTaskFunc(taskThreadFunc, task);
#endif
}
/************************************************************************************/
/**
 ** timeout != 0 means that we are getting stuck for a timeout amount (or infinitely)
 ** if no task are there. Can be used when we want to really wait for some things to be done
 **/
bool TaskQueue::pollTask(int timeout)
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_CYAN);
#if !defined WIN32 || defined NOWIN32BUILTIN
    if(m_dataReadyEvent && (timeout != 0))
        m_dataReadyEvent->WaitOnEvent(timeout);
    m_taskQueueLock.Enter();
    
    std::pair<ThreadFunc, void*> newFunc;
    if (m_taskQueue.ReadData(newFunc))
    {
        m_taskQueueLock.Exit(); //we need to exit here so that we don't keep the queue locked while the task runs
        newFunc.first(newFunc.second); //run the task and return true
        return true;
    }
    
    m_taskQueueLock.Exit();
    return false; //no tasks
#else
    //NASSERT(GetCurrentThreadId() == m_threadID, "You are calling from the wrong thread! Don't do this!");
    // http://msdn.microsoft.com/en-us/library/windows/desktop/ms687036(v=VS.85).aspx
    DWORD ret = WaitForSingleObjectEx(m_thread/*->GetHandle()*/, timeout, TRUE); //timeout = 0 : it returns immediately if there are no work items
    return ret == WAIT_IO_COMPLETION;
#endif
}

/************************************************************************************/
/**
 ** 
 **/
void TaskQueue::taskThreadFunc(void* params)
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_CYAN);
    TaskBase* task = (TaskBase*)params;
    //
    // Call the heart of the Task to perform
    //
#ifdef DBGTHREAD
    LOGDBG("invoking task %s %p\n", task->getDbgString(), task);
    //if(((unsigned long)params) > 0x11000000)
    //{
    //    LOGI("invoking task %p\n", params);
    //}
#endif
    task->Invoke();
    
    if (task->m_queueCountRef)
    {
        //NXPROFILEFUNCCOL("InterlockedDecrement", COLOR_MAGENTA);
#ifdef WIN32
        InterlockedDecrement(task->m_queueCountRef);
#else
        __sync_fetch_and_sub(task->m_queueCountRef, 1);
#endif
    }
    {
        //NXPROFILEFUNCCOL("task->Done", COLOR_BLUE);
        task->Done();
    }
}

/************************************************************************************/
/**
 ** 
 **/
void TaskQueue::FlushTasks(bool waitAlertable /*= false*/)
{
    //TODO : we should spin into this untill the work is done (because we assume we added ourselves to the end of the queue)
}

//#pragma mark - Thread worker
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/

static NThreadLocalVar<TaskQueue*> g_tl_currentTaskQueue;
TaskQueue* getCurrentTaskQueue()
{
  return g_tl_currentTaskQueue;
}
void setCurrentTaskQueue(TaskQueue * tb)
{
  g_tl_currentTaskQueue = tb;
}
// keep locally the number of the thread
static NThreadLocalVar<int> g_tl_ThreadNumber;
int getThreadNumber()
{
  return g_tl_ThreadNumber;
}
void setThreadNumber(int n)
{
  g_tl_ThreadNumber = n;
}


/**
 ** 
 **/
ThreadWorker::ThreadWorker(const std::string& threadName, bool discardQueuedOnExit, bool waitAleratableOnExit) :
#if !defined WIN32 || defined NOWIN32BUILTIN
  m_invoker(&m_dataReadyEvent),
#endif
  m_discardQueuedOnExit(discardQueuedOnExit), //m_alertableOnExit(waitAleratableOnExit),
  m_threadName(threadName)
{
    static int cnt = 0;
    m_workerID = cnt++;
#ifdef WIN32
    unsigned int temp;
    // http://msdn.microsoft.com/en-us/library/kdzttdcb(v=VS.90).aspx
    m_invoker.m_thread = (NThreadHandle)_beginthreadex(NULL, 0, threadFunc, this, 0, &temp);
    m_invoker.m_threadID = temp;
#else
    pthread_create(&m_invoker.m_thread, NULL, threadFunc, this);
#endif
}

/************************************************************************************/
/**
 ** 
 **/
ThreadWorker::~ThreadWorker()
{
  NXPROFILEFUNCCOL(__FUNCTION__, COLOR_RED2);
  //fire the done event
  m_doneEvent.Set();
#if !defined WIN32 || defined NOWIN32BUILTIN
  //wake up the thread if it's sleeping (so it can end)
  m_dataReadyEvent.Set();
#endif
#if !defined WIN32
  void* exitValue;
  //if (m_alertableOnExit)
  //{
  //  NASSERT(0, "The alertable on exit flag has no effect on non-windows. The calling thread is totally blocked until this thread exits");
  //  pthread_join(m_invoker.m_thread, &exitValue); //wait for the thread to terminate
  //}
  //else
  {
    pthread_join(m_invoker.m_thread, &exitValue); //wait for the thread to terminate
  }
#else
  //if (m_alertableOnExit)
  //{
  //  NPROFILE_FUNCTION_TYPED(NPMT_WAITING);
  //  while(WaitForSingleObjectEx(m_invoker.m_thread, INFINITE, TRUE) != WAIT_OBJECT_0)
  //  {
  //    //do nothing...we will periodically be woken if an APC fires or something so sleep again if it's not the event
  //  }
  //}
  //else
  {
    WaitForSingleObjectEx(m_invoker.m_thread, INFINITE, FALSE); //wait for the thread to terminate
  }
#endif
}

#ifdef WIN32
/**
 **
 **/
void NYieldCPU(uint aproxMilliseconds)
{
  Sleep(aproxMilliseconds);
}
#else
void NYieldCPU(uint aproxMilliseconds)
{
  usleep(aproxMilliseconds * 1000);
}
#endif

#ifdef WIN32
//From http://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx
const DWORD MS_VC_EXCEPTION=0x406D1388;

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
  DWORD dwType; // Must be 0x1000.
  LPCSTR szName; // Pointer to name (in user addr space).
  DWORD dwThreadID; // Thread ID (-1=caller thread).
  DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

/************************************************************************************/
/**
 **
 **/
static void MSVCSetThreadName( DWORD dwThreadID, const char* threadName)
{
  if (!IsDebuggerPresent())
    return;

  THREADNAME_INFO info;
  info.dwType = 0x1000;
  info.szName = threadName;
  info.dwThreadID = dwThreadID;
  info.dwFlags = 0;

  __try
  {
    RaiseException( MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info );
  }
  __except(EXCEPTION_EXECUTE_HANDLER)
  {
  }
}

#endif
/************************************************************************************/
/**
 **
 **/
void ThreadWorker::SetThreadName(const std::string& n)
{
  CCriticalSectionHolder h(m_threadNameSec);
  m_threadName = n;
#ifdef WIN32
  MSVCSetThreadName(m_invoker.m_threadID, m_threadName.c_str());
#endif
  //ONCE(NASSERT(0, "SetThreadName doesn't work on non-Win32"));
}


const std::string& ThreadWorker::GetThreadName()
{
  CCriticalSectionHolder h(m_threadNameSec);
  return m_threadName;
}

#ifdef WIN32
/************************************************************************************/
/**
 ** \brief Task for thread priority setup
 ** This Task is used with the Queuing system : we push this task to change
 ** the tasks that will be pushed after this one
 **/
class SetThreadPriortyTask : public TaskBase
{
  int m_priority;
public:
  SetThreadPriortyTask(int p) : m_priority(p) { }
  virtual void Invoke()
  {
    SetThreadPriority(GetCurrentThread(), m_priority);
  }
#ifdef DBGTHREAD
    const char *getDbgString() { return __PRETTY_FUNCTION__; };
#endif
};
#else
/************************************************************************************/
/**
 ** \brief Task for thread priority setup
 ** This Task is used with the Queuing system : we push this task to change
 ** the tasks that will be pushed after this one
 **/
class SetThreadPriortyTask : public TaskBase
{
  int m_priority;
public:
  /// \brief contructor is also used to store arguments for SetThreadPriortyTask::Invoke
  SetThreadPriortyTask(int p) : m_priority(p) { }
  /// \brief the typical entry point for task execution
  virtual void Invoke()
  {
    NThreadHandle me;
    me = pthread_self();

    // get the current settings
    int policy;
    struct sched_param param;
    pthread_getschedparam(me, &policy, &param);

    // clamp our desired priority to the allowable range
    int min_prio = sched_get_priority_min(policy);
    int max_prio = sched_get_priority_max(policy);
    int mx = min_prio > m_priority ? min_prio : m_priority;
    int prio = max_prio < mx ? max_prio : mx;

    param.sched_priority = prio;
    pthread_setschedparam(me, policy, &param);
  }
#ifdef DBGTHREAD
    const char *getDbgString() { return __PRETTY_FUNCTION__; };
#endif
};
#endif
/**
 ** Pushes SetThreadPriortyTask Task in the list to set the bgnd mode
 **/
void ThreadWorker::SetBackgroundMode(bool b)
{
  // THREAD_MODE_BACKGROUND_BEGIN and THREAD_MODE_BACKGROUND_END are special
  // Windows priority flags. When you use BEGIN, it remembers what priority
  // you were at before switching to background, so when you use END it can
  // restore the thread's original priority.
  //#warning I dont think the background begin/end mode exists for pthreads. Using hard-coded priority values instead.
#ifdef WIN32
  m_invoker.pushTask(new SetThreadPriortyTask(b ? THREAD_MODE_BACKGROUND_BEGIN : THREAD_MODE_BACKGROUND_END));
#else
  m_invoker.pushTask(new SetThreadPriortyTask(b ? 0 : 99));
#endif
}

#if !defined WIN32 || defined NOWIN32BUILTIN
/************************************************************************************/
/**
 ** 
 **/
#ifdef WIN32
unsigned
#else
void*
#endif
NTHREAD_PROC_CALL_CONV ThreadWorker::threadFunc(void* p)
{
  NXPROFILEFUNCCOL(__FUNCTION__, COLOR_RED2);
  ThreadWorker* t = (ThreadWorker*)p;

  g_tl_ThreadNumber = t->GetWorkerID();
  // Set the m_invoker as the current TaskQueue
  g_tl_currentTaskQueue = &t->GetTaskQueue();
  {
    CCriticalSectionHolder h(t->m_threadNameSec);
    if (t->m_threadName.size())
    {
      // set the name of the current thread (like, the actual thread) with the name from the ThreadWorker
      LOGDBG("SetThreadName doesn't work on non-Win32");
    }
  }

  //check if we should exit : sent when the WorkerThread gets destroyed
  while(!t->m_doneEvent.WaitOnEvent(0))
  {
    //wait for a new task (the cross thread invoker will signal this event
    // see TaskQueue::Invoke(...) 
    // m_dataReadyEvent shared with TaskQueue
    t->m_dataReadyEvent.WaitOnEvent();
    // Execute all the Jobs that are inside the m_invoker
    while(t->m_invoker.pollTask())
    {
      //pump out the rest of the tasks that are there now that we are ready
      //we could just omit the m_dataReadyEvent event but we don't want to waste cpu polling when our invoker's queue is empty
    }
  }
  // the done event fired so return
  // When destroyed, maybe we want the queued Jobs to be done :
  if (!t->m_discardQueuedOnExit)
  {
    while(t->m_invoker.pollTask())
    {
      //pump out the rest of the tasks unless we are discarding them
    }
  }
  // release Local memory of this thread
  //NThreadLocalNonPODBase::DeleteAllTLSDataInThisThread();
  return NULL;
}
#else
unsigned NTHREAD_PROC_CALL_CONV ThreadWorker::threadFunc(void* p)
{
  ThreadWorker* t = (ThreadWorker*)p;

  g_tl_ThreadNumber = t->GetWorkerID();
    // Set the m_invoker as the current TaskQueue
  g_tl_currentTaskQueue = &t->GetTaskQueue();

  {
    CCriticalSectionHolder h(t->m_threadNameSec);
    if (t->m_threadName.size())
    {
      MSVCSetThreadName(-1, t->m_threadName.c_str()); //-1 == current thread
    }
  }


  // Originally :
  //t->m_doneEvent.WaitAlertable(); //we are alertable here so the cross thread invoker will do it's thing
  // But equivalent to spin in APC events :
  NX_RANGEPUSHCOL("ThreadWorker::threadFunc WaitForSingleObjectEx", COLOR_RED2);
  while(WaitForSingleObjectEx(t->m_doneEvent.GetHandle(), INFINITE, TRUE) != WAIT_OBJECT_0)
  {
    NX_RANGEPOP();
    NX_RANGEPUSHCOL("ThreadWorker::threadFunc WaitForSingleObjectEx", COLOR_RED2);
    //do nothing...we will periodically be woken if an APC fires or something so sleep again if it's not the event
  }
  NX_RANGEPOP();
  //the done event fired so return

  if (!t->m_discardQueuedOnExit)
  {
    NX_RANGEPUSHCOL("ThreadWorker::threadFunc pollTask", COLOR_RED2);
    while(t->m_invoker.pollTask())
    {
      //pump out the rest of the tasks unless we are discarding them
    }
    NX_RANGEPOP();
  }

  //NThreadLocalNonPODBase::DeleteAllTLSDataInThisThread();
  return 0;
}
#endif

/************************************************************************************/
/**
 ** 
 **/
void TaskSyncCall::Done()
{
  //trigger the done event and DON'T delete us
  m_doneEvent.Set();
}

/************************************************************************************/
/**
 ** 
 **/
TaskSyncCall::TaskSyncCall() : m_doneEvent()
{

}

/************************************************************************************/
/**
 ** 
 **/
void TaskSyncCall::Call(TaskQueue* destQueue, bool waitAltertable)
{
  NXPROFILEFUNCCOL(__FUNCTION__, COLOR_YELLOW);
    LOGDBG("TaskSyncCall::Call() for %s\n", this->getDbgString());
  if(destQueue)
  {
    // Note that it may fail if the targeted destQueue is the one from the current thread
    // I think that the "altertable" mode would solve this issue... TODO and TO Test
    TaskQueue* curTB = getCurrentTaskQueue();
    if(curTB == destQueue)
    {
        Invoke();
        return;
    }
    destQueue->pushTask(this);
    if(waitAltertable)
        m_doneEvent.WaitOnEventAltertable();
    else
        m_doneEvent.WaitOnEvent();
    LOGDBG("TaskSyncCall::Call() DONE\n");
  }
}
//#pragma mark - QueuedWorkProcessorTask

/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/**
 ** 
 **/
ThreadWorkerPool::QueuedWorkProcessorTask::QueuedWorkProcessorTask(bool discardOnExit) : 
    m_discardOnExit(discardOnExit), 
    m_dataReadySem(0), 
    m_doneEvent(false, true), //manual reset since all threads read it
    m_dataProcessedSem(0),
    m_taskQueue(64)
{
    
}

/************************************************************************************/
/**
 ** since each thread has it's own queue, we run a uber-task that loops and runs other tasks from
 ** the main queue
 **  This function is called by WorkerThreads for NWTPS_SHARED_QUEUE mode
 **   m_queueTask = new QueuedWorkProcessorTask(discardQueuedOnExit);
 **   for (uint i = 0; i < m_threadCount; i++)
 **   {
 **     m_threads[i].GetTaskQueue().InvokeTask(m_queueTask);
 **   }
 ** }
 ** 
 **/
void ThreadWorkerPool::QueuedWorkProcessorTask::Invoke()
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_YELLOW2);
    while(!m_doneEvent.WaitOnEvent(0))
    {
        m_dataReadySem.AcquireSemaphore(); //wait for some data to be pushed in the TaskQueue
        
        //our thread woke up because there is something to eat in the TaskQueue
        TaskBase* childTask = NULL;
        {
            CCriticalSectionHolder h(m_taskQueueLock);
            m_taskQueue.ReadData(childTask); //will read something if it's there or do nothing
        }
        
        if (childTask)
        {
            childTask->Invoke();
            childTask->Done();
        }
        
        m_dataProcessedSem.ReleaseSemaphore();
    }
    
    //get got the quit event
    if (!m_discardOnExit)
    {
        //but we need to finish the other tasks
        while (true)
        {
            TaskBase* childTask = NULL;
            {
                CCriticalSectionHolder h(m_taskQueueLock);
                if (!m_taskQueue.ReadData(childTask))
                    break; //we are done if the buffer is empty
            }
            childTask->Invoke();
            childTask->Done();
            
            m_dataProcessedSem.ReleaseSemaphore();
        }
    }
}

/************************************************************************************/
/**
 ** 
 **/
void ThreadWorkerPool::QueuedWorkProcessorTask::Done()
{
    //do nothing
}


//#pragma mark - ThreadWorkerPool
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/**
 ** 
 **/
ThreadWorkerPool::ThreadWorkerPool(uint numThreads, bool discardQueuedOnExit, bool waitAleratableOnExit, NWORKER_THREADPOOL_SCHEDULE sched, const std::string& threadName) : 
m_threadCount(numThreads), m_schedule(sched), m_invokedTaskCount(0), m_queueTask(NULL)
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_GREEN);
    m_threads = new ThreadWorker[m_threadCount];
    //why you can't pass a parameter to things being constructed in an array I will never know...
    for (uint i = 0; i < m_threadCount; i++)
    {
        char ss[100];
        m_threads[i].SetDiscardQueuedOnExit(discardQueuedOnExit);
        //m_threads[i].SetWaitAleratbleOnExit(waitAleratableOnExit);
        if (threadName.size())
        {
#ifdef WIN32
            sprintf_s(ss, 99, "%s[%d]", threadName.c_str(), i);
#else
            sprintf(ss, "%s[%d]", threadName.c_str(), i);
#endif
            m_threads[i].SetThreadName(ss);
        }
    }
    
    if (m_schedule == NWTPS_SHARED_QUEUE)
    {
        m_queueTask = new QueuedWorkProcessorTask(discardQueuedOnExit);
        for (uint i = 0; i < m_threadCount; i++)
        {
            //put the queue manager task on each thread
            m_threads[i].GetTaskQueue().pushTask(m_queueTask);
        }
    }
}

/************************************************************************************/
/**
 ** 
 **/
ThreadWorkerPool::~ThreadWorkerPool()
{
    Terminate();
}
/************************************************************************************/
/**
 ** 
 **/
void ThreadWorkerPool::Terminate()
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_GREEN);
    if (m_queueTask)
    {
        m_queueTask->m_doneEvent.Set();
        //make sure all the threads wake up (the event autoresets after one thread gets it)
        for (uint i = 0; i < m_threadCount; i++)
        {
            m_queueTask->m_dataReadySem.ReleaseSemaphore();
        }
    }
    if(m_threads)
        delete [] m_threads;
    m_threads = NULL;
    m_threadCount = 0;
    //we are sure nobody is in the task now
    delete m_queueTask;
    m_queueTask = NULL;
}

/************************************************************************************/
/**
 ** this is not 100% accurate since some tasks could finish while we are doing
 ** this but it's close enough since it's not super-critical which task goes where.
 **/
void ThreadWorkerPool::pushTask(TaskBase* task)
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_GREEN);
    m_invokedTaskCount++;
    if (m_schedule == NWTPS_LEAST_QUEUED_TASKS)
    {
        int minTaskCount = m_threads[0].GetTaskQueue().GetQueuedTaskCount();
        ThreadWorker* minThread = &m_threads[0];
        
        for (uint i = 1; i < m_threadCount; i++)
        {
            int curCount = m_threads[i].GetTaskQueue().GetQueuedTaskCount();
            if (curCount < minTaskCount)
            {
                minTaskCount = curCount;
                minThread = &m_threads[i];
            }
        }
        
        minThread->GetTaskQueue().pushTask(task);
    }
    else if (m_schedule == NWTPS_ROUND_ROBIN)
    {
        m_threads[m_invokedTaskCount % m_threadCount].GetTaskQueue().pushTask(task);
    }
    else if (m_schedule == NWTPS_SHARED_QUEUE)
    {
        CCriticalSectionHolder h(m_queueTask->m_taskQueueLock);
        m_queueTask->m_taskQueue.WriteData(task);
        //wake up somebody
        m_queueTask->m_dataReadySem.ReleaseSemaphore();
    }
}
/************************************************************************************/
/**
 ** 
 **/
ThreadWorker * ThreadWorkerPool::getThreadWorker(int n)
{
    if((n < 0)||(n >= (int)m_threadCount))
        return NULL;
    return m_threads+n;
}
/************************************************************************************/
/**
 ** 
 **/
void ThreadWorkerPool::SetBackgroundMode(bool b)
{
    for (uint i = 0; i < m_threadCount; i++)
    {
        m_threads[i].SetBackgroundMode(b);
    }
}

/************************************************************************************/
/**
 ** 
 **/
void ThreadWorkerPool::FlushTasks(bool waitAlertable)
{
    if (m_schedule == NWTPS_SHARED_QUEUE)
    {
        //wait until the queue is empty
        while(true)
        {
            {
                CCriticalSectionHolder h(m_queueTask->m_taskQueueLock);
                if (m_queueTask->m_taskQueue.GetStoredSize() == 0)
                    break; //ok it's empty
            }
          //TODO
          //if (waitAlertable)
          //  m_queueTask->m_dataProcessedSem.AcquireSemaphoreAlertable();
          //else
            m_queueTask->m_dataProcessedSem.AcquireSemaphore();
        }
    }
    else
    {
        //normal case
        for (uint i = 0; i < m_threadCount; i++)
        {
            m_threads[i].GetTaskQueue().FlushTasks(waitAlertable);
        }
    }
    
}


#define NV_FAKE_WAIT_ALERTABLE_SLICES_MS 5
/************************************************************************************/
/**
 ** 
 **/
#if !defined WIN32 || defined NOWIN32BUILTIN
bool CEventAlertable::WaitOnEventAltertable()
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_RED);
    TaskQueue * tb = getCurrentTaskQueue();
    if (tb)
    {
        while(!WaitOnEvent(NV_FAKE_WAIT_ALERTABLE_SLICES_MS))
        {
          tb->pollTask(); //do one request per slice
        }
    }
    else
    {
        //no invoker so just wait
        WaitOnEvent();
    }
    return true;
}
bool CEventAlertable::WaitOnEventAltertable(uint msTimeOut)
{
  NXPROFILEFUNCCOL(__FUNCTION__, COLOR_RED);
  int remainingTimeout = msTimeOut;
  TaskQueue * tb = getCurrentTaskQueue();
  if (tb)
  {
    while(!WaitOnEvent((NV_FAKE_WAIT_ALERTABLE_SLICES_MS < remainingTimeout) ? NV_FAKE_WAIT_ALERTABLE_SLICES_MS : remainingTimeout))
    {
      tb->pollTask(); //do one request per slice
      remainingTimeout -= NV_FAKE_WAIT_ALERTABLE_SLICES_MS; //reduce our left over timeout
      if (remainingTimeout <= 0)
        return false; //we timed out
    }
    return true; //we got it!
  }
  else
  {
    //no invoker so just wait
    return WaitOnEvent(msTimeOut);
  }
}
#else
bool CEventAlertable::WaitOnEventAltertable()
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_RED);
    while(WaitForSingleObjectEx(m_event, INFINITE, TRUE) != WAIT_OBJECT_0)
    {
    //do nothing...we will periodically be woken if an APC fires or something so sleep again if it's not the event
    }
    return true;
}
bool CEventAlertable::WaitOnEventAltertable(uint msTimeOut)
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_RED);
    return WaitForSingleObjectEx(m_event, msTimeOut, TRUE) == WAIT_OBJECT_0;
}
#endif


