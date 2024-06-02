/*!	@file common/src/pixel.c

	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#include "headers.h"

#ifndef WIN32
#define stricmp strcasecmp
#endif

/*!
	@brief Force a pixel value to be in range
*/
PIXEL ClampPixel(int32_t value)
{
	// Check for values that are outside the range (for debugging)
	assert(PIXEL_MIN <= value && value <= PIXEL_MAX);

	if (value < PIXEL_MIN)
		value = PIXEL_MIN;
	else
	if (value > PIXEL_MAX)
		value = PIXEL_MAX;

	return (PIXEL)value;
}

/*!
	@brief Return a printable string for the pixel format
*/
const char *PixelFormatName(PIXEL_FORMAT format)
{
	static char name[1024];

	switch (format)
	{
	case PIXEL_FORMAT_BYR3:
		strcpy(name, "BYR3");
		break;

	case PIXEL_FORMAT_BYR4:
		strcpy(name, "BYR4");
		break;

	case PIXEL_FORMAT_DPX_50:
		strcpy(name, "DPX0");
		break;

	case PIXEL_FORMAT_YUY2:
		strcpy(name, "YUY2");
		break;

	case PIXEL_FORMAT_RG48:
		strcpy(name, "RG48");
		break;

	case PIXEL_FORMAT_B64A:
		strcpy(name, "B64A");
		break;

	case PIXEL_FORMAT_NV12:
		strcpy(name, "NV12");
		break;

	default:
		strcpy(name, "unknown");
		break;
	}

	// Return a pointer to a static memory location
	return name;
}

/*!
	@brief Translate the string representation of a pixel format into the enumerated value
*/
PIXEL_FORMAT PixelFormat(const char *string)
{
	struct _pixel_format_table_entry
	{
		char *string;
		PIXEL_FORMAT format;
	};

	static const struct _pixel_format_table_entry pixel_format_table[] =
	{
		{"byr3", PIXEL_FORMAT_BYR3},
		{"byr4", PIXEL_FORMAT_BYR4},
		{"rg48", PIXEL_FORMAT_RG48},
        {"b64a", PIXEL_FORMAT_B64A},
	};

	static const int pixel_format_table_length = sizeof(pixel_format_table) / sizeof(pixel_format_table[0]);

	int index;

	for (index = 0; index < pixel_format_table_length; index++)
	{
		if (stricmp(string, pixel_format_table[index].string) == 0) {
			return pixel_format_table[index].format;
		}
	}

	// Did not find the format string in the pixel format table
	return PIXEL_FORMAT_UNKNOWN;
}
