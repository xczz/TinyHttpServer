#ifndef SAFE_QUEUE_H
#define SAFE_QUEUE_H


#include "Condition.h"
#include "Mutex.h"
#include <assert.h>
#include <deque>

template<typename T>
class SafeQueue
{
public:
	SafeQueue(MutexLock& mutex):mutex_(mutex),notEmpty_(mutex_),queue_()
	{
	}

	void put(const T& x)
	{
		MutexLockGuard lock(mutex_);

		queue_.push_back(x);
		notEmpty_.notify();
	}

	void put(T&& x)
	{
		MutexLockGuard lock(mutex_);

		queue_.push_back(std::move(x));
		notEmpty_.notify();

	}

	T take()
	{
		MutexLockGuard lock(mutex_);
		while(queue_.empty())
		{
			notEmpty_.wait();
		}

		assert(!queue_.empty());
		T front(std::move(queue_.front()));
		queue_.pop_front();

		return front;
	}

	size_t size()const
	{
		MutexLockGuard lock(mutex_);

		return queue_.size();
	}


private:
	mutable MutexLock mutex_;
	Condition notEmpty_;
	std::deque_<T> queue_;

}


#endif