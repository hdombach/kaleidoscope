#include "ThreadPool.hpp"
#include "util/log.hpp"
#include <mutex>

TPTask::TPTask(std::string const &name, Func &&f): _name(name), _f(f) { }

std::string const &TPTask::name() const {
	return _name;
}

void TPTask::run() {
	_f();
}

ThreadPool ThreadPool::DEFAULT;

ThreadPool::ThreadPool(): _worker_busy(0), _shutdown_imminent(false) {
	_worker = std::thread(&ThreadPool::_worker_main, this);
}

void ThreadPool::add_task(std::string const &name, TPTask::Func &&f) {
	add_task(TPTask(name, std::move(f)));
}

void ThreadPool::add_task(TPTask &&task) {
	std::lock_guard l(_tasks_lock);

	_tasks.push(std::move(task));

	if (_tasks.size() == 1) {
		_worker_busy.release();
	}
	//Work will prob stall a bit until this scope finished
}

void ThreadPool::shutdown() {
	_shutdown_imminent = true;

	{
		std::lock_guard l(_tasks_lock);
		if (_tasks.empty()) {
			_worker_busy.release();
		}
	}
	_worker.join();
}

const char *ThreadPool::cur_task() const {
	return _cur_task;
}

void ThreadPool::_worker_main() {
	while (1) { // technically useless

de_start:
		if (_shutdown_imminent) return;
		_worker_busy.acquire();
		while (1) {
			TPTask *t = nullptr;
			{
				std::lock_guard l(_tasks_lock);
				if (_tasks.empty()) goto de_start;
				t = &_tasks.front();
				_cur_task = t->name().c_str();
			}
			log_trace() << "Starting task: " << _cur_task << std::endl;
			t->run();
			{
				std::lock_guard l(_tasks_lock);
				_tasks.pop();
				_cur_task = nullptr;
			}
		}
	}
}
