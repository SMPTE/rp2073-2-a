/*!	@file unpack.h

	Routines for unpacking frames that contain packed pixels into separate channels.

	Note that some frame unpacking routines are defined in other modules.

	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _UNPACK_H
#define _UNPACK_H

#ifdef __cplusplus
extern "C" {
#endif

CODEC_ERROR UnpackImageRowYUY2(uint8_t *input, DIMENSION width, PIXEL *buffer[],
							   PRECISION bits_per_component[], int channel_count,
							   ENABLED_PARTS enabled_parts);

CODEC_ERROR UnpackImageRowRG48(uint8_t *input, DIMENSION width, PIXEL *buffer[],
							   PRECISION bits_per_component[], int channel_count,
							   ENABLED_PARTS enabled_parts);

CODEC_ERROR UnpackImageRowB64A(uint8_t *input, DIMENSION width, PIXEL *buffer[],
							   PRECISION bits_per_component[], int channel_count,
							   ENABLED_PARTS enabled_parts);

CODEC_ERROR UnpackImageRowNV12(uint8_t *input, DIMENSION width, PIXEL *buffer[],
							   PRECISION bits_per_component[], int channel_count,
							   ENABLED_PARTS enabled_parts);

CODEC_ERROR UnpackImageNV12(uint8_t *input_buffer,
							DIMENSION width,
							DIMENSION height,
							PIXEL *output_buffer_list[]);

#ifdef __cplusplus
}
#endif

#endif
