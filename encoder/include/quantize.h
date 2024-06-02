/*! @file encoder/include/quantize.h

	Declaration of the data structures and constants used for quantization.

	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _QUANTIZE_H
#define _QUANTIZE_H

extern QUANT quant_table[MAX_SUBBAND_COUNT];

#if 0
typedef void *custom_quant;


/*!
	@brief Data structure for specifying custom quantization parameters

	This facility is not supported in the baseline codec.
*/
typedef void *CUSTOM_QUANT;
#endif


#ifdef __cplusplus
extern "C" {
#endif

// Quantize a wavelet band using the specified quantization divisor
CODEC_ERROR QuantizeBand(WAVELET *wavelet, int band, int divisor);

// Quantize a row of 16-bit signed coefficients using inplace computation
CODEC_ERROR QuantizeRow16s(PIXEL *rowptr, int length, int divisor);

// Quantize a row of 16-bit signed coefficients without overwriting the input
CODEC_ERROR QuantizeRow16sTo16s(PIXEL *input, PIXEL *output, int length, int divisor);

// Quantize a pixel value and clamp the result to the valid pixel range
PIXEL QuantizePixel(int32_t value, QUANT divisor, QUANT midpoint_prequant);

// Compute the midpoint value for quantization
QUANT QuantizerMidpoint(QUANT midpoint, QUANT divisor);


#if (_DEBUG || _TIMING)

// Dequantize the quantized value (for debugging)
PIXEL DequantizePixel(int32_t value, QUANT divisor);

// Print the prescaling used for the transform wavelets
CODEC_ERROR PrintTransformPrescale(TRANSFORM *transform, int wavelet_count, FILE *logfile);

// Print the quantization vectors in the transform wavelets
//CODEC_ERROR PrintTransformQuantization(TRANSFORM *transform, int wavelet_count, FILE *logfile);

#endif

#ifdef __cplusplus
}
#endif

#endif
