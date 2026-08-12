#pragma once

#include<cstdint>
#include <atomic>
#include <functional>
#include <thread>
#include"Timer.h"
#include <vector>
#include <mutex>


namespace prac {
	constexpr uint64_t kIters = 200'000'000ULL;
	constexpr size_t kSize = 200'000'000;
	struct SharedLine {
		alignas(8) std::atomic<uint64_t> a{ 0 };
		alignas(8) std::atomic<uint64_t> b{ 0 };
	};

	struct PaddedLine {
		alignas(64) std::atomic<uint64_t> a{ 0 };
		char padding[64 - sizeof(std::atomic<uint64_t>)];
		alignas(64) std::atomic<uint64_t> b{ 0 };
		char padding2[64 - sizeof(std::atomic<uint64_t>)];
	};

	class FalseSharing {
		template <typename T>
		double runTest(const char* label, Timer t) {
			T data;
			auto worker1 = [&data]() {
				for (uint64_t i = 0; i < kIters; ++i) {
					data.a.fetch_add(1, std::memory_order_relaxed);
				}
			};
			std::function<void()> worker2 = [&data]() {
				for (uint64_t i = 0; i < kIters; ++i) {
					data.b.fetch_add(1, std::memory_order_relaxed);
				}
			};

			t.start();
			std::thread t1(worker1);
			std::thread t2(worker2);
			t1.join();
			t2.join();
			double count = t.stop("", false);
			double m_incs = (2.0 * kIters) / (count * 1e6);

			std::cout << label << " - Time: " << count << " seconds, Throughput: " << m_incs << " M increments/sec\n";

			if (data.a.load(std::memory_order_relaxed) != kIters || data.b.load(std::memory_order_relaxed) != kIters) {
				std::cerr << "Error: Incorrect final values!\n";
			}
			return count;
		}

	public:

		void run(Timer t) {
			double sec1 = runTest<SharedLine>("SharedLine", t);
			double sec2 = runTest<PaddedLine>("Padded", t);
			std::cout << "Speedup: " << sec1 / sec2 << "x\n";
		}
	};

	class CompilerHazard {
		struct nonAtomic {
			bool ready = false;
		};
		struct atomic {
			std::atomic<bool> ready{ false };
		};

		template<typename T>
		void runTest(const char* label) {
			T data;
			std::thread t1([&data]() {
				std::this_thread::sleep_for(std::chrono::milliseconds(7000));
				data.ready = true;
			});
			while (!data.ready) {

			}

			std::cout << label << " - Finished waiting for ready flag.\n";

			t1.join();
		}
	public:
		void run() {
			std::thread t1(&CompilerHazard::runTest<nonAtomic>, this, "nonAtomic");
			std::thread t2(&CompilerHazard::runTest<atomic>, this, "atomic");

			t1.join();
			t2.join();
		}
	};

	class LockFreeCheck {
		struct OneLong { int a; alignas(8) char b;  };
		struct TwoLong { long long a; long long b; };
		struct ThreeLong { long long a; long long b; long long c; };

	public:
		void run() {
			std::atomic<OneLong> atomicOneLong;
			std::atomic<TwoLong> atomicTwoLong;
			std::atomic<ThreeLong> atomicThreeLong;
			std::atomic<bool> flag;
			std::atomic<int> atomicInt;

			std::cout << "Is OneLong lock-free? " << atomicOneLong.is_lock_free() << ", always: " << std::atomic<OneLong>::is_always_lock_free << ", size: " << sizeof(OneLong) << "\n";
			std::cout << "Is TwoLong lock-free? " << atomicTwoLong.is_lock_free() << ", always: " << std::atomic<TwoLong>::is_always_lock_free << ", size: " << sizeof(TwoLong) << "\n";
			std::cout << "Is ThreeLong lock-free? " << atomicThreeLong.is_lock_free() << ", always: " << std::atomic<ThreeLong>::is_always_lock_free << ", size: " << sizeof(ThreeLong) << "\n";
			std::cout << "Is bool lock-free? " << flag.is_lock_free() << ", always: " << std::atomic<bool>::is_always_lock_free << ", size: " << sizeof(bool) << "\n";
			std::cout << "Is int lock-free? " << atomicInt.is_lock_free() << ", always: " << std::atomic<int>::is_always_lock_free << ", size: " << sizeof(int) << "\n";
		}

	};

	class Sum {
	private:
		std::vector<int> x;
		size_t chunk = kSize / 4;
		std::atomic<long long> naiveSum{ 0 };
		std::mutex mtx;
		long long sum = 0;
		std::atomic<long long> atomicSum{ 0 };
		long long naiveMutexSum = 0;
		Timer t;

	private:
		double naiveSum_() {
			t.start();
			std::function<void(size_t start, size_t end)> worker = [this](size_t start, size_t end) {
				for (size_t i = start; i < end; ++i) {
					naiveSum.fetch_add(x[i], std::memory_order_relaxed);
				}
				};
			std::vector<std::thread> threads;
			threads.reserve(4);
			for (size_t i = 0; i < 4; ++i) {
				size_t start = i * chunk;
				size_t end = (i + 1) * chunk;
				threads.push_back(std::thread(worker, start, end));
			}
			for (int i = 0; i < 4; i++) {
				threads[i].join();
			}
			return t.stop("Naive Sum", false);
		}

		double naiveMutex_() {
			t.start();
			std::function<void(size_t start, size_t end)> worker = [this](size_t start, size_t end) {
				for (size_t i = start; i < end; ++i) {
					std::lock_guard<std::mutex> lock(mtx);
					naiveMutexSum += x[i];
				}
				};
			std::vector<std::thread> threads;
			threads.reserve(4);
			for (size_t i = 0; i < 4; ++i) {
				size_t start = i * chunk;
				size_t end = (i + 1) * chunk;
				threads.push_back(std::thread(worker, start, end));
			}

			for (int i = 0; i < 4; i++) {
				threads[i].join();
			}
			return t.stop("Naive mutex sum", false);
		}

		double mutexSum() {
			t.start();
			std::function<void(size_t start, size_t end)> worker = [this](size_t start, size_t end) {
				long long localSum = 0;
				for (size_t i = start; i < end; ++i) {
					localSum += x[i];
				}
				std::lock_guard<std::mutex> lock(mtx);
				sum += localSum;
				};
			std::vector<std::thread> threads;
			threads.reserve(4);
			for (size_t i = 0; i < 4; ++i) {
				size_t start = i * chunk;
				size_t end = (i + 1) * chunk;
				threads.push_back(std::thread(worker, start, end));
			}
			for (int i = 0; i < 4; i++) {
				threads[i].join();
			}
			return t.stop("Mutex sum", false);
		}

		double atomicSum_() {
			t.start();
			std::function<void(size_t start, size_t end)> worker = [this](size_t start, size_t end) {
				long long localSum = 0;
				for (size_t i = start; i < end; ++i) {
					localSum += x[i];
				}
				atomicSum.fetch_add(localSum, std::memory_order_relaxed);
				};
			std::vector<std::thread> threads;
			threads.reserve(4);
			for (size_t i = 0; i < 4; ++i) {
				size_t start = i * chunk;
				size_t end = (i + 1) * chunk;
				threads.push_back(std::thread(worker, start, end));
			}
			for (int i = 0; i < 4; i++) {
				threads[i].join();
			}
			return t.stop("Atomic sum", false);
		}



	public:
		Sum(Timer& t) : x(kSize, 1), t(t) {}
		void run() {
			double naiveTime = naiveSum_();
			double naiveMutex = naiveMutex_();
			double mutexTime = mutexSum();
			double atomicTime = atomicSum_();

			std::cout <<"Naive sum: " << naiveSum <<  ", time: " << naiveTime << " s\n";
			std::cout << "Naive Mutex sum: " << naiveMutexSum << ", time: " << naiveMutex << " s\n";
			std::cout << "Mutex sum: " << sum << ", time: " << mutexTime << " s\n";
			std::cout << "Atomic sum: " << atomicSum << ", time: " << atomicTime << " s\n";

			std::cout << "Speedup (Naive Mutex/Atomic): " <<  naiveMutex / naiveTime << "x\n";
			std::cout << "Speedup (Mutex/Atomic): " << mutexTime/ atomicTime << "x\n";
		}

	};

	struct node {
		int val;
		node* next;
	};

	class LockFreeStack {
	private:
		std::atomic<node*> head{ nullptr };
	public:
		node* pop() {
			node* old = head.load();
			while (old && !head.compare_exchange_weak(old, old->next, std::memory_order_release, std::memory_order_relaxed)) {}
			return old;
		}

		void push(node* n) {
			node* old = head.load();
			do {
				n->next = old;
			} while (!head.compare_exchange_weak(old, n, std::memory_order_release, std::memory_order_relaxed));
		}
	};

	class LockFreeStackTest {
	private:
		int kworker = 4;
		int kitr = 500'000;
	public:
		void run() {
			LockFreeStack st;
			std::atomic<long long> sum{ 0 };
			std::atomic<long long> count{ 0 };

			std::function<void()> workerPush = [this, &st]() {
				for (int i = 0; i < kitr; i++) {
					st.push(new node{ i, nullptr });
				}
			};

			std::function<void()> workerPop = [this, &st, &count, &sum]() {
				while(node* value = st.pop()){
					count.fetch_add(1, std::memory_order_relaxed);
					sum.fetch_add(value->val, std::memory_order_relaxed);
					delete value;
				}
			};
			{

				std::vector<std::thread> threads;
				threads.reserve(4);
				for (int i = 0; i < kworker; i++) {
					threads.emplace_back(workerPush);
				}

				for (int i = 0; i < kworker; i++) {
					threads[i].join();
				}
			}

			{

				std::vector<std::thread> threads;
				threads.reserve(4);
				for (int i = 0; i < kworker; i++) {
					threads.emplace_back(workerPop);
				}

				for (int i = 0; i < kworker; i++) {
					threads[i].join();
				}
			}

			long long expectedCount = (long long)kitr * kworker;
			long long expectedSum = ((long long)kitr * (kitr - 1) / 2) * kworker;

			std::cout << "Expected count: " << expectedCount << " ,Observed count: " << count.load() << std::endl;
			std::cout << "Expected sum: " << expectedSum << " ,Observed sum: " << sum.load() << std::endl;
		}
	};
}