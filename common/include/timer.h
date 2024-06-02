/*!	@file common/include/timer.h

	Implementation of a high-resolution performance timer.

	Note: The timer is not implemented on all platforms.
	
	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _TIMER_H
#define _TIMER_H

#ifndef _TIMING
#define _TIMING (1 && !defined(_PRODUCTION))
#endif

#ifndef ASSERT
#define ASSERT assert
#endif

#if _TIMING

#if _WIN32

typedef struct timer
{
	__int64 time;

} TIMER;

void InitTimer(TIMER *timer);

void StartTimer(TIMER *timer);

void StopTimer(TIMER *timer);

float TimeSecs(TIMER *timer);

float TimeMS(TIMER *timer);

float TimerPercentage(TIMER *timer1, TIMER *timer2);

#else

// This is not a Windows build
#error Timing code is only available for Windows

#endif

#else

// Define a null timer for use when timing is disabled

typedef void *TIMER;

inline static void InitTimer(TIMER *timer)
{
	(void)timer;
}

inline static void StartTimer(TIMER *timer)
{
	(void)timer;
}

inline static void StopTimer(TIMER *timer)
{
	(void)timer;
}

#endif

#endif
