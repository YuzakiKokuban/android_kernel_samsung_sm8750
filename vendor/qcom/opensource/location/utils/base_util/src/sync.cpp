/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

 Sync utilities

 GENERAL DESCRIPTION
 This component provides utilities used for synchronization among threads

 Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 SPDX-License-Identifier: BSD-3-Clause-Clear
 =============================================================================*/
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <new>

#include <base_util/queue.h>
#include <base_util/log.h>
#include <base_util/sync.h>

#define BREAK_IF_ZERO(ERR,X) if(0==(X)) {result = (ERR); break;}
#define BREAK_IF_NON_ZERO(ERR,X) if(0!=(X)) {result = (ERR); break;}
#define BREAK_IF_NON_ZERO_RC(ERR,RC,X) if(0!=(RC=(X))) {result = (ERR); break;}

namespace qc_loc_fw
{

// non-thread safe version, to be used only in threading related code
void log_error_no_lock(const char * const local_log_tag, const char * const format, ...);

static const char * const MUTEX_DEFAULT_TAG = "Mutex";
static const char * const AUTOLOCK_DEFAULT_TAG = "AutoLock";
static const char * const WAITABLE_BASE_DEFAULT_TAG = "WaitableBase";
static const char * const QUEUE_DEFAULT_TAG = "BlockingQueue";
static const char * const THREAD_DEFAULT_TAG = "AutoLock";

class MutexImpl: public Mutex
{
public:
  MutexImpl(const char * const tag, const bool verboseLog);
  virtual ~MutexImpl();
  int lock();
  int unlock();

  pthread_mutex_t * getRawMutex()
  {
    return &m_mutex;
  }
private:

  // this variable is only set once, at creation
  // after that, it can never change, for it is not protected by a mutex
  const char * m_tag;

  // this variable is only set once, at creation
  // after that, it can never change, for it is not protected by a mutex
  const bool m_flagEnableVerboseLog;

  pthread_mutex_t m_mutex;
};

MutexImpl::MutexImpl(const char * const tag, const bool verboseLog) :
        m_tag(tag),
        m_flagEnableVerboseLog(verboseLog)
{
  int result = 1;
  pthread_mutexattr_t mutex_attr;
  bool destroy_attr = false;

  do
  {
    if(0 == m_tag)
    {
      m_tag = MUTEX_DEFAULT_TAG;
    }
    BREAK_IF_NON_ZERO(2, pthread_mutexattr_init(&mutex_attr));
    destroy_attr = true;
    BREAK_IF_NON_ZERO(3, pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK));
    BREAK_IF_NON_ZERO(4, pthread_mutex_init(&m_mutex, &mutex_attr));
    result = 0;
  } while (0);

  if (destroy_attr == true)
  {
    if (0 != pthread_mutexattr_destroy(&mutex_attr))
    {
      result = 5;
    }
  }

  if(0 != result)
  {
    log_error_no_lock(m_tag, "MutexImpl: result: %d, error: %d, [%s]", result, errno, strerror(errno));
  }
}

MutexImpl::~MutexImpl()
{
  int rc = pthread_mutex_destroy(&m_mutex);
  if(0 != rc)
  {
    log_error_no_lock(m_tag, "~MutexImpl: pthread_mutex_destroy rc: %d, [%s]", rc, strerror(rc));
  }
}

int MutexImpl::lock()
{
  if(m_flagEnableVerboseLog)
  {
    log_verbose(m_tag, "lock: about to lock");
  }

  int rc = pthread_mutex_lock(&m_mutex);
  if(0 != rc)
  {
    log_error_no_lock(m_tag, "lock: pthread_mutex_lock return code: %d, [%s]", rc, strerror(rc));
    return 1;
  }

  if(m_flagEnableVerboseLog)
  {
    log_verbose(m_tag, "lock: just acquired");
  }
  return 0;
}

int MutexImpl::unlock()
{
  if(0 != pthread_mutex_unlock(&m_mutex))
  {
    log_error_no_lock(m_tag, "unlock: pthread_mutex_unlock error: %d, [%s]", errno, strerror(errno));
    return 1;
  }

  if(m_flagEnableVerboseLog)
  {
    log_verbose(m_tag, "lock: just released");
  }
  return 0;
}

Mutex::~Mutex()
{
  // dummy destructor
}

Mutex * Mutex::createInstance(const char * const tag, const bool verboseLog)
{
  return new (std::nothrow) MutexImpl(tag, verboseLog);
}

AutoLock::AutoLock(Mutex * const pMutex, const char * log_tag) :
        m_pMutex(pMutex),
        m_tag(log_tag),
        m_locked(false)
{
  int result = 1;
  do
  {
    if(0 == m_tag)
    {
      m_tag = AUTOLOCK_DEFAULT_TAG;
    }

    BREAK_IF_ZERO(2, m_pMutex);

    BREAK_IF_NON_ZERO(3, m_pMutex->lock());

    m_locked = true;
    result = 0;
  } while (0);

  if(0 != result)
  {
    log_error_no_lock(m_tag, "AutoLock: lock error: %d", result);
  }
}

AutoLock::~AutoLock()
{
  int result = 1;
  do
  {
    BREAK_IF_ZERO(2, m_pMutex);

    if (m_locked)
    {
      // Do not unlock the lock if we haven't acquired it.
      BREAK_IF_NON_ZERO(3, m_pMutex->unlock());
    }

    m_locked = false;
    result = 0;
  } while (0);

  if(0 != result)
  {
    log_error_no_lock(m_tag, "AutoLock: unlock error: %d", result);
  }
}

int AutoLock::ZeroIfLocked()
{
  if(m_locked)
  {
    return 0;
  }
  return 1;
}

class WaitableBase
{
public:
  virtual int lock_and_wait(const TimeDiff & timeout_diff);
  virtual int unlock();

  WaitableBase(const char * const name, const bool verboseLog);
  virtual ~WaitableBase() = 0;

  // the call flow for internal state modification should be
  // 'lock' -> modify state -> 'signal_one_and_then_unlock'
  virtual int lock();
  virtual int signal_one_and_then_unlock();
  virtual int signal_all_and_then_unlock();

  // called by lock_and_wait to make sure we need to wait after
  // locked but before the wait
  virtual int ZeroIfShouldWaitAgain_locked() = 0;
private:
  pthread_cond_t m_cond;
  MutexImpl m_mutex;
  const char * m_tag;
  bool m_verboseLog;
};

WaitableBase::WaitableBase(const char * const name, const bool verboseLog) :
        m_mutex(name, verboseLog),
        m_tag(name),
        m_verboseLog(verboseLog)
{
  int result = 1;
  pthread_condattr_t cond_attr;
  bool destroy_cond = false;

  do
  {
    if(0 == m_tag)
    {
      m_tag = WAITABLE_BASE_DEFAULT_TAG;
    }

    errno = 0;
    BREAK_IF_NON_ZERO(2, pthread_condattr_init(&cond_attr));
    destroy_cond = true;

    BREAK_IF_NON_ZERO(3, pthread_condattr_setclock(&cond_attr, CLOCK_REALTIME));

    BREAK_IF_NON_ZERO(4, pthread_cond_init(&m_cond, &cond_attr));

    result = 0;
  } while(0);

  if (destroy_cond == true)
  {
    if (0 != pthread_condattr_destroy(&cond_attr))
    {
      result = 5;
    }
  }

  if(0 != result)
  {
    log_error_no_lock(m_tag, "WaitableBase: pthread conditional variable error: %d, errno=%d, [%s]",
        result, errno, strerror(errno));
  }
}

WaitableBase::~WaitableBase()
{
  if(0 != pthread_cond_destroy(&m_cond))
  {
    log_error_no_lock(m_tag, "~WaitableBase: pthread_cond_destroy error: %d, [%s]", errno, strerror(errno));
  }
}

int WaitableBase::lock()
{
  int result = 1;
  do
  {
    if(m_verboseLog)
    {
      log_verbose(m_tag, "about to lock for state modification");
    }

    // if we're locked, we should not lock again
    BREAK_IF_NON_ZERO(3, m_mutex.lock());

    if(m_verboseLog)
    {
      log_verbose(m_tag, "just acquired lock for state modification");
    }

    result = 0;
  } while (0);

  if(0 != result)
  {
    log_error_no_lock(m_tag, "lock : result %d", result);
  }
  return result;
}

int WaitableBase::unlock()
{
  int result = 1;
  do
  {
    // if we're not locked, we should not unlock again

    if(m_verboseLog)
    {
      log_verbose(m_tag, "about to unlock");
    }

    BREAK_IF_NON_ZERO(3, m_mutex.unlock());

    if(m_verboseLog)
    {
      log_verbose(m_tag, "unlocked");
    }

    result = 0;
  } while (0);

  if(0 != result)
  {
    log_error_no_lock(m_tag, "unlock : result %d", result);
  }
  return result;
}

int WaitableBase::signal_one_and_then_unlock()
{
  int result = 1;
  do
  {
    // we should be already locked

    if(m_verboseLog)
    {
      log_verbose(m_tag, "about to signal");
    }

    result = 100;
    if(0 != pthread_cond_signal(&m_cond))
    {
      result = 101;
    }

    BREAK_IF_NON_ZERO(4, unlock());

    if(100 != result)
    {
      break;
    }

    result = 0;
  } while (0);

  if(0 != result)
  {
    log_error_no_lock(m_tag, "WaitableBase: signal_one_and_then_unlock result: %d, errno %d, [%s]", result, errno,
        strerror(errno));
  }
  return result;
}

int WaitableBase::signal_all_and_then_unlock()
{
  int result = 1;
  do
  {
    // we should be already locked

    if(m_verboseLog)
    {
      log_verbose(m_tag, "about to signal");
    }

    result = 100;
    if(0 != pthread_cond_broadcast(&m_cond))
    {
      result = 101;
    }

    BREAK_IF_NON_ZERO(4, unlock());

    if(100 != result)
    {
      break;
    }

    result = 0;
  } while (0);

  if(0 != result)
  {
    log_error_no_lock(m_tag, "WaitableBase: signal_all_and_then_unlock result: %d, errno: %d, [%s]", result, errno,
        strerror(errno));
  }
  return result;
}

int WaitableBase::lock_and_wait(const TimeDiff & timeout_diff)
{
  int result = 1;
  do
  {
    // we should not be already locked

    if(m_verboseLog)
    {
      log_verbose(m_tag, "about to lock for wait");
    }

    BREAK_IF_NON_ZERO(2, m_mutex.lock());

    bool flagTimeout = false;
    result = 10;
    // if this function returns non-zero, we return 0 immediately without waiting
    while ((!flagTimeout) && (0 == ZeroIfShouldWaitAgain_locked()))
    {

      if(m_verboseLog)
      {
        log_verbose(m_tag, "about to wait");
      }

      int rc = 0;
      if(!timeout_diff.is_valid())
      {
        // timeout is invalid, which means we are going to wait forever
        rc = pthread_cond_wait(&m_cond, m_mutex.getRawMutex());
      }
      else
      {
        // same pthread_cond_timedwait, but now time should be absolute realtime time
        const Timestamp now_realtime(CLOCK_REALTIME);
        Timestamp timeout_realtime(now_realtime + timeout_diff);
        if (!timeout_realtime.is_valid())
        {
          // this shall not happen
          result = 12;
          break;
        }
        if(timeout_diff.get_total_sec() > 0)
        {
          rc = pthread_cond_timedwait(&m_cond, m_mutex.getRawMutex(),
               timeout_realtime.getTimestampPtr());
        }
        else
        {
          // removing the timeout parameter - essentially making value of zero
          rc = pthread_cond_timedwait(&m_cond, m_mutex.getRawMutex(),
               now_realtime.getTimestampPtr());
        }
      }

      if(m_verboseLog)
      {
        log_verbose(m_tag, "just waken up from wait");
      }

      if(0 == rc)
      {
        // good
      }
      else if(ETIMEDOUT == rc)
      {
        // timeout_diff
        flagTimeout = true;
      }
      else
      {
        // something is wrong
        log_error_no_lock(m_tag, "lock_and_wait: pthread_cond_wait error: %d, [%s]", errno, strerror(errno));
        result = 15;
      }

      if(10 != result)
      {
        break;
      }
    }

    if(10 != result)
    {
      break;
    }

    result = 0;
  } while (0);

  if(0 != result)
  {
    log_error_no_lock(m_tag, "WaitableBase: lock_and_wait error: %d", result);
  }
  return result;
}

BlockingQueue::~BlockingQueue()
{
  // dummy destructor
}

class BlockingQueueImpl: public BlockingQueue, protected WaitableBase
{
public:
  BlockingQueueImpl(const char * tag, const bool verboseLog);
  ~BlockingQueueImpl();

  virtual int push(void * const ptr);
  virtual int close();
  virtual int pop(void ** const pptr, const timespec * const timeout_abs_realtime, bool * const p_is_queue_closed);
  virtual int pop(void ** const pptr, const TimeDiff & timeout, bool * const p_is_queue_closed);

  virtual int ZeroIfShouldWaitAgain_locked();

  Queue<void *> m_queue;
  const char * m_tag;
  bool m_isClosed;
  bool m_verboseLog;
};

BlockingQueueImpl::BlockingQueueImpl(const char * tag, const bool verboseLog) :
        WaitableBase("BlockingQueue", verboseLog),
        m_tag(tag),
        m_isClosed(false),
        m_verboseLog(verboseLog)
{
  if(0 == m_tag)
  {
    m_tag = QUEUE_DEFAULT_TAG;
  }
}

BlockingQueueImpl::~BlockingQueueImpl()
{
  if(m_queue.getSize() > 0)
  {
    log_warning(m_tag, "~BlockingQueueImpl: memory leak");
  }
}

int BlockingQueueImpl::ZeroIfShouldWaitAgain_locked()
{
  if(m_isClosed)
  {
    return -1;
  }
  else
  {
    return m_queue.getSize();
  }
}

int BlockingQueueImpl::push(void * const ptr)
{
  int result = 1;
  do
  {
    BREAK_IF_NON_ZERO(2, lock());

    if(m_isClosed)
    {
      result = 3;
    }
    else
    {
      result = 100;
      if(0 != m_queue.push(ptr))
      {
        // something is wrong, but we cannot leave just now
        result = 101;
      }
    }

    BREAK_IF_NON_ZERO(4, signal_one_and_then_unlock());

    if(100 != result)
    {
      break;
    }

    result = 0;
  } while (0);

  if(0 != result)
  {
    log_error_no_lock(m_tag, "push: error: %d", result);
  }
  return result;
}

int BlockingQueueImpl::close()
{
  int result = 1;
  do
  {
    BREAK_IF_NON_ZERO(2, lock());
    m_isClosed = true;
    // note we have to release ALL threads blocking on us
    BREAK_IF_NON_ZERO(3, signal_all_and_then_unlock());
    result = 0;
  } while (0);

  if(0 != result)
  {
    log_error_no_lock(m_tag, "close: error: %d", result);
  }
  return result;
}

int BlockingQueueImpl::pop(void ** const pptr, const timespec * const timeout_abs_realtime, bool * const p_is_queue_closed)
{
  int result = 1;
  do
  {
    // warn the API user about deprecated feature
    log_warning(m_tag, "pop 0: deprecated.");

    BREAK_IF_ZERO(2, pptr);
    *pptr = 0;

    const Timestamp now_wallclock(CLOCK_REALTIME);
    TimeDiff timeout_diff(false);
    if(0 != timeout_abs_realtime)
    {
      const Timestamp timeout_wallclock(CLOCK_REALTIME, *timeout_abs_realtime);
      if(timeout_wallclock >= now_wallclock)
      {
        timeout_diff = timeout_wallclock - now_wallclock;
      }
      else
      {
        // this is valid 0, which means we are not going to wait at all
        timeout_diff.reset(true);
      }
    }
    else
    {
      // timeout_diff is reset to invalid, which means we are going to wait forever
      timeout_diff.reset(false);
    }

    BREAK_IF_NON_ZERO(10, pop(pptr, timeout_diff, p_is_queue_closed));

    result = 0;
  } while (0);

  if(0 != result)
  {
    log_error_no_lock(m_tag, "pop 0: error: %d", result);
  }
  return result;
}

int BlockingQueueImpl::pop(void ** const pptr, const TimeDiff & timeout, bool * const p_is_queue_closed)
{
  int result = 1;
  do
  {
    BREAK_IF_ZERO(2, pptr);
    *pptr = 0;

    if(0 != p_is_queue_closed)
    {
      *p_is_queue_closed = false;
    }

    BREAK_IF_NON_ZERO(4, lock_and_wait(timeout));

    // note we're now locked, so it's okay to copy the members
    if(0 != p_is_queue_closed)
    {
      *p_is_queue_closed = m_isClosed;
    }
    else
    {
      if(m_isClosed)
      {
        // warn to API user that he/she should provide p_is_queue_closed for us
        // to notify this condition
        log_warning(m_tag, "pop: queue is already closed");
      }
      else
      {
        // ignore
      }
    }

    if(m_queue.getSize() > 0)
    {
      if(0 != m_queue.pop(pptr))
      {
        // something is wrong, but we cannot leave just now
        result = 5;
      }
    }
    else
    {
      // timeout or queue closed
    }

    BREAK_IF_NON_ZERO(5, unlock());

    result = 0;
  } while (0);

  if(0 != result)
  {
    log_error_no_lock(m_tag, "pop: error: %d", result);
  }
  return result;
}

BlockingQueue * BlockingQueue::createInstance(const char * tag, const bool verboseLog)
{
  return new (std::nothrow) BlockingQueueImpl(tag, verboseLog);
}

Runnable::~Runnable()
{
  // dummy destructor
}

Thread::~Thread()
{
  // dummy destructor
}

class ThreadImpl: public Thread
{
public:
  ThreadImpl(const char * tag, Runnable * const pRunnable, const bool delete_runnable_at_destruction);
  virtual ~ThreadImpl();
  virtual int launch();
  virtual int join();

  const char * m_tag;
  Runnable * m_pRunnable;
  pthread_t m_thread;
  bool m_delete_runnable_at_destruction;

  enum ThreadState
  {
    TS_NOT_CREATED_YET, TS_LAUNCHED_JOINABLE, TS_JOINED, TS_DETACHED,
  };
  ThreadState m_state;

  static void * thread_func(void * ptr);
};

ThreadImpl::~ThreadImpl()
{
  switch (m_state)
  {
  case TS_NOT_CREATED_YET:
  case TS_JOINED:
  case TS_DETACHED:
    // we're okay to leave
    if(m_delete_runnable_at_destruction)
    {
      delete m_pRunnable;
    }
    m_pRunnable = 0;
    break;

  default:
    log_error_no_lock(m_tag, "~ThreadImpl: thread is probably still running");
    break;
  }
}

ThreadImpl::ThreadImpl(const char * tag, Runnable * const pRunnable, const bool delete_runnable_at_destruction) :
        m_tag(tag),
        m_pRunnable(pRunnable),
        m_delete_runnable_at_destruction(delete_runnable_at_destruction),
        m_state(TS_NOT_CREATED_YET)
{
  // KlocWorks complains about m_thread not initialized
  memset(&m_thread, 0, sizeof(m_thread));

  int result = 1;
  do
  {
    if(0 == m_tag)
    {
      m_tag = THREAD_DEFAULT_TAG;
    }

    BREAK_IF_ZERO(2, m_pRunnable);

    result = 0;
  } while (0);

  if(0 != result)
  {
    log_error_no_lock(m_tag, "ThreadImpl: result: %d", result);
  }
}

int ThreadImpl::launch()
{
  int result = 1;
  pthread_attr_t thread_attr;
  bool destroy_attr = false;

  do
  {
    if(TS_NOT_CREATED_YET != m_state)
    {
      result = 2;
      break;
    }

    BREAK_IF_NON_ZERO(3, pthread_attr_init(&thread_attr));
    destroy_attr = true;
    // this is actually the default setting
    BREAK_IF_NON_ZERO(4, pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE));
    BREAK_IF_NON_ZERO(5, pthread_create(&m_thread, &thread_attr, thread_func, this));
    BREAK_IF_NON_ZERO(5, pthread_setname_np(m_thread, "loc-mq-thread"));
    m_state = TS_LAUNCHED_JOINABLE;

    result = 0;
  } while (0);

  if (destroy_attr == true)
  {
    if (0 != pthread_attr_destroy(&thread_attr))
    {
      result = 5;
    }
  }

  if(0 != result)
  {
    log_error_no_lock(m_tag, "launch: result: %d, errno: %d, [%s]", result, errno, strerror(errno));
  }
  return result;
}

int ThreadImpl::join()
{
  int result = 1;
  do
  {
    if(TS_LAUNCHED_JOINABLE != m_state)
    {
      result = 2;
      break;
    }

// thread must have been created
// thread must be joinable, not detached, for this to work
    void * ptr = 0;
    BREAK_IF_NON_ZERO(3, pthread_join(m_thread, &ptr));

    m_state = TS_JOINED;

    result = 0;
  } while (0);

  if(0 != result)
  {
    log_error_no_lock(m_tag, "join: result: %d, errno: %d, [%s]", result, errno, strerror(errno));
  }
  return result;
}

void * ThreadImpl::thread_func(void * ptr)
{
  ThreadImpl * obj = reinterpret_cast<ThreadImpl *>(ptr);
  obj->m_pRunnable->run();
  return 0;
}

Thread * Thread::createInstance(const char * tag, Runnable * const pRunnable, const bool delete_runnable_at_destruction)
{
  return new (std::nothrow) ThreadImpl(tag, pRunnable, delete_runnable_at_destruction);
}

} // namespace qc_loc_fw

