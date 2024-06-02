/*! @file common/include/wavelet.h

	This file defines the data structures for the wavelet tree

	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _WAVELET_H
#define _WAVELET_H

/*!
	@brief Data structure used for wavelets

	This data structure is used for wavelets and can be used for images since
	an image with multiple planar channels and a wavelet with multiple bands
	are similar data structures.

	The pitch is the distance between rows in bytes and must always be
	an integer multiple of the pixel size in bytes.

	The wavelet data structure contains an array of the scale factor for
	each band that is the cummulative result of the application of the
	wavelet transforms that created the wavelet.
*/
typedef struct _wavelet
{
	DIMENSION width;					//!< Width of the image in pixels
	DIMENSION height;					//!< Height of the image in lines
	DIMENSION pitch;					//!< Distance between rows (in bytes)
	uint16_t band_count;				//!< Number of bands in a wavelet
	uint32_t valid_band_mask;			//!< Mask indicating which bands have been decoded
	uint16_t scale[MAX_BAND_COUNT];		//!< Cumulative scaling by the wavelet transforms
	QUANT quant[MAX_BAND_COUNT];		//!< Quantization value for each band
	PIXEL *data[MAX_BAND_COUNT];		//!< Data buffer for each band

} WAVELET;

//! Indices for the wavelet bands in the image data structure
typedef enum
{
	LL_BAND = 0,	//!< Lowpass transform of lowpass intermediate result
	LH_BAND,		//!< Lowpass transform of highpass intermediate result
	HL_BAND,		//!< Highpass transform of lowpass intermediate result
	HH_BAND			//!< Highpass transform of highpass intermediate result
} WAVELET_BAND;

//! Types of wavelet tranforms
enum
{
	WAVELET_TYPE_HORIZONTAL = 1,
	WAVELET_TYPE_VERTICAL = 2,
	WAVELET_TYPE_TEMPORAL = 4,

	//! The baseline profile only supports spatial wavelets
	WAVELET_TYPE_SPATIAL = (WAVELET_TYPE_HORIZONTAL | WAVELET_TYPE_VERTICAL),

};

#if 0
/*!
	@brief Enumeration for the types of transforms

	The baseline profile only supports spatial transform which is an
	I-frame only encoding with a simple tree of spatial wavelets.
*/
typedef enum transform_type
{
	TRANSFORM_TYPE_SPATIAL = 0,	// Transform does not use temporal wavelets

	TRANSFORM_TYPE_COUNT,		// Number of transform types

	// First transform type that has been implemented
	TRANSFORM_TYPE_FIRST = TRANSFORM_TYPE_SPATIAL,

	// Last transform type that has been implemented
	//TRANSFORM_TYPE_LAST = TRANSFORM_TYPE_FIELDPLUS
	TRANSFORM_TYPE_LAST = (TRANSFORM_TYPE_COUNT - 1)

} TRANSFORM_TYPE;
#endif

//! Data structure for the wavelet tree (one channel)
typedef struct _transform
{
	//! Prescale the input by the specified shift before the transform
	PRESCALE prescale[MAX_WAVELET_COUNT];

	//! List of the wavelets in the transform for one channel
	WAVELET *wavelet[MAX_WAVELET_COUNT];

} TRANSFORM;


#ifdef __cplusplus
extern "C" {
#endif

CODEC_ERROR AllocWavelet(ALLOCATOR *allocator, WAVELET *wavelet, DIMENSION width, DIMENSION height);
CODEC_ERROR ReleaseWavelet(ALLOCATOR *allocator, WAVELET *wavelet);

WAVELET *CreateWavelet(ALLOCATOR *allocator, DIMENSION width, DIMENSION height);
CODEC_ERROR DeleteWavelet(ALLOCATOR *allocator, WAVELET *wavelet);

CODEC_ERROR TransformInverseSpatialQuantLowpass(ALLOCATOR *allocator, WAVELET *input, WAVELET *output, uint16_t prescale);

CODEC_ERROR TransformInverseSpatialQuantArray(ALLOCATOR *allocator,
											  WAVELET *input,
											  COMPONENT_VALUE *output_buffer,
											  DIMENSION output_width,
											  DIMENSION output_height,
											  size_t output_pitch,
											  PRESCALE prescale);

CODEC_ERROR SetTransformScale(TRANSFORM *transform);

CODEC_ERROR SetTransformPrescale(TRANSFORM *transform, int precision);

uint32_t BandValidMask(int band);

bool BandsAllValid(WAVELET *wavelet);
#define AllBandsValid BandsAllValid

CODEC_ERROR UpdateWaveletValidBandMask(WAVELET *wavelet, int band);

CODEC_ERROR ResetWaveletValidBandMask(WAVELET *wavelet);

int SubbandWaveletIndex(int subband);

int SubbandBandIndex(int subband);

CODEC_ERROR ResetTransformFlags(TRANSFORM transform[], int transform_count);

CODEC_ERROR ReleaseTransform(ALLOCATOR *allocator, TRANSFORM *transform);

bool IsTransformPrescaleDefault(TRANSFORM *transform, int precision);

PIXEL *WaveletRowAddress(WAVELET *wavelet, int band, int row);

#if _DEBUG

// Print the prescaling used for the transform wavelets
CODEC_ERROR PrintTransformPrescale(TRANSFORM *transform, int wavelet_count, FILE *logfile);

// Print the quantization vectors in the transform wavelets
CODEC_ERROR PrintTransformQuantization(const TRANSFORM *transform, int wavelet_count, FILE *logfile);

#endif

#ifdef __cplusplus
}
#endif

#endif
