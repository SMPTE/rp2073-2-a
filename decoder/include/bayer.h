/*!	@file decoder/include.bayer.h
	Declaration of routines for packing a row of pixels into a Bayer image.
	Note that the packing routines for non-Bayer image formats are defined
	in other files.

	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _BAYER_H
#define _BAYER_H

#ifdef __cplusplus
extern "C" {
#endif

CODEC_ERROR PackBufferRowsToBYR3(PIXEL *input_buffer, size_t input_pitch,
								 PIXEL *output_buffer, size_t output_pitch,
								 DIMENSION width, DIMENSION height,
                                 ENABLED_PARTS enabled_parts);

CODEC_ERROR PackBufferRowsToBYR4(PIXEL *input_buffer, size_t input_pitch,
								 PIXEL *output_buffer, size_t output_pitch,
								 DIMENSION width, DIMENSION height,
                                 ENABLED_PARTS enabled_parts);

CODEC_ERROR PackComponentsToBYR4(const UNPACKED_IMAGE *image,
								 PIXEL *output_buffer, size_t output_pitch,
								 DIMENSION width, DIMENSION height,
                                 ENABLED_PARTS enabled_parts);

CODEC_ERROR PackComponentsToRG48(const UNPACKED_IMAGE *image,
								 PIXEL *output_buffer, size_t output_pitch,
								 DIMENSION width, DIMENSION height,
                                 ENABLED_PARTS enabled_parts);

CODEC_ERROR PackComponentsToB64A(const UNPACKED_IMAGE *image,
								 PIXEL *output_buffer, size_t output_pitch,
								 DIMENSION width, DIMENSION height,
                                 ENABLED_PARTS enabled_parts);
    
CODEC_ERROR PackComponentsToNV12(const UNPACKED_IMAGE *image,
                                 PIXEL *output_buffer, size_t output_pitch,
                                 DIMENSION width, DIMENSION height,
                                 ENABLED_PARTS enabled_parts);

CODEC_ERROR PackComponentsToDPX0(const UNPACKED_IMAGE *image,
								 PIXEL *output_buffer, size_t output_pitch,
								 DIMENSION width, DIMENSION height,
                                 ENABLED_PARTS enabled_parts);

#ifdef __cplusplus
}
#endif

#endif
