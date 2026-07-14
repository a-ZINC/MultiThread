#pragma once
#include<random>
#include<thread>
#include<iostream>
#include<vector>
#include<mutex>
#include<span>
#include"Timer.h"

class Thread {
private:
	Timer& t;
	std::mutex mtx;
public:
	Thread(Timer& t);
	void complexFunction(int& cnt);
	void complexFunctionWithMutex(int& cnt);
	std::vector<std::vector<int>> generateDataset();
	void processBatch(int& sum, std::span<int> batch);
	void singleThread();
	void simpleMultiThreadRaceCondition();
	void simpleMultiThreadMutex();
	void simpleMultiThreadStoragePerThread();
	void simpleMultiThreadStoragePerThreadWithAligned();
	void simpleMultiThreadStoragePerThreadSingleBatch();
	void simpleMultiThreadStoragePerThreadMultiBatch();
};