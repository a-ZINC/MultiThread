#pragma once
#include<random>
#include<thread>
#include<iostream>
#include<vector>
#include<mutex>
#include<span>
#include"Timer.h"
#include <condition_variable>
#include <cmath>
#include <numbers>
#include <array>
#include <fstream>
#include <format>
#include <numeric>
#include <algorithm>
#include <ranges>

typedef long long int64;

constexpr size_t WORKER_COUNT = 4;
constexpr size_t CHUNK_SIZE = 1'000;
constexpr size_t CHUNK_COUNT = 100;
constexpr size_t LIGHT_IT = 100;
constexpr size_t HEAVY_IT = 1000;
constexpr double HEAVY_PROBABILITY = 0.15;

static_assert(CHUNK_SIZE % WORKER_COUNT == 0, "Worker should be multiple of chunk size");

class Task {
public:
	double val;
	bool heavy;

	unsigned int process() const {
		size_t iterations = heavy ? HEAVY_IT : LIGHT_IT;
		double intermediate = val;

		for (auto i = 0; i < iterations; i++) {
			intermediate = static_cast<double>(static_cast<unsigned int>(std::abs(std::sin(std::cos(intermediate)) * 10'000'000)) % 100'000) / 10'000;
		}
		return static_cast<unsigned int>(std::exp(intermediate));
	}
};

using Chunk = std::array<Task, CHUNK_SIZE>;

class Thread {
private:
	Timer& t;
	std::mutex mtx;
	

public:
	Thread(Timer& t);
	void complexFunction(int& cnt);
	void complexFunctionWithMutex(int& cnt);
	std::vector<std::vector<int>> generateDataset();
	std::vector<Chunk> generateRandomDataSet();
	std::vector<Chunk> generateEvenlyDataSet();
	std::vector<Chunk> generateStackedDataSet();
	void processBatch(int& sum, std::span<int> batch);
	void singleThread();
	void simpleMultiThreadRaceCondition();
	void simpleMultiThreadMutex();
	void simpleMultiThreadStoragePerThread();
	void simpleMultiThreadStoragePerThreadWithAligned();
	void simpleMultiThreadStoragePerThreadSingleBatch();
	void simpleMultiThreadStoragePerThreadMultiBatch();
	void simpleMultiThreadStoragePerThreadMultiBatchWithMasterWorker();
	void simpleMultiThreadStoragePerThreadMultiBatchWithMasterWorkerController(char s);
	void simpleMultiThreadTaskQueue();
	void simpleMultiThreadTaskPool();
	void simpleMultiThreadQueueTaskPool();
	void simpleFuturePromise();
};

class Master {
private:
	std::mutex mtx;

	std::condition_variable cv;
	int doneCount;
	int workerCount;

public:
	Master(int workerCount) : doneCount(0), workerCount(workerCount) {};

	void set_done() {
		{
			std::lock_guard<std::mutex> lock(mtx);
			doneCount++;
		}
		if (doneCount == workerCount) {
			cv.notify_one();
		}
	}

	void wait_for_all() {
		std::unique_lock<std::mutex> lock(mtx);
		cv.wait(lock, [this]() -> bool { return doneCount == workerCount; });
		doneCount = 0; // Reset for next batch
	}
};

class Worker {
private:
	Master* master;
	std::mutex mtx;
	std::condition_variable cv;
	std::span<int> input_batch;
	int* output = nullptr;
	bool dying = false;
	std::jthread t;

private:
	void Run_() {
		std::unique_lock<std::mutex> lock(mtx);
		while (true) {
			cv.wait(lock, [this]() -> bool { return output != nullptr || dying; });
			if (dying) {
				break;
			}
			processBatch(*output, input_batch);

			this->output = nullptr;
			input_batch = {};

			master->set_done();
		}
	}

	void processBatch(int& sum, std::span<int> batch) {
		int64 local_sum = 0;
		for (const int& num : batch) {
			local_sum += num;
		}
		sum += (int)local_sum;
	}
public:
	Worker(Master* mas) : master(mas), t(&Worker::Run_, this) {};
	
	void setJob(std::span<int>& batch, int* output) {
		{
			std::lock_guard<std::mutex> lock(mtx);
			input_batch = batch;
			this->output = output;
		}
		cv.notify_one();
	}

	void kill() {
		{
			std::lock_guard<std::mutex> lock(mtx);
			dying = true;
		}
		cv.notify_one();
	}
};

class MasterController {
private:
	std::mutex mtx;
	std::condition_variable cv;
	int worker_count;
	int done_count;
public:
	MasterController(int worker) : worker_count(worker), done_count(0) {}
	void signalDone() {
		{
			std::lock_guard<std::mutex> lock(mtx);
			done_count++;
		}
		if (worker_count == done_count) {
			cv.notify_one();
		}
	}

	void wait_for_all() {
		std::unique_lock<std::mutex> lock(mtx);
		cv.wait(lock, [this]() -> bool {return done_count == worker_count;});
		done_count = 0;
	}
};

class WorkerController {
private:
	std::mutex mtx;
	std::condition_variable cv;
	bool dying = false;
	std::span<const Task> input;
	unsigned int* output = nullptr;
	MasterController* master;
	std::jthread thread;
	size_t heavyCount = 0;
	float timeSpendPerWorker = -1.f;
	Timer timer;

private:
	void Run_() {
		std::unique_lock<std::mutex> lock(mtx);
		while (true) {
			cv.wait(lock, [this]() ->  bool { return input.size() != 0 || dying; });
			if (dying) {
				break;
			}

			timer.start();
			processTasks();
			timeSpendPerWorker = timer.stop("Worker Processing Time", false);
			input = {};
			this->output = nullptr;

			master->signalDone();
		}
	}

	std::vector<Chunk> generateRandomDataSet() {
		std::vector<Chunk> chunks;
		chunks.reserve(CHUNK_COUNT);

		std::mt19937 rnd(2828);
		std::uniform_real_distribution<double> v_dist{ 0, std::numbers::pi };
		std::bernoulli_distribution h_dist{ HEAVY_PROBABILITY };

		for (size_t c = 0; c < chunks.size(); c++) {
			Chunk chunk;
			for (auto& task : chunk) {
				task.val = v_dist(rnd);
				task.heavy = h_dist(rnd);
			}
		}
		return chunks;
	}

	std::vector<Chunk> generateEvenlyDataSet() {
		std::vector<Chunk> chunks;
		chunks.reserve(CHUNK_COUNT);

		std::mt19937 rnd(2828);
		std::uniform_real_distribution<double> v_dist{ 0, std::numbers::pi };
		int nth = static_cast<int>(1.0 / HEAVY_PROBABILITY);
		for (size_t c = 0; c < chunks.size(); c++) {
			Chunk chunk;
			for (auto it = 0; it < chunk.size(); it++) {
				chunk[it].val = v_dist(rnd);
				chunk[it].heavy = (it % nth == 0);
			}
		}
		return chunks;
	}

	std::vector<Chunk> generateStackedDataSet() {
		std::vector<Chunk> chunks = generateEvenlyDataSet();
		for (auto& chunk : chunks) {
			std::ranges::partition(chunk, [](const Task& t) {return t.heavy; });
		}
		return chunks;
	}

	void processTasks() {
		heavyCount = 0;
		for (const auto& task : input) {
			*output += task.process();
			heavyCount += task.heavy ? 1 : 0;
		}
	}

public:
	WorkerController(MasterController* master, Timer& timer) : master(master), thread(std::jthread(&WorkerController::Run_, this)), timer(timer) {};
	~WorkerController() {
		kill();
	}
	WorkerController(const WorkerController&) = delete;
	WorkerController& operator=(const WorkerController&) = delete;

	// 3. DEFAULT Move Constructor and Move Assignment (Allows vector reallocation)
	WorkerController(WorkerController&&) noexcept = default;
	WorkerController& operator=(WorkerController&&) noexcept = default;

	void kill() {
		{
			std::lock_guard<std::mutex> lock(mtx);
			dying = true;
		}
		cv.notify_one();
	}

	void setJob(std::span<Task> input, unsigned int* output) {
		{
			std::lock_guard<std::mutex> lock(mtx);
			this->input = input;
			this->output = output;
		}
		cv.notify_one();
	}

	float getTimeSpentPerWorker() const {
		return timeSpendPerWorker;
	}

	size_t getHeavyCount() const {
		return heavyCount;
	}

};

struct ChunkTimingInfo {
	std::array<float, WORKER_COUNT> timeSpentPerWorker;
	std::array<size_t, WORKER_COUNT> heavyCountPerWorker;
	float totalChunkTime;
};