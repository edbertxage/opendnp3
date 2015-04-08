
#ifndef ASIOPAL_STEADYCLOCK_H
#define ASIOPAL_STEADYCLOCK_H

#include <chrono>

namespace asiopal {

#if (defined WIN32 && (_MSC_VER <= 1800)) // Windows with VS 2013 or earlier

#include <Windows.h>

/*
Custom steady clock implementation to handle the situation where the Windows steady clock
implementation is not monotonic. Normal steady clock implementation is used on other platforms.
*/
struct SteadyClockWindows
{
	// Type defininitions for this clock - ticks are in nanoseconds for this clock
	typedef LONGLONG representation;
	typedef std::ratio_multiply<std::ratio<1, 1>, std::nano> period;
	typedef std::chrono::duration<representation, period> duration;
	typedef std::chrono::time_point<SteadyClockWindows, duration> time_point;


	static time_point now() {
		// Special case for WIN32 because std::chrono::steady_clock is broken
		LARGE_INTEGER freq;
		LONGLONG nanoSecondsPerTick = 0;
		if (QueryPerformanceFrequency(&freq)) {
			nanoSecondsPerTick = LONGLONG(1000000000.0L / freq.QuadPart);
		}

		LARGE_INTEGER performanceCount;
		if (nanoSecondsPerTick <= 0 || !QueryPerformanceCounter(&performanceCount)) {
			// Error condition, return 0 time
			return time_point();
		}

		// Return time in nanoseconds
		LONGLONG timeNanos = performanceCount.QuadPart * nanoSecondsPerTick;
		return time_point(duration(timeNanos));
	}
};

// use this custom steady clock
typedef SteadyClockWindows asiopal_steady_clock;

#else

// In all other situations use the normal steady clock
typedef std::chrono::steady_clock asiopal_steady_clock;
	
#endif

}

#endif