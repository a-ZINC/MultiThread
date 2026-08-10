#pragma once

#include<cstdint>
#include <atomic>
#include <functional>
#include <thread>
#include"Timer.h"


namespace prac {
	constexpr uint64_t kIters = 200'000'000ULL;
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
}