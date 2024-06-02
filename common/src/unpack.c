/*!	@file common/src/unpack.c

	Implementation of the pixel unpacking routines.

	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#include "headers.h"

/*!
	@brief Unpack a row of pixels in the 8-bit YUV 4:2:2 format
	
	The YUV 4:2:2 format is unpacked into the row-oriented format that
	is used internally by the codec as an intermediate representation.

	@todo Add a vector of offsets to each component so that the layout of the components
	can accomodate requirements on memory alignment.

*/
CODEC_ERROR UnpackImageRowYUY2(uint8_t *input_buffer, DIMENSION width, PIXEL *buffer[],
							   PRECISION bits_per_component[], int channel_count,
							   ENABLED_PARTS enabled_parts)
{
	uint8_t *input = (uint8_t *)input_buffer;
	uint16_t *Y_output = (uint16_t *)buffer[0];
	uint16_t *U_output = (uint16_t *)buffer[2];
	uint16_t *V_output = (uint16_t *)buffer[1];

	// Precision of the input pixels
	const int input_precision = 8;
    
    // Internal precision of the encoded values
    const int internal_precision = 12;

	// Scale each pixel to the internal precision for intermediate results
	const int scale_shift = (internal_precision - input_precision);
	//const int scale_shift = (encoded_precision - input_precision);

	int column;

	//TODO: Use the bits per component to compute the scale shift
	(void)bits_per_component;

	(void)channel_count;

	// The frame width must be an even number
	assert((width % 2) == 0);

	// Separate each packed 10-bit DPX pixel into a buffer of 16-bit pixels for each plane
	for (column = 0; column < width; column += 2)
	{
		uint16_t Y1;
		uint16_t Y2;
		uint16_t Cb;
		uint16_t Cr;

		// TODO: Check the order of the chroma components
		Y1 = input[2 * column + 0];
		Y2 = input[2 * column + 2];
		Cb = input[2 * column + 1];
		Cr = input[2 * column + 3];

		Y_output[column + 0] = (uint16_t)(Y1 << scale_shift);
		Y_output[column + 1] = (uint16_t)(Y2 << scale_shift);
		U_output[column/2] = (uint16_t)(Cb << scale_shift);
		V_output[column/2] = (uint16_t)(Cr << scale_shift);
	}

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Unpack a row of pixels in the 16-bit RGB format
	
	The 16-bit RGB format uses a 16-bit unsigned integer to represent each component value
	such that the maximum component value is the maximum unsigned 16-bit integer.
*/
CODEC_ERROR UnpackImageRowRG48(uint8_t *input_buffer, DIMENSION width, PIXEL *buffer[],
							   PRECISION bits_per_component[], int channel_count,
							   ENABLED_PARTS enabled_parts)
{
	uint16_t *input = (uint16_t *)input_buffer;
	uint16_t *R_output;
	uint16_t *G_output;
	uint16_t *B_output;

	// Precision of the input pixels
	const int input_precision = 16;

    // Internal precision of the encoded values
    const int internal_precision = 12;

	// Scale each pixel to the internal precision for intermediate results
	const int scale_shift = (input_precision - internal_precision);

	int column;

	//TODO: Use the bits per component to compute the scale shift
	(void)bits_per_component;

	(void)channel_count;

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
	if (IsPartEnabled(enabled_parts, VC5_PART_IMAGE_FORMATS))
	{
		R_output = (uint16_t *)buffer[1];
		G_output = (uint16_t *)buffer[0];
		B_output = (uint16_t *)buffer[2];
	}
	else
#endif
	{
		R_output = (uint16_t *)buffer[0];
		G_output = (uint16_t *)buffer[1];
		B_output = (uint16_t *)buffer[2];
	}

	// Separate each packed 16-bit RG48 pixel into a buffer of 16-bit pixels for each plane
	for (column = 0; column < width; column++)
	{
		uint16_t R = input[3 * column + 0];
		uint16_t G = input[3 * column + 1];
		uint16_t B = input[3 * column + 2];

		R_output[column] = (uint16_t)(R >> scale_shift);
		G_output[column] = (uint16_t)(G >> scale_shift);
		B_output[column] = (uint16_t)(B >> scale_shift);
	}

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Unpack a row of pixels in the 16-bit ARGB format
	
	The 16-bit ARGB format uses a 16-bit unsigned integer to represent each component value
	such that the maximum component value is the maximum unsigned 16-bit integer.  Component
	values are in big endian order.
*/
CODEC_ERROR UnpackImageRowB64A(uint8_t *input_buffer, DIMENSION width, PIXEL *buffer[],
							   PRECISION bits_per_component[], int channel_count,
							   ENABLED_PARTS enabled_parts)
{
	uint16_t *input = (uint16_t *)input_buffer;

	uint16_t *A_output = (uint16_t *)buffer[0];
	uint16_t *R_output = (uint16_t *)buffer[1];
	uint16_t *G_output = (uint16_t *)buffer[2];
	uint16_t *B_output = (uint16_t *)buffer[3];

	// Precision of the input pixels
	const int input_precision = 16;

    // Internal precision of the encoded values
    const int internal_precision = 12;

	// Scale each pixel to the internal precision for intermediate results
	const int scale_shift = (input_precision - internal_precision);

	int column;

	//TODO: Use the bits per component to compute the scale shift
	(void)bits_per_component;

	(void)channel_count;

	// Separate each packed 16-bit B64A pixel into a buffer of 16-bit pixels for each plane
	for (column = 0; column < width; column++)
	{
		uint16_t A = Swap16(input[4 * column + 0]);
		uint16_t R = Swap16(input[4 * column + 1]);
		uint16_t G = Swap16(input[4 * column + 2]);
		uint16_t B = Swap16(input[4 * column + 3]);

		A_output[column] = (uint16_t)(A >> scale_shift);
		R_output[column] = (uint16_t)(R >> scale_shift);
		G_output[column] = (uint16_t)(G >> scale_shift);
		B_output[column] = (uint16_t)(B >> scale_shift);
	}

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Unpack an image in NV12 format into separate component arrays
	
	The NV12 format uses a plane of 8-bit luma components followed by a plane of
	interleaved packed 8-bit color difference components.
 
    The 8-bit pixels are scaled to 12 bits to match the processing path used by
    other formats such as BYR4.
*/
CODEC_ERROR UnpackImageNV12(uint8_t *input_buffer,
							DIMENSION width,
							DIMENSION height,
							PIXEL *output_buffer_list[3])
{
	uint8_t *upper_input_plane = (uint8_t *)input_buffer;
	uint8_t *lower_input_plane = upper_input_plane + (width * height);

	PIXEL *Y_output_array = output_buffer_list[0];
	PIXEL *U_output_array = output_buffer_list[1];
	PIXEL *V_output_array = output_buffer_list[2];
    
    int scale = 4;

	int luma_row;

	for (luma_row = 0; luma_row < height; luma_row++)
	{
		uint8_t *upper_input_row = upper_input_plane + luma_row * width;
		PIXEL *Y_output_row = Y_output_array + luma_row * width;
		int column;

		// Unpack the row of luma values into the component array for luma
		for (column = 0; column < width; column++)
		{
            PIXEL Y = upper_input_row[column];
            Y_output_row[column] = Y << scale;
		}

		// Unpack a row of color difference components for every second row of luma components
		if ((luma_row % 2) == 0)
		{
			// Unpack the row of color difference components
            int chroma_row = luma_row/2;
			uint8_t *lower_input_row = lower_input_plane + chroma_row * width;
			PIXEL *U_output_row = U_output_array + chroma_row * width/2;
			PIXEL *V_output_row = V_output_array + chroma_row * width/2;
			int column;

			for (column = 0; column < width/2; column++)
			{
				PIXEL U = lower_input_row[2 * column + 0];
				PIXEL V = lower_input_row[2 * column + 1];

                U_output_row[column] = U << scale;
                V_output_row[column] = V << scale;
            }
		}
	}

	return CODEC_ERROR_OKAY;
}
