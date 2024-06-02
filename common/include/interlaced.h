/*! @file common/include/interlaced.h

	Declarations of data types and functions for handling interlaced image.

	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _INTERLACED_H
#define _INTERLACED_H

#if VC5_ENABLED_PART(VC5_PART_LAYERS)

#ifdef __cplusplus
extern "C" {
#endif

CODEC_ERROR ComposeFields(IMAGE image_array[], int frame_count, IMAGE *output_image);

CODEC_ERROR UnpackFields(IMAGE *input_image, IMAGE *top_field, IMAGE *bottom_field);

CODEC_ERROR DecomposeFields(IMAGE *interlaced_image, IMAGE *image_array[], int frame_count);

#ifdef __cplusplus
}
#endif

#endif

#endif
