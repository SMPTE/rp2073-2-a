/*!	@file encoder/src/forward.c

	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#include "headers.h"


//! Rounding added to the highpass sum before division
static const int32_t rounding = 4;


/*!
	@brief Apply the horizontal wavelet filter to a row of pixels
*/
CODEC_ERROR FilterHorizontalRow(PIXEL *input, PIXEL *lowpass, PIXEL *highpass, int width, int prescale)
{
	int last_input_column;		// Column at which right border processing is done
	int column = 0;
	int32_t sum;

	//uint16_t *input = (uint16_t *)input_buffer;

	//TODO: Check that the rounding is correct for all prescale values
	int prescale_rounding = (1 << prescale) - 1;

	//TODO Test this routine with other prescale values
	assert(prescale == 0 || prescale == 2);

	last_input_column = ((width % 2) == 0) ? width - 2 : width - 1;


	/***** Process the left border using the formula for boundary conditions *****/

	// Compute the lowpass coefficient
	lowpass[0] = (input[0] + input[1] + prescale_rounding) >> prescale;

	// Initialize the sum for computing the highpass coefficient
	sum = 0;

	sum +=  5 * ((input[0] + prescale_rounding) >> prescale);
	sum -= 11 * ((input[1] + prescale_rounding) >> prescale);
	sum +=  4 * ((input[2] + prescale_rounding) >> prescale);
	sum +=  4 * ((input[3] + prescale_rounding) >> prescale);
	sum -=  1 * ((input[4] + prescale_rounding) >> prescale);
	sum -=  1 * ((input[5] + prescale_rounding) >> prescale);
	sum += rounding;
	sum = DivideByShift(sum, 3);
	highpass[0] = ClampPixel(sum);


	/***** Process the internal pixels using the normal wavelet formula *****/

	for (column = 2; column < last_input_column; column += 2)
	{
		// Column index should always be divisible by two
		assert((column % 2) == 0);

		// Compute the lowpass coefficient
		lowpass[column/2] = (input[column + 0] + input[column + 1] + prescale_rounding) >> prescale;

		// Initialize the sum for computing the highpass coefficient
		sum = 0;

		sum -= (input[column - 2] + prescale_rounding) >> prescale;
		sum -= (input[column - 1] + prescale_rounding) >> prescale;
		sum += (input[column + 2] + prescale_rounding) >> prescale;
		if ((column + 3) < width)
		{
			// Use the value in the last column
			sum += (input[column + 3] + prescale_rounding) >> prescale;
		} 
		else
		{
			// Duplicate the value in the last column
			sum += (input[column + 2] + prescale_rounding) >> prescale;
		}
		sum += rounding;
		sum = DivideByShift(sum, 3);
		sum += (input[column + 0] + prescale_rounding) >> prescale;
		sum -= (input[column + 1] + prescale_rounding) >> prescale;
		highpass[column/2] = ClampPixel(sum);
	}

	// Should have exited the loop at the last column
	assert(column == last_input_column);


	/***** Process the right border using the formula for boundary conditions *****/

	// Compute the lowpass coefficient
	if ((column + 1) < width)
	{
		// Use the value in the last column
		lowpass[column/2] = (input[column + 0] + input[column + 1] + prescale_rounding) >> prescale;
	}
	else
	{
		// Duplicate the value in the last column
		lowpass[column/2] = (input[column + 0] + input[column + 0] + prescale_rounding) >> prescale;
	}

	sum = 0;

	if ((column + 1) < width)
	{
		// Use the value in the last column
		sum -=  5 * ((input[column + 1] + prescale_rounding) >> prescale);
	}
	else
	{
		// Duplicate the value in the last column
		sum -=  5 * ((input[column + 0] + prescale_rounding) >> prescale);
	}
	sum += 11 * ((input[column + 0] + prescale_rounding) >> prescale);
	sum -=  4 * ((input[column - 1] + prescale_rounding) >> prescale);
	sum -=  4 * ((input[column - 2] + prescale_rounding) >> prescale);
	sum +=  1 * ((input[column - 3] + prescale_rounding) >> prescale);
	sum +=  1 * ((input[column - 4] + prescale_rounding) >> prescale);
	sum += rounding;
	sum = DivideByShift(sum, 3);
	highpass[column/2] = ClampPixel(sum);

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Apply the vertical wavelet filter to the first row

	This routine uses the wavelet formulas for the top row of an image

	The midpoint prequant argument is not the offset that is added to the
	value prior to quantization.  It is a setting indicating which midpoint
	offset to use.

	@todo Change the midpoint_prequant argument to midpoint_setting?
*/
CODEC_ERROR FilterVerticalTopRow(PIXEL *lowpass[], PIXEL *highpass[],
								 PIXEL *output[], DIMENSION pitch,
								 int band_count, int input_row,
								 int wavelet_width, QUANT quant[],
								 int32_t midpoint_prequant)
{
	int column;

	//uint16_t **lowpass = (uint16_t **)lowpass_buffer;

	assert(input_row == 0 && band_count == 4);

	//TODO: Setup quantization parameters here or in caller

	for (column = 0; column < wavelet_width; column++)
	{
		int32_t sum;

		// Apply the lowpass vertical filter to the lowpass horizontal results
		sum  = lowpass[0][column];
		sum += lowpass[1][column];
		output[LL_BAND][column] = ClampPixel(sum);
		assert(output[LL_BAND][column] >= 0);

		// Apply the highpass vertical filter to the lowpass horizontal results
		sum  =  5 * lowpass[0][column];
		sum -= 11 * lowpass[1][column];
		sum +=  4 * lowpass[2][column];
		sum +=  4 * lowpass[3][column];
		sum -=  1 * lowpass[4][column];
		sum -=  1 * lowpass[5][column];
		sum += rounding;
		sum = DivideByShift(sum, 3);
		output[HL_BAND][column] = QuantizePixel(sum, quant[HL_BAND], midpoint_prequant);	//ClampPixel(sum);

		// Apply the lowpass vertical filter to the highpass horizontal results
		sum  = highpass[0][column];
		sum += highpass[1][column];
		output[LH_BAND][column] = QuantizePixel(sum, quant[LH_BAND], midpoint_prequant);	//ClampPixel(sum);

		// Apply the highpass vertical filter to the highpass horizontal results
		sum  =  5 * highpass[0][column];
		sum -= 11 * highpass[1][column];
		sum +=  4 * highpass[2][column];
		sum +=  4 * highpass[3][column];
		sum -=  1 * highpass[4][column];
		sum -=  1 * highpass[5][column];
		sum += rounding;
		sum = DivideByShift(sum, 3);
		output[HH_BAND][column] = QuantizePixel(sum, quant[HH_BAND], midpoint_prequant);	//ClampPixel(sum);
	}

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Apply the vertical wavelet filter to a middle row

	This routine uses the wavelet formulas for the middle rows of an image
*/
CODEC_ERROR FilterVerticalMiddleRow(PIXEL *lowpass[], PIXEL *highpass[],
									PIXEL *output[], DIMENSION pitch,
									int band_count, int input_row,
									int wavelet_width, QUANT quant[],
									int32_t midpoint_prequant)
{
	PIXEL *result[MAX_BAND_COUNT];
	int column;
	int band;

	//uint16_t **lowpass = (uint16_t **)lowpass_buffer;

	int output_row = input_row / 2;

	assert(band_count == 4);

	// Compute the address of each output row
	for (band = 0; band < band_count; band++)
	{
		uint8_t *band_row_ptr = (uint8_t *)output[band];
		band_row_ptr += output_row * pitch;
		result[band] = (PIXEL *)band_row_ptr;
	}

	for (column = 0; column < wavelet_width; column++)
	{
		int32_t sum;

		// Apply the lowpass vertical filter to the lowpass horizontal results
		sum  = lowpass[2][column];
		sum += lowpass[3][column];
		result[LL_BAND][column] = ClampPixel(sum);
		assert(result[LL_BAND][column] >= 0);

		// Apply the highpass vertical filter to the lowpass horizontal results
		sum  = -1 * lowpass[0][column];
		sum += -1 * lowpass[1][column];
		sum +=  1 * lowpass[4][column];
		sum +=  1 * lowpass[5][column];
		//sum += 4;
		//sum >>= 3;
		sum +=	rounding;
		sum = DivideByShift(sum, 3);
		sum +=  1 * lowpass[2][column];
		sum += -1 * lowpass[3][column];
		result[HL_BAND][column] = QuantizePixel(sum, quant[HL_BAND], midpoint_prequant);

		// Apply the lowpass vertical filter to the highpass horizontal results
		sum  = highpass[2][column];
		sum += highpass[3][column];
		result[LH_BAND][column] = QuantizePixel(sum, quant[LH_BAND], midpoint_prequant);

		// Apply the highpass vertical filter to the highpass horizontal results
		sum  = -1 * highpass[0][column];
		sum += -1 * highpass[1][column];
		sum +=  1 * highpass[4][column];
		sum +=  1 * highpass[5][column];
		//sum += 4;
		//sum >>= 3;
		sum +=	rounding;
		sum = DivideByShift(sum, 3);
		sum +=  1 * highpass[2][column];
		sum += -1 * highpass[3][column];
		result[HH_BAND][column] = QuantizePixel(sum, quant[HH_BAND], midpoint_prequant);
	}

	return CODEC_ERROR_OKAY;
}

CODEC_ERROR FilterVerticalBottomRow(PIXEL *lowpass[], PIXEL *highpass[],
									PIXEL *output[], DIMENSION pitch,
									int band_count, int input_row,
									int wavelet_width, QUANT quant[],
									int32_t midpoint_prequant)
{
	PIXEL *result[MAX_BAND_COUNT];
	int column;
	int band;

	//uint16_t **lowpass = (uint16_t **)lowpass_buffer;

	int output_row = input_row / 2;

	assert(band_count == 4);

	// Compute the address of each output row
	for (band = 0; band < band_count; band++)
	{
		uint8_t *band_row_ptr = (uint8_t *)output[band];
		band_row_ptr += output_row * pitch;
		result[band] = (PIXEL *)band_row_ptr;
	}

	for (column = 0; column < wavelet_width; column++)
	{
		int32_t sum;

		// Apply the lowpass vertical filter to the lowpass horizontal results
		sum  = lowpass[4][column];
		sum += lowpass[5][column];
		result[LL_BAND][column] = ClampPixel(sum);
		assert(result[LL_BAND][column] >= 0);

		// Apply the highpass vertical filter to the lowpass horizontal results
		sum  = 11 * lowpass[4][column];
		sum -=  5 * lowpass[5][column];
		sum -=  4 * lowpass[3][column];
		sum -=  4 * lowpass[2][column];
		sum +=  1 * lowpass[1][column];
		sum +=  1 * lowpass[0][column];
		sum +=  rounding;
		sum = DivideByShift(sum, 3);
		result[HL_BAND][column] = QuantizePixel(sum, quant[HL_BAND], midpoint_prequant);	//ClampPixel(sum);

		// Apply the lowpass vertical filter to the highpass horizontal results
		sum  = highpass[4][column];
		sum += highpass[5][column];
		result[LH_BAND][column] = QuantizePixel(sum, quant[LH_BAND], midpoint_prequant);	//ClampPixel(sum);

		// Apply the highpass vertical filter to the highpass horizontal results
		sum  = 11 * highpass[4][column];
		sum -=  5 * highpass[5][column];
		sum -=  4 * highpass[3][column];
		sum -=  4 * highpass[2][column];
		sum +=  1 * highpass[1][column];
		sum +=  1 * highpass[0][column];
		sum +=  rounding;
		sum = DivideByShift(sum, 3);
		result[HH_BAND][column] = QuantizePixel(sum, quant[HH_BAND], midpoint_prequant);	//ClampPixel(sum);
	}

	return CODEC_ERROR_OKAY;
}
