#pragma once

#include<iostream>
#include<chrono>
#include<string>

class Timer {
private:
	std::chrono::time_point<std::chrono::high_resolution_clock> scopeStartTime, scopeEndTime;
	std::chrono::duration<double> scopeDuration;

	std::chrono::time_point<std::chrono::high_resolution_clock>  subStartTime, subEndTime;
	std::chrono::duration<double> subDuration;

public:
	Timer();
	~Timer();

	void start();
	float stop(std::string subParts);
};
