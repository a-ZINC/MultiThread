#include<iostream>
#include"Timer.h"
#include"Thread.h"

Timer t;

int main() {
	std::cout << "Hello, World!" << std::endl;
	Thread thread(t);
	//thread.singleThread();
	//thread.simpleMultiThreadRaceCondition();
	////thread.simpleMultiThreadMutex();
	//thread.simpleMultiThreadStoragePerThread();
	//thread.simpleMultiThreadStoragePerThreadWithAligned();
	thread.simpleMultiThreadStoragePerThreadSingleBatch();
	thread.simpleMultiThreadStoragePerThreadMultiBatch();
	
	return 0;
}