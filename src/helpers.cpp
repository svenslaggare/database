#include <iostream>
#include "helpers.h"

TimePoint Helpers::timeNow() {
	return std::chrono::system_clock::now();
}

long Helpers::durationMicroseconds(TimePoint x, TimePoint y) {
	return std::chrono::duration_cast<std::chrono::microseconds>(x - y).count();
}

double Helpers::durationMilliseconds(TimePoint x, TimePoint y) {
	return durationMicroseconds(x, y) / 1E3;
}

Timing::Timing(const std::string& before)
	: before(before) {

}

Timing::~Timing() {
	std::cout << before << Helpers::durationMilliseconds(Helpers::timeNow(), startTime) << " ms" << std::endl;
}
