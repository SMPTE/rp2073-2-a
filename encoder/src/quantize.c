/*!	@file encoder/src/quantize.c

	Implementation of the routines for quantization.

	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#include "headers.h"

#ifndef DEBUG
//! Control debugging statements in this module
#define DEBUG  (1 && _DEBUG)
#endif

/*!
	@brief Table of quantization

	The quantization table is indexed by the subband number.

	The lowpass subband (zero) should never be quantized so the quantization
	value should always be set to one.

	@todo Create a two-level table where the first index is the quality level.
*/
//QUANT quant_table[] = {1, 24, 24, 12, 24, 24, 12, 32, 32, 48}; // CineForm Filmscan-2
QUANT quant_table[] = {1, 24, 24, 12, 24, 24, 12, 96, 96, 144}; // CineForm Filmscan-1
//QUANT quant_table[] = {1, 24, 24, 12, 32, 32, 24, 128, 128, 192}; // CineForm High
//QUANT quant_table[] = {1, 24, 24, 12, 48, 48, 32, 256, 256, 384}; // CineForm Medium
//QUANT quant_table[] = {1, 24, 24, 12, 64, 64, 48, 512, 512, 768}; // CineForm Low


//! Number of entries in the quantization table
int quant_table_length = sizeof(quant_table)/sizeof(quant_table[0]);


//! Datatype used for computing upper half of product of 16-bit multiplication
typedef union _longword
{
	uint32_t longword;

	struct _halfword
	{
		unsigned short lower;
		unsigned short upper;

	} halfword;

} LONGWORD;


/*!
	@brief Quantize the value using the quantization value and midpoint rounding

	The midpoint prequant parameter is used to compute the rounding that is applied
	before quantization.
*/
PIXEL QuantizePixel(int32_t value, QUANT divisor, QUANT midpoint_prequant)
{
	int32_t prequant_midpoint;
	uint32_t multiplier;

	LONGWORD result;

	if (divisor <= 1) {
		return ClampPixel(value);
	}

	prequant_midpoint = QuantizerMidpoint(midpoint_prequant, divisor);

	// Change division to multiplication by a fraction
	multiplier = (uint32_t)(1 << 16) / divisor;

	//TODO: Use division instead to make the implementation simpler

	if (value >= 0)
	{
		result.longword = (value + prequant_midpoint) * multiplier;
		value = result.halfword.upper;
	}
	else
	{
		value = neg(value);
		result.longword = (value + prequant_midpoint) * multiplier;
		value = neg(result.halfword.upper);
	}

	return ClampPixel(value);
}

/*!
	@brief Compute the rounding value for quantization
*/
QUANT QuantizerMidpoint(QUANT correction, QUANT divisor)
{
	int32_t midpoint = 0;

	if (correction >= 2 && correction < 9)
	{
		midpoint = divisor / correction;

		if (correction == 2)
		{
			// CFEncode_Premphasis_Original
			if (midpoint) {
				midpoint--;
			}
		}
	}

	return midpoint;
}


//#if (_DEBUG || _TIMING)
#if _DEBUG

//! Dequantize a pixel (for debugging)
PIXEL DequantizePixel(int32_t value, QUANT divisor)
{
	const int midpoint = 0;

	// Invert the companding curve (if any)
	value = UncompandedValue(value);

	// Dequantize the absolute value
	if (value > 0)
	{
		value = (divisor * value) + midpoint;
	}
	else if (value < 0)
	{
		value = neg(value);
		value = (divisor * value) + midpoint;
		value = neg(value);
	}

	return ClampPixel(value);
}

//! Print the transform prescale values (for debugging)
CODEC_ERROR PrintTransformPrescale(TRANSFORM *transform, int wavelet_count, FILE *file)
{
	if (transform != NULL)
	{
		fprintf(file, "Transform prescale: %d %d %d\n",
			transform->prescale[0], transform->prescale[1], transform->prescale[2]);
	}

	return CODEC_ERROR_OKAY;
}

#if 0
//! Print the transform quantization (for debugging)
CODEC_ERROR PrintTransformQuantization(TRANSFORM *transform, int wavelet_count, FILE *file)
{
	//int num_wavelets = MAX_WAVELET_COUNT;
	int k;

	for (k = 0; k < wavelet_count; k++)
	{
		WAVELET *wavelet = transform->wavelet[k];
		assert(wavelet != NULL);

#if 0	//(CODEC_PROFILE > CODEC_PROFILE_BASELINE)
		switch (wavelet->wavelet_type)
		{

		case WAVELET_TYPE_HORIZONTAL:		// One highpass band
		case WAVELET_TYPE_VERTICAL:
		case WAVELET_TYPE_TEMPORAL:
			fprintf(file, "Wavelet quant: %d %d\n", wavelet->quant[0], wavelet->quant[1]);
			break;

		case WAVELET_TYPE_SPATIAL:			// Three highpass bands
		case WAVELET_TYPE_HORZTEMP:
		case WAVELET_TYPE_VERTTEMP:
			fprintf(file, "Wavelet quant: %d %d %d %d\n",
					wavelet->quant[0], wavelet->quant[1], wavelet->quant[2], wavelet->quant[3]);
			break;

		case WAVELET_TYPE_IMAGE:			// Not really a wavelet
		case WAVELET_TYPE_TEMPQUAD:			// Should not occur in normal code
		case WAVELET_TYPE_HORZQUAD:
		default:
			assert(0);
			break;
		}
#else
	fprintf(file, "Wavelet quant: %d %d %d %d\n",
			wavelet->quant[0], wavelet->quant[1], wavelet->quant[2], wavelet->quant[3]);
#endif
	}

	return CODEC_ERROR_OKAY;
}
#endif

#endif
