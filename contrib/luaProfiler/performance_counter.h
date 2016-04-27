#ifndef PERFORMANCE_COUNTER_H
#define PERFORMANCE_COUNTER_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

__forceinline long long get_frequency()
{
	static BOOL inited = FALSE;
	static LARGE_INTEGER frequency;
	if (!inited)
	{
		inited = QueryPerformanceFrequency(&frequency);
	}
	return frequency.QuadPart;
}

__forceinline long long get_current_count()
{
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	return counter.QuadPart;
}

#endif