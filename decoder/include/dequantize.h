/*! @file decoder/include/dequantize.h

	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _QUANTIZE_H
#define _QUANTIZE_H

CODEC_ERROR DequantizeBandRow16s(PIXEL *input, int width, int quantization, PIXEL *output);

PIXEL DequantizedValue(int32_t value, int quantization);

#endif
