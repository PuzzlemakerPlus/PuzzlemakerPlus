#pragma once
#include <chrono>
#include <iostream>

// timer that measure time between construction and destruction and prints to console
// 1 line timer useful for debugging
struct ScopedTimer
{
	using clock = std::chrono::high_resolution_clock;
	std::string name;
	clock::time_point start;

	ScopedTimer(std::string name) : name(std::move(name)), start(clock::now()) {}

	~ScopedTimer()
	{
		auto end = clock::now();
		auto ms = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
		std::cout << name << ": " << ms << " microseconds\n";
	}
};
