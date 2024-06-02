/*! @file encoder/include/forward.h

	Implementation of the forward wavelet transforms.

	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _FORWARD_H
#define _FORWARD_H

CODEC_ERROR FilterHorizontalRow(PIXEL *input, PIXEL *lowpass, PIXEL *highpass, int width, int prescale);

CODEC_ERROR FilterVerticalTopRow(PIXEL *lowpass[], PIXEL *highpass[],
								 PIXEL *output[], DIMENSION pitch,
								 int band_count, int input_row, int wavelet_width,
								 QUANT quant[], int32_t midpoint_prequant);

CODEC_ERROR FilterVerticalMiddleRow(PIXEL *lowpass[], PIXEL *highpass[],
									PIXEL *output[], DIMENSION pitch,
									int band_count, int input_row, int wavelet_width,
									QUANT quant[], int32_t midpoint_prequant);

CODEC_ERROR FilterVerticalBottomRow(PIXEL *lowpass[], PIXEL *highpass[],
									PIXEL *output[], DIMENSION pitch,
									int band_count, int input_row, int wavelet_width,
									QUANT quant[], int32_t midpoint_prequant);

#endif
