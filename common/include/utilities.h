/*!	@file utilities.h

	Utility routines used by the code for testing the codec.

	The code in this file is not part of teh VC-5 standard.

	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _UTILITIES_H
#define _UTILITIES_H

#include "fileinfo.h"
#include "parseargs.h"

#ifdef __cplusplus
extern "C" {
#endif

//CODEC_ERROR ReadImage(IMAGE *image, const char *pathname);
PIXEL_FORMAT PixelFormat(const char *string);

CODEC_ERROR ReadImage(IMAGE *image, const char *pathname);

CODEC_ERROR ReadImageFile(IMAGE *image,
						  DIMENSION image_width,
						  DIMENSION image_height,
						  PIXEL_FORMAT image_format,
						  const char *pathname);


#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
const char *ImageFormatString(IMAGE_FORMAT image_format);
#endif

CODEC_ERROR CheckEnabledParts(ENABLED_PARTS *enabled_parts_ref);

CODEC_ERROR VerifyEnabledParts(ENABLED_PARTS enabled_parts);

CODEC_ERROR PrintEnabledParts(ENABLED_PARTS enabled_parts);

#ifdef __cplusplus
}
#endif

#endif
