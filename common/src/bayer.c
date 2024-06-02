/*!	@file common/src/bayer.c

	Implementation of the routines for unpacking Bayer frames into rows of pixels
	for encoding.

	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#include "headers.h"

/*!
	@brief Unpack the Bayer BYR3 format into an unpacked representation
	
	The Bayer BYR3 format is unpacked into the row-oriented format that
	is used internally by the codec as an intermediate representation.

	@todo Add a vector of offsets to each component so that the layout of the components
	can accommodate requirements on memory alignment.
*/
CODEC_ERROR UnpackImageRowBYR3(uint8_t *input, DIMENSION width, PIXEL *buffer[],
							   PRECISION bits_per_component[], int channel_count,
							   ENABLED_PARTS enabled_parts)
{
	// Define pointers to the rows for each component
	uint16_t *R_input_row_ptr;
	uint16_t *G1_input_row_ptr;
	uint16_t *G2_input_row_ptr;
	uint16_t *B_input_row_ptr;

	// Define pointers to the rows for each output component
	uint16_t *G_output_row_ptr;
	uint16_t *RG_output_row_ptr;
	uint16_t *BG_output_row_ptr;
	uint16_t *GD_output_row_ptr;

	//int input_quarter_width;
	//int output_quarter_width;

	const int input_precision = 10;		// BYR3 has a 10 bit pixel depth

    // Internal precision of the encoded values
    const int internal_precision = 12;

	// Compute the amount of shift to scale to the internal precision
	const int shift = (internal_precision - input_precision);

	const int32_t midpoint = (1 << (internal_precision - 1));

	int column;

	// The width should be a multiple of four
	assert((width % 4) == 0);

	R_input_row_ptr = (uint16_t *)input;
	G1_input_row_ptr = R_input_row_ptr + width;
	G2_input_row_ptr = G1_input_row_ptr + width;
	B_input_row_ptr = G2_input_row_ptr + width;

	assert(channel_count == 4);

	G_output_row_ptr = (uint16_t *)buffer[0];
	RG_output_row_ptr = (uint16_t *)buffer[1];
	BG_output_row_ptr = (uint16_t *)buffer[2];
	GD_output_row_ptr = (uint16_t *)buffer[3];

	// Pack the rows of Bayer components into the BYR3 pattern
	for (column = 0; column < width; column++)
	{
		int32_t R, G1, G2, B;
		int32_t G, RG, BG, GD;

		R = R_input_row_ptr[column];
		G1 = G1_input_row_ptr[column];
		G2 = G2_input_row_ptr[column];
		B = B_input_row_ptr[column];

		// Scale the input values to the full precision of the intermediate format
		G1 <<= shift;
		G2 <<= shift;
		R <<= shift;
		B <<= shift;

		// Difference the green components and subtract green from the red and blue components
		G = (G1 + G2) >> 1;
		GD = (G1 - G2) >> 1;
		RG = (R - G) >> 1;
		BG = (B - G) >> 1;

		// Convert signed values to unsigned values
		GD += midpoint;
		RG += midpoint;
		BG += midpoint;

		G_output_row_ptr[column] = clamp_uint(G, internal_precision);
		RG_output_row_ptr[column] = clamp_uint(RG, internal_precision);
		BG_output_row_ptr[column] = clamp_uint(BG, internal_precision);
		GD_output_row_ptr[column] = clamp_uint(GD, internal_precision);
	}

	return CODEC_ERROR_OKAY;
}

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
/*!
	@brief Unpack the rows of Bayer components

	This routine applies the component transform for Bayer images defined in
	VC-5 Part 3: Image Formats.

	The Bayer image format assigns component arrays to channel numbers in the following order:
	G sum, G difference, R difference, B difference
*/
CODEC_ERROR UnpackImageRowBYR4(uint8_t *input, DIMENSION width, PIXEL *buffer[],
							   PRECISION bits_per_component[], int channel_count,
							   ENABLED_PARTS enabled_parts)
{
	// Define pointers to the rows for each component
	uint16_t *input_row1_ptr;
	uint16_t *input_row2_ptr;

	// Define pointers to the rows for each output component
	uint16_t *GS_output_row_ptr;
	uint16_t *GD_output_row_ptr;
	uint16_t *RG_output_row_ptr;
	uint16_t *BG_output_row_ptr;

	const int input_precision = 16;		// BYR4 has a 16 bit pixel depth
    
    const int internal_precision = 12;

	// Compute the amount of shift to scale to the internal precision
	const int shift = (input_precision - internal_precision);

	// Compute the midpoint for converting signed to unsigned values
	const int32_t midpoint = (1 << (internal_precision - 1));

	int column;

	// The width should be a multiple of four
	assert((width % 4) == 0);

	input_row1_ptr = (uint16_t *)input;
	input_row2_ptr = input_row1_ptr + 2 * width;

	assert(channel_count == 4);

	GS_output_row_ptr = (uint16_t *)buffer[0];
	GD_output_row_ptr = (uint16_t *)buffer[3];
	RG_output_row_ptr = (uint16_t *)buffer[1];
	BG_output_row_ptr = (uint16_t *)buffer[2];

	// Unpack the row of Bayer components from the BYR4 pattern elements
	for (column = 0; column < width; column++)
	{
		int32_t R1, G1, G2, B1;
		int32_t GS, GD, RG, BG;

		R1 = input_row1_ptr[2 * column + 0];
		G1 = input_row1_ptr[2 * column + 1];
		G2 = input_row2_ptr[2 * column + 0];
		B1 = input_row2_ptr[2 * column + 1];

		// Scale the input precision down to the encoded precision
		R1 >>= shift;
		G1 >>= shift;
		G2 >>= shift;
		B1 >>= shift;

		// Difference the green components and subtract green from the red and blue components
		GS = (G1 + G2) >> 1;
		GD = (G1 - G2 + 2 * midpoint) >> 1;
		RG = (R1 - GS + 2 * midpoint) >> 1;
		BG = (B1 - GS + 2 * midpoint) >> 1;

		// Convert signed values to unsigned values
		//GD += midpoint;
		//RG += midpoint;
		//BG += midpoint;

		GS_output_row_ptr[column] = clamp_uint(GS, internal_precision);
		GD_output_row_ptr[column] = clamp_uint(GD, internal_precision);
		RG_output_row_ptr[column] = clamp_uint(RG, internal_precision);
		BG_output_row_ptr[column] = clamp_uint(BG, internal_precision);
	}

	return CODEC_ERROR_OKAY;
}
#else
/*!
	@brief Unpack the rows of Bayer components

	This routine unpacked that Bayer components into component arrays without
	applying a component transform which is not defined in VC-5 Part 1.
*/
CODEC_ERROR UnpackImageRowBYR4(uint8_t *input, DIMENSION width, PIXEL *buffer[],
							   PRECISION bits_per_component[], int channel_count,
							   ENABLED_PARTS enabled_parts)
{
	// Define pointers to the rows for each component
	uint16_t *input_row1_ptr;
	uint16_t *input_row2_ptr;

	// Define pointers to the rows for each output component
	uint16_t *C1_output_row_ptr;
	uint16_t *C2_output_row_ptr;
	uint16_t *C3_output_row_ptr;
	uint16_t *C4_output_row_ptr;

	const int input_precision = 16;		// BYR4 has a 16 bit pixel depth

	// Compute the amount of shift to scale to the internal precision
	const int shift = (input_precision - internal_precision);

	//const int32_t midpoint = (1 << (internal_precision - 1));

	int column;

	input_row1_ptr = (uint16_t *)input;
	input_row2_ptr = input_row1_ptr + 2 * width;

	assert(channel_count == 4);

	C1_output_row_ptr = (uint16_t *)buffer[0];
	C2_output_row_ptr = (uint16_t *)buffer[1];
	C3_output_row_ptr = (uint16_t *)buffer[2];
	C4_output_row_ptr = (uint16_t *)buffer[3];

	// Unpack the rows of Bayer components into component arrays
	for (column = 0; column < width; column++)
	{
		int32_t R, G1, G2, B;
		//int32_t C1, C2, C3, C4;

		R  = input_row1_ptr[2 * column + 0];
		G1 = input_row1_ptr[2 * column + 1];
		G2 = input_row2_ptr[2 * column + 0];
		B  = input_row2_ptr[2 * column + 1];

		// Scale the input precision down
		G1 >>= shift;
		G2 >>= shift;
		R >>= shift;
		B >>= shift;

		C1_output_row_ptr[column] = clamp_uint14(R);
		C2_output_row_ptr[column] = clamp_uint14(G1);
		C3_output_row_ptr[column] = clamp_uint14(G2);
		C4_output_row_ptr[column] = clamp_uint14(B);
	}

	return CODEC_ERROR_OKAY;
}
#endif
