/*!	@file common/src/timer.c

	Implementation of a high-resolution performance timer

	This module implements an interface to the high-resolution timer on
	Windows.  The same interface can be used for timers on other platforms.
	
	@todo Implement a high-resolution timer on Linux and Macintosh.

	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#include "platform.h"

#if _WIN32

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#ifndef _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES		1
#endif

// Windows Header Files
#include <windows.h>

// Standard C Library
#include <assert.h>

#endif

#ifndef ASSERT
#define ASSERT(x)	assert(x)
#endif

#include "timer.h"

#if _TIMING

//! Frequency of the performance timer (clock ticks per second)
static __int64 frequency = 0;

/*!
	@brief Initialize a timer
	
	The frequency of the performance timer is determined if it has not
	already been obtained.
*/
void InitTimer(TIMER *timer)
{
	// Has the timer frequency been set?
	if (frequency == 0)
	{
		// Set the timer frequency
		if (!QueryPerformanceFrequency((LARGE_INTEGER *)&frequency))
		{
			ASSERT(0);
		}
	}
	ASSERT(frequency > 0);

	// Initialize the timer value to zero
	timer->time = 0;
}

void StartTimer(TIMER *timer)
{
	LARGE_INTEGER current_time;
	QueryPerformanceCounter(&current_time);
	timer->time -= current_time.QuadPart;
}

void StopTimer(TIMER *timer)
{
	LARGE_INTEGER current_time;
	QueryPerformanceCounter(&current_time);
	timer->time += current_time.QuadPart;
}

float TimeSecs(TIMER *timer)
{
	return ((float)timer->time / (float)frequency);
}

float TimeMS(TIMER *timer)
{
	return (1000 * TimeSecs(timer));
}

float Percentage(TIMER *timer1, TIMER *timer2)
{
	if (timer2->time > 0)
	{
		return (float)(100.0 * timer1->time / timer2->time);
	}

	return 0;
}

#endif
