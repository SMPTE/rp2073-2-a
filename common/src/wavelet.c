/*!	@file common/src/wavelet.c

	Implementation of the module for wavelet data structures and transforms

	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#include "headers.h"


/*!
	@brief Table of prescale values for the spatial wavelet transform

	Each prescale value is indexed by the wavelet level.  Index level zero
	corresponds to the input frame, the other prescale values correspond to
	wavelets in the transform: frame, spatial, spatial, ...

	Note that the prescale values depend on the encoded precision.  The default
	precale values are for 10-bit precision and the actual will depend on the
	encoded precision.
*/
const int spatial_prescale[] = {0, 2, 0, 0, 0, 0, 0, 0};


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
*/
CODEC_ERROR AllocWavelet(ALLOCATOR *allocator, WAVELET *wavelet, DIMENSION width, DIMENSION height)
{
	// Initialize the fields in the wavelet data structure
	InitWavelet(wavelet, width, height);

	if (width > 0 && height > 0)
	{
		int band;

		//TODO: Align the pitch?
		DIMENSION pitch = width * sizeof(PIXEL);

		size_t band_data_size = height * pitch;

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
	@brief Compute the amount of scaling for each band in the wavelet tree

	The forward wavelet transforms increase the number of bits required to
	represent the coefficients.  This routine computes the amount by which
	the input pixels are scaled as each band is computed.  The horizontal or
	vertical transform scales the lowpass values by one bit, but the hgihpass
	values are not scaled.  The lowpass wavelet band will be scaled by two
	bits, the first and second highpass bands will be scaled by one bit, and
	the third highpass band will not be scaled.
*/
CODEC_ERROR SetTransformScale(TRANSFORM *transform)
{
#if 0	//(CODEC_PROFILE > CODEC_PROFILE_BASELINE)
	int num_wavelets = transform->num_wavelets;
	int num_spatial = transform->num_spatial;
#else
	//int num_wavelets = 3;
	int num_spatial = 2;
#endif
	int num_lowpass_spatial;	// Number of spatial transforms for lowpass temporal band
	//int num_highpass_spatial;	// Number of spatial transforms for highpass temporal band
	int num_frame_wavelets;		// Number of frame wavelets at the base of the pyramid

	// Area of the temporal lowpass filter
	int temporal_lowpass_area = 2;

	// Area of the each wavelet filter
	int horizontal_lowpass_area = 2;
	int vertical_lowpass_area = 2;

	// Combination of the horizontal and vertical wavelet transforms
	int spatial_lowpass_area = (horizontal_lowpass_area * vertical_lowpass_area);

	int temporal_lowpass_scale;
	int temporal_highpass_scale;

	WAVELET *wavelet = NULL;
	//WAVELET *temporal;

	int k;
	int i;

	// Coefficients in each band are scaled by the forward wavelet filters
	int scale[4] = {1, 1, 1, 1};

#if 0
	//TODO: The baseline profile only supports the spatial transform
	TRANSFORM_TYPE transform_type = TRANSFORM_TYPE_SPATIAL;

	//switch (transform->type)
	switch (transform_type)
	{
	case TRANSFORM_TYPE_SPATIAL:
#endif
		// Compute the number of frame and spatial wavelets
		num_frame_wavelets = 1;
		num_lowpass_spatial = num_spatial;

		// Compute the change in scale due to the filters used in the frame transform
		temporal_lowpass_scale = temporal_lowpass_area * scale[0];
		temporal_highpass_scale = scale[0];

		// Compute the scale factors for the first wavelet
		scale[0] = horizontal_lowpass_area * temporal_lowpass_scale;
		scale[1] = temporal_lowpass_scale;
		scale[2] = horizontal_lowpass_area * temporal_highpass_scale;
		scale[3] = temporal_highpass_scale;

		for (k = 0; k < num_frame_wavelets; k++)
		{
			wavelet = transform->wavelet[k];
            assert(wavelet != NULL);
            if (! (wavelet != NULL)) {
                return CODEC_ERROR_UNEXPECTED;
            }

			wavelet->scale[0] = scale[0];
			wavelet->scale[1] = scale[1];
			wavelet->scale[2] = scale[2];
			wavelet->scale[3] = scale[3];
		}

		// Compute the scale factors for the spatial wavelets
		for (i = 0; i < num_lowpass_spatial; i++)
		{
			WAVELET *spatial = transform->wavelet[k++];
			//int k;

			assert(spatial != NULL);
            if (! (spatial != NULL)) {
                return CODEC_ERROR_UNEXPECTED;
            }
            
            assert(wavelet != NULL);
            if (! (wavelet != NULL)) {
                return CODEC_ERROR_UNEXPECTED;
            }

			// The lowpass band is the input to the spatial transform
			temporal_lowpass_scale = wavelet->scale[0];

			spatial->scale[0] = (spatial_lowpass_area * temporal_lowpass_scale);// >> _LOWPASS_PRESCALE;
			spatial->scale[1] = (vertical_lowpass_area * temporal_lowpass_scale);// >> _LOWPASS_PRESCALE;
			spatial->scale[2] = (horizontal_lowpass_area * temporal_lowpass_scale);// >> _LOWPASS_PRESCALE;
			spatial->scale[3] = (temporal_lowpass_scale);// >> _LOWPASS_PRESCALE;

			// The spatial wavelet is the input for the next level
			wavelet = spatial;
		}
		//break;

#if 0	//(CODEC_PROFILE > CODEC_PROFILE_BASELINE)
	case TRANSFORM_TYPE_FIELD:

		// Accumulate the scale factors due to filtering as the wavelet tree is computed
		for (k = 0; k < num_wavelets; k++)
		{
			i = 0;

			wavelet = transform->wavelet[k];

			// Adjust the scale of the pixel display range
			switch (wavelet->wavelet_type)
			{
			case WAVELET_TYPE_HORZTEMP:
				// The horizontal-temporal transforms do not require additional scaling
				break;

			case WAVELET_TYPE_TEMPORAL:
				// Temporal transforms use just one filter pass so need less scaling
				for (; i < 4; i++) scale[i] *= 2;
				break;

			case WAVELET_TYPE_SPATIAL:
				// Transforms that use two filter passes require more scaling
				for (; i < 4; i++)
				{
					scale[i] *= 4;
				}
				break;

			default:
				// Need to add scaling adjustments for other wavelet types
				assert(0);
				break;
			}

			// Save the scale factors in the wavelet data structure
			memcpy(wavelet->scale, scale, sizeof(scale));
		}
		break;
#endif
#if 0	//(CODEC_PROFILE > CODEC_PROFILE_BASELINE)
	case TRANSFORM_TYPE_FIELDPLUS:

		// Compute the number of frame and spatial wavelets
		num_frame_wavelets = 2;
		num_highpass_spatial = 1;
		num_lowpass_spatial = num_spatial - num_highpass_spatial;

		// Compute the change in scale due to the filters used in the frame transform
		temporal_lowpass_scale = temporal_lowpass_area * scale[0];
		temporal_highpass_scale = scale[0];

		// Compute the scale factors for the first two wavelets
		scale[0] = horizontal_lowpass_area * temporal_lowpass_scale;
		scale[1] = temporal_lowpass_scale;
		scale[2] = horizontal_lowpass_area * temporal_highpass_scale;
		scale[3] = temporal_highpass_scale;

		for (k = 0; k < num_frame_wavelets; k++)
		{
			wavelet = transform->wavelet[k];

			wavelet->scale[0] = scale[0];
			wavelet->scale[1] = scale[1];
			wavelet->scale[2] = scale[2];
			wavelet->scale[3] = scale[3];
		}

		// Compute the scale factors for the temporal wavelet between frames
		temporal = transform->wavelet[k++];

		temporal->scale[0] = temporal_lowpass_area * scale[0];
		temporal->scale[1] = scale[0];
		temporal->scale[2] = 0;
		temporal->scale[3] = 0;

		// The temporal highpass band is the input for the following chain of spatial transforms
		wavelet = temporal;
		temporal_highpass_scale = wavelet->scale[1];

		// Compute the scale factors for the spatial wavelets from the temporal highpass band
		for (i = 0; i < num_highpass_spatial; i++)
		{
			WAVELET *spatial = transform->wavelet[k++];

			assert(spatial != NULL);

			spatial->scale[0] = spatial_lowpass_area * temporal_highpass_scale;
			spatial->scale[1] = vertical_lowpass_area * temporal_highpass_scale;
			spatial->scale[2] = horizontal_lowpass_area * temporal_highpass_scale;
			spatial->scale[3] = temporal_highpass_scale;

			// The spatial wavelet is the input for the next level
			wavelet = spatial;

			// The lowpass output band is the input for the next spatial level
			temporal_highpass_scale = wavelet->scale[0];
		}

		// The temporal lowpass band is the input for the following chain of spatial transforms
		wavelet = temporal;

		// Compute the scale factors for the spatial wavelets from the temporal lowpass band
		for (i = 0; i < num_lowpass_spatial; i++)
		{
			WAVELET *spatial = transform->wavelet[k++];
			int k;

			assert(spatial != NULL);

			// The lowpass band is the input to the spatial transform
			temporal_lowpass_scale = wavelet->scale[0];

			spatial->scale[0] = (spatial_lowpass_area * temporal_lowpass_scale);// >> _LOWPASS_PRESCALE;
			spatial->scale[1] = (vertical_lowpass_area * temporal_lowpass_scale);// >> _LOWPASS_PRESCALE;
			spatial->scale[2] = (horizontal_lowpass_area * temporal_lowpass_scale);// >> _LOWPASS_PRESCALE;
			spatial->scale[3] = (temporal_lowpass_scale);// >> _LOWPASS_PRESCALE;

			// The spatial wavelet is the input for the next level
			wavelet = spatial;
		}
		break;
#endif
#if 0
	default:
		assert(0);
		break;
	}
#endif

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Calculate the amount of prescaling required to prevent overflow

	This routine calculates the arithmetic right shift that is applied to each
	lowpass band prior to computation of the wavelet transform.  Prescaling is
	required to prevent overflow in the wavelet transforms.
*/
CODEC_ERROR SetTransformPrescale(TRANSFORM *transform, int precision)
{
	if (precision == 8)
	{
		memset(transform->prescale, 0, sizeof(transform->prescale));
		return CODEC_ERROR_OKAY;
	}
	else if (precision == 10)
	{
		PRESCALE spatial_prescale[]  = {0, 2, 2, 0, 0, 0, 0, 0};
		memcpy(transform->prescale, spatial_prescale, sizeof(transform->prescale));
	}
	else if (precision == 12)
	{
		// frame, spatial, spatial, ...
		PRESCALE spatial_prescale[]  = {0, 2, 2, 0, 0, 0, 0, 0};
		memcpy(transform->prescale, spatial_prescale, sizeof(transform->prescale));
	}
	else
	{
		//TODO: Need to handle other precisions
		assert(0);
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
	@brief Compute the wavelet index from the subband index

	All subbands that are encoded into the bitstream, including the
	lowpass band at the highest wavelet level, are numbered in decode
	order starting with zero for the lowpass band.

	This routine maps the subband index to the index of the wavelet
	that contains the specified subband.

	Note the sifference between a wavelet band and a subband: The bands in
	each wavelet are numbered starting at zero, while the subband index
	applies to all wavelet bands in the encoded sample and does not include
	the lowpass bands that are reconstructed during decoding from the bands
	that were encoded into the bitstream.
*/
int SubbandWaveletIndex(int subband)
{
	//TODO: Adjust for other transform types and decoded resolutions
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
	//TODO: Adjust for other transform types and decoded resolutions
	static int subband_band_index[] = {0, 1, 2, 3, 1, 2, 3, 1, 2, 3};

	assert(0 <= subband && subband < MAX_SUBBAND_COUNT);

	// Return the index to the band within the wavelet
	return subband_band_index[subband];
}

/*!
	@brief Free the wavelets allocated for this transform
*/
CODEC_ERROR ReleaseTransform(ALLOCATOR *allocator, TRANSFORM *transform)
{
	int wavelet_index;

	for (wavelet_index = 0; wavelet_index < MAX_WAVELET_COUNT; wavelet_index++)
	{
		WAVELET *wavelet = transform->wavelet[wavelet_index];
		if (wavelet != NULL) {
			DeleteWavelet(allocator, wavelet);
			transform->wavelet[wavelet_index] = NULL;
		}
	}
	
	return CODEC_ERROR_OKAY;
}

/*!
	@brief Return true if the prescale table is the same as the default table

	This routine compares the prescale values used by the transform with the default
	table of prescale values.  If the actual prescale values are the same as the
	default values, then the table of prescale values do not have to be encoded
	into the bitstream.
*/
bool IsTransformPrescaleDefault(TRANSFORM *transform, int precision)
{
	int prescale_count = sizeof(transform->prescale) / sizeof(transform->prescale[0]);
	int total = 0;
	int i;

	if (precision == 8)
	{
		for (i = 0; i < prescale_count; i++) {
			total += transform->prescale[i];
		}
		return (total == 0);
	}
#if 0
	switch (transform_type)
	{
	case TRANSFORM_TYPE_SPATIAL:
#endif
		for (i = 0; i < prescale_count; i++) {
			total += abs(transform->prescale[i] - spatial_prescale[i]);
		}
		for(; i < MAX_PRESCALE_COUNT; i++) {
			total += spatial_prescale[i];			
		}
		return (total == 0);
		//break;

#if 0	//(CODEC_PROFILE > CODEC_PROFILE_BASELINE)
	case TRANSFORM_TYPE_FIELDPLUS:
		for (i = 0; i < prescale_count; i++) {
			total += abs(transform->prescale[i] - fieldplus_prescale[i]);
		}
		for(; i < MAX_PRESCALE_COUNT; i++) {
			total += spatial_prescale[i];			
		}
		return (total == 0);
		break;
#endif
#if 0
	default:
		assert(0);
		return false;
		break;
	}
#endif
}

PIXEL *WaveletRowAddress(WAVELET *wavelet, int band, int row)
{
	assert(wavelet != NULL);
	if (! (wavelet != NULL)) {
		return NULL;
	}

	assert(0 <= row && row < wavelet->height);
	if (! (0 <= row && row < wavelet->height))
	{
		return NULL;
	}
	else 
	{
		uint8_t *address = (uint8_t *)wavelet->data[band];
		address += row * wavelet->pitch;
		return (PIXEL *)address;
	}
}

/*!
	@brief Print the quantization vectors in the transform wavelets
*/
CODEC_ERROR PrintTransformQuantization(const TRANSFORM *transform, int wavelet_count, FILE *file)
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
