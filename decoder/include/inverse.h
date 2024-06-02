/*! @file decoder/include/inverse.h

	Implementation of the inverse wavelet transforms.

	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _INVERSE_H
#define _INVERSE_H

CODEC_ERROR InvertSpatialQuant16s(ALLOCATOR *allocator,
								  PIXEL *lowlow_band, int lowlow_pitch,
								  PIXEL *lowhigh_band, int lowhigh_pitch,
								  PIXEL *highlow_band, int highlow_pitch,
								  PIXEL *highhigh_band, int highhigh_pitch,
								  PIXEL *output_image, int output_pitch,
								  DIMENSION input_width, DIMENSION input_height,
								  DIMENSION output_width, DIMENSION output_height,
								  QUANT quantization[]);

CODEC_ERROR InvertHorizontal16s(PIXEL *lowpass, PIXEL *highpass, PIXEL *output,
								DIMENSION input_width, DIMENSION output_width);

CODEC_ERROR InvertSpatialQuantDescale16s(ALLOCATOR *allocator,
										 PIXEL *lowlow_band, int lowlow_pitch,
										 PIXEL *lowhigh_band, int lowhigh_pitch,
										 PIXEL *highlow_band, int highlow_pitch,
										 PIXEL *highhigh_band, int highhigh_pitch,
										 PIXEL *output_image, int output_pitch,
										 DIMENSION input_width, DIMENSION input_height,
										 DIMENSION output_width, DIMENSION output_height,
										 //ROI roi, PIXEL *buffer, size_t buffer_size,
										 int descale, QUANT quantization[]);

CODEC_ERROR InvertSpatialWavelet(ALLOCATOR *allocator,
								 PIXEL *lowlow_band, int lowlow_pitch,
								 PIXEL *lowhigh_band, int lowhigh_pitch,
								 PIXEL *highlow_band, int highlow_pitch,
								 PIXEL *highhigh_band, int highhigh_pitch,
								 COMPONENT_VALUE *output_image, int output_pitch,
								 DIMENSION input_width, DIMENSION input_height,
								 DIMENSION output_width, DIMENSION output_height,
								 int descale, QUANT quantization[]);

CODEC_ERROR InvertHorizontalDescale16s(PIXEL *lowpass, PIXEL *highpass, PIXEL *output,
									   DIMENSION input_width, DIMENSION output_width, int descale);

CODEC_ERROR InvertSpatialTopRow(PIXEL **input_data[], DIMENSION input_width[], DIMENSION input_pitch[],
								PIXEL *output_buffer, DIMENSION output_width, DIMENSION output_pitch,
								DIMENSION output_byte_offset[],
								int input_row, int channel_count, int precision, QUANT *quantization[],
								ALLOCATOR *allocator);

CODEC_ERROR InvertSpatialMiddleRow(PIXEL **input_data[], DIMENSION input_width[], DIMENSION input_pitch[],
								   PIXEL *output_buffer, DIMENSION output_width, DIMENSION output_pitch,
								   DIMENSION output_byte_offset[],
								   int input_row, int channel_count, int precision, QUANT *quantization[],
								   ALLOCATOR *allocator);

CODEC_ERROR InvertSpatialBottomRow(PIXEL **input_data[], DIMENSION input_width[], DIMENSION input_pitch[],
								   PIXEL *output_buffer, DIMENSION output_width, DIMENSION output_pitch,
								   DIMENSION output_byte_offset[],
								   int input_row, int channel_count, int precision, QUANT *quantization[],
								   ALLOCATOR *allocator);

CODEC_ERROR InvertHorizontalScaled16s(PIXEL *lowpass,
									  PIXEL *highpass,
									  PIXEL *output,
									  DIMENSION input_width,
									  DIMENSION output_width,
									  int precision);

#endif
