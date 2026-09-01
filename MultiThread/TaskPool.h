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

//class TaskPool {
//public:
//	void Run(Task_ task) {
//		std::lock_guard<std::mutex> lock(mtx);
//		if (auto i = std::ranges::find(workers, false, &Worker::isBusy); i != workers.end()) {
//			(*i)->setTask(task);
//		} else{
//			workers.push_back(std::make_unique<Worker>());
//			workers.back()->setTask(task);
//		}
//	}
//
//	bool isRunningTask() {
//		std::lock_guard<std::mutex> lock(mtx);
//		return std::ranges::any_of(workers, [](const auto& w) { return w->isBusy(); });
//	}
//
//private:
//	class Worker {
//	private:
//		void Run(std::stop_token st) {
//			std::unique_lock<std::mutex> lock(mtx);
//			while (true) {
//				cv.wait(lock, st, [this]() {return busy_.load(std::memory_order_acquire);});
//				if (st.stop_requested()) return;
//
//				if (input != nullptr) {
//					input();
//				}
//				else {
//					std::cout << "Function: err" << std::endl;
//				}
//				input = {};
//				busy_ = false;
//			}
//		}
//	public:
//		Worker() : t([this](std::stop_token st) { Run(st); }) {}
//		bool isBusy() {
//			return busy_.load(std::memory_order_acquire);
//		}
//		void setTask(Task_ task) {
//			input = std::move(task);
//			busy_.store(true, std::memory_order_release);
//			cv.notify_one();
//		}
//	private:
//		Task_ input;
//		std::atomic<bool> busy_{ false };
//		std::mutex mtx;
//		std::condition_variable_any cv;
//		std::jthread t;
//	};
//
//private:
//	std::vector<std::unique_ptr<Worker>> workers;
//	std::mutex mtx;
//};
//
//class QueueTaskPool {
//public:
//	QueueTaskPool(int min, int max) : minWorker(min), maxWorker(max) {
//		workers_.reserve(minWorker);
//		for (int i = 0; i < minWorker; i++) {
//			workers_.push_back(std::make_unique<Worker>(this));
//		}
//	}
//
//	void Run(Task_ task) {
//		{
//			std::lock_guard<std::mutex> lock(mtx_);
//			tasks_.push_back(task);
//			if (tasks_.size() > workers_.size() && workers_.size() < maxWorker) {
//				workers_.push_back(std::make_unique<Worker>(this));
//			}
//		}
//		queueCv_.notify_one();
//	}
//
//	Task_ getTask(std::stop_token st) {
//		Task_ task;
//		std::unique_lock<std::mutex> lock(mtx_);
//		queueCv_.wait(lock, st, [this] { return !tasks_.empty();});
//		if (!st.stop_requested()) {
//			task = std::move(tasks_.front());
//			tasks_.pop_front();
//		}
//		return task;
//	}
//
//	void waitTP() {
//		std::unique_lock<std::mutex> lock(mtx_);
//		allDoneCv_.wait(lock, [this] {return tasks_.empty() && !anyBusy();});
//	}
//
//	bool anyBusy() {
//		return std::ranges::any_of(workers_, [](const auto& w) {return w->isBusy();});
//	}
//
//private:
//	class Worker {
//	private:
//		void RunCore(std::stop_token st) {
//			while (Task_ task = tp->getTask(st)) {
//				busy.store(true, std::memory_order_release);
//				task();
//				busy.store(false, std::memory_order_release);
//
//				{
//					std::lock_guard<std::mutex> lock(tp->mtx_);
//					if (!tp->anyBusy() && tp->tasks_.empty()) {
//						tp->allDoneCv_.notify_all();
//					}
//				}
//			}
//		}
//	public:
//		Worker(QueueTaskPool* tp) : tp(tp), thread([this](std::stop_token st) { RunCore(st); }) {}
//
//		bool isBusy() {
//			return busy.load(std::memory_order_acquire);
//		}
//	private:
//		std::atomic<bool> busy;
//		QueueTaskPool* tp;
//		std::jthread thread;
//	};
//
//private:
//	int minWorker;
//	int maxWorker;
//	std::mutex mtx_;
//	std::condition_variable_any queueCv_;
//	std::condition_variable allDoneCv_;
//	std::deque<Task_> tasks_;
//	std::vector<std::unique_ptr<Worker>> workers_;
//};

template <typename T>
class SharedState {
public:
	template <typename R>
	void set(R&& value) {
		if (!result.has_value()) {
			result = std::forward<R>(value);
			s.release();
		}
	}

	T get() {
		s.acquire();
		return std::move(result.value());

	}
private:
	std::optional<T> result;
	std::binary_semaphore s{ 0 };
};

template<typename T>
class Future;

template <typename T>
class Promise {
public:
	Promise() : ss(std::make_shared<SharedState<T>>()) {}

	template<typename R>
	void set(R&& value) {
		ss->set(std::forward<R>(value));
	}

	Future<T> getFuture() {
		assert(future_avail && "Future already retrieved");
		future_avail = false;
		return Future(ss);
	}
private:
	std::shared_ptr<SharedState<T>> ss; 
	bool future_avail = true;
};

template <typename T>
class Future {
private:
	Future(std::shared_ptr<SharedState<T>> ss) : ss(ss) {}
	friend class Promise<T>;
public:
	T get() {
		assert(result_avail && "Result already retrieved");
		result_avail = false;
		return std::move(ss->get());
	}

private:
	std::shared_ptr<SharedState<T>> ss;
	bool result_avail = true;
};



class Task_ {
public:
	void operator ()() {
		func_();
	}

	Task_() = default;
	Task_(const Task_&) = delete;
	Task_& operator=(const Task_&) = delete;
	Task_(Task_&&) = default;
	Task_& operator=(Task_&&) = default;

	template<typename Func, typename ...Args>
	static auto make(Func&& func, Args&& ...args) {
		using ReturnType = std::invoke_result_t<Func, Args...>;

		Promise<ReturnType> prom;
		Future<ReturnType> fut = prom.getFuture();

		return std::make_pair(Task_(std::move(prom), std::forward<Func>(func), std::forward<Args>(args)...),
			std::move(fut));
	}

private:
	template<typename Prom, typename Func, typename ...Args>
	Task_(Prom&& prom, Func&& func, Args&&... args) {
		func_ = [
			p = std::forward<Prom>(prom), 
			f = std::forward<Func>(func), 
			...a = std::forward<Args>(args)
		]() mutable {
			p.set(f(a...));
			};
	}
	
private:
	std::function<void()> func_;
};

