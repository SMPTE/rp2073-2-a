/*!	@file common/include/companding.h

	Declaration of the routines for computing the companding curves that is applied
	to the quantized coefficient magnitudes.

	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _COMPANDING_H
#define _COMPANDING_H

#ifdef __cplusplus
extern "C" {
#endif

int32_t CompandedValue(int32_t value);
//PIXEL CompandedPixel(PIXEL value);

uint32_t CompandingParameter();

CODEC_ERROR ComputeCubicTable(int16_t cubic_table[], int cubic_table_length, int16_t maximum_value);

// Invert the companding curve applied to a quantized coefficient magnitude (for debugging)
int32_t UncompandedValue(int32_t value);
PIXEL UncompandedPixel(PIXEL value);
CODEC_ERROR InvertCompanding(PIXEL *image, DIMENSION width, DIMENSION height, DIMENSION pitch);

#endif
