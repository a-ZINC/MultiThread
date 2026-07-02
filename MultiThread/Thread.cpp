#include"Thread.h"

Thread::Thread(Timer& t) : t(t) {}

void Thread::complexFunctionWithMutex(int& cnt) {
	for (int i = 0; i < 1000000; ++i) {
			std::lock_guard<std::mutex> lock(mtx);
			cnt += (int)(i * ((double)rand() / RAND_MAX)) % 2;
	}
}

void Thread::complexFunction(int& cnt) {
	for (int i = 0; i < 1000000; ++i) {
		cnt += (int)(i * ((double)rand() / RAND_MAX)) % 2;
	}
}


void Thread::singleThread() {
	t.start();
	int cnt = 0;
	for (int i = 0; i < 4; i++) {
		complexFunction(cnt);
	}
	t.stop("Single Thread");
	std::cout << "cnt: " << cnt << "\n";
}

void Thread::simpleMultiThreadRaceCondition() {
	t.start();
	int cnt = 0;
	std::vector<std::thread> threads;
	for (int i = 0; i < 4; i++) {
		threads.push_back(std::thread(&Thread::complexFunction, this, std::ref(cnt)));
	}
	for (auto& th : threads) {
		if (th.joinable()) {
			th.join();
		}
	}
	t.stop("Multi Thread Race Condition");
	std::cout << "cnt: " << cnt << "\n";
}

void Thread::simpleMultiThreadMutex() {
	t.start();
	int cnt = 0;
	std::vector<std::thread> threads;
	for (int i = 0; i < 4; i++) {
		threads.push_back(std::thread(&Thread::complexFunctionWithMutex, this, std::ref(cnt)));
	}
	for (auto& th : threads) {
		if (th.joinable()) {
			th.join();
		}
	}
	t.stop("Multi Thread Mutex");
	std::cout << "cnt: " << cnt << "\n";
	
}