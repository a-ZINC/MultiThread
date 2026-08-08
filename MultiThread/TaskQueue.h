#pragma once

#include <numbers>
#include <array>
#include <cmath>
#include <mutex>
#include <span>
#include <condition_variable>
#include <atomic>

namespace tq {
	constexpr size_t WORKER_COUNT = 4;
	constexpr size_t CHUNK_SIZE = 1'00;
	constexpr size_t CHUNK_COUNT = 1000;
	constexpr size_t LIGHT_IT = 100;
	constexpr size_t HEAVY_IT = 1000;
	constexpr double HEAVY_PROBABILITY = 0.15;

	static_assert(CHUNK_SIZE% WORKER_COUNT == 0, "Worker should be multiple of chunk size");
	class Task {
	public:
		double val;
		bool heavy;

		unsigned int process() const {
			size_t iterations = heavy ? HEAVY_IT : LIGHT_IT;
			double intermediate = val;

			for (auto i = 0; i < iterations; i++) {
				intermediate = double(unsigned int(std::abs(std::sin(std::cos(intermediate)) * 10'000'000)) % 100'000) / 10'000;
			}
			return unsigned int(std::exp(intermediate));
		}
	};

	using Chunk = std::array<Task, CHUNK_SIZE>;
	class Master {
	private:
		std::span<Task> chunk;
		std::condition_variable cv;
		std::mutex mtx;
		std::atomic<int> idx;
		int doneCount = 0;
	public:
		Master() {};
		void setChunk(std::span<Task> c) {
			{
				std::lock_guard<std::mutex> lock(mtx);
				chunk = c;
				idx = 0;
			}
		}

		void setDone() {
			bool notify = false;
			{
				std::lock_guard<std::mutex> lock(mtx);
				++doneCount;
				notify = (doneCount == WORKER_COUNT);
			}
			if (notify) cv.notify_one();
		}

		__declspec(noinline) const Task* getTask() {
			int currentIdx = idx++;
			if (currentIdx >= CHUNK_SIZE) return nullptr;
			return &chunk[currentIdx];
		}

		void wait_for_all() {
			std::unique_lock<std::mutex> lock(mtx);
			cv.wait(lock, [this]() ->  bool { return doneCount == WORKER_COUNT; });
			doneCount = 0;
		}

	};

	class Worker {
	private:
		Master* master;
		bool working;
		bool dying;
		std::condition_variable cv;
		std::mutex mtx;
		std::jthread thread;
		unsigned int accumulation;

	private:
		void Run_() {
			std::unique_lock<std::mutex> lock(mtx);
			while (true) {
				cv.wait(lock, [this]() -> bool { return working || dying; });
				if (dying) {
					return;
				}
				while (const Task* task = master->getTask()) {
					accumulation += task->process();
				}

				working = false;
				master->setDone();
			}
		}
	public:
		Worker(Master* master) : master(master), working(false), dying(false), accumulation(0), thread(&Worker::Run_, this) {};
		void startWorking() {
			{
				std::unique_lock<std::mutex> lock(mtx);
				working = true;
			}
			cv.notify_one();
		}
		void Kill()
		{
			{
				std::lock_guard<std::mutex> lg(mtx);
				dying = true;
			}
			cv.notify_one();
		}

		double GetResult() const { return accumulation; }
	};
}