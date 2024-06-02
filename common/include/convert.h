/*!	@file convert.h

	Routines for converting between pixel formats

	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _CONVERT_H
#define _CONVERT_H


CODEC_ERROR ConvertBYR3ToDPX0(void *input_buffer, size_t input_pitch,
							  void *output_buffer, size_t output_pitch,
							  int width, int height);

CODEC_ERROR ConvertBYR4ToDPX0(void *input_buffer, size_t input_pitch,
							  void *output_buffer, size_t output_pitch,
							  int width, int height);

CODEC_ERROR ConvertRG48ToDPX0(void *input_buffer, size_t input_pitch,
							  void *output_buffer, size_t output_pitch,
							  int width, int height);

CODEC_ERROR ConvertB64AToDPX0(void *input_buffer, size_t input_pitch,
							  void *output_buffer, size_t output_pitch,
							  int width, int height);

CODEC_ERROR ConvertComponentsToDPX0(const UNPACKED_IMAGE *input,
									PIXEL_FORMAT format,
									IMAGE *output,
									ENABLED_PARTS enabled_parts);

CODEC_ERROR ConvertComponentsBYR4ToDPX0(const UNPACKED_IMAGE *input,
										IMAGE *output,
										ENABLED_PARTS enabled_parts);

CODEC_ERROR ConvertComponentsRG48ToDPX0(const UNPACKED_IMAGE *input,
										IMAGE *output,
										ENABLED_PARTS enabled_parts);

CODEC_ERROR ConvertComponentsB64AToDPX0(const UNPACKED_IMAGE *input,
										IMAGE *output,
										ENABLED_PARTS enabled_parts);

CODEC_ERROR ConvertComponentsDPX0ToDPX0(const UNPACKED_IMAGE *input,
										IMAGE *output,
										ENABLED_PARTS enabled_parts);

#endif
