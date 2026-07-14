#include"Thread.h"

struct AlignedCount {
	alignas(64) int cnt = 0;
};

Thread::Thread(Timer& t) : t(t) {}

void Thread::complexFunctionWithMutex(int& cnt) {
	const double PI_HALF = 1.5707963267948966;
	for (int i = 0; i < 100000000; ++i) {
			std::lock_guard<std::mutex> lock(mtx);
			double angle = (double(i) / 100000000) * PI_HALF;
			cnt += (int)(std::sin(angle) * 2);
	}
}

void Thread::complexFunction(int& cnt) {
	const double PI_HALF = 1.5707963267948966;
	for (int i = 0; i < 100000000; ++i) {
		double angle = (double(i) / 100000000) * PI_HALF;
		cnt += (int)(std::sin(angle) * 2);
	}
}

std::vector<std::vector<int>> Thread::generateDataset() {
	const int num_batches = 4;
	const int batch_size = 100000000;

	std::vector<std::vector<int>> dataset(num_batches, std::vector<int>(batch_size));

	for (int i = 0; i < num_batches; ++i) {
		for (int j = 0; j < batch_size; ++j) {
			dataset[i][j] = j % 10;
		}
	}
	return dataset;
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

void Thread::simpleMultiThreadStoragePerThread() {
	t.start();
	int sum = 0;
	std::vector<int> cnts(4, 0);
	std::vector<std::thread> threads;
	for (int i = 0; i < 4; i++) {
		threads.push_back(std::thread(&Thread::complexFunction, this, std::ref(cnts[i])));
	}

	for (auto& t : threads) {
		if (t.joinable()) {
			t.join();
		}
	}

	for (auto& cnt : cnts) {
		sum += cnt;
	}
	t.stop("Multi Thread Storage Per Thread");
	std::cout << "cnt: " << sum << "\n";
}
void Thread::simpleMultiThreadStoragePerThreadWithAligned() {
	t.start();
	int sum = 0;
	std::vector<AlignedCount> cnts(4);
	std::vector<std::thread> threads;
	for (int i = 0; i < 4; i++) {
		threads.push_back(std::thread(&Thread::complexFunction, this, std::ref(cnts[i].cnt)));
	}

	for (auto& t : threads) {
		if (t.joinable()) {
			t.join();
		}
	}

	for (auto& cnt : cnts) {
		sum += cnt.cnt;
	}
	t.stop("Multi Thread Storage Per Thread With Aligned");
	std::cout << "cnt: " << sum << "\n";
}

void Thread::processBatch(int& sum, std::span<int> batch) {
	long long local_sum = 0;
	for (const int& num : batch) {
		local_sum += num;
	}
	sum += (int)local_sum;
}

void Thread::simpleMultiThreadStoragePerThreadSingleBatch() {
	std::vector<std::vector<int>> dataset = generateDataset();
	t.start();
	std::vector<int> cnts(4, 0);
	{
		std::vector<std::jthread> threads;
		for (int i = 0; i < 4; i++) {
			threads.push_back(std::jthread(&Thread::processBatch, this, std::ref(cnts[i]), std::span<int>(dataset[i])));
		}
	}

	t.stop("Multi Thread Storage Per Thread Single Batch");
	for (int i = 0; i < 4; i++) {
		std::cout << "Batch " << i << " sum: " << cnts[i] << "\n";
	}
}

void Thread::simpleMultiThreadStoragePerThreadMultiBatch() {
	std::vector<std::vector<int>> dataset = generateDataset();
	t.start();
	std::vector<int> cnts(4, 0);

	for (int subpart = 0; subpart < 1000; subpart++) {
		int subset = 100000000 / 1000;
		{
			std::vector<std::jthread> threads;
			for (int j = 0; j < 4; j++) {
				int* start_ptr = &dataset[j][subpart * subset];
				std::span<int> batch_span(start_ptr, subset);
				threads.push_back(std::jthread(& Thread::processBatch, this, std::ref(cnts[j]), std::span<int>(batch_span)));
			}
		}
	}

	t.stop("Multi Thread Storage Per Thread Multi Batch");

	for (int i = 0; i < 4; i++) {
		std::cout << "Batch " << i << " sum: " << cnts[i] << "\n";
	}
}