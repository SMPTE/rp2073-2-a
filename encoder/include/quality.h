/*! @file encoder/include/quality.h

	Quality settings.

	Not used in the VC-5 sample encoder.

	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _QUALITY_H
#define _QUALITY_H

typedef enum _encoded_quality
{
	ENCODED_QUALITY_FIXED = 0,
	ENCODED_QUALITY_LOW,
	ENCODED_QUALITY_MEDIUM,
	ENCODED_QUALITY_HIGH,
	ENCODED_QUALITY_FILM_SCAN_1,
	ENCODED_QUALITY_FILM_SCAN_2,

	// Default encoding quality
	ENCODED_QUALITY_DEFAULT = ENCODED_QUALITY_FILM_SCAN_1

} ENCODED_QUALITY;

#endif
