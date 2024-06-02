/*! @file decoder/src/wavelet.c

	Implementation of the module for wavelet data structures and transforms

	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#include "headers.h"

/*!
	@brief Initialize a wavelet data structure with the specified dimensions
*/
CODEC_ERROR InitWavelet(WAVELET *wavelet, DIMENSION width, DIMENSION height)
{
	assert(wavelet != NULL);
	if (! (wavelet != NULL)) {
		return CODEC_ERROR_NULLPTR;
	}

	memset(wavelet, 0, sizeof(WAVELET));

	wavelet->width = width;
	wavelet->height = height;
	wavelet->band_count = 4;

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Allocate a wavelet data structure with the specified dimensions

	@todo Is it necessary to align the pitch?
*/
CODEC_ERROR AllocWavelet(ALLOCATOR *allocator, WAVELET *wavelet, DIMENSION width, DIMENSION height)
{
	// Initialize the fields in the wavelet data structure
	InitWavelet(wavelet, width, height);

	if (width > 0 && height > 0)
	{
		DIMENSION pitch = width * sizeof(PIXEL);

		size_t band_data_size = height * pitch;
		int band;

		// Allocate the wavelet bands
		for (band = 0; band < MAX_BAND_COUNT; band++)
		{
			wavelet->data[band] = (PIXEL *)Alloc(allocator, band_data_size);
			if (wavelet->data[band] == NULL) {
				ReleaseWavelet(allocator, wavelet);
				return CODEC_ERROR_OUTOFMEMORY;
			}
		}

		wavelet->pitch = pitch;
	}

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Release all resources allocated to the wavelet

	The wavelet data structure itself is not reallocated.
*/
CODEC_ERROR ReleaseWavelet(ALLOCATOR *allocator, WAVELET *wavelet)
{
	int band;

	for (band = 0; band < MAX_BAND_COUNT; band++) {
		Free(allocator, wavelet->data[band]);
		wavelet->data[band] = NULL;
	}

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Create and allocate a wavelet data structure
*/
WAVELET *CreateWavelet(ALLOCATOR *allocator, DIMENSION width, DIMENSION height)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;

	if (width > 0 && height > 0)
	{
		// Allocate the image data structure for the wavelet
		WAVELET *wavelet = (WAVELET *)Alloc(allocator, sizeof(WAVELET));
		assert(wavelet != NULL);
		if (! (wavelet != NULL)) {
			return NULL;
		}

		// Allocate space for the wavelet bands
		error = AllocWavelet(allocator, wavelet, width, height);
		if (error == CODEC_ERROR_OKAY) {
			return wavelet;
		}

		// Avoid a memory leak
		DeleteWavelet(allocator, wavelet);
	}

	return NULL;
}

/*!
	@brief Release all resources the free the wavelet data structure
*/
CODEC_ERROR DeleteWavelet(ALLOCATOR *allocator, WAVELET *wavelet)
{
	ReleaseWavelet(allocator, wavelet);
	Free(allocator, wavelet);
	return CODEC_ERROR_OKAY;
}

/*!
	@brief Apply the inverse wavelet transform to reconstruct a lowpass band

	This routine reconstructs the lowpass band in the output wavelet from the
	decoded bands in the input wavelet.  The prescale argument is used to undo
	scaling that may have been performed during encoding to prevent overflow.
	
	@todo Replace the two different routines for different prescale shifts with
	a single routine that can handle any prescale shift.
*/
CODEC_ERROR TransformInverseSpatialQuantLowpass(ALLOCATOR *allocator, WAVELET *input, WAVELET *output, uint16_t prescale)
{
	// Dimensions of each wavelet band
	DIMENSION input_width = input->width;
	DIMENSION input_height = input->height;

	// The output width may be less than twice the input width if the output width is odd
	DIMENSION output_width = output->width;

	// The output height may be less than twice the input height if the output height is odd
	DIMENSION output_height = output->height;

	// Check that a valid input image has been provided
	assert(input != NULL);
	assert(input->data[0] != NULL);
	assert(input->data[1] != NULL);
	assert(input->data[2] != NULL);
	assert(input->data[3] != NULL);

	// Check that the output image is a gray image or a lowpass wavelet band
	assert(output->data[0] != NULL);

	// Check for valid quantization values
	if (input->quant[0] == 0) {
		input->quant[0] = 1;
	}

	assert(input->quant[0] > 0);
	assert(input->quant[1] > 0);
	assert(input->quant[2] > 0);
	assert(input->quant[3] > 0);

	if (prescale > 1)
	{
		// This is a spatial transform for the lowpass temporal band
		assert(prescale == 2);

		// Apply the inverse spatial transform for a lowpass band that is not prescaled
		InvertSpatialQuantDescale16s(allocator,
									 (PIXEL *)input->data[0], input->pitch,
									 (PIXEL *)input->data[1], input->pitch,
									 (PIXEL *)input->data[2], input->pitch,
									 (PIXEL *)input->data[3], input->pitch,
									 output->data[0], output->pitch,
									 input_width, input_height,
									 output_width, output_height,
									 prescale, input->quant);
	}
	else
	{
		// This case does not handle any prescaling applied during encoding
		assert(prescale == 0);

		// Apply the inverse spatial transform for a lowpass band that is not prescaled
		InvertSpatialQuant16s(allocator,
							  (PIXEL *)input->data[0], input->pitch,
							  (PIXEL *)input->data[1], input->pitch,
							  (PIXEL *)input->data[2], input->pitch,
							  (PIXEL *)input->data[3], input->pitch,
							  output->data[0], output->pitch,
							  input_width, input_height,
							  output_width, output_height,
							  input->quant);
	}

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Apply the inverse wavelet transform to reconstruct a component array

	This routine reconstructs the lowpass band in the output wavelet from the
	decoded bands in the input wavelet.  The prescale argument is used to undo
	scaling that may have been performed during encoding to prevent overflow.
*/
CODEC_ERROR TransformInverseSpatialQuantArray(ALLOCATOR *allocator,
											  WAVELET *input,
											  COMPONENT_VALUE *output_buffer,
											  DIMENSION output_width,
											  DIMENSION output_height,
											  size_t output_pitch,
											  PRESCALE prescale)
{
	// Dimensions of each wavelet band
	DIMENSION input_width = input->width;
	DIMENSION input_height = input->height;

	// Check that a valid input image has been provided
	assert(input != NULL);
	assert(input->data[0] != NULL);
	assert(input->data[1] != NULL);
	assert(input->data[2] != NULL);
	assert(input->data[3] != NULL);

	// Check for valid quantization values
	if (input->quant[0] == 0) {
		input->quant[0] = 1;
	}

	assert(input->quant[0] > 0);
	assert(input->quant[1] > 0);
	assert(input->quant[2] > 0);
	assert(input->quant[3] > 0);

	assert(output_width > 0 && output_height > 0 && output_pitch > 0 && output_buffer != NULL);

	//TODO: Change the following code to invoke InvertSpatialWavelet

	if (prescale > 1)
	{
		// This is a spatial transform for the lowpass temporal band
		assert(prescale == 2);

		// Apply the inverse spatial transform for a lowpass band that is not prescaled
		InvertSpatialQuantDescale16s(allocator,
									 (PIXEL *)input->data[0], input->pitch,
									 (PIXEL *)input->data[1], input->pitch,
									 (PIXEL *)input->data[2], input->pitch,
									 (PIXEL *)input->data[3], input->pitch,
									 (PIXEL *)output_buffer, (int)output_pitch,
									 input_width, input_height,
									 output_width, output_height,
									 prescale, input->quant);
	}
	else
	{
		// This case does not handle any prescaling applied during encoding
		assert(prescale == 0);

		// Apply the inverse spatial transform for a lowpass band that is not prescaled
		InvertSpatialQuant16s(allocator,
							  (PIXEL *)input->data[0], input->pitch,
							  (PIXEL *)input->data[1], input->pitch,
							  (PIXEL *)input->data[2], input->pitch,
							  (PIXEL *)input->data[3], input->pitch,
							  (PIXEL *)output_buffer, (int)output_pitch,
							  input_width, input_height,
							  output_width, output_height,
							  input->quant);
	}

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Return a mask for the specified wavelet band

	The wavelet data structure contains a mask that indicates which
	bands have been decoded.
*/
uint32_t BandValidMask(int band)
{
	return (1 << band);
}

/*!
	@brief Check that all bands are valid

	The wavelet valid band mask is checked to determine whether
	all of the bands in the wavelet have been decoded.
*/
bool BandsAllValid(WAVELET *wavelet)
{
	uint32_t all_bands_valid_mask = ((1 << wavelet->band_count) - 1);
	return (wavelet->valid_band_mask == all_bands_valid_mask);
}

/*!
	@brief Set the bit for the specified band in the valid band mask
*/
CODEC_ERROR UpdateWaveletValidBandMask(WAVELET *wavelet, int band)
{
	if (0 <= band && band < MAX_BAND_COUNT)
	{
		// Update the valid wavelet band flags
		wavelet->valid_band_mask |= (1 << band);
		return CODEC_ERROR_OKAY;
	}
	return CODEC_ERROR_INVALID_BAND;
}

/*!
 
*/
CODEC_ERROR ResetWaveletValidBandMask(WAVELET *wavelet)
{
    wavelet->valid_band_mask = 0;
    return CODEC_ERROR_OKAY;
}

/*!
	@brief Compute the wavelet index from the subband index

	All subbands that are encoded into the bitstream, including the
	lowpass band at the highest wavelet level, are numbered in decode
	order starting with zero for the lowpass band.

	This routine maps the subband index to the index of the wavelet
	that contains the specified subband.

	Note the difference between a wavelet band and a subband: The bands in
	each wavelet are numbered starting at zero, while the subband index
	applies to all wavelet bands in the encoded sample and does not include
	the lowpass bands that are reconstructed during decoding from the bands
	that were encoded into the bitstream.
*/
int SubbandWaveletIndex(int subband)
{
	static int subband_wavelet_index[] = {2, 2, 2, 2, 1, 1, 1, 0, 0, 0};

	assert(0 <= subband && subband < MAX_SUBBAND_COUNT);

	// Return the index of the wavelet corresponding to this subband
	return subband_wavelet_index[subband];
}

/*!
	@brief Compute the index for the band in a wavelet from the subband index

	See the explanation of wavelet bands and subbands in the documentation for
	@ref SubbandWaveletIndex.
*/
int SubbandBandIndex(int subband)
{
	static int subband_band_index[] = {0, 1, 2, 3, 1, 2, 3, 1, 2, 3};

	assert(0 <= subband && subband < MAX_SUBBAND_COUNT);

	// Return the index to the band within the wavelet
	return subband_band_index[subband];
}


#if _DEBUG

/*!
	@brief Print the quantization vectors in the transform wavelets
*/
CODEC_ERROR PrintTransformQuantization(const TRANSFORM *transform, int wavelet_count, FILE *file)
{
	int k;

	for (k = 0; k < wavelet_count; k++)
	{
		WAVELET *wavelet = transform->wavelet[k];
		assert(wavelet != NULL);

	fprintf(file, "Wavelet quant: %d %d %d %d\n",
			wavelet->quant[0], wavelet->quant[1], wavelet->quant[2], wavelet->quant[3]);
	}

	return CODEC_ERROR_OKAY;
}

#endif
