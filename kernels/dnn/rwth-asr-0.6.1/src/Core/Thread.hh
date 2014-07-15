// Copyright 2011 RWTH Aachen University. All rights reserved.
//
// Licensed under the RWTH ASR License (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.hltpr.rwth-aachen.de/rwth-asr/rwth-asr-license.html
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef _CORE_THREAD_HH
#define _CORE_THREAD_HH

#include <pthread.h>
#include <sys/time.h>
#include <ctime>

#ifndef PTHREAD_MUTEX_RECURSIVE_NP
// PTHREAD_MUTEX_RECURSIVE_NP is not available on some plattforms
#define PTHREAD_MUTEX_RECURSIVE_NP PTHREAD_MUTEX_RECURSIVE
#endif

namespace Core {

/**
 * Abstract Thread.
 *
 * Thread implementation based on pthreads
 */
class Thread
{
public:
    Thread() : running_(false) {}

    virtual ~Thread() {}

    /**
     * Start the thread
     */
    bool start() {
	int retval = pthread_create(&thread_, 0, Thread::startRun,
				    static_cast<void*>(this));
	if (retval != 0)
	    return false;
	running_ = true;
	return true;
    }

    /**
     * Wait for thread to exit
     */
    void wait() {
	if(this->running_) {
	    pthread_join(thread_, 0);
	}
    }


protected:
    /**
     * Allow that the thread is cancelled by another thread
     */
    void enableCancel() {
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);
    }
    /**
     * Forbid other threads to cancel this thread
     */
    void disableCancel() {
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
    }
    /**
     * The thread can only be chancelled on certain points
     */
    void setCancelDeferred()
    {
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, 0);
    }
    /**
     * The thread can be chanceled at any time
     */
    void setCancelAsync() {
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, 0);
    }

    /**
     * Terminates the thread
     */
    void exitThread() {
	if(this->running_)
	    pthread_exit(0);
	running_ = false;
    }

public:
    /**
     * Override this in a threaded class
     */
    virtual void run() {}
    /**
     * Cleanup, when thread is ended
     */
    virtual void cleanup() {}

private:
    static void* startRun(void *obj) {
	Thread *curObj = static_cast<Thread *>(obj);
	pthread_cleanup_push(Thread::startCleanup, obj);
	curObj->run();
	pthread_cleanup_pop(0);
	return 0;

    }
    static void startCleanup(void *obj) {
	Thread *curObj = static_cast<Thread *>(obj);
	curObj->cleanup();
    }
    bool running_;
    pthread_t thread_;
};


/**
 * A mutex.
 *
 * A mutex is a MUTual EXclusion device, and is useful for protecting
 * shared data structures from concurrent modifications, and implementing
 * critical sections and monitors
 */
class Mutex
{
private:
    // disable copying
    Mutex(const Mutex&);
    Mutex& operator=(const Mutex&);
public:
    Mutex() {
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutex_init(&mutex_, &attr);
	pthread_mutexattr_destroy(&attr);
    }
    explicit Mutex(const pthread_mutex_t &m)
	: mutex_(m) {}

    ~Mutex() {
	pthread_mutex_destroy(&mutex_);
    }

    bool lock() {
	return (pthread_mutex_lock(&mutex_) == 0);
    }

    bool unlock() {
	return (pthread_mutex_unlock(&mutex_) == 0);
    }

    bool release() {
	return unlock();
    }

    bool trylock() {
	return (pthread_mutex_trylock(&mutex_) == 0);
    }

private:
    pthread_mutex_t mutex_;
    friend class Condition;
};

/**
 * Lock mutex in current scope
 */
class MutexLock
{
public:
    explicit MutexLock(Mutex *mutex) : mutex_(mutex) {
	mutex_->lock();
    }
    ~MutexLock() {
	mutex_->unlock();
    }
private:
    Mutex *mutex_;
};


/**
 * A thread-safety smart pointer.
 * See A. Alexandrescu, "volatile - Multithreaded Programmer's Best Friend"
 */
template<class T>
class LockingPointer {
public:
    LockingPointer(volatile T &obj, const volatile Mutex &mutex) :
	obj_(const_cast<T*>(&obj)), mutex_(const_cast<Mutex*>(&mutex)) {
	mutex_->lock();
    }
    ~LockingPointer() {
	mutex_->unlock();
    }
    T& operator*() {
	return *obj_;
    }
    T* operator->() {
	return obj_;
    }
    Mutex& getMutex() {
	return *mutex_;
    }
private:
    T *obj_;
    Mutex *mutex_;
    LockingPointer(const LockingPointer&);
    void operator=(const LockingPointer&);
};

/**
 * A read-write lock
 */
class ReadWriteLock
{
private:
    ReadWriteLock(const ReadWriteLock&);
    ReadWriteLock& operator=(const ReadWriteLock&);
public:
    ReadWriteLock() {
	pthread_rwlock_init(&lock_, 0);
    }

    ~ReadWriteLock() {
	pthread_rwlock_destroy(&lock_);
    }

    /**
     * Acquire a read lock
     */
    bool readLock() {
	return (pthread_rwlock_rdlock(&lock_) == 0);
    }
    /**
     * try to acquire a read lock
     */
    bool tryReadLock() {
	return (pthread_rwlock_tryrdlock(&lock_) == 0);
    }
    /**
     * Acquire a write lock
     */
    bool writeLock() {
	return (pthread_rwlock_wrlock(&lock_) == 0);
    }
    /**
     * Try to acquire a write lock
     */
    bool tryWriteLock() {
	return (pthread_rwlock_trywrlock(&lock_) == 0);
    }
    /**
     * Unlock
     */
    bool unlock() {
	return (pthread_rwlock_unlock(&lock_) == 0);
    }
private:
    pthread_rwlock_t lock_;
};


/**
 * A condition variable.
 *
 * A condition variable is a synchronization device that allows
 * threads to suspend execution and relinquish the processors
 * until some predicate on shared data is satisfied.
 */
class Condition
{
public:
    Condition() {
	pthread_cond_init(&cond_, 0);
    }

    ~Condition() {
	pthread_cond_destroy(&cond_);
    }

    /**
     * Restarts one of the threads that are waiting on the condition
     * variable.
     *
     * @param broadcastSignal Restarts all waiting threads
     */
    bool signal(bool broadcastSignal = false) {
	if (broadcastSignal)
	    return broadcast();
	return(pthread_cond_signal(&cond_) == 0);
    }

    /**
     * Restart all waiting threads.
     * @see signal
     */
    bool broadcast() {
	return (pthread_cond_broadcast(&cond_) == 0);
    }

    /**
     * Wait for the condition to be signaled.
     *
     * The thread execution is suspended and does not consume any CPU time
     * until the condition variable is signaled.
     */
    bool wait() {
	mutex_.lock();
	bool r = (pthread_cond_wait(&cond_, &mutex_.mutex_) == 0);
	mutex_.unlock();
	return r;
    }

    bool wait(Mutex &mutex) {
	bool r = (pthread_cond_wait(&cond_, &mutex.mutex_) == 0);
	return r;
    }

    /**
     * Wait for the condition to be signaled with a bounded wait time.
     * @see wait
     */
    bool timedWait(unsigned long microsonds) {
	mutex_.lock();
	timespec timeout = getTimeout(microsonds);
	bool r = (pthread_cond_timedwait(&cond_, &mutex_.mutex_, &timeout) == 0);
	mutex_.unlock();
	return r;
    }

private:
    timespec getTimeout(unsigned long microseconds) {
	timeval now;
	gettimeofday(&now, 0);
	timespec timeout;
	timeout.tv_sec = now.tv_sec + microseconds / 1000000;
	timeout.tv_nsec = now.tv_usec * 1000 + microseconds % 1000000;
	return timeout;
    }

    pthread_cond_t cond_;

    /**
     * A condition variable must always be associated with a mutex,
     * to avoid the race condition where a thread prepares to wait
     * on a condition variable and another thread signals the condition
     * just before the first thread actually waits on it.
     */
    Mutex mutex_;

    Condition(const Condition&);
    void operator=(const Condition&);
};

}

#endif // _CORE_THREAD_HH
