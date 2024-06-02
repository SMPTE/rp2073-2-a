/*!	@file decoder/src/inverse.c

	Implementation of the module for inverse spatial transforms.

	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#include "headers.h"

//! Enable debugging output in this module
#ifndef DEBUG
#define DEBUG (1 && _DEBUG)
#endif


//! Rounding adjustment used by the inverse wavelet transforms
static const int32_t rounding = 4;


/*!
	@brief Apply the inverse spatial wavelet filter

	Dequantize the coefficients in the highpass bands and apply the
	inverse spatial wavelet filter to compute a lowpass band that
	has twice the width and height of the input bands.

	The inverse vertical filter is applied to the upper and lower bands
	on the left and the upper and lower bands on the right.  The inverse
	horizontal filter is applied to the left and right (lowpass and highpass)
	results from the vertical inverse.  Each application of the inverse
	vertical filter produces two output rows and each application of the
	inverse horizontal filter produces an output row that is twice as wide.

	The inverse wavelet filter is a three tap filter.
	
	For the even output values, add and subtract the off-center values,
	add the rounding correction, and divide by eight, then add the center
	value, add the highpass coefficient, and divide by two.
	
	For the odd output values, the add and subtract operations for the
	off-center values are reversed the the highpass coefficient is subtracted.

	Divisions are implemented by right arithmetic shifts.

	Special formulas for the inverse vertical filter are applied to the top
	and bottom rows.
*/
CODEC_ERROR InvertSpatialQuant16s(ALLOCATOR *allocator,
								  PIXEL *lowlow_band, int lowlow_pitch,
								  PIXEL *lowhigh_band, int lowhigh_pitch,
								  PIXEL *highlow_band, int highlow_pitch,
								  PIXEL *highhigh_band, int highhigh_pitch,
								  PIXEL *output_image, int output_pitch,
								  DIMENSION input_width, DIMENSION input_height,
								  DIMENSION output_width, DIMENSION output_height,
								  QUANT quantization[])
{
	PIXEL *lowlow = (PIXEL *)lowlow_band;
	PIXEL *lowhigh = lowhigh_band;
	PIXEL *highlow = highlow_band;
	PIXEL *highhigh = highhigh_band;
	PIXEL *output = output_image;
	PIXEL *even_lowpass;
	PIXEL *even_highpass;
	PIXEL *odd_lowpass;
	PIXEL *odd_highpass;
	PIXEL *even_output;
	PIXEL *odd_output;
	size_t buffer_row_size;
	int last_row = input_height - 1;
	int row, column;

	PIXEL *lowhigh_row[3];

	PIXEL *lowhigh_line[3];
	PIXEL *highlow_line;
	PIXEL *highhigh_line;

	QUANT highlow_quantization = quantization[HL_BAND];
	QUANT lowhigh_quantization = quantization[LH_BAND];
	QUANT highhigh_quantization = quantization[HH_BAND];

	// Pointer to the last row used from the LH band (for debugging)
	PIXEL *last_lowhigh_row_ptr = NULL;

	// Compute positions within the temporary buffer for each row of horizontal lowpass
	// and highpass intermediate coefficients computed by the vertical inverse transform
	buffer_row_size = input_width * sizeof(PIXEL);

	// Compute the positions of the even and odd rows of coefficients
	even_lowpass = (PIXEL *)Alloc(allocator, buffer_row_size);
	even_highpass = (PIXEL *)Alloc(allocator, buffer_row_size);
	odd_lowpass = (PIXEL *)Alloc(allocator, buffer_row_size);
	odd_highpass = (PIXEL *)Alloc(allocator, buffer_row_size);

	// Compute the positions of the dequantized highpass rows
	lowhigh_line[0] = (PIXEL *)Alloc(allocator, buffer_row_size);
	lowhigh_line[1] = (PIXEL *)Alloc(allocator, buffer_row_size);
	lowhigh_line[2] = (PIXEL *)Alloc(allocator, buffer_row_size);
	highlow_line = (PIXEL *)Alloc(allocator, buffer_row_size);
	highhigh_line = (PIXEL *)Alloc(allocator, buffer_row_size);

	// Convert pitch from bytes to pixels
	lowlow_pitch /= sizeof(PIXEL);
	lowhigh_pitch /= sizeof(PIXEL);
	highlow_pitch /= sizeof(PIXEL);
	highhigh_pitch /= sizeof(PIXEL);
	output_pitch /= sizeof(PIXEL);

	// Initialize the pointers to the even and odd output rows
	even_output = output;
	odd_output = output + output_pitch;

	// Apply the vertical border filter to the first row
	row = 0;

	// Set pointers to the first three rows in the first highpass band
	lowhigh_row[0] = lowhigh + 0 * lowhigh_pitch;
	lowhigh_row[1] = lowhigh + 1 * lowhigh_pitch;
	lowhigh_row[2] = lowhigh + 2 * lowhigh_pitch;

	// Dequantize three rows of highpass coefficients in the first highpass band
	DequantizeBandRow16s(lowhigh_row[0], input_width, lowhigh_quantization, lowhigh_line[0]);
	DequantizeBandRow16s(lowhigh_row[1], input_width, lowhigh_quantization, lowhigh_line[1]);
	DequantizeBandRow16s(lowhigh_row[2], input_width, lowhigh_quantization, lowhigh_line[2]);

	// Dequantize one row of coefficients each in the second and third highpass bands
	DequantizeBandRow16s(highlow, input_width, highlow_quantization, highlow_line);
	DequantizeBandRow16s(highhigh, input_width, highhigh_quantization, highhigh_line);

	for (column = 0; column < input_width; column++)
	{
		int32_t even = 0;		// Result of convolution with even filter
		int32_t odd = 0;		// Result of convolution with odd filter


		/***** Compute the vertical inverse for the left two bands *****/

		// Apply the even reconstruction filter to the lowpass band
		even += 11 * lowlow[column + 0 * lowlow_pitch];
		even -=  4 * lowlow[column + 1 * lowlow_pitch];
		even +=  1 * lowlow[column + 2 * lowlow_pitch];
		even += rounding;
		even = DivideByShift(even, 3);

		// Add the highpass correction
		even += highlow_line[column];
		even >>= 1;

		// The inverse of the left two bands should be a positive number
		//assert(0 <= even && even <= INT16_MAX);

		// Place the even result in the even row
		even_lowpass[column] = ClampPixel(even);

		// Apply the odd reconstruction filter to the lowpass band
		odd += 5 * lowlow[column + 0 * lowlow_pitch];
		odd += 4 * lowlow[column + 1 * lowlow_pitch];
		odd -= 1 * lowlow[column + 2 * lowlow_pitch];
		odd += rounding;
		odd = DivideByShift(odd, 3);

		// Subtract the highpass correction
		odd -= highlow_line[column];
		odd >>= 1;

		// The inverse of the left two bands should be a positive number
		//assert(0 <= odd && odd <= INT16_MAX);

		// Place the odd result in the odd row
		odd_lowpass[column] = ClampPixel(odd);


		/***** Compute the vertical inverse for the right two bands *****/

		even = 0;
		odd = 0;

		// Apply the even reconstruction filter to the lowpass band
		even += 11 * lowhigh_line[0][column];
		even -=  4 * lowhigh_line[1][column];
		even +=  1 * lowhigh_line[2][column];
		even += rounding;
		even = DivideByShift(even, 3);

		// Add the highpass correction
		even += highhigh_line[column];
		even >>= 1;

		// Place the even result in the even row
		even_highpass[column] = ClampPixel(even);

		// Apply the odd reconstruction filter to the lowpass band
		odd += 5 * lowhigh_line[0][column];
		odd += 4 * lowhigh_line[1][column];
		odd -= 1 * lowhigh_line[2][column];
		odd += rounding;
		odd = DivideByShift(odd, 3);

		// Subtract the highpass correction
		odd -= highhigh_line[column];
		odd >>= 1;

		// Place the odd result in the odd row
		odd_highpass[column] = ClampPixel(odd);
	}

	// Apply the inverse horizontal transform to the even and odd rows
	InvertHorizontal16s(even_lowpass, even_highpass, even_output, input_width, output_width);
	InvertHorizontal16s(odd_lowpass, odd_highpass, odd_output, input_width, output_width);

	// Advance to the next pair of even and odd output rows
	even_output += 2 * output_pitch;
	odd_output += 2 * output_pitch;

	// Always advance the highpass row pointers
	highlow += highlow_pitch;
	highhigh += highhigh_pitch;

	// Advance the row index
	row++;

	// Process the middle rows using the interior reconstruction filters
	for (; row < last_row; row++)
	{
		// Dequantize one row from each of the two highpass bands
		DequantizeBandRow16s(highlow, input_width, highlow_quantization, highlow_line);
		DequantizeBandRow16s(highhigh, input_width, highhigh_quantization, highhigh_line);

		// Process the entire row
		for (column = 0; column < input_width; column++)
		{
			int32_t even = 0;		// Result of convolution with even filter
			int32_t odd = 0;		// Result of convolution with odd filter


			/***** Compute the vertical inverse for the left two bands *****/

			// Apply the even reconstruction filter to the lowpass band
			even += lowlow[column + 0 * lowlow_pitch];
			even -= lowlow[column + 2 * lowlow_pitch];
			even += 4;
			even >>= 3;
			even += lowlow[column + 1 * lowlow_pitch];

			// Add the highpass correction
			even += highlow_line[column];
			even >>= 1;

			// The inverse of the left two bands should be a positive number
			//assert(0 <= even && even <= INT16_MAX);

			// Place the even result in the even row
			even_lowpass[column] = ClampPixel(even);

			// Apply the odd reconstruction filter to the lowpass band
			odd -= lowlow[column + 0 * lowlow_pitch];
			odd += lowlow[column + 2 * lowlow_pitch];
			odd += 4;
			odd >>= 3;
			odd += lowlow[column + 1 * lowlow_pitch];

			// Subtract the highpass correction
			odd -= highlow_line[column];
			odd >>= 1;

			// The inverse of the left two bands should be a positive number
			//assert(0 <= odd && odd <= INT16_MAX);

			// Place the odd result in the odd row
			odd_lowpass[column] = ClampPixel(odd);


			/***** Compute the vertical inverse for the right two bands *****/

			even = 0;
			odd = 0;

			// Apply the even reconstruction filter to the lowpass band
			even += lowhigh_line[0][column];
			even -= lowhigh_line[2][column];
			even += 4;
			even >>= 3;
			even += lowhigh_line[1][column];

			// Add the highpass correction
			even += highhigh_line[column];
			even >>= 1;

			// Place the even result in the even row
			even_highpass[column] = ClampPixel(even);

			// Apply the odd reconstruction filter to the lowpass band
			odd -= lowhigh_line[0][column];
			odd += lowhigh_line[2][column];
			odd += 4;
			odd >>= 3;
			odd += lowhigh_line[1][column];

			// Subtract the highpass correction
			odd -= highhigh_line[column];
			odd >>= 1;

			// Place the odd result in the odd row
			odd_highpass[column] = ClampPixel(odd);
		}

		// Apply the inverse horizontal transform to the even and odd rows and descale the results
		InvertHorizontal16s(even_lowpass, even_highpass, even_output, input_width, output_width);
		InvertHorizontal16s(odd_lowpass, odd_highpass, odd_output, input_width, output_width);

		// Advance to the next input row in each band
		lowlow += lowlow_pitch;
		lowhigh += lowhigh_pitch;
		highlow += highlow_pitch;
		highhigh += highhigh_pitch;

		// Advance to the next pair of even and odd output rows
		even_output += 2 * output_pitch;
		odd_output += 2 * output_pitch;

		if (row < last_row - 1)
		{
			// Compute the address of the next row in the lowhigh band
			PIXEL *lowhigh_row_ptr = (lowhigh + 2 * lowhigh_pitch);
			//PIXEL *lowhigh_row_ptr = (lowhigh + lowhigh_pitch);

			// Shift the rows in the buffer of dequantized lowhigh bands
			PIXEL *temp = lowhigh_line[0];
			lowhigh_line[0] = lowhigh_line[1];
			lowhigh_line[1] = lowhigh_line[2];
			lowhigh_line[2] = temp;

			// Undo quantization for the next row in the lowhigh band
			DequantizeBandRow16s(lowhigh_row_ptr, input_width, lowhigh_quantization, lowhigh_line[2]);

			// Save the pointer to the last row in the LH band (for debugging)
			last_lowhigh_row_ptr = lowhigh_row_ptr;
		}
	}

	// Should have exited the loop at the last row
	assert(row == last_row);

	// Advance the lowlow pointer to the last row in the band
	lowlow += lowlow_pitch;

	// Check that the band pointers are on the last row in each wavelet band
	assert(lowlow == (lowlow_band + last_row * lowlow_pitch));
	assert(last_lowhigh_row_ptr == (lowhigh_band + last_row * lowhigh_pitch));
	assert(highlow == (highlow_band + last_row * highlow_pitch));
	assert(highhigh == (highhigh_band + last_row * highhigh_pitch));

	// Undo quantization for the highlow and highhigh bands
	DequantizeBandRow16s(highlow, input_width, highlow_quantization, highlow_line);
	DequantizeBandRow16s(highhigh, input_width, highhigh_quantization, highhigh_line);

	// Apply the vertical border filter to the last row
	for (column = 0; column < input_width; column++)
	{
		int32_t even = 0;		// Result of convolution with even filter
		int32_t odd = 0;		// Result of convolution with odd filter


		/***** Compute the vertical inverse for the left two bands *****/

		// Apply the even reconstruction filter to the lowpass band
		even += 5 * lowlow[column + 0 * lowlow_pitch];
		even += 4 * lowlow[column - 1 * lowlow_pitch];
		even -= 1 * lowlow[column - 2 * lowlow_pitch];
		even += 4;
		even = DivideByShift(even, 3);

		// Add the highpass correction
		even += highlow_line[column];
		even >>= 1;

		// The inverse of the left two bands should be a positive number
		//assert(0 <= even && even <= INT16_MAX);

		// Place the even result in the even row
		even_lowpass[column] = ClampPixel(even);

		// Apply the odd reconstruction filter to the lowpass band
		odd += 11 * lowlow[column + 0 * lowlow_pitch];
		odd -=  4 * lowlow[column - 1 * lowlow_pitch];
		odd +=  1 * lowlow[column - 2 * lowlow_pitch];
		odd += 4;
		odd = DivideByShift(odd, 3);

		// Subtract the highpass correction
		odd -= highlow_line[column];
		odd >>= 1;

		// The inverse of the left two bands should be a positive number
		//assert(0 <= odd && odd <= INT16_MAX);

		// Place the odd result in the odd row
		odd_lowpass[column] = ClampPixel(odd);


		// Compute the vertical inverse for the right two bands //

		even = 0;
		odd = 0;

		// Apply the even reconstruction filter to the lowpass band
		even += 5 * lowhigh_line[2][column];
		even += 4 * lowhigh_line[1][column];
		even -= 1 * lowhigh_line[0][column];
		even += 4;
		even = DivideByShift(even, 3);

		// Add the highpass correction
		even += highhigh_line[column];
		even >>= 1;

		// Place the even result in the even row
		even_highpass[column] = ClampPixel(even);

		// Apply the odd reconstruction filter to the lowpass band
		odd += 11 * lowhigh_line[2][column];
		odd -=  4 * lowhigh_line[1][column];
		odd +=  1 * lowhigh_line[0][column];
		odd += 4;
		odd = DivideByShift(odd, 3);

		// Subtract the highpass correction
		odd -= highhigh_line[column];
		odd >>= 1;

		// Place the odd result in the odd row
		odd_highpass[column] = ClampPixel(odd);
	}

	// Apply the inverse horizontal transform to the even and odd rows and descale the results
	InvertHorizontal16s(even_lowpass, even_highpass, even_output, input_width, output_width);

	// Is the output wavelet shorter than twice the height of the input wavelet?
	if (2 * row + 1 < output_height) {
		InvertHorizontal16s(odd_lowpass, odd_highpass, odd_output, input_width, output_width);
	}

	// Free the scratch buffers
	Free(allocator, even_lowpass);
	Free(allocator, even_highpass);
	Free(allocator, odd_lowpass);
	Free(allocator, odd_highpass);

	Free(allocator, lowhigh_line[0]);
	Free(allocator, lowhigh_line[1]);
	Free(allocator, lowhigh_line[2]);
	Free(allocator, highlow_line);
	Free(allocator, highhigh_line);

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Apply the inverse horizontal wavelet transform

	This routine applies the inverse wavelet transform to a row of
	lowpass and highpass coefficients, producing an output row that
	is write as wide.
*/
CODEC_ERROR InvertHorizontal16s(PIXEL *lowpass,			//!< Horizontal lowpass coefficients
								PIXEL *highpass,		//!< Horizontal highpass coefficients
								PIXEL *output,			//!< Row of reconstructed results
								DIMENSION input_width,	//!< Number of values in the input row
								DIMENSION output_width  //!< Number of values in the output row
								)
{
	const int last_column = input_width - 1;

	int32_t even;
	int32_t odd;

	// Start processing at the beginning of the row
	int column = 0;

	// Process the first two output points with special filters for the left border
	even = 0;
	odd = 0;

	// Apply the even reconstruction filter to the lowpass band
	even += 11 * lowpass[column + 0];
	even -=  4 * lowpass[column + 1];
	even +=  1 * lowpass[column + 2];
	even += rounding;
	even = DivideByShift(even, 3);

	// Add the highpass correction
	even += highpass[column];
	even >>= 1;

	// The lowpass result should be a positive number
	//assert(0 <= even && even <= INT16_MAX);

	// Apply the odd reconstruction filter to the lowpass band
	odd += 5 * lowpass[column + 0];
	odd += 4 * lowpass[column + 1];
	odd -= 1 * lowpass[column + 2];
	odd += rounding;
	odd = DivideByShift(odd, 3);

	// Subtract the highpass correction
	odd -= highpass[column];
	odd >>= 1;

	// The lowpass result should be a positive number
	//assert(0 <= odd && odd <= INT16_MAX);

	// Store the last two output points produced by the loop
	output[2 * column + 0] = ClampPixel(even);
	output[2 * column + 1] = ClampPixel(odd);

	// Advance to the next input column (second pair of output values)
	column++;

	// Process the rest of the columns up to the last column in the row
	for (; column < last_column; column++)
	{
		int32_t even = 0;		// Result of convolution with even filter
		int32_t odd = 0;		// Result of convolution with odd filter

		// Apply the even reconstruction filter to the lowpass band

		even += lowpass[column - 1];
		even -= lowpass[column + 1];
		even += 4;
		even >>= 3;
		even += lowpass[column + 0];

		// Add the highpass correction
		even += highpass[column];
		even >>= 1;

		// The lowpass result should be a positive number
		//assert(0 <= even && even <= INT16_MAX);

		// Place the even result in the even column
		//output[2 * column + 0] = clamp_uint12(even);
        output[2 * column + 0] = ClampPixel(even);

		// Apply the odd reconstruction filter to the lowpass band
		odd -= lowpass[column - 1];
		odd += lowpass[column + 1];
		odd += 4;
		odd >>= 3;
		odd += lowpass[column + 0];

		// Subtract the highpass correction
		odd -= highpass[column];
		odd >>= 1;

		// The lowpass result should be a positive number
		//assert(0 <= odd && odd <= INT16_MAX);

		// Place the odd result in the odd column
		//output[2 * column + 1] = clamp_uint12(odd);
        output[2 * column + 1] = ClampPixel(odd);
	}

	// Should have exited the loop at the column for right border processing
	assert(column == last_column);

	// Process the last two output points with special filters for the right border
	even = 0;
	odd = 0;

	// Apply the even reconstruction filter to the lowpass band
	even += 5 * lowpass[column + 0];
	even += 4 * lowpass[column - 1];
	even -= 1 * lowpass[column - 2];
	even += rounding;
	even = DivideByShift(even, 3);

	// Add the highpass correction
	even += highpass[column];
	even >>= 1;

	// The lowpass result should be a positive number
	//assert(0 <= even && even <= INT16_MAX);

	// Place the even result in the even column
	output[2 * column + 0] = ClampPixel(even);

    if (2 * column + 1 < output_width)
    {
        // Apply the odd reconstruction filter to the lowpass band
        odd += 11 * lowpass[column + 0];
        odd -=  4 * lowpass[column - 1];
        odd +=  1 * lowpass[column - 2];
        odd += rounding;
        odd = DivideByShift(odd, 3);

        // Subtract the highpass correction
        odd -= highpass[column];
        odd >>= 1;

        // The lowpass result should be a positive number
        //assert(0 <= odd && odd <= INT16_MAX);

        // Place the odd result in the odd column
        output[2 * column + 1] = ClampPixel(odd);
    }
    
	return CODEC_ERROR_OKAY;
}

/*!
	@brief Apply the inverse spatial transform with descaling

	This routine is similar to @ref InvertSpatialQuant16s, but a scale factor
	that was applied during encoding is removed from the output values.
*/
CODEC_ERROR InvertSpatialQuantDescale16s(ALLOCATOR *allocator,
										 PIXEL *lowlow_band, int lowlow_pitch,
										 PIXEL *lowhigh_band, int lowhigh_pitch,
										 PIXEL *highlow_band, int highlow_pitch,
										 PIXEL *highhigh_band, int highhigh_pitch,
										 PIXEL *output_image, int output_pitch,
										 DIMENSION input_width, DIMENSION input_height,
										 DIMENSION output_width, DIMENSION output_height,
										 int descale, QUANT quantization[])
{
	PIXEL *lowlow = lowlow_band;
	PIXEL *lowhigh = lowhigh_band;
	PIXEL *highlow = highlow_band;
	PIXEL *highhigh = highhigh_band;
	PIXEL *output = output_image;
	PIXEL *even_lowpass;
	PIXEL *even_highpass;
	PIXEL *odd_lowpass;
	PIXEL *odd_highpass;
	PIXEL *even_output;
	PIXEL *odd_output;
	size_t buffer_row_size;
	int last_row = input_height - 1;
	int row, column;

	PIXEL *lowhigh_row[3];

	PIXEL *lowhigh_line[3];
	PIXEL *highlow_line;
	PIXEL *highhigh_line;

	QUANT highlow_quantization = quantization[HL_BAND];
	QUANT lowhigh_quantization = quantization[LH_BAND];
	QUANT highhigh_quantization = quantization[HH_BAND];

	// Pointer to the last row used from the LH band (for debugging)
	PIXEL *last_lowhigh_row_ptr = NULL;

	// Compute positions within the temporary buffer for each row of horizontal lowpass
	// and highpass intermediate coefficients computed by the vertical inverse transform
	buffer_row_size = input_width * sizeof(PIXEL);

	// Allocate space for the even and odd rows of results from the inverse vertical filter
	even_lowpass = (PIXEL *)Alloc(allocator, buffer_row_size);
	even_highpass = (PIXEL *)Alloc(allocator, buffer_row_size);
	odd_lowpass = (PIXEL *)Alloc(allocator, buffer_row_size);
	odd_highpass = (PIXEL *)Alloc(allocator, buffer_row_size);

	// Allocate scratch space for the dequantized highpass coefficients
	lowhigh_line[0] = (PIXEL *)Alloc(allocator, buffer_row_size);
	lowhigh_line[1] = (PIXEL *)Alloc(allocator, buffer_row_size);
	lowhigh_line[2] = (PIXEL *)Alloc(allocator, buffer_row_size);
	highlow_line = (PIXEL *)Alloc(allocator, buffer_row_size);
	highhigh_line = (PIXEL *)Alloc(allocator, buffer_row_size);

	// Convert pitch from bytes to pixels
	lowlow_pitch /= sizeof(PIXEL);
	lowhigh_pitch /= sizeof(PIXEL);
	highlow_pitch /= sizeof(PIXEL);
	highhigh_pitch /= sizeof(PIXEL);
	output_pitch /= sizeof(PIXEL);

	// Initialize the pointers to the even and odd output rows
	even_output = output;
	odd_output = output + output_pitch;

	// Apply the vertical border filter to the first row
	row = 0;

	// Set pointers to the first three rows in the first highpass band
	lowhigh_row[0] = lowhigh + 0 * lowhigh_pitch;
	lowhigh_row[1] = lowhigh + 1 * lowhigh_pitch;
	lowhigh_row[2] = lowhigh + 2 * lowhigh_pitch;

	// Dequantize three rows of highpass coefficients in the first highpass band
	DequantizeBandRow16s(lowhigh_row[0], input_width, lowhigh_quantization, lowhigh_line[0]);
	DequantizeBandRow16s(lowhigh_row[1], input_width, lowhigh_quantization, lowhigh_line[1]);
	DequantizeBandRow16s(lowhigh_row[2], input_width, lowhigh_quantization, lowhigh_line[2]);

	// Dequantize one row of coefficients each in the second and third highpass bands
	DequantizeBandRow16s(highlow, input_width, highlow_quantization, highlow_line);
	DequantizeBandRow16s(highhigh, input_width, highhigh_quantization, highhigh_line);

	for (column = 0; column < input_width; column++)
	{
		int32_t even = 0;		// Result of convolution with even filter
		int32_t odd = 0;		// Result of convolution with odd filter


		/***** Compute the vertical inverse for the left two bands *****/

		// Apply the even reconstruction filter to the lowpass band
		even += 11 * lowlow[column + 0 * lowlow_pitch];
		even -=  4 * lowlow[column + 1 * lowlow_pitch];
		even +=  1 * lowlow[column + 2 * lowlow_pitch];
		even += rounding;
		even = DivideByShift(even, 3);

		// Add the highpass correction
		even += highlow_line[column];
		even = DivideByShift(even, 1);

		// The inverse of the left two bands should be a positive number
		//assert(0 <= even && even <= INT16_MAX);

		// Place the even result in the even row
		even_lowpass[column] = ClampPixel(even);

		// Apply the odd reconstruction filter to the lowpass band
		odd += 5 * lowlow[column + 0 * lowlow_pitch];
		odd += 4 * lowlow[column + 1 * lowlow_pitch];
		odd -= 1 * lowlow[column + 2 * lowlow_pitch];
		odd += rounding;
		odd = DivideByShift(odd, 3);

		// Subtract the highpass correction
		odd -= highlow_line[column];
		odd = DivideByShift(odd, 1);

		// The inverse of the left two bands should be a positive number
		//assert(0 <= odd && odd <= INT16_MAX);

		// Place the odd result in the odd row
		odd_lowpass[column] = ClampPixel(odd);


		/***** Compute the vertical inverse for the right two bands *****/

		even = 0;
		odd = 0;

		// Apply the even reconstruction filter to the lowpass band
		even += 11 * lowhigh_line[0][column];
		even -=  4 * lowhigh_line[1][column];
		even +=  1 * lowhigh_line[2][column];
		even += rounding;
		even = DivideByShift(even, 3);

		// Add the highpass correction
		even += highhigh_line[column];
		even = DivideByShift(even, 1);

		// Place the even result in the even row
		even_highpass[column] = ClampPixel(even);

		// Apply the odd reconstruction filter to the lowpass band
		odd += 5 * lowhigh_line[0][column];
		odd += 4 * lowhigh_line[1][column];
		odd -= 1 * lowhigh_line[2][column];
		odd += rounding;
		odd = DivideByShift(odd, 3);

		// Subtract the highpass correction
		odd -= highhigh_line[column];
		odd = DivideByShift(odd, 1);

		// Place the odd result in the odd row
		odd_highpass[column] = ClampPixel(odd);
	}

	// Apply the inverse horizontal transform to the even and odd rows and descale the results
	InvertHorizontalDescale16s(even_lowpass, even_highpass, even_output,
							   input_width, output_width, descale);

	InvertHorizontalDescale16s(odd_lowpass, odd_highpass, odd_output,
							   input_width, output_width, descale);

	// Advance to the next pair of even and odd output rows
	even_output += 2 * output_pitch;
	odd_output += 2 * output_pitch;

	// Always advance the highpass row pointers
	highlow += highlow_pitch;
	highhigh += highhigh_pitch;

	// Advance the row index
	row++;

	// Process the middle rows using the interior reconstruction filters
	for (; row < last_row; row++)
	{
		// Dequantize one row from each of the two highpass bands
		DequantizeBandRow16s(highlow, input_width, highlow_quantization, highlow_line);
		DequantizeBandRow16s(highhigh, input_width, highhigh_quantization, highhigh_line);

		// Process the entire row
		for (column = 0; column < input_width; column++)
		{
			int32_t even = 0;		// Result of convolution with even filter
			int32_t odd = 0;		// Result of convolution with odd filter


			/***** Compute the vertical inverse for the left two bands *****/

			// Apply the even reconstruction filter to the lowpass band
			even += lowlow[column + 0 * lowlow_pitch];
			even -= lowlow[column + 2 * lowlow_pitch];
			even += 4;
			even >>= 3;
			even += lowlow[column + 1 * lowlow_pitch];

			// Add the highpass correction
			even += highlow_line[column];
			even = DivideByShift(even, 1);

			// The inverse of the left two bands should be a positive number
			//assert(0 <= even && even <= INT16_MAX);

			// Place the even result in the even row
			even_lowpass[column] = ClampPixel(even);

			// Apply the odd reconstruction filter to the lowpass band
			odd -= lowlow[column + 0 * lowlow_pitch];
			odd += lowlow[column + 2 * lowlow_pitch];
			odd += 4;
			odd >>= 3;
			odd += lowlow[column + 1 * lowlow_pitch];

			// Subtract the highpass correction
			odd -= highlow_line[column];
			odd = DivideByShift(odd, 1);

			// The inverse of the left two bands should be a positive number
			//assert(0 <= odd && odd <= INT16_MAX);

			// Place the odd result in the odd row
			odd_lowpass[column] = ClampPixel(odd);


			/***** Compute the vertical inverse for the right two bands *****/

			even = 0;
			odd = 0;

			// Apply the even reconstruction filter to the lowpass band
			even += lowhigh_line[0][column];
			even -= lowhigh_line[2][column];
			even += 4;
			even >>= 3;
			even += lowhigh_line[1][column];

			// Add the highpass correction
			even += highhigh_line[column];
			even = DivideByShift(even, 1);

			// Place the even result in the even row
			even_highpass[column] = ClampPixel(even);

			// Apply the odd reconstruction filter to the lowpass band
			odd -= lowhigh_line[0][column];
			odd += lowhigh_line[2][column];
			odd += 4;
			odd >>= 3;
			odd += lowhigh_line[1][column];

			// Subtract the highpass correction
			odd -= highhigh_line[column];
			odd = DivideByShift(odd, 1);

			// Place the odd result in the odd row
			odd_highpass[column] = ClampPixel(odd);
		}

		// Apply the inverse horizontal transform to the even and odd rows and descale the results
		InvertHorizontalDescale16s(even_lowpass, even_highpass, even_output,
								   input_width, output_width, descale);

		InvertHorizontalDescale16s(odd_lowpass, odd_highpass, odd_output,
								   input_width, output_width, descale);

		// Advance to the next input row in each band
		lowlow += lowlow_pitch;
		lowhigh += lowhigh_pitch;
		highlow += highlow_pitch;
		highhigh += highhigh_pitch;

		// Advance to the next pair of even and odd output rows
		even_output += 2 * output_pitch;
		odd_output += 2 * output_pitch;

		if (row < last_row - 1)
		{
			// Compute the address of the next row in the lowhigh band
			PIXEL *lowhigh_row_ptr = (lowhigh + 2 * lowhigh_pitch);

			// Shift the rows in the buffer of dequantized lowhigh bands
			PIXEL *temp = lowhigh_line[0];
			lowhigh_line[0] = lowhigh_line[1];
			lowhigh_line[1] = lowhigh_line[2];
			lowhigh_line[2] = temp;

			// Undo quantization for the next row in the lowhigh band
			DequantizeBandRow16s(lowhigh_row_ptr, input_width, lowhigh_quantization, lowhigh_line[2]);

			// Save the pointer to the last row in the LH band (for debugging)
			last_lowhigh_row_ptr = lowhigh_row_ptr;
		}
	}

	// Should have exited the loop at the last row
	assert(row == last_row);

	// Advance the lowlow pointer to the last row in the band
	lowlow += lowlow_pitch;

	// Check that the band pointers are on the last row in each wavelet band
	assert(lowlow == (lowlow_band + last_row * lowlow_pitch));
	assert(last_lowhigh_row_ptr == (lowhigh_band + last_row * lowhigh_pitch));
	assert(highlow == (highlow_band + last_row * highlow_pitch));
	assert(highhigh == (highhigh_band + last_row * highhigh_pitch));

	// Undo quantization for the highlow and highhigh bands
	DequantizeBandRow16s(highlow, input_width, highlow_quantization, highlow_line);
	DequantizeBandRow16s(highhigh, input_width, highhigh_quantization, highhigh_line);

	// Apply the vertical border filter to the last row
	for (column = 0; column < input_width; column++)
	{
		int32_t even = 0;		// Result of convolution with even filter
		int32_t odd = 0;		// Result of convolution with odd filter


		/***** Compute the vertical inverse for the left two bands *****/

		// Apply the even reconstruction filter to the lowpass band
		even += 5 * lowlow[column + 0 * lowlow_pitch];
		even += 4 * lowlow[column - 1 * lowlow_pitch];
		even -= 1 * lowlow[column - 2 * lowlow_pitch];
		even += rounding;
		even = DivideByShift(even, 3);

		// Add the highpass correction
		even += highlow_line[column];
		even = DivideByShift(even, 1);

		// The inverse of the left two bands should be a positive number
		//assert(0 <= even && even <= INT16_MAX);

		// Place the even result in the even row
		even_lowpass[column] = ClampPixel(even);

		// Apply the odd reconstruction filter to the lowpass band
		odd += 11 * lowlow[column + 0 * lowlow_pitch];
		odd -=  4 * lowlow[column - 1 * lowlow_pitch];
		odd +=  1 * lowlow[column - 2 * lowlow_pitch];
		odd += rounding;
		odd = DivideByShift(odd, 3);

		// Subtract the highpass correction
		odd -= highlow_line[column];
		odd = DivideByShift(odd, 1);

		// The inverse of the left two bands should be a positive number
		//assert(0 <= odd && odd <= INT16_MAX);

		// Place the odd result in the odd row
		odd_lowpass[column] = ClampPixel(odd);


		/***** Compute the vertical inverse for the right two bands *****/

		even = 0;
		odd = 0;

		// Apply the even reconstruction filter to the lowpass band
		even += 5 * lowhigh_line[2][column];
		even += 4 * lowhigh_line[1][column];
		even -= 1 * lowhigh_line[0][column];
		even += rounding;
		even = DivideByShift(even, 3);

		// Add the highpass correction
		even += highhigh_line[column];
		even = DivideByShift(even, 1);

		// Place the even result in the even row
		even_highpass[column] = ClampPixel(even);

		// Apply the odd reconstruction filter to the lowpass band
		odd += 11 * lowhigh_line[2][column];
		odd -=  4 * lowhigh_line[1][column];
		odd +=  1 * lowhigh_line[0][column];
		odd += rounding;
		odd = DivideByShift(odd, 3);

		// Subtract the highpass correction
		odd -= highhigh_line[column];
		odd = DivideByShift(odd, 1);

		// Place the odd result in the odd row
		odd_highpass[column] = ClampPixel(odd);
	}

	// Apply the inverse horizontal transform to the even and odd rows and descale the results
	InvertHorizontalDescale16s(even_lowpass, even_highpass, even_output,
							   input_width, output_width, descale);

	// Is the output wavelet shorter than twice the height of the input wavelet?
	if (2 * row + 1 < output_height) {
		InvertHorizontalDescale16s(odd_lowpass, odd_highpass, odd_output,
								   input_width, output_width, descale);
	}

	// Free the scratch buffers
	Free(allocator, even_lowpass);
	Free(allocator, even_highpass);
	Free(allocator, odd_lowpass);
	Free(allocator, odd_highpass);

	Free(allocator, lowhigh_line[0]);
	Free(allocator, lowhigh_line[1]);
	Free(allocator, lowhigh_line[2]);
	Free(allocator, highlow_line);
	Free(allocator, highhigh_line);

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Apply the inverse spatial transform with descaling

	This routine is similar to @ref InvertSpatialQuant16s, but a scale factor
	that was applied during encoding is removed from the output values.

	@todo Change the output arguments to pass a pointer to a component array?
*/
CODEC_ERROR InvertSpatialWavelet(ALLOCATOR *allocator,
								 PIXEL *lowlow_band, int lowlow_pitch,
								 PIXEL *lowhigh_band, int lowhigh_pitch,
								 PIXEL *highlow_band, int highlow_pitch,
								 PIXEL *highhigh_band, int highhigh_pitch,
								 COMPONENT_VALUE *output_image, int output_pitch,
								 DIMENSION input_width, DIMENSION input_height,
								 DIMENSION output_width, DIMENSION output_height,
								 int descale, QUANT quantization[])
{
	PIXEL *lowlow = lowlow_band;
	PIXEL *lowhigh = lowhigh_band;
	PIXEL *highlow = highlow_band;
	PIXEL *highhigh = highhigh_band;
	COMPONENT_VALUE *output = output_image;
	PIXEL *even_lowpass;
	PIXEL *even_highpass;
	PIXEL *odd_lowpass;
	PIXEL *odd_highpass;
	PIXEL *even_output;
	PIXEL *odd_output;
	size_t buffer_row_size;
	int last_row = input_height - 1;
	int row, column;

	PIXEL *lowhigh_row[3];

	PIXEL *lowhigh_line[3];
	PIXEL *highlow_line;
	PIXEL *highhigh_line;

	QUANT highlow_quantization = quantization[HL_BAND];
	QUANT lowhigh_quantization = quantization[LH_BAND];
	QUANT highhigh_quantization = quantization[HH_BAND];

	// Compute positions within the temporary buffer for each row of horizontal lowpass
	// and highpass intermediate coefficients computed by the vertical inverse transform
	buffer_row_size = input_width * sizeof(PIXEL);

	// Allocate space for the even and odd rows of results from the inverse vertical filter
	even_lowpass = (PIXEL *)Alloc(allocator, buffer_row_size);
	even_highpass = (PIXEL *)Alloc(allocator, buffer_row_size);
	odd_lowpass = (PIXEL *)Alloc(allocator, buffer_row_size);
	odd_highpass = (PIXEL *)Alloc(allocator, buffer_row_size);

	// Allocate scratch space for the dequantized highpass coefficients
	lowhigh_line[0] = (PIXEL *)Alloc(allocator, buffer_row_size);
	lowhigh_line[1] = (PIXEL *)Alloc(allocator, buffer_row_size);
	lowhigh_line[2] = (PIXEL *)Alloc(allocator, buffer_row_size);
	highlow_line = (PIXEL *)Alloc(allocator, buffer_row_size);
	highhigh_line = (PIXEL *)Alloc(allocator, buffer_row_size);

	// Convert pitch from bytes to pixels
	lowlow_pitch /= sizeof(PIXEL);
	lowhigh_pitch /= sizeof(PIXEL);
	highlow_pitch /= sizeof(PIXEL);
	highhigh_pitch /= sizeof(PIXEL);
	output_pitch /= sizeof(PIXEL);

	// Initialize the pointers to the even and odd output rows
	even_output = (PIXEL *)output;
	odd_output = even_output + output_pitch;

	// Apply the vertical border filter to the first row
	row = 0;

	// Set pointers to the first three rows in the first highpass band
	lowhigh_row[0] = lowhigh + 0 * lowhigh_pitch;
	lowhigh_row[1] = lowhigh + 1 * lowhigh_pitch;
	lowhigh_row[2] = lowhigh + 2 * lowhigh_pitch;

	// Dequantize three rows of highpass coefficients in the first highpass band
	DequantizeBandRow16s(lowhigh_row[0], input_width, lowhigh_quantization, lowhigh_line[0]);
	DequantizeBandRow16s(lowhigh_row[1], input_width, lowhigh_quantization, lowhigh_line[1]);
	DequantizeBandRow16s(lowhigh_row[2], input_width, lowhigh_quantization, lowhigh_line[2]);

	// Dequantize one row of coefficients each in the second and third highpass bands
	DequantizeBandRow16s(highlow, input_width, highlow_quantization, highlow_line);
	DequantizeBandRow16s(highhigh, input_width, highhigh_quantization, highhigh_line);

	for (column = 0; column < input_width; column++)
	{
		int32_t even = 0;		// Result of convolution with even filter
		int32_t odd = 0;		// Result of convolution with odd filter


		/***** Compute the vertical inverse for the left two bands *****/

		// Apply the even reconstruction filter to the lowpass band
		even += 11 * lowlow[column + 0 * lowlow_pitch];
		even -=  4 * lowlow[column + 1 * lowlow_pitch];
		even +=  1 * lowlow[column + 2 * lowlow_pitch];
		even += rounding;
		even = DivideByShift(even, 3);

		// Add the highpass correction
		even += highlow_line[column];
		even = DivideByShift(even, 1);

		// Place the even result in the even row
		even_lowpass[column] = ClampPixel(even);

		// Apply the odd reconstruction filter to the lowpass band
		odd += 5 * lowlow[column + 0 * lowlow_pitch];
		odd += 4 * lowlow[column + 1 * lowlow_pitch];
		odd -= 1 * lowlow[column + 2 * lowlow_pitch];
		odd += rounding;
		odd = DivideByShift(odd, 3);

		// Subtract the highpass correction
		odd -= highlow_line[column];
		odd = DivideByShift(odd, 1);

		// Place the odd result in the odd row
		odd_lowpass[column] = ClampPixel(odd);


		/***** Compute the vertical inverse for the right two bands *****/

		even = 0;
		odd = 0;

		// Apply the even reconstruction filter to the lowpass band
		even += 11 * lowhigh_line[0][column];
		even -=  4 * lowhigh_line[1][column];
		even +=  1 * lowhigh_line[2][column];
		even += rounding;
		even = DivideByShift(even, 3);

		// Add the highpass correction
		even += highhigh_line[column];
		even = DivideByShift(even, 1);

		// Place the even result in the even row
		even_highpass[column] = ClampPixel(even);

		// Apply the odd reconstruction filter to the lowpass band
		odd += 5 * lowhigh_line[0][column];
		odd += 4 * lowhigh_line[1][column];
		odd -= 1 * lowhigh_line[2][column];
		odd += rounding;
		odd = DivideByShift(odd, 3);

		// Subtract the highpass correction
		odd -= highhigh_line[column];
		odd = DivideByShift(odd, 1);

		// Place the odd result in the odd row
		odd_highpass[column] = ClampPixel(odd);
	}

	// Apply the inverse horizontal transform to the even and odd rows and descale the results
	InvertHorizontalDescale16s(even_lowpass, even_highpass, even_output,
							   input_width, output_width, descale);

	InvertHorizontalDescale16s(odd_lowpass, odd_highpass, odd_output,
							   input_width, output_width, descale);

	// Advance to the next pair of even and odd output rows
	even_output += 2 * output_pitch;
	odd_output += 2 * output_pitch;

	// Always advance the highpass row pointers
	highlow += highlow_pitch;
	highhigh += highhigh_pitch;

	// Advance the row index
	row++;

	// Process the middle rows using the interior reconstruction filters
	for (; row < last_row; row++)
	{
		// Dequantize one row from each of the two highpass bands
		DequantizeBandRow16s(highlow, input_width, highlow_quantization, highlow_line);
		DequantizeBandRow16s(highhigh, input_width, highhigh_quantization, highhigh_line);

		// Process the entire row
		for (column = 0; column < input_width; column++)
		{
			int32_t even = 0;		// Result of convolution with even filter
			int32_t odd = 0;		// Result of convolution with odd filter


			/***** Compute the vertical inverse for the left two bands *****/

			// Apply the even reconstruction filter to the lowpass band
			even += lowlow[column + 0 * lowlow_pitch];
			even -= lowlow[column + 2 * lowlow_pitch];
			even += 4;
			even >>= 3;
			even += lowlow[column + 1 * lowlow_pitch];

			// Add the highpass correction
			even += highlow_line[column];
			even = DivideByShift(even, 1);

			// Place the even result in the even row
			even_lowpass[column] = ClampPixel(even);

			// Apply the odd reconstruction filter to the lowpass band
			odd -= lowlow[column + 0 * lowlow_pitch];
			odd += lowlow[column + 2 * lowlow_pitch];
			odd += 4;
			odd >>= 3;
			odd += lowlow[column + 1 * lowlow_pitch];

			// Subtract the highpass correction
			odd -= highlow_line[column];
			odd = DivideByShift(odd, 1);

			// Place the odd result in the odd row
			odd_lowpass[column] = ClampPixel(odd);


			/***** Compute the vertical inverse for the right two bands *****/

			even = 0;
			odd = 0;

			// Apply the even reconstruction filter to the lowpass band
			even += lowhigh_line[0][column];
			even -= lowhigh_line[2][column];
			even += 4;
			even >>= 3;
			even += lowhigh_line[1][column];

			// Add the highpass correction
			even += highhigh_line[column];
			even = DivideByShift(even, 1);

			// Place the even result in the even row
			even_highpass[column] = ClampPixel(even);

			// Apply the odd reconstruction filter to the lowpass band
			odd -= lowhigh_line[0][column];
			odd += lowhigh_line[2][column];
			odd += 4;
			odd >>= 3;
			odd += lowhigh_line[1][column];

			// Subtract the highpass correction
			odd -= highhigh_line[column];
			odd = DivideByShift(odd, 1);

			// Place the odd result in the odd row
			odd_highpass[column] = ClampPixel(odd);
		}

		// Apply the inverse horizontal transform to the even and odd rows and descale the results
		InvertHorizontalDescale16s(even_lowpass, even_highpass, even_output,
								   input_width, output_width, descale);

		InvertHorizontalDescale16s(odd_lowpass, odd_highpass, odd_output,
								   input_width, output_width, descale);

		// Advance to the next input row in each band
		lowlow += lowlow_pitch;
		lowhigh += lowhigh_pitch;
		highlow += highlow_pitch;
		highhigh += highhigh_pitch;

		// Advance to the next pair of even and odd output rows
		even_output += 2 * output_pitch;
		odd_output += 2 * output_pitch;

		if (row < last_row - 1)
		//if (row < last_row)
		{
			// Compute the address of the next row in the lowhigh band
			PIXEL *lowhigh_row_ptr = (lowhigh + 2 * lowhigh_pitch);

			// Shift the rows in the buffer of dequantized lowhigh bands
			PIXEL *temp = lowhigh_line[0];
			lowhigh_line[0] = lowhigh_line[1];
			lowhigh_line[1] = lowhigh_line[2];
			lowhigh_line[2] = temp;

			// Undo quantization for the next row in the lowhigh band
			DequantizeBandRow16s(lowhigh_row_ptr, input_width, lowhigh_quantization, lowhigh_line[2]);
		}
	}

	// Should have exited the loop at the last row
	assert(row == last_row);

	// Advance the lowlow pointer to the last row in the band
	lowlow += lowlow_pitch;

	// Check that the band pointers are on the last row in each wavelet band
	assert(lowlow == (lowlow_band + last_row * lowlow_pitch));
	assert(highlow == (highlow_band + last_row * highlow_pitch));
	assert(highhigh == (highhigh_band + last_row * highhigh_pitch));

	// Undo quantization for the highlow and highhigh bands
	DequantizeBandRow16s(highlow, input_width, highlow_quantization, highlow_line);
	DequantizeBandRow16s(highhigh, input_width, highhigh_quantization, highhigh_line);

	// Apply the vertical border filter to the last row
	for (column = 0; column < input_width; column++)
	{
		int32_t even = 0;		// Result of convolution with even filter
		int32_t odd = 0;		// Result of convolution with odd filter


		/***** Compute the vertical inverse for the left two bands *****/

		// Apply the even reconstruction filter to the lowpass band
		even += 5 * lowlow[column + 0 * lowlow_pitch];
		even += 4 * lowlow[column - 1 * lowlow_pitch];
		even -= 1 * lowlow[column - 2 * lowlow_pitch];
		even += rounding;
		even = DivideByShift(even, 3);

		// Add the highpass correction
		even += highlow_line[column];
		even = DivideByShift(even, 1);

		// Place the even result in the even row
		even_lowpass[column] = ClampPixel(even);

		// Apply the odd reconstruction filter to the lowpass band
		odd += 11 * lowlow[column + 0 * lowlow_pitch];
		odd -=  4 * lowlow[column - 1 * lowlow_pitch];
		odd +=  1 * lowlow[column - 2 * lowlow_pitch];
		odd += rounding;
		odd = DivideByShift(odd, 3);

		// Subtract the highpass correction
		odd -= highlow_line[column];
		odd = DivideByShift(odd, 1);

		// Place the odd result in the odd row
		odd_lowpass[column] = ClampPixel(odd);


		/***** Compute the vertical inverse for the right two bands *****/

		even = 0;
		odd = 0;

		// Apply the even reconstruction filter to the lowpass band
		even += 5 * lowhigh_line[2][column];
		even += 4 * lowhigh_line[1][column];
		even -= 1 * lowhigh_line[0][column];
		even += rounding;
		even = DivideByShift(even, 3);

		// Add the highpass correction
		even += highhigh_line[column];
		even = DivideByShift(even, 1);

		// Place the even result in the even row
		even_highpass[column] = ClampPixel(even);

		// Apply the odd reconstruction filter to the lowpass band
		odd += 11 * lowhigh_line[2][column];
		odd -=  4 * lowhigh_line[1][column];
		odd +=  1 * lowhigh_line[0][column];
		odd += rounding;
		odd = DivideByShift(odd, 3);

		// Subtract the highpass correction
		odd -= highhigh_line[column];
		odd = DivideByShift(odd, 1);

		// Place the odd result in the odd row
		odd_highpass[column] = ClampPixel(odd);
	}

	// Apply the inverse horizontal transform to the even and odd rows and descale the results
	InvertHorizontalDescale16s(even_lowpass, even_highpass, even_output,
							   input_width, output_width, descale);

	// Is the output wavelet shorter than twice the height of the input wavelet?
	if (2 * row + 1 < output_height) {
		InvertHorizontalDescale16s(odd_lowpass, odd_highpass, odd_output,
								   input_width, output_width, descale);
	}

	// Free the scratch buffers
	Free(allocator, even_lowpass);
	Free(allocator, even_highpass);
	Free(allocator, odd_lowpass);
	Free(allocator, odd_highpass);

	Free(allocator, lowhigh_line[0]);
	Free(allocator, lowhigh_line[1]);
	Free(allocator, lowhigh_line[2]);
	Free(allocator, highlow_line);
	Free(allocator, highhigh_line);

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Apply the inverse horizontal wavelet transform

	This routine is similar to @ref InvertHorizontal16s, but a scale factor
	that was applied during encoding is removed from the output values.
*/
CODEC_ERROR InvertHorizontalDescale16s(PIXEL *lowpass, PIXEL *highpass, PIXEL *output,
									   DIMENSION input_width, DIMENSION output_width,
									   int descale)
{
	const int last_column = input_width - 1;

	// Start processing at the beginning of the row
	int column = 0;

	int descale_shift = 0;

	int32_t even;
	int32_t odd;

	/*
		The implementation of the inverse filter includes descaling by a factor of two
		because the last division by two in the computation of the even and odd results
		that is performed using a right arithmetic shift has been omitted from the code.
	*/
	if (descale == 2) {
		descale_shift = 1;
	}

	// Check that the descaling value is reasonable
	assert(descale_shift >= 0);

	// Process the first two output points with special filters for the left border
	even = 0;
	odd = 0;

	// Apply the even reconstruction filter to the lowpass band
	even += 11 * lowpass[column + 0];
	even -=  4 * lowpass[column + 1];
	even +=  1 * lowpass[column + 2];
	even += rounding;
	even = DivideByShift(even, 3);

	// Add the highpass correction
	even += highpass[column];

	// Remove any scaling used during encoding
	even <<= descale_shift;

	// The lowpass result should be a positive number
	//assert(0 <= even && even <= INT16_MAX);

	// Apply the odd reconstruction filter to the lowpass band
	odd += 5 * lowpass[column + 0];
	odd += 4 * lowpass[column + 1];
	odd -= 1 * lowpass[column + 2];
	odd += rounding;
	odd = DivideByShift(odd, 3);

	// Subtract the highpass correction
	odd -= highpass[column];

	// Remove any scaling used during encoding
	odd <<= descale_shift;

	// The lowpass result should be a positive number
	//assert(0 <= odd && odd <= INT16_MAX);

	output[2 * column + 0] = ClampPixel(even);
	output[2 * column + 1] = ClampPixel(odd);

	// Advance to the next input column (second pair of output values)
	column++;

	// Process the rest of the columns up to the last column in the row
	for (; column < last_column; column++)
	{
		int32_t even = 0;		// Result of convolution with even filter
		int32_t odd = 0;		// Result of convolution with odd filter

		// Apply the even reconstruction filter to the lowpass band
		even += lowpass[column - 1];
		even -= lowpass[column + 1];
		even += 4;
		even >>= 3;
		even += lowpass[column + 0];

		// Add the highpass correction
		even += highpass[column];

		// Remove any scaling used during encoding
		even <<= descale_shift;

		// The lowpass result should be a positive number
		//assert(0 <= even && even <= INT16_MAX);

		// Place the even result in the even column
		output[2 * column + 0] = ClampPixel(even);

		// Apply the odd reconstruction filter to the lowpass band
		odd -= lowpass[column - 1];
		odd += lowpass[column + 1];
		odd += 4;
		odd >>= 3;
		odd += lowpass[column + 0];

		// Subtract the highpass correction
		odd -= highpass[column];

		// Remove any scaling used during encoding
		odd <<= descale_shift;

		// The lowpass result should be a positive number
		//assert(0 <= odd && odd <= INT16_MAX);

		// Place the odd result in the odd column
		output[2 * column + 1] = ClampPixel(odd);
	}

	// Should have exited the loop at the column for right border processing
	assert(column == last_column);

	// Process the last two output points with special filters for the right border
	even = 0;
	odd = 0;

	// Apply the even reconstruction filter to the lowpass band
	even += 5 * lowpass[column + 0];
	even += 4 * lowpass[column - 1];
	even -= 1 * lowpass[column - 2];
	even += rounding;
	even = DivideByShift(even, 3);

	// Add the highpass correction
	even += highpass[column];

	// Remove any scaling used during encoding
	even <<= descale_shift;

	// The lowpass result should be a positive number
	//assert(0 <= even && even <= INT16_MAX);

	// Place the even result in the even column
	output[2 * column + 0] = ClampPixel(even);

	if (2 * column + 1 < output_width)
	{
		// Apply the odd reconstruction filter to the lowpass band
		odd += 11 * lowpass[column + 0];
		odd -=  4 * lowpass[column - 1];
		odd +=  1 * lowpass[column - 2];
		odd += rounding;
		odd = DivideByShift(odd, 3);

		// Subtract the highpass correction
		odd -= highpass[column];

		// Remove any scaling used during encoding
		odd <<= descale_shift;

		// The lowpass result should be a positive number
		//assert(0 <= odd && odd <= INT16_MAX);

		// Place the odd result in the odd column
		output[2 * column + 1] = ClampPixel(odd);
	}

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Apply the inverse wavelet transforms to the first row in each wavelet

	The inverse vertical and horizontal filters are applied to the first row in the
	final decoded wavelet in each channel.  This routine uses special filters for
	the first row at the top border of the input wavelet bands.
*/
CODEC_ERROR InvertSpatialTopRow(PIXEL **input_data[], DIMENSION input_width[], DIMENSION input_pitch[],
								PIXEL *output_buffer, DIMENSION output_width, DIMENSION output_pitch,
								DIMENSION output_byte_offset[],
								int input_row, int channel_count, int precision, QUANT *quantization[],
								ALLOCATOR *allocator)
{
	PIXEL *even_lowpass;
	PIXEL *even_highpass;
	PIXEL *odd_lowpass;
	PIXEL *odd_highpass;

	uint8_t *even_output_ptr = (uint8_t *)output_buffer;
	uint8_t *odd_output_ptr = even_output_ptr + output_pitch;

	int channel;

	size_t vertical_buffer_size = 0;

	// Compute the maximum size of the intermediate results from the inverse vertical transform
	for (channel = 0; channel < channel_count; channel++)
	{
		size_t buffer_row_size = input_width[channel] * sizeof(PIXEL);

		if (vertical_buffer_size < buffer_row_size) {
			vertical_buffer_size = buffer_row_size;
		}

		// Check that the pitch is a multiple of the pixel size
		assert((input_pitch[channel] % sizeof(PIXEL)) == 0);
	}

	// Allocate buffers for the even and odd rows of vertical transform results
	even_lowpass = (PIXEL *)Alloc(allocator, vertical_buffer_size);
	even_highpass = (PIXEL *)Alloc(allocator, vertical_buffer_size);
	odd_lowpass = (PIXEL *)Alloc(allocator, vertical_buffer_size);
	odd_highpass = (PIXEL *)Alloc(allocator, vertical_buffer_size);

	// This routine should be called for the first row only
	assert(input_row == 0);

	for (channel = 0; channel < channel_count; channel++)
	{
		// Get the wavelet bands for this channel
		PIXEL *lowlow = input_data[channel][LL_BAND];
		PIXEL *lowhigh = input_data[channel][LH_BAND];
		PIXEL *highlow = input_data[channel][HL_BAND];
		PIXEL *highhigh = input_data[channel][HH_BAND];

		// Compute the location of the even and odd output rows for this channel
		PIXEL *even_output = (PIXEL *)(even_output_ptr + output_byte_offset[channel]);
		PIXEL *odd_output = (PIXEL *)(odd_output_ptr + output_byte_offset[channel]);

		// Convert pitch from bytes to pixels
		DIMENSION input_row_width = input_pitch[channel] / sizeof(PIXEL);

		// Start at the first column
		int column = 0;

		// Apply the vertical border filter to the first row
		for (; column < input_width[channel]; column++)
		{
			int32_t even = 0;		// Result of convolution with even filter
			int32_t odd = 0;		// Result of convolution with odd filter

			PIXEL lowhigh_value[3];

			// Dequantize the highpass values used in this loop iteration
			PIXEL highlow_value = DequantizedValue(highlow[column], quantization[channel][HL_BAND]);
			PIXEL highhigh_value = DequantizedValue(highhigh[column], quantization[channel][HH_BAND]);

			// Dequantize the highpass values in three rows from the upper right highpass band
			lowhigh_value[0] = DequantizedValue(lowhigh[column + 0 * input_row_width], quantization[channel][LH_BAND]);
			lowhigh_value[1] = DequantizedValue(lowhigh[column + 1 * input_row_width], quantization[channel][LH_BAND]);
			lowhigh_value[2] = DequantizedValue(lowhigh[column + 2 * input_row_width], quantization[channel][LH_BAND]);


			/***** Compute the vertical inverse for the left two bands *****/

			// Apply the even reconstruction filter to the lowpass band
			even += 11 * lowlow[column + 0 * input_row_width];
			even -=  4 * lowlow[column + 1 * input_row_width];
			even +=  1 * lowlow[column + 2 * input_row_width];
			even += rounding;
			even = DivideByShift(even, 3);

			// Add the highpass correction
			even += highlow_value;
			even = DivideByShift(even, 1);

			// Place the even result in the even row
			even_lowpass[column] = ClampPixel(even);

			// Apply the odd reconstruction filter to the lowpass band
			odd += 5 * lowlow[column + 0 * input_row_width];
			odd += 4 * lowlow[column + 1 * input_row_width];
			odd -= 1 * lowlow[column + 2 * input_row_width];
			odd += rounding;
			odd = DivideByShift(odd, 3);

			// Subtract the highpass correction
			odd -= highlow_value;
			odd = DivideByShift(odd, 1);

			// Place the odd result in the odd row
			odd_lowpass[column] = ClampPixel(odd);


			/***** Compute the vertical inverse for the right two bands *****/

			even = 0;
			odd = 0;

			// Apply the even reconstruction filter to the lowpass band
			even += 11 * lowhigh_value[0];
			even -=  4 * lowhigh_value[1];
			even +=  1 * lowhigh_value[2];
			even += rounding;
			even = DivideByShift(even, 3);

			// Add the highpass correction
			even += highhigh_value;
			even = DivideByShift(even, 1);

			// Place the even result in the even row
			even_highpass[column] = ClampPixel(even);

			// Apply the odd reconstruction filter to the lowpass band
			odd += 5 * lowhigh_value[0];
			odd += 4 * lowhigh_value[1];
			odd -= 1 * lowhigh_value[2];
			odd += rounding;
			odd = DivideByShift(odd, 3);

			// Subtract the highpass correction
			odd -= highhigh_value;
			odd = DivideByShift(odd, 1);

			// Place the odd result in the odd row
			odd_highpass[column] = ClampPixel(odd);
		}

		// Apply the inverse horizontal transform to the even and odd rows
		InvertHorizontalScaled16s(even_lowpass, even_highpass, even_output, input_width[channel], output_width, precision);
		InvertHorizontalScaled16s(odd_lowpass, odd_highpass, odd_output, input_width[channel], output_width, precision);
	}

	// Deallocate the scratch buffers used for the results from the inverse vertical transform
	Free(allocator, even_lowpass);
	Free(allocator, even_highpass);
	Free(allocator, odd_lowpass);
	Free(allocator, odd_highpass);

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Apply the inverse wavelet transforms to the middle rows in each wavelet

	The inverse vertical and horizontal filters are applied to the rows in the final
	decoded wavelet in each channel, except the top and bottom rows that are handled
	by other routines that use special filters for the image borders.
*/
CODEC_ERROR InvertSpatialMiddleRow(PIXEL **input_data[], DIMENSION input_width[], DIMENSION input_pitch[],
								   PIXEL *output_buffer, DIMENSION output_width, DIMENSION output_pitch,
								   DIMENSION output_byte_offset[],
								   int input_row, int channel_count, int precision, QUANT *quantization[],
								   ALLOCATOR *allocator)
{
	PIXEL *even_lowpass;
	PIXEL *even_highpass;
	PIXEL *odd_lowpass;
	PIXEL *odd_highpass;

	uint8_t *even_output_ptr = (uint8_t *)output_buffer + 2 * input_row * output_pitch;
	uint8_t *odd_output_ptr = even_output_ptr + output_pitch;

	int channel;

	size_t vertical_buffer_size = 0;

	// Compute the maximum size of the intermediate results from the inverse vertical transform
	for (channel = 0; channel < channel_count; channel++)
	{
		size_t buffer_row_size = input_width[channel] * sizeof(PIXEL);

		if (vertical_buffer_size < buffer_row_size) {
			vertical_buffer_size = buffer_row_size;
		}

		// Check that the pitch is a multiple of the pixel size
		assert((input_pitch[channel] % sizeof(PIXEL)) == 0);
	}

	// Allocate buffers for the even and odd rows of vertical transform results
	even_lowpass = (PIXEL *)Alloc(allocator, vertical_buffer_size);
	even_highpass = (PIXEL *)Alloc(allocator, vertical_buffer_size);
	odd_lowpass = (PIXEL *)Alloc(allocator, vertical_buffer_size);
	odd_highpass = (PIXEL *)Alloc(allocator, vertical_buffer_size);

	// This routine should not be called to process the first or last row
	assert(input_row > 0);

	for (channel = 0; channel < channel_count; channel++)
	{
		// Get the wavelet bands for this channel
		uint8_t *lowlow_band = (uint8_t *)input_data[channel][LL_BAND];
		uint8_t *lowhigh_band = (uint8_t *)input_data[channel][LH_BAND];
		uint8_t *highlow_band = (uint8_t *)input_data[channel][HL_BAND];
		uint8_t *highhigh_band = (uint8_t *)input_data[channel][HH_BAND];

		// Compute the pointer to the current row in each band
		PIXEL *lowlow = (PIXEL *)(lowlow_band + input_row * input_pitch[channel]);
		PIXEL *lowhigh = (PIXEL *)(lowhigh_band + input_row * input_pitch[channel]);
		PIXEL *highlow = (PIXEL *)(highlow_band + input_row * input_pitch[channel]);
		PIXEL *highhigh = (PIXEL *)(highhigh_band + input_row * input_pitch[channel]);

		// Compute the location of the even and odd output rows for this channel
		PIXEL *even_output = (PIXEL *)(even_output_ptr + output_byte_offset[channel]);
		PIXEL *odd_output = (PIXEL *)(odd_output_ptr + output_byte_offset[channel]);

		// Convert the wavelet band pitch from bytes to pixels
		DIMENSION input_row_width = input_pitch[channel] / sizeof(PIXEL);

		// Start at the first column
		int column = 0;

		// Apply the inverse vertical filter to the middle rows
		for (; column < input_width[channel]; column++)
		{
			int32_t even = 0;		// Result of convolution with even filter
			int32_t odd = 0;		// Result of convolution with odd filter

			PIXEL lowhigh_value[3];

			// Dequantize the highpass values used in this loop iteration
			PIXEL highlow_value = DequantizedValue(highlow[column], quantization[channel][HL_BAND]);
			PIXEL highhigh_value = DequantizedValue(highhigh[column], quantization[channel][HH_BAND]);

			// Dequantize the highpass values in three rows from the upper right highpass band
			lowhigh_value[0] = DequantizedValue(lowhigh[column - 1 * input_row_width], quantization[channel][LH_BAND]);
			lowhigh_value[1] = DequantizedValue(lowhigh[column + 0 * input_row_width], quantization[channel][LH_BAND]);
			lowhigh_value[2] = DequantizedValue(lowhigh[column + 1 * input_row_width], quantization[channel][LH_BAND]);


			/***** Compute the vertical inverse transform for the left two bands *****/

			// Apply the even reconstruction filter to the lowpass band
			even += lowlow[column - 1 * input_row_width];
			even -= lowlow[column + 1 * input_row_width];
			even += rounding;
			even = DivideByShift(even, 3);
			even += lowlow[column + 0 * input_row_width];

			// Add the highpass correction
			even += highlow_value;
			even = DivideByShift(even, 1);

			// Place the even result in the even row
			even_lowpass[column] = ClampPixel(even);

			// Apply the odd reconstruction filter to the lowpass band
			odd -= lowlow[column - 1 * input_row_width];
			odd += lowlow[column + 1 * input_row_width];
			odd += rounding;
			odd = DivideByShift(odd, 3);
			odd += lowlow[column + 0 * input_row_width];

			// Subtract the highpass correction
			odd -= highlow_value;
			odd = DivideByShift(odd, 1);

			// Place the odd result in the odd row
			odd_lowpass[column] = ClampPixel(odd);


			/***** Compute the vertical inverse transform for the right two bands *****/

			even = 0;
			odd = 0;

			// Apply the even reconstruction filter to the lowpass band
			even += lowhigh_value[0];
			even -= lowhigh_value[2];
			even += rounding;
			even = DivideByShift(even, 3);
			even += lowhigh_value[1];

			// Add the highpass correction
			even += highhigh_value;
			even = DivideByShift(even, 1);

			// Place the even result in the even row
			even_highpass[column] = ClampPixel(even);

			// Apply the odd reconstruction filter to the lowpass band
			odd -= lowhigh_value[0];
			odd += lowhigh_value[2];
			odd += rounding;
			odd = DivideByShift(odd, 3);
			odd += lowhigh_value[1];

			// Subtract the highpass correction
			odd -= highhigh_value;
			odd = DivideByShift(odd, 1);

			// Place the odd result in the odd row
			odd_highpass[column] = ClampPixel(odd);
		}

		// Apply the inverse horizontal transform to the even and odd rows
		InvertHorizontalScaled16s(even_lowpass, even_highpass, even_output, input_width[channel], output_width, precision);
		InvertHorizontalScaled16s(odd_lowpass, odd_highpass, odd_output, input_width[channel], output_width, precision);
	}

	// Deallocate the scratch buffers used for the results from the inverse vertical transform
	Free(allocator, even_lowpass);
	Free(allocator, even_highpass);
	Free(allocator, odd_lowpass);
	Free(allocator, odd_highpass);

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Apply the inverse wavelet transforms to the last row in each wavelet

	The inverse vertical and horizontal filters are applied to the last row in the
	final decoded wavelet in each channel.  This routine uses special filters for
	the last row at the bottom border of the input wavelet bands.

	@todo May have to pass the height of the wavelet bands as an argument so that this
	routine can check that the input row is in bounds and valid.
*/
CODEC_ERROR InvertSpatialBottomRow(PIXEL **input_data[], DIMENSION input_width[], DIMENSION input_pitch[],
								   PIXEL *output_buffer, DIMENSION output_width, DIMENSION output_pitch,
								   DIMENSION output_byte_offset[],
								   int input_row, int channel_count, int precision, QUANT *quantization[],
								   ALLOCATOR *allocator)
{
	PIXEL *even_lowpass;
	PIXEL *even_highpass;
	PIXEL *odd_lowpass;
	PIXEL *odd_highpass;

	uint8_t *even_output_ptr = (uint8_t *)output_buffer + 2 * input_row * output_pitch;
	uint8_t *odd_output_ptr = even_output_ptr + output_pitch;

	int channel;

	size_t vertical_buffer_size = 0;

	// Compute the maximum size of the intermediate results from the inverse vertical transform
	for (channel = 0; channel < channel_count; channel++)
	{
		size_t buffer_row_size = input_width[channel] * sizeof(PIXEL);

		if (vertical_buffer_size < buffer_row_size) {
			vertical_buffer_size = buffer_row_size;
		}

		// Check that the pitch is a multiple of the pixel size
		assert((input_pitch[channel] % sizeof(PIXEL)) == 0);
	}

	// Allocate buffers for the even and odd rows of vertical transform results
	even_lowpass = (PIXEL *)Alloc(allocator, vertical_buffer_size);
	even_highpass = (PIXEL *)Alloc(allocator, vertical_buffer_size);
	odd_lowpass = (PIXEL *)Alloc(allocator, vertical_buffer_size);
	odd_highpass = (PIXEL *)Alloc(allocator, vertical_buffer_size);

	// This routine should be called for the last row only
	assert(input_row > 0);

	for (channel = 0; channel < channel_count; channel++)
	{
		// Get the wavelet bands for this channel
		uint8_t *lowlow_band = (uint8_t *)input_data[channel][LL_BAND];
		uint8_t *lowhigh_band = (uint8_t *)input_data[channel][LH_BAND];
		uint8_t *highlow_band = (uint8_t *)input_data[channel][HL_BAND];
		uint8_t *highhigh_band = (uint8_t *)input_data[channel][HH_BAND];

		// Compute the pointer to the current row in each band
		PIXEL *lowlow = (PIXEL *)(lowlow_band + input_row * input_pitch[channel]);
		PIXEL *lowhigh = (PIXEL *)(lowhigh_band + input_row * input_pitch[channel]);
		PIXEL *highlow = (PIXEL *)(highlow_band + input_row * input_pitch[channel]);
		PIXEL *highhigh = (PIXEL *)(highhigh_band + input_row * input_pitch[channel]);

		// Compute the location of the even and odd output rows for this channel
		PIXEL *even_output = (PIXEL *)(even_output_ptr + output_byte_offset[channel]);
		PIXEL *odd_output = (PIXEL *)(odd_output_ptr + output_byte_offset[channel]);

		// Convert pitch from bytes to pixels
		DIMENSION input_row_width = input_pitch[channel] / sizeof(PIXEL);

		// Start at the first column
		int column = 0;

		// Apply the vertical border filter to the last row
		for (column = 0; column < input_width[channel]; column++)
		{
			int32_t even = 0;		// Result of convolution with even filter
			int32_t odd = 0;		// Result of convolution with odd filter

			PIXEL lowhigh_value[3];

			// Dequantize the highpass values used in this loop iteration
			PIXEL highlow_value = DequantizedValue(highlow[column], quantization[channel][HL_BAND]);
			PIXEL highhigh_value = DequantizedValue(highhigh[column], quantization[channel][HH_BAND]);

			// Dequantize the highpass values in three rows from the upper right highpass band
			lowhigh_value[0] = DequantizedValue(lowhigh[column - 0 * input_row_width], quantization[channel][LH_BAND]);
			lowhigh_value[1] = DequantizedValue(lowhigh[column - 1 * input_row_width], quantization[channel][LH_BAND]);
			lowhigh_value[2] = DequantizedValue(lowhigh[column - 2 * input_row_width], quantization[channel][LH_BAND]);


			/***** Compute the vertical inverse for the left two bands *****/

			// Apply the even reconstruction filter to the lowpass band
			even += 5 * lowlow[column + 0 * input_row_width];
			even += 4 * lowlow[column - 1 * input_row_width];
			even -= 1 * lowlow[column - 2 * input_row_width];
			even += rounding;
			even = DivideByShift(even, 3);

			// Add the highpass correction
			even += highlow_value;
			even = DivideByShift(even, 1);

			// Place the even result in the even row
			even_lowpass[column] = ClampPixel(even);

			// Apply the odd reconstruction filter to the lowpass band
			odd += 11 * lowlow[column + 0 * input_row_width];
			odd -=  4 * lowlow[column - 1 * input_row_width];
			odd +=  1 * lowlow[column - 2 * input_row_width];
			odd += rounding;
			odd = DivideByShift(odd, 3);

			// Subtract the highpass correction
			odd -= highlow_value;
			odd = DivideByShift(odd, 1);

			// Place the odd result in the odd row
			odd_lowpass[column] = ClampPixel(odd);


			/***** Compute the vertical inverse for the right two bands *****/

			even = 0;
			odd = 0;

			// Apply the even reconstruction filter to the lowpass band
			even += 5 * lowhigh_value[0];
			even += 4 * lowhigh_value[1];
			even -= 1 * lowhigh_value[2];
			even += rounding;
			even = DivideByShift(even, 3);

			// Add the highpass correction
			even += highhigh_value;
			even = DivideByShift(even, 1);

			// Place the even result in the even row
			even_highpass[column] = ClampPixel(even);

			// Apply the odd reconstruction filter to the lowpass band
			odd += 11 * lowhigh_value[0];
			odd -=  4 * lowhigh_value[1];
			odd +=  1 * lowhigh_value[2];
			odd += rounding;
			odd = DivideByShift(odd, 3);

			// Subtract the highpass correction
			odd -= highhigh_value;
			odd = DivideByShift(odd, 1);

			// Place the odd result in the odd row
			odd_highpass[column] = ClampPixel(odd);
		}

		// Apply the inverse horizontal transform to the even and odd rows
		InvertHorizontalScaled16s(even_lowpass, even_highpass, even_output, input_width[channel], output_width, precision);
		InvertHorizontalScaled16s(odd_lowpass, odd_highpass, odd_output, input_width[channel], output_width, precision);
	}

	// Deallocate the scratch buffers used for the results from the inverse vertical transform
	Free(allocator, even_lowpass);
	Free(allocator, even_highpass);
	Free(allocator, odd_lowpass);
	Free(allocator, odd_highpass);

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Apply the inverse horizontal transform and scale the result to 16-bits

	This routine is used to apply the inverse horizontal wavelet filter in the last
	inverse transform that reconstructs the output frame.  This routine outputs an
	intermediate result using the row organization that is used internally by the
	codec.  The intermediate values are scaled to 16 bits of precision.
*/
CODEC_ERROR InvertHorizontalScaled16s(PIXEL *lowpass,
									  PIXEL *highpass,
									  PIXEL *output,
									  DIMENSION input_width,
									  DIMENSION output_width,
									  int precision)
{
	const int last_column = input_width - 1;

	// Calculate the shift required to output 16-bit pixels
	int scale_shift = (16 - precision);

	int32_t even;
	int32_t odd;

	// Start processing at the beginning of the row
	int column = 0;

	// Process the first two output points with special filters for the left border
	even = 0;
	odd = 0;

	assert(lowpass[column] >= 0);

	// Apply the even reconstruction filter to the lowpass band
	even += 11 * lowpass[column + 0];
	even -=  4 * lowpass[column + 1];
	even +=  1 * lowpass[column + 2];
	even += rounding;
	even = DivideByShift(even, 3);

	// Add the highpass correction
	even += highpass[column];
	even = DivideByShift(even, 1);
	assert(even >= 0);

	// Scale the result to the full 16-bit range
	even <<= scale_shift;
	output[0] = clamp_uint16(even);

	// Apply the odd reconstruction filter to the lowpass band
	odd += 5 * lowpass[column + 0];
	odd += 4 * lowpass[column + 1];
	odd -= 1 * lowpass[column + 2];
	odd += rounding;
	odd = DivideByShift(odd, 3);

	// Subtract the highpass correction
	odd -= highpass[column];
	odd = DivideByShift(odd, 1);
	assert(odd >= 0);

	// Scale the result to the full 16-bit range
	odd <<= scale_shift;
	output[1] = clamp_uint16(odd);

	// Advance to the next input column
	column++;

	// Process the rest of the columns up to the last column in the row
	for (; column < last_column; column++)
	{
		even = 0;
		odd = 0;

		assert(lowpass[column] >= 0);

		// Apply the even reconstruction filter to the lowpass band
		even += lowpass[column - 1];
		even -= lowpass[column + 1];
		even += rounding;
		even >>= 3;
		even += lowpass[column + 0];

		// Add the highpass correction
		even += highpass[column];
		even = DivideByShift(even, 1);
		//assert(even >= 0);

		// Scale the result to the full 16-bit range
		even <<= scale_shift;

		// Place the even result in the even column
		output[2 * column + 0] = clamp_uint16(even);

		// Apply the odd reconstruction filter to the lowpass band
		odd -= lowpass[column - 1];
		odd += lowpass[column + 1];
		odd += rounding;
		odd >>= 3;
		odd += lowpass[column + 0];

		// Subtract the highpass correction
		odd -= highpass[column];
		odd = DivideByShift(odd, 1);
		//assert(odd >= 0);

		// Scale the result to the full 16-bit range
		odd <<= scale_shift;

		// Place the odd result in the odd column
		output[2 * column + 1] = clamp_uint16(odd);
	}

	// Should have exited the loop at the column for right border processing
	assert(column == last_column);

	// Process the last two output points with special filters for the right border
	even = 0;
	odd = 0;

	assert(lowpass[column] >= 0);

	// Apply the even reconstruction filter to the lowpass band
	even += 5 * lowpass[column + 0];
	even += 4 * lowpass[column - 1];
	even -= 1 * lowpass[column - 2];
	even += rounding;
	even = DivideByShift(even, 3);

	// Add the highpass correction
	even += highpass[column];
	even = DivideByShift(even, 1);
	assert(even >= 0);

	// Scale the result to the full 16-bit range
	even <<= scale_shift;

	// Place the even result in the even column
	output[2 * column + 0] = clamp_uint16(even);

	// Apply the odd reconstruction filter to the lowpass band
	odd += 11 * lowpass[column + 0];
	odd -=  4 * lowpass[column - 1];
	odd +=  1 * lowpass[column - 2];
	odd += rounding;
	odd = DivideByShift(odd, 3);

	// Subtract the highpass correction
	odd -= highpass[column];
	odd = DivideByShift(odd, 1);
	assert(odd >= 0);

	// Scale the result to the full 16-bit range
	odd <<= scale_shift;

	if ((2 * column + 1) < output_width)
	{
		// Place the odd result in the odd column
		output[2 * column + 1] = clamp_uint16(odd);
	}

	return CODEC_ERROR_OKAY;
}
