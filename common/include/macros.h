/*!	@file common/include/macros.h

	Definitions of macros used by the VC-5 codec implementation.

	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _MACROS_H
#define _MACROS_H

#ifndef neg
#define neg(x)	(-(x))
#endif

//TODO: Fix compiler warnings about undefined operation
#define DivideByShift(x, s)		((x) >> (s))

inline static uint16_t clamp_uint16(int32_t value)
{
	if (value < 0)
		value = 0;
	else
	if (value > UINT16_MAX)
		value = UINT16_MAX;

	return (uint16_t)value;
}

inline static uint16_t clamp_uint14(int32_t value)
{
	const int32_t limit = ((1 << 14) - 1);

	if (value < 0)
		value = 0;
	else
	if (value > limit)
		value = limit;

	return (uint16_t)value;
}

inline static uint16_t clamp_uint12(int32_t value)
{
	const int32_t limit = ((1 << 12) - 1);

	if (value < 0)
		value = 0;
	else
	if (value > limit)
		value = limit;

	return (uint16_t)value;
}

inline static uint8_t clamp_uint8(int32_t value)
{
    const int32_t limit = ((1 << 8) - 1);
    
    if (value < 0)
        value = 0;
    else
        if (value > limit)
            value = limit;
    
    return (uint8_t)value;
}

inline static uint16_t clamp_uint(int32_t value, PRECISION precision)
{
	const int32_t limit = ((1 << precision) - 1);

	assert(0 <= value && value <= limit);

	if (value < 0)
		value = 0;
	else
	if (value > limit)
		value = limit;

	return (uint16_t)value;
}

#ifndef _MSC_VER
inline static int min(a, b)
{
    return (a < b) ? a : b;
}
#endif

#endif
