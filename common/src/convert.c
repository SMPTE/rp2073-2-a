/*!	@file common/src/convert.c

	Implementation of functions for converting between the pixel formats
	used by the codec and pixel formats used by test programs and file
	formats used during debugging.

	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

//! Control debugging statements in this module
#ifndef DEBUG
#define DEBUG (1 && _DEBUG)
#endif

#include "headers.h"
#include "dpxfile.h"


/*!
	@brief Convert the Bayer BYR3 format to the format used for DPX files

	The output format is 10-bit RGB packed into a 32-bit word with two
	bits unused.  The repacking routine may byte swap the 32-bit word.
*/
CODEC_ERROR ConvertBYR3ToDPX0(void *input_buffer, size_t input_pitch,
							  void *output_buffer, size_t output_pitch,
							  int width, int height)
{

	uint8_t *input_row_ptr = (uint8_t *)input_buffer;
	size_t input_row_pitch = input_pitch;
	size_t quarter_row_pitch = input_pitch / 4;

	uint8_t *output_row_ptr = (uint8_t *)output_buffer;
	size_t output_row_pitch = output_pitch;

	int row;

	assert(input_buffer != NULL && output_buffer != NULL);

	for (row = 0; row < height; row++)
	{
		uint16_t *R_pixel_ptr = (uint16_t *)input_row_ptr;
		uint16_t *G1_pixel_ptr = (uint16_t *)(input_row_ptr + quarter_row_pitch);
		uint16_t *G2_pixel_ptr = (uint16_t *)(input_row_ptr + 2 * quarter_row_pitch);
		uint16_t *B_pixel_ptr = (uint16_t *)(input_row_ptr + 3 * quarter_row_pitch);
		uint32_t *output_pixel_ptr = (uint32_t *)output_row_ptr;

		int column;

		// Convert each 10-bit Bayer tuple to a packed 10-bit DPX pixel
		for (column = 0; column < width; column++)
		{
			int32_t R = *(R_pixel_ptr++);
			int32_t G1 = *(G1_pixel_ptr++);
			int32_t G2 = *(G2_pixel_ptr++);
			int32_t B = *(B_pixel_ptr++);

			int32_t G;

			// Scale the values to 16 bits
			R <<= 6;
			G1 <<= 6;
			G2 <<= 6;
			B <<= 6;

			// Average the green pixels
			G = (G1 + G2) / 2;

			// Clamp the pixel values to the range of 16-bit unsigned integers
			if (R > UINT16_MAX) R = UINT16_MAX;
			else if (R < 0) R = 0;

			if (G > UINT16_MAX) G = UINT16_MAX;
			else if (G < 0) G = 0;

			if (B > UINT16_MAX) B = UINT16_MAX;
			else if (B < 0) B = 0;

			// Pack 16-bit components into a 32-bit word of 10-bit components
			*(output_pixel_ptr++) = Pack10(R, G, B);
		}

		input_row_ptr += input_row_pitch;
		output_row_ptr += output_row_pitch;
	}

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Convert 16-bit Bayer to the common DPX pixel format

	The output format is 10-bit RGB packed into a 32-bit word with two
	bits unused.  The repacking routine may byte swap the 32-bit word.
*/
CODEC_ERROR ConvertBYR4ToDPX0(void *input_buffer, size_t input_pitch,
							  void *output_buffer, size_t output_pitch,
							  int width, int height)
{
	uint8_t *input_row_ptr = (uint8_t *)input_buffer;
	size_t input_row_pitch = input_pitch;
	size_t half_row_pitch = input_pitch / 2;

	uint8_t *output_row_ptr = (uint8_t *)output_buffer;
	size_t output_row_pitch = output_pitch;

	int row;

	assert(input_buffer != NULL && output_buffer != NULL);

	for (row = 0; row < height; row++)
	{
		uint16_t *row1_pixel_ptr = (uint16_t *)input_row_ptr;
		uint16_t *row2_pixel_ptr = (uint16_t *)(input_row_ptr + half_row_pitch);
		uint32_t *output_pixel_ptr = (uint32_t *)output_row_ptr;

		int column;

		// Convert each 16-bit Bayer tuple to a packed 10-bit DPX pixel
		for (column = 0; column < width; column++)
		{
			int32_t R = *(row1_pixel_ptr++);
			int32_t G1 = *(row1_pixel_ptr++);
			int32_t G2 = *(row2_pixel_ptr++);
			int32_t B = *(row2_pixel_ptr++);

			// Average the green pixels
			int32_t G = (G1 + G2) / 2;

			// Clamp the pixel values to the range of 16-bit unsigned integers
			if (R > UINT16_MAX) R = UINT16_MAX;
			else if (R < 0) R = 0;

			if (G > UINT16_MAX) G = UINT16_MAX;
			else if (G < 0) G = 0;

			if (B > UINT16_MAX) B = UINT16_MAX;
			else if (B < 0) B = 0;

			// Pack 16-bit components into a 32-bit word of 10-bit components
			*(output_pixel_ptr++) = Pack10(R, G, B);
		}

		input_row_ptr += input_row_pitch;
		output_row_ptr += output_row_pitch;
	}

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Convert packed 16-bit RGB to the common DPX pixel format

	The output format is 10-bit RGB packed into a 32-bit word with two
	bits unused.  The repacking routine may byte swap the 32-bit word.
*/
CODEC_ERROR ConvertRG48ToDPX0(void *input_buffer, size_t input_pitch,
							  void *output_buffer, size_t output_pitch,
							  int width, int height)
{
	uint8_t *input_row_ptr = (uint8_t *)input_buffer;
	size_t input_row_pitch = input_pitch;

	uint8_t *output_row_ptr = (uint8_t *)output_buffer;
	size_t output_row_pitch = output_pitch;

	int row;

	assert(input_buffer != NULL && output_buffer != NULL);

	for (row = 0; row < height; row++)
	{
		uint16_t *input = (uint16_t *)input_row_ptr;
		uint32_t *output = (uint32_t *)output_row_ptr;
		int column;

		// Convert each 16-bit RGB tuple to a packed 10-bit DPX pixel
		for (column = 0; column < width; column++)
		{
			uint16_t R = input[3 * column + 0];
			uint16_t G = input[3 * column + 1];
			uint16_t B = input[3 * column + 2];

			// Pack 16-bit components into a 32-bit word of 10-bit components
			output[column] = Pack10(R, G, B);
		}

		input_row_ptr += input_row_pitch;
		output_row_ptr += output_row_pitch;
	}

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Convert packed 16-bit BGRA to the common DPX pixel format

	The output format is 10-bit RGB packed into a 32-bit word with two
	bits unused.  The repacking routine may byte swap the 32-bit word.
*/
CODEC_ERROR ConvertB64AToDPX0(void *input_buffer, size_t input_pitch,
							  void *output_buffer, size_t output_pitch,
							  int width, int height)
{
	uint8_t *input_row_ptr = (uint8_t *)input_buffer;
	size_t input_row_pitch = input_pitch;

	uint8_t *output_row_ptr = (uint8_t *)output_buffer;
	size_t output_row_pitch = output_pitch;

	int row;

	assert(input_buffer != NULL && output_buffer != NULL);

	for (row = 0; row < height; row++)
	{
		uint16_t *input = (uint16_t *)input_row_ptr;
		uint32_t *output = (uint32_t *)output_row_ptr;
		int column;

		// Convert each 16-bit RGB tuple to a packed 10-bit DPX pixel
		for (column = 0; column < width; column++)
		{
			uint16_t R = Swap16(input[4 * column + 1]);
			uint16_t G = Swap16(input[4 * column + 2]);
			uint16_t B = Swap16(input[4 * column + 3]);

			// Pack 16-bit components into a 32-bit word of 10-bit components
			output[column] = Pack10(R, G, B);
		}

		input_row_ptr += input_row_pitch;
		output_row_ptr += output_row_pitch;
	}

	return CODEC_ERROR_OKAY;
}

/*!
	Pack a set of component arrays into DPX0 format

	This routine was written for debugging.
*/
CODEC_ERROR ConvertComponentsToDPX0(const UNPACKED_IMAGE *input,
									PIXEL_FORMAT format,
									IMAGE *output,
									ENABLED_PARTS enabled_parts)
{
	switch (format)
	{
	case PIXEL_FORMAT_BYR4:
		return ConvertComponentsBYR4ToDPX0(input, output, enabled_parts);
		break;

	case PIXEL_FORMAT_RG48:
		return ConvertComponentsRG48ToDPX0(input, output, enabled_parts);
		break;

	case PIXEL_FORMAT_B64A:
		return ConvertComponentsB64AToDPX0(input, output, enabled_parts);
		break;

	case PIXEL_FORMAT_DPX0:
		return ConvertComponentsDPX0ToDPX0(input, output, enabled_parts);
		break;

	default:
		// The arrangement of color components in the unpacked image is not supported
		assert(0);
		break;
	}

	// Could not handle the pixel format of the unpacked color components
	return CODEC_ERROR_PIXEL_FORMAT;
}

/*!
	Pack a set of BYR4 component arrays into DPX0 format

	This routine was written for debugging and assumes that the component arrays
	contain BYR4 color components in the same order as the BYR4 pixel format.
*/
CODEC_ERROR ConvertComponentsBYR4ToDPX0(const UNPACKED_IMAGE *input,
										IMAGE *output,
										ENABLED_PARTS enabled_parts)
{
	uint8_t *C1_input_buffer;
	uint8_t *C2_input_buffer;
	uint8_t *C3_input_buffer;
	uint8_t *C4_input_buffer;

	DIMENSION width;
	DIMENSION height;

	uint8_t *output_row_ptr = (uint8_t *)output->buffer;
    
    // The precision of the Bayer components is the maximum number of bits in all channels
    const int max_bits_per_component = MaxBitsPerComponent(input);

	// Shift the component values to 16 bits for the DPX packing routine
	const int shift = 16 - max_bits_per_component;

	int row;

	(void)enabled_parts;

	C1_input_buffer = (uint8_t *)input->component_array_list[0].data;
	C2_input_buffer = (uint8_t *)input->component_array_list[1].data;
	C3_input_buffer = (uint8_t *)input->component_array_list[2].data;
	C4_input_buffer = (uint8_t *)input->component_array_list[3].data;

	width = input->component_array_list[0].width;
	height = input->component_array_list[0].height;

	for (row = 0; row < height; row++)
	{
		COMPONENT_VALUE *C1_input_row_ptr = (COMPONENT_VALUE *)(C1_input_buffer + row * input->component_array_list[0].pitch);
		COMPONENT_VALUE *C2_input_row_ptr = (COMPONENT_VALUE *)(C2_input_buffer + row * input->component_array_list[1].pitch);
		COMPONENT_VALUE *C3_input_row_ptr = (COMPONENT_VALUE *)(C3_input_buffer + row * input->component_array_list[2].pitch);
		COMPONENT_VALUE *C4_input_row_ptr = (COMPONENT_VALUE *)(C4_input_buffer + row * input->component_array_list[3].pitch);

		uint32_t *output_pixel_ptr = (uint32_t *)output_row_ptr;

		int column;

		// Convert each 16-bit Bayer tuple to a packed 10-bit DPX pixel
		for (column = 0; column < width; column++)
		{
			int32_t C1 = *(C1_input_row_ptr++);
			int32_t C2 = *(C2_input_row_ptr++);
			int32_t C3 = *(C3_input_row_ptr++);
			int32_t C4 = *(C4_input_row_ptr++);
			int32_t R, G, B;

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
			if (IsPartEnabled(enabled_parts, VC5_PART_IMAGE_FORMATS))
			{
				int32_t G1, G2;
				//const int32_t midpoint = (1 << (internal_precision - 1));
                const int32_t midpoint = (1 << (max_bits_per_component - 1));

				// Convert the color differences to signed values
				C2 -= midpoint;
				C3 -= midpoint;
				C4 -= midpoint;

				// Apply the inverse Bayer component transform
				G1 = (C1 + C4);
				G2 = (C1 - C4);
				R = (C1 + (C2 << 1));
				B = (C1 + (C3 << 1));

				// Average the green component values
				G = (G1 + G1) / 2;
			}
			else
#endif
			{
				// Average the green component values
				R = C1;
				G = (C2 + C3) / 2;
				B = C4;
			}

			R <<= shift;
			G <<= shift;
			B <<= shift;

			// Pack 16-bit components into a 32-bit word of 10-bit components
			*(output_pixel_ptr++) = Pack10(R, G, B);
		}

		output_row_ptr += output->pitch;
	}

	return CODEC_ERROR_OKAY;
}

/*!
	Pack a set of RG48 component arrays into DPX0 format

	This routine was written for debugging and assumes that the component arrays
	contain RGB color components in the same order as the RG48 pixel format.
*/
CODEC_ERROR ConvertComponentsRG48ToDPX0(const UNPACKED_IMAGE *input,
										IMAGE *output,
										ENABLED_PARTS enabled_parts)
{
	uint8_t *R_input_buffer;
	uint8_t *G_input_buffer;
	uint8_t *B_input_buffer;

	DIMENSION width;
	DIMENSION height;

	uint8_t *output_row_ptr = (uint8_t *)output->buffer;

    // The precision of the Bayer components is the maximum number of bits in all channels
    const int max_bits_per_component = MaxBitsPerComponent(input);
    
    // Shift the component values to 16 bits for the DPX packing routine
    const int shift = 16 - max_bits_per_component;
    
	int row;

	(void)enabled_parts;

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
	if (IsPartEnabled(enabled_parts, VC5_PART_IMAGE_FORMATS))
	{
		R_input_buffer = (uint8_t *)input->component_array_list[1].data;
		G_input_buffer = (uint8_t *)input->component_array_list[0].data;
		B_input_buffer = (uint8_t *)input->component_array_list[2].data;
	}
	else
#endif
	{
		R_input_buffer = (uint8_t *)input->component_array_list[0].data;
		G_input_buffer = (uint8_t *)input->component_array_list[1].data;
		B_input_buffer = (uint8_t *)input->component_array_list[2].data;
	}

	width = input->component_array_list[0].width;
	height = input->component_array_list[0].height;

	for (row = 0; row < height; row++)
	{
		COMPONENT_VALUE *R_input_row_ptr = (COMPONENT_VALUE *)(R_input_buffer + row * input->component_array_list[0].pitch);
		COMPONENT_VALUE *G_input_row_ptr = (COMPONENT_VALUE *)(G_input_buffer + row * input->component_array_list[1].pitch);
		COMPONENT_VALUE *B_input_row_ptr = (COMPONENT_VALUE *)(B_input_buffer + row * input->component_array_list[2].pitch);

		uint32_t *output_pixel_ptr = (uint32_t *)output_row_ptr;

		int column;

		// Convert each 16-bit tuple to a packed 10-bit DPX pixel
		for (column = 0; column < width; column++)
		{
			int32_t R = *(R_input_row_ptr++);
			int32_t G = *(G_input_row_ptr++);
			int32_t B = *(B_input_row_ptr++);

			R <<= shift;
			G <<= shift;
			B <<= shift;

			// Pack 16-bit components into a 32-bit word of 10-bit components
			*(output_pixel_ptr++) = Pack10(R, G, B);
		}

		output_row_ptr += output->pitch;
	}

	return CODEC_ERROR_OKAY;
}

/*!
	Pack a set of B64A component arrays into DPX0 format

	This routine was written for debugging and assumes that the component arrays
	contain RGB color components in the same order as the B64A pixel format.
*/
CODEC_ERROR ConvertComponentsB64AToDPX0(const UNPACKED_IMAGE *input,
										IMAGE *output,
										ENABLED_PARTS enabled_parts)
{
	uint8_t *A_input_buffer;
	uint8_t *R_input_buffer;
	uint8_t *G_input_buffer;
	uint8_t *B_input_buffer;

	DIMENSION width;
	DIMENSION height;

	uint8_t *output_row_ptr = (uint8_t *)output->buffer;

	// Shift the component values to 16 bits for the DPX packing routine
	const int shift = 4;

	int row;
	
	(void)enabled_parts;

	A_input_buffer = (uint8_t *)input->component_array_list[0].data;
	R_input_buffer = (uint8_t *)input->component_array_list[1].data;
	G_input_buffer = (uint8_t *)input->component_array_list[2].data;
	B_input_buffer = (uint8_t *)input->component_array_list[3].data;

	width = input->component_array_list[0].width;
	height = input->component_array_list[0].height;

	for (row = 0; row < height; row++)
	{
		COMPONENT_VALUE *A_input_row_ptr = (COMPONENT_VALUE *)(A_input_buffer + row * input->component_array_list[0].pitch);
		COMPONENT_VALUE *R_input_row_ptr = (COMPONENT_VALUE *)(R_input_buffer + row * input->component_array_list[1].pitch);
		COMPONENT_VALUE *G_input_row_ptr = (COMPONENT_VALUE *)(G_input_buffer + row * input->component_array_list[2].pitch);
		COMPONENT_VALUE *B_input_row_ptr = (COMPONENT_VALUE *)(B_input_buffer + row * input->component_array_list[3].pitch);

		uint32_t *output_pixel_ptr = (uint32_t *)output_row_ptr;

		int column;

		// Convert each 16-bit tuple to a packed 10-bit DPX pixel
		for (column = 0; column < width; column++)
		{
			int32_t A = *(A_input_row_ptr++);
			int32_t R = *(R_input_row_ptr++);
			int32_t G = *(G_input_row_ptr++);
			int32_t B = *(B_input_row_ptr++);

			A <<= shift;
			R <<= shift;
			G <<= shift;
			B <<= shift;

			// Pack 16-bit components into a 32-bit word of 10-bit components
			*(output_pixel_ptr++) = Pack10(R, G, B);
		}

		output_row_ptr += output->pitch;
	}

	return CODEC_ERROR_OKAY;
}

/*!
	Pack a set of DPX0 component arrays into DPX0 format

	This routine was written for debugging and assumes that the component arrays
	contain RGB color components in the same order as the DPX0 pixel format.
*/
CODEC_ERROR ConvertComponentsDPX0ToDPX0(const UNPACKED_IMAGE *input,
										IMAGE *output,
										ENABLED_PARTS enabled_parts)
{
	uint8_t *R_input_buffer;
	uint8_t *G_input_buffer;
	uint8_t *B_input_buffer;

	DIMENSION width;
	DIMENSION height;

	uint8_t *output_row_ptr = (uint8_t *)output->buffer;

	// Shift the component values to 16 bits for the DPX packing routine
	const int shift = 4;
	//const int shift = 6;

	int row;
	
	(void)enabled_parts;

	R_input_buffer = (uint8_t *)input->component_array_list[0].data;
	G_input_buffer = (uint8_t *)input->component_array_list[1].data;
	B_input_buffer = (uint8_t *)input->component_array_list[2].data;

	width = input->component_array_list[0].width;
	height = input->component_array_list[0].height;

	for (row = 0; row < height; row++)
	{
		COMPONENT_VALUE *R_input_row_ptr = (COMPONENT_VALUE *)(R_input_buffer + row * input->component_array_list[0].pitch);
		COMPONENT_VALUE *G_input_row_ptr = (COMPONENT_VALUE *)(G_input_buffer + row * input->component_array_list[1].pitch);
		COMPONENT_VALUE *B_input_row_ptr = (COMPONENT_VALUE *)(B_input_buffer + row * input->component_array_list[2].pitch);

		uint32_t *output_pixel_ptr = (uint32_t *)output_row_ptr;

		int column;

		// Convert each 16-bit Bayer tuple to a packed 10-bit DPX pixel
		for (column = 0; column < width; column++)
		{
			int32_t R = *(R_input_row_ptr++);
			int32_t G = *(G_input_row_ptr++);
			int32_t B = *(B_input_row_ptr++);

			R <<= shift;
			G <<= shift;
			B <<= shift;

			// Pack 16-bit components into a 32-bit word of 10-bit components
			*(output_pixel_ptr++) = Pack10(R, G, B);
		}

		output_row_ptr += output->pitch;
	}

	return CODEC_ERROR_OKAY;
}
