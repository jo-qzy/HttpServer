#ifndef __THREAD_POOL_HPP__
#define __THREAD_POOL_HPP__

#include <iostream>
#include <queue>

#define DEFAULT_NUM 5

typedef void* (*handler_t)(int);

class Task
{
	private:
		int _sock;
		handler_t _handler;
	public:
		Task(int sock, handler_t handler)
			: _sock(sock)
			, _handler(handler)
		{}

		void Run()
		{
			_handler(_sock);
		}

		void operator()(int sock)
		{}
};

class ThreadPool
{
	private:
		int _total_num;
		int _idle_num;
		std::queue<Task> _task_queue;
		pthread_mutex_t _lock;
		pthread_cond_t _cond;
		bool _quit_flag;
	private:
		void LockQueue()
		{
			pthread_mutex_lock(&_lock);
		}

		void UnlockQueue()
		{
			pthread_mutex_unlock(&_lock);
		}
		
		void WakeUpOneThread()
		{
			pthread_cond_signal(&_cond);
		}

		void WakeUpAllThread()
		{
			pthread_cond_broadcast(&_cond);
		}

		bool QueueEmpty()
		{
			return _task_queue.empty();
		}

		void ThreadIdle()
		{
			if (_quit_flag)
			{
				UnlockQueue();
				_total_num--;
				pthread_exit(NULL);
			}
			_idle_num++;
			pthread_cond_wait(&_cond, &_lock);
			_idle_num--;
		}

		Task& GetTask()
		{
			Task& t = _task_queue.front();
			_task_queue.pop();
			return t;
		}

		static void* start_routine(void* arg)
		{
			pthread_detach(pthread_self());
			ThreadPool* tp = (ThreadPool*)arg;
			while (1)
			{
				tp->LockQueue();
				while (tp->QueueEmpty())
				{
					tp->ThreadIdle();
				}
				Task t = tp->GetTask();
				tp->UnlockQueue();
				t.Run();
			}
		}
	public:
		ThreadPool(int num = DEFAULT_NUM)
			: _total_num(num)
			, _idle_num(0)
			, _quit_flag(false)
		{
			pthread_mutex_init(&_lock, NULL);
			pthread_cond_init(&_cond, NULL);
			for (int i = 0; i < _total_num; i++)
			{
				pthread_t tid;
				pthread_create(&tid, NULL, start_routine, this);
			}
		}

		void InitThreadPool()
		{
		}

		void PushTask(Task t)
		{
			LockQueue();
			if (_quit_flag)
			{
				UnlockQueue();
				return;
			}
			_task_queue.push(t);
			WakeUpOneThread();
			UnlockQueue();
		}

		void Stop()
		{
			LockQueue();
			_quit_flag = true;
			UnlockQueue();

			while (_idle_num > 0)
			{
				WakeUpAllThread();
			}
		}

		~ThreadPool()
		{
			pthread_mutex_destroy(&_lock);
			pthread_cond_destroy(&_cond);
		}
};

#endif
