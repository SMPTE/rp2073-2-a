/*!	@file decoder/src/dequantize.c

	This module implements that calculations that remove companding
	and quantization from the encoded highpass band.

	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#include "headers.h"


// Not using midpoint correction in dequantization
static const int midpoint = 0;


/*!
	@brief Dequantize a band with the specified dimensions

	The companding curve is inverted and the value is multiplied by the
	quantization value that was used by the encoder to compress the band.
*/
CODEC_ERROR DequantizeBandRow16s(PIXEL *input, int width, int quantization, PIXEL *output)
{
	int column;

	// Undo quantization in the entire row
	for (column = 0; column < width; column++)
	{
		int32_t value = input[column];

		// Invert the companding curve (if any)
		value = UncompandedValue(value);

		// Dequantize the absolute value
		if (value > 0)
		{
			value = (quantization * value) + midpoint;
		}
		else if (value < 0)
		{
			value = neg(value);
			value = (quantization * value) + midpoint;
			value = neg(value);
		}

		// Store the dequantized coefficient
		output[column] = ClampPixel(value);
	}

	return CODEC_ERROR_OKAY;
}

/*!
	@brief This function dequantizes the pixel value

	The inverse companding curve is applied to convert the pixel value
	to its quantized value and then the pixel value is multiplied by
	the quantization parameter.
*/
PIXEL DequantizedValue(int32_t value, int quantization)
{
	// Invert the companding curve (if any)
	value = UncompandedValue(value);

	// Dequantize the absolute value
	if (value > 0)
	{
		value = (quantization * value) + midpoint;
	}
	else if (value < 0)
	{
		value = neg(value);
		value = (quantization * value) + midpoint;
		value = neg(value);
	}

	return ClampPixel(value);
}
