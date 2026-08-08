#include"Timer.h"


	Timer::Timer() {
		//scopeStartTime = std::chrono::high_resolution_clock::now();
	}
	Timer::~Timer() {
		//scopeEndTime = std::chrono::high_resolution_clock::now();
		//scopeDuration = scopeEndTime - scopeStartTime;
		//std::cout << "[Scope]: " << scopeDuration.count() << " seconds\n";
	}

	void Timer::start() {
		subStartTime = std::chrono::high_resolution_clock::now();
	}
	float Timer::stop(std::string subParts, bool flag = false) {
		subEndTime = std::chrono::high_resolution_clock::now();
		subDuration = subEndTime - subStartTime;
		if (flag) {
			std::cout << "[" << subParts << "]: " << subDuration.count() << " seconds\n";
		}
		return subDuration.count();
	}