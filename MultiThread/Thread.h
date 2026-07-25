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

typedef long long int64;

constexpr size_t WORKER_COUNT = 4;
constexpr size_t CHUNK_SIZE = 1000;
constexpr size_t CHUNK_COUNT = 100;
constexpr size_t LIGHT_IT = 100;
constexpr size_t HEAVY_IT = 1000;
constexpr double HEAVY_PROBABILITY = 0.05;

static_assert(CHUNK_SIZE % WORKER_COUNT == 0, "Worker should be multiple of chunk size");

class Task {
public:
	int val;
	bool heavy;

	double process() {
		size_t iterations = heavy ? HEAVY_IT : LIGHT_IT;
		double intermediate = val;

		for (auto i = 0; i < iterations; i++) {
			intermediate = std::sin(std::cos(intermediate));
		}
		return intermediate;
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