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
#ifndef _CORE_THREAD_POOL_HH
#define _CORE_THREAD_POOL_HH

#include <vector>
#include <deque>
#include <Core/Assertions.hh>
#include <Core/Thread.hh>
#include <Core/Types.hh>

namespace Core {

/**
 * A basic thread pool implementation offering a very simple shared memory
 * Map-Reduce framework.
 *
 * The threads are created once and live as long as the pool lives.
 *
 * The idea is that a list of task gets processed by a number of Mapper
 * objects and the results of the Mappers are afterwards combined with a
 * Reducer object. The mapper should store the intermediate results.
 * Each mapper runs in a separate thread. Only one reducer will be used.
 *
 * See ThreadPool.
 */


template<class M>
class NullReducer {
public:
    void reduce(M*) {}
};

template<class T>
class NullMapper {
public:
    NullMapper* clone() const { return new NullMapper(); }
    void map(const T &t) {}
    void reset() {}
};

template<class Pool>
class ThreadPoolThread : public Thread {
public:
    typedef typename Pool::Mapper Mapper;

    ThreadPoolThread(Pool *pool, Mapper *mapper) :
	pool_(pool), mapper_(mapper) {}
    virtual ~ThreadPoolThread() {}

    void run() {
	while (pool_->executeTask(mapper_));
	pool_->threadTerminated(this);
    }
    Mapper* getMapper() {
	return mapper_;
    }

private:
    Pool *pool_;
    Mapper *mapper_;
};


/**
 * Thread pool implementation used by ThreadPool.
 * Implementation is inspired and partly adapted from
 * thread_pool in the Boost library.
 */
template<class T, class M, class R>
class ThreadPoolImpl
{
public:
    typedef T Task;
    typedef M Mapper;
    typedef R Reducer;
    typedef ThreadPoolImpl<T, M, R> Self;
    typedef ThreadPoolThread<Self> WorkerThread;

    ThreadPoolImpl() :
	active_threads_(0), running_threads_(0), terminate_(false) {}
    ~ThreadPoolImpl() {
	for (typename std::vector<WorkerThread*>::iterator t = threads_.begin(); t != threads_.end(); ++t) {
	    delete (*t)->getMapper();
	    delete *t;
	}
    }

    void init(u32 num_threads, const Mapper &mapper) volatile {
	LockingPointer<Self> self(*this, monitor_);
	verify_eq(self->threads_.size(), 0);
	active_threads_ = 0;
	for (u32 t = 0; t < num_threads; ++t) {
	    self->threads_.push_back(new WorkerThread(const_cast<Self*>(this), mapper.clone()));
	    self->threads_.back()->getMapper()->reset();
	    ++active_threads_;
	    ++running_threads_;
	    self->threads_.back()->start();
	}
    }

    void reset() volatile {
	wait();
	LockingPointer<Self> self(*this, monitor_);
	for (u32 t = 0; t < self->threads_.size(); ++t) {
	    self->threads_[t]->getMapper()->reset();
	}
    }

    void submit(const Task &task) volatile {
	LockingPointer<Self> self(*this, monitor_);
	self->tasks_.push_back(task);
	self->new_task_.signal();
    }

    void wait() const volatile {
	// remove the volatile flag from this
	LockingPointer<const Self> self(*this, monitor_);
	while (self->active_threads_ > 0 || !self->tasks_.empty()) {
	    self->thread_idle_.wait(self.getMutex());
	}
    }

    void shutdown()
    {
	wait();
	terminateThreads();
    }

    /**
     * execute the given task.
     * called by the worker threads.
     */
    bool executeTask(Mapper *mapper) volatile
    {
	Task task;
	bool have_task = false;
	{
	    LockingPointer<Self> self(*this, monitor_);
	    if (self->terminate_)
		return false;

	    while (self->tasks_.empty()) {
		if (self->terminate_) {
		    return false;
		} else {
		    --active_threads_;
		    self->thread_idle_.broadcast();
		    self->new_task_.wait(self.getMutex());
		    ++active_threads_;
		}
	    }
	    task = self->tasks_.front();
	    self->tasks_.pop_front();
	    have_task = true;
	}
	if (have_task) {
	    mapper->map(task);
	}
	return true;
    }

    void threadTerminated(WorkerThread *) volatile
    {
	LockingPointer<Self> self(*this, monitor_);
	--active_threads_;
	--running_threads_;
	self->thread_idle_.broadcast();
    }

    void combine(Reducer *reducer)
    {
	wait();
	for (u32 t = 0; t < threads_.size(); ++t) {
	    reducer->reduce(threads_[t]->getMapper());
	}
    }

    u32 numRunningThreads() const volatile
    {
	return active_threads_;
    }

    u32 numWaitingTasks() const volatile
    {
	LockingPointer<Self> self(*this, monitor_);
	return self->tasks_.size();
    }

    bool empty() const volatile
    {
	LockingPointer<Self> self(*this, monitor_);
	return self->tasks_.empty();
    }

private:
    void terminateThreads() volatile
    {
	LockingPointer<Self> self(*this, monitor_);
	self->terminate_ = true;
	self->new_task_.broadcast();
	while (running_threads_ > 0) {
	    self->thread_idle_.wait(self.getMutex());
	}
	for (typename std::vector<WorkerThread*>::iterator t = self->threads_.begin(); t != self->threads_.end(); ++t) {
	    (*t)->wait();
	}
    }

    std::vector<WorkerThread*> threads_;
    std::deque<Task> tasks_;
    // active threads is accessed by multiple threads
    volatile u32 active_threads_;
    volatile u32 running_threads_;
    bool terminate_;
    mutable Mutex monitor_;
    mutable Condition thread_idle_;
    mutable Condition new_task_;
private:
    ThreadPoolImpl(const ThreadPoolImpl&);
    void operator=(const ThreadPoolImpl&);
};

/**
 * The class Task (T) should carry the input data. It has no required members.
 * Task can either be a type or a pointer to the task type. In the latter case
 * the Mapper or the Reducer has to delete the Task.
 * Pointers should be chosen if copying a Task object is expensive, if the
 * Task is modified by the Mapper, or if the Tasks are used in the Reducer.
 *
 * The class Mapper (M) must have the following methods:
 * class Mapper {
 *   // create a new Mapper object
 *   Mapper* Clone() const;
 *   // process task data
 *   void Map(Task &t);
 *   // clear all intermediate results.
 *   // called with ThreadPool::Reset()
 *   void Reset()
 *  };
 *
 *  The class Reducer (R) must have the following method:
 *  class Reducer {
 *    // accumulate the results of a Mapper
 *    void Reduce(Mapper *mapper);
 *  };
 */
template<class T, class M = NullMapper<T>, class R = NullReducer<M> >
class ThreadPool {
public:
    typedef ThreadPoolImpl<T, M, R> Impl;
    typedef typename Impl::Task Task;
    typedef typename Impl::Mapper Mapper;
    typedef typename Impl::Reducer Reducer;

    ThreadPool() : impl_(new Impl()) {}
    virtual ~ThreadPool() {
	impl_->shutdown();
	delete impl_;
    }

    /**
     * initialize the pool with the given number of threads and copies of
     * of the given prototype Mapper.
     */
    void init(u32 num_threads, const Mapper &mapper = Mapper()) {
	impl_->init(num_threads, mapper);
    }
    /**
     * reset all mappers.
     * waits for all tasks to be finished before resetting the mappers.
     */
    void reset() {
	impl_->reset();
    }
    /**
     * submit a new task to be processed by a mapper.
     */
    void submit(const Task &task) {
	impl_->submit(task);
    }
    /**
     * wait for all tasks to be completed.
     */
    void wait() {
	impl_->wait();
    }
    /**
     * combine the results of the mappers using the given reducer.
     * waits for all tasks to be finished before applying the reducer.
     */
    void combine(Reducer *reducer) {
	impl_->combine(reducer);
    }
protected:
    Impl *impl_;
private:
    ThreadPool(const ThreadPool&);
    void operator=(const ThreadPool&);
};

} // namespace Core

#endif  // _CORE_THREAD_POOL_HH
