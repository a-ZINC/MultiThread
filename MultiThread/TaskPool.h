#pragma once

#include<iostream>
#include<thread>
#include<vector>
#include <functional>
#include <mutex>
#include <condition_variable>
#include<algorithm>
#include <format>
#include <deque>
#include <optional>
#include <semaphore>
#include <cassert>

using Task_ = std::function<void()>;

class TaskPool {
public:
	void Run(Task_ task) {
		std::lock_guard<std::mutex> lock(mtx);
		if (auto i = std::ranges::find(workers, false, &Worker::isBusy); i != workers.end()) {
			(*i)->setTask(task);
		} else{
			workers.push_back(std::make_unique<Worker>());
			workers.back()->setTask(task);
		}
	}

	bool isRunningTask() {
		std::lock_guard<std::mutex> lock(mtx);
		return std::ranges::any_of(workers, [](const auto& w) { return w->isBusy(); });
	}

private:
	class Worker {
	private:
		void Run(std::stop_token st) {
			std::unique_lock<std::mutex> lock(mtx);
			while (true) {
				cv.wait(lock, st, [this]() {return busy_.load(std::memory_order_acquire);});
				if (st.stop_requested()) return;

				if (input != nullptr) {
					input();
				}
				else {
					std::cout << "Function: err" << std::endl;
				}
				input = {};
				busy_ = false;
			}
		}
	public:
		Worker() : t([this](std::stop_token st) { Run(st); }) {}
		bool isBusy() {
			return busy_.load(std::memory_order_acquire);
		}
		void setTask(Task_ task) {
			input = std::move(task);
			busy_.store(true, std::memory_order_release);
			cv.notify_one();
		}
	private:
		Task_ input;
		std::atomic<bool> busy_{ false };
		std::mutex mtx;
		std::condition_variable_any cv;
		std::jthread t;
	};

private:
	std::vector<std::unique_ptr<Worker>> workers;
	std::mutex mtx;
};

class QueueTaskPool {
public:
	QueueTaskPool(int min, int max) : minWorker(min), maxWorker(max) {
		workers_.reserve(minWorker);
		for (int i = 0; i < minWorker; i++) {
			workers_.push_back(std::make_unique<Worker>(this));
		}
	}

	void Run(Task_ task) {
		{
			std::lock_guard<std::mutex> lock(mtx_);
			tasks_.push_back(task);
			if (tasks_.size() > workers_.size() && workers_.size() < maxWorker) {
				workers_.push_back(std::make_unique<Worker>(this));
			}
		}
		queueCv_.notify_one();
	}

	Task_ getTask(std::stop_token st) {
		Task_ task;
		std::unique_lock<std::mutex> lock(mtx_);
		queueCv_.wait(lock, st, [this] { return !tasks_.empty();});
		if (!st.stop_requested()) {
			task = std::move(tasks_.front());
			tasks_.pop_front();
		}
		return task;
	}

	void waitTP() {
		std::unique_lock<std::mutex> lock(mtx_);
		allDoneCv_.wait(lock, [this] {return tasks_.empty() && !anyBusy();});
	}

	bool anyBusy() {
		return std::ranges::any_of(workers_, [](const auto& w) {return w->isBusy();});
	}

private:
	class Worker {
	private:
		void RunCore(std::stop_token st) {
			while (Task_ task = tp->getTask(st)) {
				busy.store(true, std::memory_order_release);
				task();
				busy.store(false, std::memory_order_release);

				{
					std::lock_guard<std::mutex> lock(tp->mtx_);
					if (!tp->anyBusy() && tp->tasks_.empty()) {
						tp->allDoneCv_.notify_all();
					}
				}
			}
		}
	public:
		Worker(QueueTaskPool* tp) : tp(tp), thread([this](std::stop_token st) { RunCore(st); }) {}

		bool isBusy() {
			return busy.load(std::memory_order_acquire);
		}
	private:
		std::atomic<bool> busy;
		QueueTaskPool* tp;
		std::jthread thread;
	};

private:
	int minWorker;
	int maxWorker;
	std::mutex mtx_;
	std::condition_variable_any queueCv_;
	std::condition_variable allDoneCv_;
	std::deque<Task_> tasks_;
	std::vector<std::unique_ptr<Worker>> workers_;
};

template<typename T>
class SharedState {
public:
	template<typename R>
	void set(R&& result) {
		if (!result_.has_value()) {
			this->result_ = std::forward<R>(result);
			ready_.release();
		}
	}

	T get() {
		ready_.acquire();
		return std::move(result_.value());
	}
private:
	std::optional<T> result_;
	std::binary_semaphore ready_{ 0 };
};
template <typename T>
class Future;

template<typename T>
class Promise {
private:
	std::shared_ptr<SharedState<T>> ss;
	bool future_avail = true;
public:
	Promise() : ss(std::make_shared<SharedState<T>>()) {}

	template<typename R>
	void set(R&& result) {
		ss->set(std::forward<R>(result));
	}

	Future<T> get_future() {
		assert(future_avail);
		future_avail = false;
		return Future<T>(ss);
	}

};

template<typename T>
class Future {
private:
	std::shared_ptr<SharedState<T>> ss;
	bool acquired = false;
private:
	Future(std::shared_ptr<SharedState<T>> ss) : ss(ss) {}
	friend class Promise<T>;

public:
	T get() {
		assert(!acquired);
		acquired = true;
		return ss->get();
	}
};

