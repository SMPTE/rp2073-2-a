/*!	@file common/include/pixel.h

	The pixel format enumerations define the pixel packing formats that are
	supported by the codec for input to the image unpacking process and for
	output from the image repacking process.
	
	The image upacking process converts a picture into component arrays that
	are input to the encoding process.  The image repacking process convert the
	component arrays outptu by the decoding process into a displayable picture.

	@todo Which formats are supported in each profile.

	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _PIXEL_H
#define _PIXEL_H


//! Data type for pixels
typedef int16_t PIXEL;

//! Minimum and maximum pixel values
enum {
	PIXEL_MIN = INT16_MIN,
	PIXEL_MAX = INT16_MAX,
};


//! Alternative definition for wavelet coefficients
typedef int16_t COEFFICIENT;

//! Minimum and maximum coefficient values
enum
{
	COEFFICIENT_MIN = INT16_MIN,
	COEFFICIENT_MAX = INT16_MAX,
};


/*!
	@brief Pixels formats supported by the codec

	The pixel format is only the packing arrangement for color components and
	does not specify whether the image is interlaced or the bottom row is first.

	@todo Need to add support for more pixel formats to the reference decoder
*/
typedef enum _pixel_format
{
	PIXEL_FORMAT_UNKNOWN = 0,

	PIXEL_FORMAT_UYVY = 1,
	PIXEL_FORMAT_YUYV = 2,
	PIXEL_FORMAT_YVYU = 3,
	PIXEL_FORMAT_RGB24 = 7,
	PIXEL_FORMAT_RGB32 = 8,

	PIXEL_FORMAT_V210 = 10,
	PIXEL_FORMAT_YU64,
	PIXEL_FORMAT_YR16,

	PIXEL_FORMAT_RG48 = 120,
	PIXEL_FORMAT_B64A,

	//TODO: The pixel formats must have the same values as the current codec implementation

	//PIXEL_FORMAT_RG64,
	//PIXEL_FORMAT_WP13,
	//PIXEL_FORMAT_W13A,
	//PIXEL_FORMAT_RG30,
	//PIXEL_FORMAT_R210,
	//PIXEL_FORMAT_AR10,
	//PIXEL_FORMAT_AB10,

	//! YCrCb 4:2:0 with one luma plane and one packed color difference plane
	PIXEL_FORMAT_NV12 = 16,

	// Cineon pixel formats (packed 10-bit RGB is the most comon)
	PIXEL_FORMAT_DPX_50 = 128,		//!< RGB 10-bit values in a 32-bit word

	// Component arrays in separate files (no file header, filename includes channel number)
	PIXEL_FORMAT_CA32 = 256,		//!< Arrays of 16-bit unsigned component values

	//! Reserve a block of pixel formats for Bayer
	PIXEL_FORMAT_BAYER = 100,
	PIXEL_FORMAT_BYR1 = 101,		//!< Bayer 8 bits per channel
	PIXEL_FORMAT_BYR2 = 102,		//!< Bayer 10 bits per channel
	PIXEL_FORMAT_BYR3 = 103,		//!< Bayer 10 bits per channel planar
	PIXEL_FORMAT_BYR4 = 104,		//!< Bayer 16 bits per channel
	PIXEL_FORMAT_BYR5 = 105,		//!< Bayer 12 bits per channel planar 8/4

	//! Input pixel formats above this value must be encoded into the sample
	PIXEL_FORMAT_TAG_REQUIRED = 100,

	//! Alternative names
	PIXEL_FORMAT_DPX0 = PIXEL_FORMAT_DPX_50,
	PIXEL_FORMAT_YUY2 = PIXEL_FORMAT_YUYV,

} PIXEL_FORMAT;


#ifdef __cplusplus
extern "C" {
#endif

inline static bool IsBayerFormat(PIXEL_FORMAT format)
{
	switch (format)
	{
	case PIXEL_FORMAT_BAYER:
	case PIXEL_FORMAT_BYR1:
	case PIXEL_FORMAT_BYR2:
	case PIXEL_FORMAT_BYR3:
	case PIXEL_FORMAT_BYR4:
	case PIXEL_FORMAT_BYR5:
		return true;
		break;

	default:
		return false;
		break;
	}
}

const char *PixelFormatName(PIXEL_FORMAT format);

PIXEL_FORMAT PixelFormat(const char *string);

PIXEL ClampPixel(int32_t value);

#ifdef __cplusplus
}

#endif
#endif
