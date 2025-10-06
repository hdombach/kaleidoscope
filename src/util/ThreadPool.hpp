#pragma once

#include <functional>
#include <mutex>
#include <thread>
#include <semaphore>
#include <string>

class TPTask {
	public:
		using Func = std::function<void()>;
		
		TPTask(std::string const &name, Func &&f);

		std::string const &name() const;

		void run();

	private:
		std::string _name;
		Func _f;
};

/**
 * @brief A class for managing longer tasks that need to be run
 * Currently only supports one helper thread since there isn't too much background work
 */
class ThreadPool {
	public:
		using Task = std::function<void()>;
		static ThreadPool DEFAULT;

		ThreadPool();

		ThreadPool(ThreadPool const &other) = delete;
		ThreadPool(ThreadPool &&other) = delete;
		ThreadPool &operator=(ThreadPool const &other) = delete;
		ThreadPool &operator=(ThreadPool &&other) = delete;

		void add_task(std::string const &name, TPTask::Func &&f);
		void add_task(TPTask &&task);

		void shutdown();

		const char *cur_task() const;

	private:
		std::thread _worker;
		std::binary_semaphore _worker_busy;
		std::queue<TPTask> _tasks;
		std::mutex _tasks_lock;
		bool _shutdown_imminent;

		const char *_cur_task;

		void _worker_main();
};
