#pragma once
#include "pch.h"

template<typename T>
class LockQueue
{
public:
	LockQueue() { }

	LockQueue(const LockQueue&) = delete;
	LockQueue& operator=(const LockQueue&) = delete;

	void Push(T value)
	{
		lock_guard<mutex> lock(_mutex);
		_queue.push(std::move(value));
		_condVar.notify_one();
	}
	bool TryPop(T& value)
	{
		lock_guard<mutex> lock(_mutex);
		if (_queue.empty())
			return false;

		value = std::move(_queue.front());
		_queue.pop();
		return true;
	}
	bool Empty()
	{
		lock_guard<mutex> lock(_mutex);
		return _queue.empty();
	}
private:
	queue<T> _queue;
	mutex _mutex;
	condition_variable _condVar;
};


