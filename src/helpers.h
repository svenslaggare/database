#pragma once
#include <string>
#include <chrono>

using TimePoint = std::chrono::time_point<std::chrono::system_clock>;

namespace Helpers {

	/*
	 * Returns the current time
	 */
	TimePoint timeNow();

	/**
	 * Returns the duration in microseconds between x and y
	 * @param x The first point in time
	 * @param y The second point in time
	 */
	long durationMicroseconds(TimePoint x, TimePoint y);

	/**
	 * Returns the duration in milliseconds between x and y
	 * @param x The first point in time
	 * @param y The second point in time
	 */
	double durationMilliseconds(TimePoint x, TimePoint y);
}

/**
 * Represents a timing class
 */
struct Timing {
	TimePoint startTime = Helpers::timeNow();
	std::string before;

	explicit Timing(const std::string& before = "");
	~Timing();
};
