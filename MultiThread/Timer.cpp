#include"Timer.h"


	Timer::Timer() {
		scopeStartTime = std::chrono::high_resolution_clock::now();
	}
	Timer::~Timer() {
		scopeEndTime = std::chrono::high_resolution_clock::now();
		scopeDuration = scopeEndTime - scopeStartTime;
		std::cout << "[Scope]: " << scopeDuration.count() << " seconds\n";
	}

	void Timer::start() {
		subStartTime = std::chrono::high_resolution_clock::now();
	}
	void Timer::stop(std::string subParts) {
		subEndTime = std::chrono::high_resolution_clock::now();
		subDuration = subEndTime - subStartTime;
		std::cout << "[" << subParts << "]: " << subDuration.count() << " seconds\n";
	}