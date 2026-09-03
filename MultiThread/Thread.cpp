#include"Thread.h"
#include"TaskQueue.h"
#include"TaskPool.h"


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

std::vector<Chunk> Thread::generateRandomDataSet() {
	std::vector<Chunk> chunks;
	chunks.reserve(CHUNK_COUNT);

	std::mt19937 rnd(2828);
	std::uniform_real_distribution<double> v_dist{ 0, std::numbers::pi };
	std::bernoulli_distribution h_dist{ HEAVY_PROBABILITY };

	for (size_t c = 0; c<CHUNK_COUNT; c++) {
		Chunk chunk;
		for (auto& task : chunk) {
			task.val = v_dist(rnd);
			task.heavy = h_dist(rnd);
		}
		chunks.push_back(chunk);
	}
	return chunks;
}

std::vector<Chunk> Thread::generateEvenlyDataSet() {
	std::vector<Chunk> chunks;
	chunks.reserve(CHUNK_COUNT);

	std::mt19937 rnd(2828);
	std::uniform_real_distribution<double> v_dist{ 0, std::numbers::pi };
	int nth = static_cast<int>(1.0 / HEAVY_PROBABILITY);
	for (size_t c = 0; c < CHUNK_COUNT; c++) {
		Chunk chunk;
		for (auto it = 0; it<chunk.size(); it++) {
			chunk[it].val = v_dist(rnd);
			chunk[it].heavy = (it % nth == 0);
		}
		chunks.push_back(chunk);
	}
	return chunks;
}

std::vector<Chunk> Thread::generateStackedDataSet() {
	std::vector<Chunk> chunks = generateEvenlyDataSet();
	for (auto& chunk : chunks) {
		std::ranges::partition(chunk, [](const Task& t) {return t.heavy; });
	}
	return chunks;
}

std::vector<tq::Chunk> generateEvenlyDataSeti() {
	std::vector<tq::Chunk> chunks;
	chunks.reserve(CHUNK_COUNT);

	std::mt19937 rnd(2828);
	std::uniform_real_distribution<double> v_dist{ 0, std::numbers::pi };
	int nth = static_cast<int>(1.0 / HEAVY_PROBABILITY);
	for (size_t c = 0; c < CHUNK_COUNT; c++) {
		tq::Chunk chunk;
		for (auto it = 0; it < chunk.size(); it++) {
			chunk[it].val = v_dist(rnd);
			chunk[it].heavy = (it % nth == 0);
		}
		chunks.push_back(chunk);
	}
	return chunks;
}
std::vector<tq::Chunk> generateStackedDataSeti() {
	std::vector<tq::Chunk> chunks = generateEvenlyDataSeti();
	for (auto& chunk : chunks) {
		std::ranges::partition(chunk, [](const tq::Task& t) {return t.heavy; });
	}
	return chunks;
}

void Thread::singleThread() {
	t.start();
	int cnt = 0;
	for (int i = 0; i < 4; i++) {
		complexFunction(cnt);
	}
	t.stop("Single Thread", true);
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
	t.stop("Multi Thread Race Condition", true);
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
	t.stop("Multi Thread Mutex", true);
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
	t.stop("Multi Thread Storage Per Thread", true);
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
	t.stop("Multi Thread Storage Per Thread With Aligned", true);
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

	t.stop("Multi Thread Storage Per Thread Single Batch", true);
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

	t.stop("Multi Thread Storage Per Thread Multi Batch", true);

	for (int i = 0; i < 4; i++) {
		std::cout << "Batch " << i << " sum: " << cnts[i] << "\n";
	}
}

void Thread::simpleMultiThreadStoragePerThreadMultiBatchWithMasterWorker() {
	std::vector<std::vector<int>> dataset = generateDataset();
	t.start();
	std::vector<int> cnts(4, 0);
	int64 sum = 0;
	Master master(4);
	std::vector<std::unique_ptr<Worker>> workers;
	
	workers.reserve(4);
	for (int i = 0; i < 4; i++) {
		workers.push_back(std::make_unique<Worker>(&master));
	}

	for (int subpart = 0; subpart < 1000; subpart++) {
		int subset = 100000000 / 1000;
		for (int j = 0; j < 4; j++) {
			int* start_ptr = &dataset[j][subpart * subset];
			std::span<int> batch_span(start_ptr, subset);
			workers[j]->setJob(batch_span, &cnts[j]);
		}
		master.wait_for_all();
		for (int s : cnts) sum += s;
	}

	for (int i = 0; i < cnts.size(); i++) {
		workers[i]->kill();
	}
	workers.clear();
	t.stop("Multi Thread Storage Per Thread Multi Batch With Master-Worker", true);
	for (int i = 0; i < 4; i++) {
		std::cout << "Batch " << i << " sum: " << cnts[i] << "\n";
	}

}

void Thread::simpleMultiThreadStoragePerThreadMultiBatchWithMasterWorkerController(char s) {
	std::vector<Chunk> chunks = (s == 'r') ? generateRandomDataSet() : (s == 'e') ? generateEvenlyDataSet() : generateStackedDataSet();
	t.start();
	std::vector<std::unique_ptr<WorkerController>> workers;
	workers.reserve(4);
	MasterController master(4);
	std::vector<unsigned int> outputs(4, 0.0);
	std::vector<ChunkTimingInfo> chunkTimings;
	chunkTimings.reserve(CHUNK_COUNT);
	for (int i = 0; i < 4; i++) {
		workers.push_back(std::make_unique<WorkerController>(&master, t));
	}

	for (auto& chunk : chunks) {
		t.start();
		for (int j = 0; j < 4; j++) {
			std::span<Task> batch(chunk.begin() + j * (CHUNK_SIZE / 4), chunk.begin() + (j + 1) * (CHUNK_SIZE / 4));
			workers[j]->setJob(batch, &outputs[j]);
		}
		master.wait_for_all();
		float chunkTime = t.stop("Chunk Processing Time", false);
		ChunkTimingInfo info;
		for (int i = 0; i < 4; i++) {
			info.timeSpentPerWorker[i] = workers[i]->getTimeSpentPerWorker();
			info.heavyCountPerWorker[i] = workers[i]->getHeavyCount();
		}
		info.totalChunkTime = chunkTime;
		chunkTimings.push_back(info);
	}
	workers.clear();

	unsigned int sum = 0;

	for (int i = 0; i < 4; i++) {
		sum += outputs[i];
	}
	std::cout << "Total sum: " << sum << "\n";
	t.stop("Multi Thread Storage Per Thread Multi Batch With Master-Worker Controller", true);

	std::ofstream outFile(std::format("chunk_timings_{}.csv", s), std::ios_base::trunc);
	for (int i = 0; i < 4; i++) {
		outFile << std::format("time_worker_{0:},idle_time_{0:},heavy_{0:},", i);
	}
	outFile << "total_chunk_time,total_idle,total_heavy\n";

	for (const auto& info : chunkTimings) {
		float totalIdleTime = 0.f;
		int totalHeavyCount = 0;
		for (int i = 0; i < 4; i++) {
			float idleTime = info.totalChunkTime - info.timeSpentPerWorker[i];
			outFile << std::format("{},{},{},", info.timeSpentPerWorker[i], idleTime, info.heavyCountPerWorker[i]);
			totalIdleTime += idleTime;
			totalHeavyCount += info.heavyCountPerWorker[i];
		}
		outFile << std::format("{},{},{}\n", info.totalChunkTime, totalIdleTime, totalHeavyCount);
	}
}

void Thread::simpleMultiThreadTaskQueue() {
	std::vector<tq::Chunk> chunks = generateStackedDataSeti();
	t.start();
	tq::Master master;
	std::vector<std::unique_ptr<tq::Worker>> workers;
	std::vector<ChunkTimingInfo> chunkTimings;
	chunkTimings.reserve(CHUNK_COUNT);
	for (int i = 0; i < WORKER_COUNT; i++) {
		workers.push_back(std::make_unique<tq::Worker>(&master, &t));
	}

	for (auto& chunk : chunks) {
		t.start();
		master.setChunk(chunk);
		for (auto& worker : workers) {
			worker->startWorking();
		}
		master.wait_for_all();
		float totalTime = t.stop("Chunk Processing Time", false);

		ChunkTimingInfo cTiming;
		for (int i = 0; i < WORKER_COUNT; i++) {
			cTiming.timeSpentPerWorker[i] = workers[i]->getTimeSpentPerWorker();
			cTiming.heavyCountPerWorker[i] = workers[i]->getHeavyCount();
		}
		cTiming.totalChunkTime = totalTime;
		chunkTimings.push_back(cTiming);
	}
	unsigned int answer = 0.0;
	for (auto& w : workers) answer += w->GetResult();
	std::cout << "Total sum: " << answer << "\n";

	for (auto& w : workers) w->Kill();
	workers.clear();
	t.stop("Multi Thread Task Queue", true);

	std::ofstream outFile("chunk_timings_task_queue.csv", std::ios_base::trunc);
	for (int i = 0; i < 4; i++) {
		outFile << std::format("time_worker_{0:},idle_time_{0:},heavy_{0:},", i);
	}
	outFile << "total_chunk_time,total_idle,total_heavy\n";

	for (auto& info : chunkTimings) {
		float totalIdleTime = 0.f;
		int totalHeavyCount = 0;
		for (int i = 0; i < 4; i++) {
			float idleTime = info.totalChunkTime - info.timeSpentPerWorker[i];
			outFile << std::format("{},{},{},", info.timeSpentPerWorker[i], idleTime, info.heavyCountPerWorker[i]);
			totalIdleTime += idleTime;
			totalHeavyCount += info.heavyCountPerWorker[i];
		}
		outFile << std::format("{},{},{}\n", info.totalChunkTime, totalIdleTime, totalHeavyCount);
	};
}

void Thread::simpleMultiThreadTaskPool() {
	//Task_ t1 = [] {
	//	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	//	std::ostringstream ss;
	//	ss << std::this_thread::get_id();
	//	std::cout << std::format("<< {} >>", ss.str());
	//};

	//TaskPool tp;
	//tp.Run(t1);
	//tp.Run(t1);
	//tp.Run(t1);
	//tp.Run(t1);
	//std::this_thread::sleep_for(std::chrono::milliseconds(600));
	//std::cout << std::endl;
	//tp.Run(t1);
	//tp.Run(t1);
	//tp.Run(t1);
	//tp.Run(t1);


	//std::this_thread::sleep_for(std::chrono::milliseconds(160));
}

void Thread::simpleMultiThreadQueueTaskPool() {
	int cnt = 1;
	auto t1 = [&cnt](int x) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		std::ostringstream ss;
		ss << std::this_thread::get_id();
		int mul = x * cnt;
		std::cout << std::format("<< {}, mul: {}, cnt: {} >>\n", ss.str(), mul, cnt);
		return mul;
		};

	auto [task, fut] = Task_::make(t1, 3);

	QueueTaskPool tp(2, 4);
	tp.Run(std::move(task));
	int a = fut.get();
	std::cout << "fut.get(): " << a << std::endl;
	//tp.Run(std::move(task));
	//int b = fut.get();
	//std::cout << "fut.get(): " << b << std::endl;
	//std::this_thread::sleep_for(std::chrono::milliseconds(500));
	//tp.Run(std::move(task));
	//std::this_thread::sleep_for(std::chrono::milliseconds(500));
	//tp.Run(std::move(task));

	//tp.waitTP();
}

void Thread::simpleFuturePromise() {
	Promise<int> prom;
	Future<int> future = prom.getFuture();

	std::thread([p = std::move(prom)]() mutable {
		try {
			std::this_thread::sleep_for(std::chrono::milliseconds(2000));
			p.set(2);
		}
		catch (const std::exception& e) {
			std::cerr << "Exception in worker thread: " << e.what() << std::endl;
		}
		catch (...) {
			std::cerr << "Unknown exception in worker thread!" << std::endl;
		}
	}).detach();

	std::cout << "val: " << future.get() << std::endl;

	Promise<int> prom2;
	Future<int> future2 = prom2.getFuture();

	auto [task, fut] = Task_::make([](int x) {
		std::this_thread::sleep_for(std::chrono::milliseconds(2000));
		return x * 2;
		}, 3);

	std::thread t{ std::move(task)};
	std::cout << "val2: " << fut.get() << std::endl;

	t.join();
}
