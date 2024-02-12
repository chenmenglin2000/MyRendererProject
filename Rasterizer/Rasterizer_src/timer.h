#pragma once

#include <chrono>
#include <iostream>

struct Timer {
	std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTimepoint;

	Timer() {
		m_StartTimepoint = std::chrono::high_resolution_clock::now();
	}

	~Timer() {
		auto endTimepoint = std::chrono::high_resolution_clock::now();

		//◊™ªª¿‡–Õ°£∫¡√Î£∫millisecond£ªŒ¢√Î£∫microseconds°£
		//long long
		auto start = std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimepoint).time_since_epoch().count();
		auto end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch().count();

		auto duration = end - start;

		double ms = duration * 0.001f;
		std::cout << duration << "us (" << ms << "ms)\n";
	}
};