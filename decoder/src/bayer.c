/*!	@file decoder/src/bayer.c

	Implementation of routines for converting ordered sets of component arrays output
	by the decoding process into packed images with a the internal representation of
	frames used by the codec into frame with a specific Bayer pattern.

	The CineForm codec uses an internal format for intermediate results that is part
	way between packed and planar where the components are unpacked into rows, but the
	rows are interleaved in a single plane rather than using one plane per component.

	This allows more efficient processing in many cases since most algorithms work better
	when the components are unpacked, but the traditional planar organization would force
	concurrent memory accesses to different cache lines when all components must be
	accessed by the algorithm.

	Each pixel in the internal representation uses the full 16 bits of precision.

	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#include "headers.h"


/*!
	@brief Convert the internal format of interleaved rows to the planar Bayer pattern

	The BYR3 Bayer pattern is an interleaved row organization with 10 bits of precision
	in each 16-bit component.  The pitch between each component row is one quarter of the
	pitch of an entire row of components.

	@todo Add an array of byte offsets to the start of each component in the row to
	decouple the alignment restrictions on the individual components from the alignment
	restrictons on the entire row.
*/
CODEC_ERROR PackBufferRowsToBYR3(PIXEL *input_buffer, size_t input_pitch,
								 PIXEL *output_buffer, size_t output_pitch,
								 DIMENSION width, DIMENSION height,
                                 ENABLED_PARTS enabled_parts)
{
	// Define pointers to the rows for each input component
	uint16_t *G_input_row_ptr;
	uint16_t *RG_input_row_ptr;
	uint16_t *BG_input_row_ptr;
	uint16_t *GD_input_row_ptr;

	// Define pointers to the rows for each output component
	uint16_t *R_output_row_ptr;
	uint16_t *G1_output_row_ptr;
	uint16_t *G2_output_row_ptr;
	uint16_t *B_output_row_ptr;

	size_t input_quarter_pitch;
	size_t output_quarter_pitch;

	int row;

	// The pitch must be divisible by four
	assert((input_pitch % 4) == 0);
	assert((output_pitch % 4) == 0);

	// Compute the size of each component (in bytes)
	input_quarter_pitch = input_pitch / 4;
	output_quarter_pitch = output_pitch / 4;

	for (row = 0; row < height; row++)
	{
		uint8_t *input_row_ptr = (uint8_t *)input_buffer + row * input_pitch;
		uint8_t *output_row_ptr = (uint8_t *)output_buffer + row * output_pitch;

		const int32_t midpoint = 32768;
		//const int32_t midpoint = 32768/2;

		int column;

		G_input_row_ptr = (uint16_t *)input_row_ptr;
		RG_input_row_ptr = (uint16_t *)(input_row_ptr + input_quarter_pitch);
		BG_input_row_ptr = (uint16_t *)(input_row_ptr + 2 * input_quarter_pitch);
		GD_input_row_ptr = (uint16_t *)(input_row_ptr + 3 * input_quarter_pitch);;

		R_output_row_ptr = (uint16_t *)output_row_ptr;
		G1_output_row_ptr = (uint16_t *)(output_row_ptr + output_quarter_pitch);
		G2_output_row_ptr = (uint16_t *)(output_row_ptr + 2 * output_quarter_pitch);
		B_output_row_ptr = (uint16_t *)(output_row_ptr + 3 * output_quarter_pitch);

		// Pack the rows of Bayer components into the BYR3 pattern
		for (column = 0; column < width; column++)
		{
			int32_t G, RG, BG, GD;
			int32_t R, G1, G2, B;

			G = G_input_row_ptr[column];
			RG = RG_input_row_ptr[column];
			BG = BG_input_row_ptr[column];
			GD = GD_input_row_ptr[column];

			// Convert unsigned values to signed values
			GD -= midpoint;
			RG -= midpoint;
			BG -= midpoint;

			R = (RG << 1) + G;
			B = (BG << 1) + G;
			G1 = G + GD;
			G2 = G - GD;

			R = clamp_uint16(R);
			G1 = clamp_uint16(G1);
			G2 = clamp_uint16(G2);
			B = clamp_uint16(B);

			// Reduce the output values to 10-bits
			R_output_row_ptr[column] = (uint16_t)(R >> 6);
			G1_output_row_ptr[column] = (uint16_t)(G1 >> 6);
			G2_output_row_ptr[column] = (uint16_t)(G2 >> 6);
			B_output_row_ptr[column] = (uint16_t)(B >> 6);
		}
	}

	return CODEC_ERROR_OKAY;
}

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
/*!
	@brief Convert the internal format of interleaved rows to the packed Bayer pattern
*/
CODEC_ERROR PackBufferRowsToBYR4(PIXEL *input_buffer, size_t input_pitch,
								 PIXEL *output_buffer, size_t output_pitch,
								 DIMENSION width, DIMENSION height,
                                 ENABLED_PARTS enabled_parts)
{
	// Define pointers to the rows for each input component
	uint16_t *G_input_row_ptr;
	uint16_t *RG_input_row_ptr;
	uint16_t *BG_input_row_ptr;
	uint16_t *GD_input_row_ptr;

	// Define pointers to the rows for each output component
	uint16_t *output_row1_ptr;
	uint16_t *output_row2_ptr;

	size_t input_quarter_pitch;
	size_t output_half_pitch;

	int row;

	// The input pitch must be divisible by four
	assert((input_pitch % 4) == 0);

	// Compute the size of each input component (in bytes)
	input_quarter_pitch = input_pitch / 4;

	// Compute the distance between the half rows in the Bayer grid
	output_half_pitch = output_pitch / 2;

	for (row = 0; row < height; row++)
	{
		uint8_t *input_row_ptr = (uint8_t *)input_buffer + row * input_pitch;
		uint8_t *output_row_ptr = (uint8_t *)output_buffer + row * output_pitch;

		const int32_t midpoint = 32768;

		int column;

		G_input_row_ptr = (uint16_t *)input_row_ptr;
		RG_input_row_ptr = (uint16_t *)(input_row_ptr + input_quarter_pitch);
		BG_input_row_ptr = (uint16_t *)(input_row_ptr + 2 * input_quarter_pitch);
		GD_input_row_ptr = (uint16_t *)(input_row_ptr + 3 * input_quarter_pitch);;

		output_row1_ptr = (uint16_t *)output_row_ptr;
		output_row2_ptr = (uint16_t *)(output_row_ptr + output_half_pitch);

		// Pack the rows of Bayer components into the BYR4 pattern
		for (column = 0; column < width; column++)
		{
			int32_t G, RG, BG, GD;
			int32_t R, G1, G2, B;

			G = G_input_row_ptr[column];
			RG = RG_input_row_ptr[column];
			BG = BG_input_row_ptr[column];
			GD = GD_input_row_ptr[column];

			// Convert unsigned values to signed values
			GD -= midpoint;
			RG -= midpoint;
			BG -= midpoint;

			R = (RG << 1) + G;
			B = (BG << 1) + G;
			G1 = G + GD;
			G2 = G - GD;

			R = clamp_uint16(R);
			G1 = clamp_uint16(G1);
			G2 = clamp_uint16(G2);
			B = clamp_uint16(B);

			output_row1_ptr[2 * column + 0] = (uint16_t)R;
			output_row1_ptr[2 * column + 1] = (uint16_t)G1;
			output_row2_ptr[2 * column + 0] = (uint16_t)G2;
			output_row2_ptr[2 * column + 1] = (uint16_t)B;
		}
	}

	return CODEC_ERROR_OKAY;
}
#else
/*!
	@brief Convert the internal format of interleaved rows to the packed Bayer pattern
*/
CODEC_ERROR PackBufferRowsToBYR4(PIXEL *input_buffer, size_t input_pitch,
								 PIXEL *output_buffer, size_t output_pitch,
								 DIMENSION width, DIMENSION height,
                                 ENABLED_PARTS enabled_parts)
{
	// Define pointers to the rows for each input component
	uint16_t *R1_input_row_ptr;
	uint16_t *G1_input_row_ptr;
	uint16_t *G2_input_row_ptr;
	uint16_t *B1_input_row_ptr;

	// Define pointers to the rows for each output component
	uint16_t *output_row1_ptr;
	uint16_t *output_row2_ptr;

	size_t input_quarter_pitch;
	size_t output_half_pitch;

	int row;

	// The input pitch must be divisible by four
	assert((input_pitch % 4) == 0);

	// Compute the size of each input component (in bytes)
	input_quarter_pitch = input_pitch / 4;

	// Compute the distance between the half rows in the Bayer grid
	output_half_pitch = output_pitch / 2;

	for (row = 0; row < height; row++)
	{
		uint8_t *input_row_ptr = (uint8_t *)input_buffer + row * input_pitch;
		uint8_t *output_row_ptr = (uint8_t *)output_buffer + row * output_pitch;

		//const int32_t midpoint = 32768;

		int column;

		R1_input_row_ptr = (uint16_t *)input_row_ptr;
		G1_input_row_ptr = (uint16_t *)(input_row_ptr + input_quarter_pitch);
		G2_input_row_ptr = (uint16_t *)(input_row_ptr + 2 * input_quarter_pitch);
		B1_input_row_ptr = (uint16_t *)(input_row_ptr + 3 * input_quarter_pitch);;

		output_row1_ptr = (uint16_t *)output_row_ptr;
		output_row2_ptr = (uint16_t *)(output_row_ptr + output_half_pitch);

		// Pack the rows of Bayer components into the BYR4 pattern
		for (column = 0; column < width; column++)
		{
			int32_t R1, G1, G2, B1;

			R1 = R1_input_row_ptr[column];
			G1 = G1_input_row_ptr[column];
			G2 = G2_input_row_ptr[column];
			B1 = B1_input_row_ptr[column];

			R1 = clamp_uint16(R1);
			G1 = clamp_uint16(G1);
			G2 = clamp_uint16(G2);
			B1 = clamp_uint16(B1);

			output_row1_ptr[2 * column + 0] = (uint16_t)R1;
			output_row1_ptr[2 * column + 1] = (uint16_t)G1;
			output_row2_ptr[2 * column + 0] = (uint16_t)G2;
			output_row2_ptr[2 * column + 1] = (uint16_t)B1;
		}
	}

	return CODEC_ERROR_OKAY;
}
#endif

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
/*!
	@brief Pack the component arrays into an output image
	
	The inverse component transform for Bayer images (VC-5 Part 3)
	is applied to the component arrays before combining the values
	into a packed image.
*/
CODEC_ERROR PackComponentsToBYR4(const UNPACKED_IMAGE *image,
								 PIXEL *output_buffer, size_t output_pitch,
								 DIMENSION width, DIMENSION height,
                                 ENABLED_PARTS enabled_parts)
{
	// Define pointers to the rows for each input component
	COMPONENT_VALUE *GS_input_buffer;
	COMPONENT_VALUE *RG_input_buffer;
	COMPONENT_VALUE *BG_input_buffer;
	COMPONENT_VALUE *GD_input_buffer;

	// Define pointers to the rows for each output component
	uint16_t *output_row1_ptr;
	uint16_t *output_row2_ptr;

	//size_t input_quarter_pitch;
	size_t output_half_pitch;

	// The BYR4 format uses 16-bit pixels
	const int shift = 4;

	int row;

	GS_input_buffer = image->component_array_list[0].data;
	RG_input_buffer = image->component_array_list[1].data;
	BG_input_buffer = image->component_array_list[2].data;
	GD_input_buffer = image->component_array_list[3].data;

	// Compute the distance between the half rows in the Bayer grid
	output_half_pitch = output_pitch / 2;

	for (row = 0; row < height; row++)
	{
		COMPONENT_VALUE *GS_input_row_ptr = (COMPONENT_VALUE *)((uintptr_t)GS_input_buffer + row * image->component_array_list[0].pitch);
		COMPONENT_VALUE *RG_input_row_ptr = (COMPONENT_VALUE *)((uintptr_t)RG_input_buffer + row * image->component_array_list[1].pitch);
		COMPONENT_VALUE *BG_input_row_ptr = (COMPONENT_VALUE *)((uintptr_t)BG_input_buffer + row * image->component_array_list[2].pitch);
		COMPONENT_VALUE *GD_input_row_ptr = (COMPONENT_VALUE *)((uintptr_t)GD_input_buffer + row * image->component_array_list[3].pitch);

		uint8_t *output_row_ptr = (uint8_t *)output_buffer + row * output_pitch;

		const int32_t midpoint = 2048;

		int column;

		output_row1_ptr = (uint16_t *)output_row_ptr;
		output_row2_ptr = (uint16_t *)(output_row_ptr + output_half_pitch);

		// Pack the rows of Bayer components into the BYR4 pattern
		for (column = 0; column < width; column++)
		{
			int32_t GS, RG, BG, GD;
			int32_t R, G1, G2, B;

			GS = GS_input_row_ptr[column];
			RG = RG_input_row_ptr[column];
			BG = BG_input_row_ptr[column];
			GD = GD_input_row_ptr[column];

			// Convert unsigned values to signed values
			GD -= midpoint;
			RG -= midpoint;
			BG -= midpoint;

			R = (RG << 1) + GS;
			B = (BG << 1) + GS;
			G1 = GS + GD;
			G2 = GS - GD;

			R <<= shift;
			B <<= shift;
			G1 <<= shift;
			G2 <<= shift;


			R = clamp_uint16(R);
			G1 = clamp_uint16(G1);
			G2 = clamp_uint16(G2);
			B = clamp_uint16(B);

			output_row1_ptr[2 * column + 0] = (uint16_t)R;
			output_row1_ptr[2 * column + 1] = (uint16_t)G1;
			output_row2_ptr[2 * column + 0] = (uint16_t)G2;
			output_row2_ptr[2 * column + 1] = (uint16_t)B;
		}
	}

	return CODEC_ERROR_OKAY;
}
#else
/*!
	@brief Pack the component arrays into an output image
*/
CODEC_ERROR PackComponentsToBYR4(const UNPACKED_IMAGE *image,
								 PIXEL *output_buffer, size_t output_pitch,
								 DIMENSION width, DIMENSION height)
{
	// Define pointers to the rows for each input component
	uint8_t *C1_input_buffer;
	uint8_t *C2_input_buffer;
	uint8_t *C3_input_buffer;
	uint8_t *C4_input_buffer;

	// Define pointers to the rows for each output component
	uint16_t *output_row1_ptr;
	uint16_t *output_row2_ptr;

	//size_t input_quarter_pitch;
	size_t output_half_pitch;

	// The BYR4 format uses 16-bit pixels
	const int shift = 4;

	int row;

	C1_input_buffer = (uint8_t *)image->component_array_list[0].data;
	C2_input_buffer = (uint8_t *)image->component_array_list[1].data;
	C3_input_buffer = (uint8_t *)image->component_array_list[2].data;
	C4_input_buffer = (uint8_t *)image->component_array_list[3].data;

	// Compute the distance between the half rows in the Bayer grid
	output_half_pitch = output_pitch / 2;

	for (row = 0; row < height; row++)
	{
		COMPONENT_VALUE *C1_input_row_ptr = (COMPONENT_VALUE *)(C1_input_buffer + row * image->component_array_list[0].pitch);
		COMPONENT_VALUE *C2_input_row_ptr = (COMPONENT_VALUE *)(C2_input_buffer + row * image->component_array_list[1].pitch);
		COMPONENT_VALUE *C3_input_row_ptr = (COMPONENT_VALUE *)(C3_input_buffer + row * image->component_array_list[2].pitch);
		COMPONENT_VALUE *C4_input_row_ptr = (COMPONENT_VALUE *)(C4_input_buffer + row * image->component_array_list[3].pitch);

		uint8_t *output_row_ptr = (uint8_t *)output_buffer + row * output_pitch;

		int column;

		output_row1_ptr = (uint16_t *)output_row_ptr;
		output_row2_ptr = (uint16_t *)(output_row_ptr + output_half_pitch);

		// Pack the rows of Bayer components into the BYR4 pattern
		for (column = 0; column < width; column++)
		{
			COMPONENT_VALUE C1, C2, C3, C4;

			C1 = C1_input_row_ptr[column];
			C2 = C2_input_row_ptr[column];
			C3 = C3_input_row_ptr[column];
			C4 = C4_input_row_ptr[column];

			// Scale the component values to 16 bits
			C1 <<= shift;
			C2 <<= shift;
			C3 <<= shift;
			C4 <<= shift;

			output_row1_ptr[2 * column + 0] = C1;
			output_row1_ptr[2 * column + 1] = C2;
			output_row2_ptr[2 * column + 0] = C3;
			output_row2_ptr[2 * column + 1] = C4;
		}
	}

	return CODEC_ERROR_OKAY;
}
#endif

/*!
	@brief Pack the component arrays into an output image
*/
CODEC_ERROR PackComponentsToRG48(const UNPACKED_IMAGE *image,
								 PIXEL *output_buffer, size_t output_pitch,
								 DIMENSION width, DIMENSION height,
                                 ENABLED_PARTS enabled_parts)
{
	// Define pointers to the rows for each input component
	uint8_t *R_input_buffer;
	uint8_t *G_input_buffer;
	uint8_t *B_input_buffer;

	// The RG48 format uses 16-bit pixels
	const int shift = 4;

	int row;

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
    if (IsPartEnabled(enabled_parts, VC5_PART_IMAGE_FORMATS))
    {
        R_input_buffer = (uint8_t *)image->component_array_list[1].data;
        G_input_buffer = (uint8_t *)image->component_array_list[0].data;
        B_input_buffer = (uint8_t *)image->component_array_list[2].data;
    }
    else
    {
        R_input_buffer = (uint8_t *)image->component_array_list[0].data;
        G_input_buffer = (uint8_t *)image->component_array_list[1].data;
        B_input_buffer = (uint8_t *)image->component_array_list[2].data;
    }
#else
    {
        R_input_buffer = (uint8_t *)image->component_array_list[0].data;
        G_input_buffer = (uint8_t *)image->component_array_list[1].data;
        B_input_buffer = (uint8_t *)image->component_array_list[2].data;
    }
#endif

	for (row = 0; row < height; row++)
	{
		COMPONENT_VALUE *R_input_row_ptr = (COMPONENT_VALUE *)(R_input_buffer + row * image->component_array_list[0].pitch);
		COMPONENT_VALUE *G_input_row_ptr = (COMPONENT_VALUE *)(G_input_buffer + row * image->component_array_list[1].pitch);
		COMPONENT_VALUE *B_input_row_ptr = (COMPONENT_VALUE *)(B_input_buffer + row * image->component_array_list[2].pitch);

		uint16_t *output_row_ptr = (uint16_t *)((uint8_t *)output_buffer + row * output_pitch);

		int column;

		// Pack the rows of RGB components into the RG48 output image
		for (column = 0; column < width; column++)
		{
			COMPONENT_VALUE R, G, B;

			R = R_input_row_ptr[column];
			G = G_input_row_ptr[column];
			B = B_input_row_ptr[column];

			// Scale the component values to 16 bits
			R <<= shift;
			G <<= shift;
			B <<= shift;

			output_row_ptr[3 * column + 0] = R;
			output_row_ptr[3 * column + 1] = G;
			output_row_ptr[3 * column + 2] = B;
		}
	}

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Pack the component arrays into an output image
*/
CODEC_ERROR PackComponentsToB64A(const UNPACKED_IMAGE *image,
								 PIXEL *output_buffer, size_t output_pitch,
								 DIMENSION width, DIMENSION height,
                                 ENABLED_PARTS enabled_parts)
{
	// Define pointers to the rows for each input component
	uint8_t *A_input_buffer;
	uint8_t *R_input_buffer;
	uint8_t *G_input_buffer;
	uint8_t *B_input_buffer;

	// The B64A format uses 16-bit pixels
	const int shift = 4;

	int row;

	A_input_buffer = (uint8_t *)image->component_array_list[0].data;
	R_input_buffer = (uint8_t *)image->component_array_list[1].data;
	G_input_buffer = (uint8_t *)image->component_array_list[2].data;
	B_input_buffer = (uint8_t *)image->component_array_list[3].data;

	for (row = 0; row < height; row++)
	{
		COMPONENT_VALUE *A_input_row_ptr = (COMPONENT_VALUE *)(A_input_buffer + row * image->component_array_list[0].pitch);
		COMPONENT_VALUE *R_input_row_ptr = (COMPONENT_VALUE *)(R_input_buffer + row * image->component_array_list[1].pitch);
		COMPONENT_VALUE *G_input_row_ptr = (COMPONENT_VALUE *)(G_input_buffer + row * image->component_array_list[2].pitch);
		COMPONENT_VALUE *B_input_row_ptr = (COMPONENT_VALUE *)(B_input_buffer + row * image->component_array_list[3].pitch);

		uint16_t *output_row_ptr = (uint16_t *)((uint8_t *)output_buffer + row * output_pitch);

		int column;

		// Pack the rows of RGB components into the B64A output image
		for (column = 0; column < width; column++)
		{
			COMPONENT_VALUE A, R, G, B;

			A = A_input_row_ptr[column];
			R = R_input_row_ptr[column];
			G = G_input_row_ptr[column];
			B = B_input_row_ptr[column];

			// Scale the component values to 16 bits
			A <<= shift;
			R <<= shift;
			G <<= shift;
			B <<= shift;

			// Byte swap and store the color components
			output_row_ptr[4 * column + 0] = Swap16(A);
			output_row_ptr[4 * column + 1] = Swap16(R);
			output_row_ptr[4 * column + 2] = Swap16(G);
			output_row_ptr[4 * column + 3] = Swap16(B);
		}
	}

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Pack the component arrays into an output image

    This routine does not belong in the module for Bayer images.

    This routine assumes that the input pitch is equal to the input width times the size
    of a component value (in bytes) so that the width can be used instead of the pitch.

    @todo Move this routine to another module for image unpacking.
 */
CODEC_ERROR PackComponentsToNV12(const UNPACKED_IMAGE *image,
                                 PIXEL *output_buffer, size_t output_pitch,
                                 DIMENSION width, DIMENSION height,
                                 ENABLED_PARTS enabled_parts)
{
    COMPONENT_VALUE *Y_input_array = image->component_array_list[0].data;
    COMPONENT_VALUE *U_input_array = image->component_array_list[1].data;
    COMPONENT_VALUE *V_input_array = image->component_array_list[2].data;

    uint8_t *upper_output_plane = (uint8_t *)output_buffer;
    uint8_t *lower_output_plane = upper_output_plane + (width * height);
    
    int output_precision = 8;
    
    int Y_shift = image->component_array_list[0].bits_per_component - output_precision;
    int U_shift = image->component_array_list[1].bits_per_component - output_precision;
    int V_shift = image->component_array_list[2].bits_per_component - output_precision;

    int luma_row;
    
    for (luma_row = 0; luma_row < height; luma_row++)
    {
        COMPONENT_VALUE *Y_input_row = Y_input_array + luma_row * image->component_array_list[0].width;
        uint8_t *upper_output_row = upper_output_plane + luma_row * width;
        int column;
        
        // Pack the row of luma values into the upper half of the output buffer
        for (column = 0; column < width; column++)
        {
            COMPONENT_VALUE Y = Y_input_row[column] >> Y_shift;
            //assert(0 <= Y && Y <= UINT8_MAX);
            upper_output_row[column] = clamp_uint8(Y);
        }
        
        // Pack a row of color difference components for every second row of luma components
        if ((luma_row % 2) == 0)
        {
            // Pack a row of color difference components
            int chroma_row = luma_row/2;
            COMPONENT_VALUE *U_input_row = U_input_array + chroma_row * image->component_array_list[1].width;
            COMPONENT_VALUE *V_input_row = V_input_array + chroma_row * image->component_array_list[2].width;
            uint8_t *lower_output_row = lower_output_plane + chroma_row * width;
            int column;
            
            for (column = 0; column < width/2; column++)
            {
                COMPONENT_VALUE U = U_input_row[column] >> U_shift;
                COMPONENT_VALUE V = V_input_row[column] >> V_shift;
                //assert(0 <= U && U <= UINT8_MAX);
                //assert(0 <= V && V <= UINT8_MAX);
                lower_output_row[2 * column + 0] = clamp_uint8(U);
                lower_output_row[2 * column + 1] = clamp_uint8(V);
            }
        }
    }
    
    return CODEC_ERROR_OKAY;
}

/*!
	@brief Pack the component arrays into an output image
	
	@todo Change this routine to use 10-bit DPX component values.
*/
CODEC_ERROR PackComponentsToDPX0(const UNPACKED_IMAGE *image,
								 PIXEL *output_buffer, size_t output_pitch,
								 DIMENSION width, DIMENSION height,
                                 ENABLED_PARTS enabled_parts)
{
	// Define pointers to the rows for each input component
	uint8_t *R_input_buffer;
	uint8_t *G_input_buffer;
	uint8_t *B_input_buffer;

	// The DPX0 format uses 10-bit pixels
	//const int shift = 6;

	// Forced the DPX0 format to use 12-bit pixels for debugging
	const int shift = 4;

	int row;

	R_input_buffer = (uint8_t *)image->component_array_list[0].data;
	G_input_buffer = (uint8_t *)image->component_array_list[1].data;
	B_input_buffer = (uint8_t *)image->component_array_list[2].data;

	for (row = 0; row < height; row++)
	{
		COMPONENT_VALUE *R_input_row_ptr = (COMPONENT_VALUE *)(R_input_buffer + row * image->component_array_list[0].pitch);
		COMPONENT_VALUE *G_input_row_ptr = (COMPONENT_VALUE *)(G_input_buffer + row * image->component_array_list[1].pitch);
		COMPONENT_VALUE *B_input_row_ptr = (COMPONENT_VALUE *)(B_input_buffer + row * image->component_array_list[2].pitch);

		uint32_t *output_row_ptr = (uint32_t *)((uint8_t *)output_buffer + row * output_pitch);

		int column;

		// Pack the rows of RGB components into the DPX pixel format
		for (column = 0; column < width; column++)
		{
			COMPONENT_VALUE R, G, B;

			R = R_input_row_ptr[column];
			G = G_input_row_ptr[column];
			B = B_input_row_ptr[column];

			// Scale the component values to 16 bits
			R <<= shift;
			G <<= shift;
			B <<= shift;

			output_row_ptr[column] = Pack10(R, G, B);
		}
	}

	return CODEC_ERROR_OKAY;
}
