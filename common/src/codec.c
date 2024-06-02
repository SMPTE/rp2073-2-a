/*!	@file common/src/codec.c

	Implementation of functions that are common to the reference decoder and encoder

	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

//! Control debugging statements in this module
#ifndef DEBUG
#define DEBUG (1 && _DEBUG)
#endif

#include "headers.h"

/*!
	@brief Initialize the codec state to before encoding or decoding the bitstream

	Most of the codec state can be deduced from the decoding parameters.
	For example, the dimensions of the first wavelet band in the bitstream
	can be deduced from the encoded frame dimensions and the structure of
	the wavelet tree.

	The encoder will not insert parameters into the bitstream if the values
	of the parameters are the same as in the codec state.  This routine
	should initialize the codec state with correct values if those values
	can be inferred by the decoder, otherwise the use incorrect or default
	values.

	Note that the default encoded format is YUV 4:2:2, but this format is not
	supported by the baseline profile encoder so the encoded format must be
	explicitly written into the bitstream.

	@todo Add more default values required to properly initialize the codec state.
*/
CODEC_ERROR PrepareCodecState(CODEC_STATE *codec)
{
	// Initialize the channel to channel number zero
	codec->channel_number = 0;

	// Initialize the subband number to subband zero (the lowpass band)
	codec->subband_number = 0;

	// The number of subbands per channel is a constant
	codec->subband_count = 10;

	// The default precision of pixels in the input frame is ten bits
	codec->bits_per_component = 12;

	// Force the encoder to insert the encoded dimensions into the bitstream
	//codec->encoded.width = 0;
	//codec->encoded.height = 0;

	/*
		TODO: Do not have to encode the display dimensions into the bitstream if the decoder
		can infer the display dimensions from the encoded dimensions and format, but must
		encode the display dimensions if the encoded dimensions include padding.

		TODO: Check that this matches the current decoder implementation.
	*/

	// Set the default precision for encoding lowpass band coefficients
	codec->lowpass_precision = 16;
    
#if VC5_ENABLED_PART(VC5_PART_LAYERS)
    codec->layer_count = 0;
    codec->layer_number = 0;
    codec->layer_pattern = 0;
    codec->decoded_layer_mask = 0;
#endif

	//TODO: Set more default values in the codec state

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Reformat a segment value into the encoder version

	The version of the encoder that created the clip may be encoded into every sample.

	@todo Document the encoder version number format
*/
uint32_t EncoderVersion(uint32_t value)
{
	return (((value >> 12) & 0x0F) << 16) |
			(((value >> 8) & 0x0F) <<  8) |
			((value) & 0xFF);
}

/*!
	@brief Unpack the version tag value into its components
*/
void SetCodecVersion(uint8_t version[3], uint16_t value)
{
	version[0] = (uint8_t)((value >> 12) & 0x0F);		// Major version
	version[1] = (uint8_t)((value >>  8) & 0x0F);		// Minor version
	version[2] = (uint8_t)(value & 0xFF);				// Revision
}

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
/*!
	@brief Return true if the image format is valid
*/
bool ValidImageFormat(IMAGE_FORMAT image_format)
{
	if (0 < image_format && image_format < IMAGE_FORMAT_COUNT) {
		return true;
	}
	return false;
}
#endif

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
/*!
	@brief Set the image format using the pixel format
*/
IMAGE_FORMAT DefaultImageFormat(PIXEL_FORMAT pixel_format)
{
	IMAGE_FORMAT image_format = IMAGE_FORMAT_UNKNOWN;

	switch (pixel_format)
	{
	default:
		image_format = IMAGE_FORMAT_UNKNOWN;
		break;

	case PIXEL_FORMAT_B64A:
	case PIXEL_FORMAT_RG48:
		image_format = IMAGE_FORMAT_RGBA;
		break;

	case PIXEL_FORMAT_BYR4:
		image_format = IMAGE_FORMAT_BAYER;
		break;
            
    case PIXEL_FORMAT_NV12:
        image_format = IMAGE_FORMAT_YCbCrA;
        break;

	//TODO: Add other pixel formats
	}

	return image_format;
}
#endif

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
/*!
	@brief Return a string for the name of the image format
 */
const char *ImageFormatString(IMAGE_FORMAT image_format)
{
	char *name_string[] = {
			"Unknown",
			"RGB(A)",
			"YCbCr(A)",
			"Bayer",
			"CFA",
	};

	//if (0 <= image_format && image_format < IMAGE_FORMAT_COUNT) {
    if (image_format < IMAGE_FORMAT_COUNT) {
		return name_string[image_format];
	}
	return name_string[IMAGE_FORMAT_UNKNOWN];
}
#endif

/*!
	@brief Return the encoded precision for an input pixel format

	This routine computes the encoded precision that should be used
	for an input frame with the specified pixel format.

	This routine is not currently used since all input formats are
	encoded using 12 bits of precision.
*/
int EncodedPrecision(PIXEL_FORMAT format)
{
	// Use the default precision if the pixel format is unknown
	int precision = 12;

	switch (format)
	{
	case PIXEL_FORMAT_BYR3:
		precision = 10;
		break;

	case PIXEL_FORMAT_BYR4:
		precision = 12;
		break;

	case PIXEL_FORMAT_DPX0:
		precision = 10;
		break;

	default:
		assert(0);
		break;
	}

	return precision;
}

/*!
	@brief Return the precision of a pixel format

	This routine returns the actual precision of a pixel format.
*/
int InputPrecision(PIXEL_FORMAT format)
{
	// Return the default precision if the pixel format is unknown
	int precision = 8;

	switch (format)
	{
	case PIXEL_FORMAT_BYR3:
		precision = 10;
		break;

	case PIXEL_FORMAT_BYR4:
		precision = 16;
		break;

	case PIXEL_FORMAT_DPX0:
		precision = 10;
		break;

	case PIXEL_FORMAT_YUY2:
		precision = 8;
		break;

	case PIXEL_FORMAT_RG48:
    case PIXEL_FORMAT_B64A:
		precision = 16;
		break;
            
    case PIXEL_FORMAT_NV12:
        precision = 8;
        break;

	default:
		assert(0);
		break;
	}

	return precision;
}

/*!
	@brief Unpack the tag value into the prescale table

	The prescale table contains the prescale value for each wavelet in the
	transform.  The prescale value is a right shift that is applied to the
	input data before the wavelet is computed.

	The prescale table is used for all transforms and does not depend on the
	channel number.
*/
CODEC_ERROR UpdatePrescaleTable(CODEC_STATE *codec, TAGWORD value)
{
	int wavelet_index;

	for (wavelet_index = 0; wavelet_index < MAX_WAVELET_COUNT; wavelet_index++)
	{
		// Unpack the prescale value
		int prescale_value = (value >> (14 - wavelet_index * 2)) & 0x03;
		assert(0 <= prescale_value && prescale_value < UINT8_MAX);

		// Store the prescale value in the codec state
		codec->prescale_table[wavelet_index] = (uint_fast8_t)prescale_value;
	}

#if (0 && DEBUG)
	printf("Prescale table: %d %d %d\n\n", codec->prescale_table[0], codec->prescale_table[1], codec->prescale_table[2]);
#endif

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Update the flags that describe the frame structure

	The frame structure includes characteristics such as interlaced versus
	progressive and top or bottom field first.
*/
CODEC_ERROR UpdateFrameStructureFlags(CODEC_STATE *codec, TAGWORD value)
{
	codec->progressive = !(value & IMAGE_STRUCTURE_INTERLACED);
	codec->top_field_first = !(value & IMAGE_STRUCTURE_BOTTOM_FIELD_FIRST);
	codec->frame_inverted = (value & IMAGE_STRUCTURE_BOTTOM_ROW_FIRST);

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Initialize the codec state using the default constructor

	This routine is like a default constructor in C++ as it guarantees that
	the codec state is initialized to a know starting state with all pointers
	set to NULL and all counters set to zero.

	The routine @ref PrepareCodecState is used to set default values for the
	codec state prior to decoding a sample.
*/
CODEC_ERROR InitCodecState(CODEC_STATE *state)
{
	// Clear the codec state
	memset(state, 0, sizeof(CODEC_STATE));
	return CODEC_ERROR_OKAY;
}

/*!
	@brief Compute the channel offset added to each lowpass pixel value

	The channel offset is added to each pixel value when decoding the
	lowpass band.  It corrects for rounding errors that occur during
	encoding and depends on the output format since the rounding errors
	are not present at higher output bit depths.
*/
int LowpassChannelOffset(CODEC_STATE *codec, PIXEL_FORMAT output_format)
{
	int channel_offset = 0;

	if (codec->lowpass_precision == 16)
	{
		if (codec->bits_per_component == 8)
		{
			channel_offset = (codec->group_length == 2 ? 64 : 32);
		}
		else if (codec->bits_per_component == 10)
		{
			switch (output_format)
			{
			case PIXEL_FORMAT_YU64:
			case PIXEL_FORMAT_YR16:
			case PIXEL_FORMAT_V210:
				channel_offset = codec->group_length == 2 ? 14 : 4;
				break;

			default:
				channel_offset = codec->group_length == 2 ? 48 : 24;
			}
		}
		else if (codec->bits_per_component == 12)
		{
			switch(output_format)
			{
			case PIXEL_FORMAT_RGB24:
			case PIXEL_FORMAT_RGB32:
				channel_offset = 8;
				break;

			case PIXEL_FORMAT_RG48:		// 16-bit precision
			//case PIXEL_FORMAT_RG64:
			//case PIXEL_FORMAT_B64A:
			//case PIXEL_FORMAT_WP13:
			//case PIXEL_FORMAT_W13A:
				channel_offset = 0;
				break;

			//case PIXEL_FORMAT_RG30:
			//case PIXEL_FORMAT_R210:
			case PIXEL_FORMAT_DPX0:
			//case PIXEL_FORMAT_AR10:
			//case PIXEL_FORMAT_AB10:
				channel_offset = 6;
				break;	

			default:
				channel_offset = 0; 
				break;
			}
		}
#if 0
		if (codec->encoded.format == ENCODED_FORMAT_BAYER) {
			// Prevent offset between uncompressed and compressed RAW frames
			channel_offset = 0;
		}
#endif
	}

	return channel_offset;
}

/*!
	@brief Set the flags that determine the band coding

	There can be up to 15 different codebooks as specified by the lower
	four bigs in the band coding flags.  Use the default codebook if the
	active codebook is zero.

	The baseline profile does not allow difference coding or alternative
	codebooks.
*/
CODEC_ERROR SetBandCoding(CODEC_STATE *codec, TAGWORD value)
{
#if _DEBUG
	uint8_t active_codebook = (uint8_t)(value & 0x0F);
	bool difference_coding = !!((value >> 4) & 0x01);

	(void) codec;
	assert(active_codebook == 1);
	assert(!difference_coding);
#endif
	
	return CODEC_ERROR_OKAY;
}

/*!
	@brief Return true if the specified part is enabled at runtime

	This predicate is used to test whether a specific part in the VC-5 standard is
	enabled at runtime by this codec implementation.
*/
bool IsPartEnabled(ENABLED_PARTS enabled_parts, int part_number)
{
	return ((enabled_parts & VC5_PART_MASK(part_number)) != 0);
}


#if VC5_ENABLED_PART(VC5_PART_SECTIONS)

/*!
    @brief Return true if the specified type of section is enabled
*/
bool IsSectionEnabled(ENABLED_SECTIONS enabled_sections, SECTION_NUMBER section_number)
{
    if (SECTION_NUMBER_MINIMUM <= section_number && section_number <= SECTION_NUMBER_MAXIMUM)
    {
        uint32_t section_mask = SECTION_NUMBER_MASK(section_number);
        
        if (enabled_sections & section_mask) {
            return true;
        }
    }
    
    return false;
}

/*!
 @brief Return true if image sections are enabled
 */
bool IsImageSectionEnabled(ENABLED_PARTS enabled_parts, ENABLED_SECTIONS enabled_sections)
{
    return (IsPartEnabled(enabled_parts, VC5_PART_SECTIONS) &&
            IsSectionEnabled(enabled_sections, SECTION_NUMBER_IMAGE));
}

#endif
