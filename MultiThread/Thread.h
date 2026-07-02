#pragma once
#include<random>
#include<thread>
#include<iostream>
#include<vector>
#include<mutex>
#include"Timer.h"

class Thread {
private:
	Timer& t;
	std::mutex mtx;
public:
	Thread(Timer& t);
	void complexFunction(int& cnt);
	void complexFunctionWithMutex(int& cnt);
	void singleThread();
	void simpleMultiThreadRaceCondition();
	void simpleMultiThreadMutex();
	void simpleMultiThreadStoragePerThread();
};