#ifndef THREADPOOL_H
#define THREADPOOL_H


#include "Condition.h"
#include "Mutex.h"
#include <vector>
#include <queue>
#include "SocketOps.h"

template<class T>
class ThreadPool
{
public:
	ThreadPool(int numThread);
	~ThreadPool();

	void start();
	void stop();

	void addTask(T* request);
  T* get();

private:
	bool running_;
	mutable MutexLock mutex_;
	Condition notEmpty_;
	std::queue<T*> queue_;
	size_t numThreads_;
	std::vector<pthread_t> tid_;

	static void* work(void *arg);
	void run();
};


template<class T>
ThreadPool<T>::ThreadPool(int numThread):
 mutex_(),
 notEmpty_(mutex_),
 numThreads_(numThread),
 running_(false),
 tid_(numThread, 0)
{
  // tid_ = new pthread_t[numThreads_];
  // assert(tid_);
}

template<class T>
ThreadPool<T>::~ThreadPool()
{
  if(running_)
  {
    stop();
  }
}


template<class T>
void ThreadPool<T>::start()
{
  running_ = true;

  for(int i = 0; i < numThreads_; i++)
  {
    Pthread_create(&tid_[i], NULL, work, this);
  }
}


template<class T>
void ThreadPool<T>::stop()
{
  {
    MutexLockGuard lock(mutex_);
    running_ = false;
    notEmpty_.notifyAll();
  }

  for(int i = 0; i < numThreads_; ++i)
  {
    Pthread_join(tid_[i], NULL);
  }

  // delete[] tid_;
}


template<class T>
void ThreadPool<T>::addTask(T* request)
{
  MutexLockGuard lock(mutex_);
  
  queue_.push(request);
  notEmpty_.notify();
}


template<class T>
T* ThreadPool<T>::get()
{
  MutexLockGuard lock(mutex_);

  while (queue_.empty() && running_)
  {
    notEmpty_.wait();
  }
  
  T *task = nullptr;

  if(!queue_.empty())
  {
    task = queue_.front();
    queue_.pop();
  }

  return task;
}


template<class T>
void* ThreadPool<T>::work(void *arg)
{
  ThreadPool *pool = (ThreadPool*)arg;

  while(pool->running_)
  {
    T* task = pool->get();
    task->doit();

    Close(task->getConnd());
    delete task;
  }
  // ThreadPool *pool = (ThreadPool*)arg;
  // pool->run();
  return pool;
}


// template<typename T>
// void ThreadPool<T>::run()
// {
//   while(running_)
//   {
//     T = queue_.take();


//     //TODO:
//     T->doit();
//   }
// }

#endif