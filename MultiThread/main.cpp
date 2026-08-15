#include<iostream>
#include"Timer.h"
//#include"Thread.h"
#include "Prac.h"

Timer t;

int main() {
	std::cout << "Hello, World!" << std::endl;
	//Thread thread(t);
	//thread.singleThread();
	//thread.simpleMultiThreadRaceCondition();
	////thread.simpleMultiThreadMutex();
	//thread.simpleMultiThreadStoragePerThread();
	//thread.simpleMultiThreadStoragePerThreadWithAligned();
	//thread.simpleMultiThreadStoragePerThreadSingleBatch();
	//thread.simpleMultiThreadStoragePerThreadMultiBatch();
	//thread.simpleMultiThreadStoragePerThreadMultiBatchWithMasterWorker();
	//{
	//	thread.simpleMultiThreadStoragePerThreadMultiBatchWithMasterWorkerController('s');
	//}
	//{
	//	thread.simpleMultiThreadTaskQueue();
	//}

	//prac::FalseSharing fs;
	//fs.run(t);

	//prac::CompilerHazard ch;
	//ch.run();

	//prac::LockFreeCheck lf;
	//lf.run();

	//prac::Sum si(t);
	//si.run();
	
	//prac::LockFreeStackTest lst;
	//lst.run();

	prac::Handoff hf;
	hf.run();
	return 0;
}