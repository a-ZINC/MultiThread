#pragma once

#include<iostream>
#include<thread>
#include<vector>
#include <functional>
#include <mutex>
#include <condition_variable>
#include<algorithm>

using Task_ = std::function<void()>;

class TaskPool {
public:
	void Run(Task_& task) {
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

				//if (input != nullptr) {
					input();
				//}
				//else {
				//	std::cout << "Function: err" << std::endl;
				//}
				input = {};
				busy_ = false;
			}
		}
	public:
		Worker() : t([this](std::stop_token st) { Run(st); }) {}
		bool isBusy() {
			return busy_.load(std::memory_order_acquire);
		}
		void setTask(Task_& task) {
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