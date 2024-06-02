/*!	@file encoder/src/sections.c
 
	Implementation of code for encoding sections
 
	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
 */

#include "headers.h"

#if VC5_ENABLED_PART(VC5_PART_SECTIONS)

/*!
    @brief Return true if specified type of section is enabled
 */
bool IsEncoderSectionEnabled(ENCODER *encoder, SECTION_NUMBER section_number)
{
    if (IsPartEnabled(encoder->enabled_parts, VC5_PART_SECTIONS))
    {
        return IsSectionEnabled(encoder->enabled_sections, section_number);
    }

    // None of the predefined VC-5 sections are enabled
    return false;
}

/*
    @brief Start a new section with the specified tag
 
    The location of the tag-value pair that marks the beginning of the new section is
    pushed onto a stack so that the tag-value pair can be updated with the actual size
    of the section when the section is ended by a call to the @ref EndSection function.
 */
CODEC_ERROR BeginSection(BITSTREAM *bitstream, TAGWORD tag)
{
    return PushSampleSize(bitstream, tag);
}

/*
    @brief End a section
 
    Update the tag-value pair that marks the section with the actual size of the section.
 */
CODEC_ERROR EndSection(BITSTREAM *bitstream)
{
    return PopSampleSize(bitstream);
}

/*!
    @brief Write an image section header into the bitstream
 */
CODEC_ERROR BeginImageSection(struct _encoder *encoder, BITSTREAM *stream)
{
    return BeginSection(stream, CODEC_TAG_ImageSectionTag);
}

/*
    @brief Write a section header for the bitstream header into the bitstream
 */
CODEC_ERROR BeginHeaderSection(struct _encoder *encoder, BITSTREAM *stream)
{
    // Write the section header for the bitstream header into the bitstream
    return BeginSection(stream, CODEC_TAG_HeaderSectionTag);
}

/*
    @brief Write a layer section header into the bitstream
     
    Any codec state parameters that are required to decode the layer must be explicitly
    written into the bitstream so that the layer sections and be decoded concurrently.
 */
CODEC_ERROR BeginLayerSection(struct _encoder *encoder, BITSTREAM *stream)
{
    CODEC_ERROR error = CODEC_ERROR_OKAY;
    
    // Duplicate all codec state parameters required for decoding the layer
    PutCodecState(encoder, stream, SECTION_NUMBER_LAYER);
    
    // Write the section header for the layer into the bitstream
    error = BeginSection(stream, CODEC_TAG_LayerSectionTag);
    
    return error;
}

/*
    @brief Write a channel section header into the bitstream
 
    Any codec state parameters that are required to decode the channel must be explicitly
    written into the bitstream so that the channel sections and be decoded concurrently.
 */
CODEC_ERROR BeginChannelSection(ENCODER *encoder, BITSTREAM *stream)
{
    CODEC_ERROR error = CODEC_ERROR_OKAY;
    
    // Duplicate all codec state parameters required for decoding the channel
    PutCodecState(encoder, stream, SECTION_NUMBER_CHANNEL);
    
    // Write the section header for the channel into the bitstream
    error = BeginSection(stream, CODEC_TAG_ChannelSectionTag);
    
    return error;
}

/*
    @brief Write a wavelet section header into the bitstream
     
    Any codec state parameters that are required to decode the wavelet must be explicitly
    written into the bitstream so that the wavelet sections and be decoded concurrently.
 */
CODEC_ERROR BeginWaveletSection(struct _encoder *encoder, BITSTREAM *stream)
{
    CODEC_ERROR error = CODEC_ERROR_OKAY;
    
    // Duplicate all codec state parameters required for decoding the wavelet
    PutCodecState(encoder, stream, SECTION_NUMBER_WAVELET);
    
    // Write the section header for the wavelet into the bitstream
    error = BeginSection(stream, CODEC_TAG_WaveletSectionTag);
    
    return error;
}

/*
    @brief Write a subband section header into the bitstream
     
    Any codec state parameters that are required to decode the subband must be explicitly
    written into the bitstream so that the subband sections and be decoded concurrently.
 */
CODEC_ERROR BeginSubbandSection(struct _encoder *encoder, BITSTREAM *stream)
{
    CODEC_ERROR error = CODEC_ERROR_OKAY;
    
    // Duplicate all codec state parameters required for decoding the subband
    PutCodecState(encoder, stream, SECTION_NUMBER_SUBBAND);
    
    // Write the section header for the subband into the bitstream
    error = BeginSection(stream, CODEC_TAG_SubbandSectionTag);
    
    return error;
}

/*
    @brief Write codec state parameters used for decoding the section into the bitstream
 
    A section element may be decoded independently from other sections of the same type.
    Concurrent decoding implies that all codec state parameters needed to decode a section
    element be present in that section element.
 
    In principle, it is only necessary to write the codec state parameters that may be changed
    as other section elements are decoded independently.  This sample encoder takes the simple
    approach and writes all non-header codec state parameters into the bitstream.
 */
CODEC_ERROR PutCodecState(ENCODER *encoder, BITSTREAM *stream, SECTION_NUMBER section_number)
{
    CODEC_STATE *codec = &encoder->codec;
    TAGWORD prescale_shift = 0;

    switch (section_number)
    {
        case SECTION_NUMBER_IMAGE:
            assert(0);
            break;
            
        case SECTION_NUMBER_HEADER:
            // No codec state parameters to be written into the bitstream
            break;
            
        case SECTION_NUMBER_CHANNEL:
            // Encode the transform prescale for the first channel (assume all channels are the same)
            prescale_shift = PackTransformPrescale(&encoder->transform[0]);

            PutTagPair(stream, CODEC_TAG_ChannelNumber, codec->channel_number);
            PutTagPair(stream, CODEC_TAG_SubbandNumber, codec->subband_number);
            PutTagPair(stream, CODEC_TAG_LowpassPrecision, codec->lowpass_precision);
            PutTagPair(stream, CODEC_TAG_Quantization, codec->band.quantization);
            PutTagPair(stream, CODEC_TAG_PrescaleShift, prescale_shift);

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
            if (!IsPartEnabled(encoder->enabled_parts, VC5_PART_IMAGE_FORMATS))
            {
                PutTagPair(stream, CODEC_TAG_ChannelWidth, codec->channel_width);
                PutTagPair(stream, CODEC_TAG_ChannelHeight, codec->channel_height);
            }
#endif
#if VC5_ENABLED_PART(VC5_PART_LAYERS)
            if (IsPartEnabled(encoder->enabled_parts, VC5_PART_LAYERS))
            {
                PutTagPair(stream, CODEC_TAG_LayerNumber, codec->layer_number);
            }
#endif
            break;
            
        case SECTION_NUMBER_WAVELET:
            PutTagPair(stream, CODEC_TAG_ChannelNumber, codec->channel_number);
            PutTagPair(stream, CODEC_TAG_SubbandNumber, codec->subband_number);
            PutTagPair(stream, CODEC_TAG_LowpassPrecision, codec->lowpass_precision);
            //PutTagPair(stream, CODEC_TAG_Quantization, codec->band.quantization);
            //PutTagPair(stream, CODEC_TAG_PrescaleShift, prescale_shift);
            break;
            
        case SECTION_NUMBER_SUBBAND:
            PutTagPair(stream, CODEC_TAG_ChannelNumber, codec->channel_number);
            PutTagPair(stream, CODEC_TAG_SubbandNumber, codec->subband_number);
            PutTagPair(stream, CODEC_TAG_LowpassPrecision, codec->lowpass_precision);
            PutTagPair(stream, CODEC_TAG_Quantization, codec->band.quantization);
            //PutTagPair(stream, CODEC_TAG_PrescaleShift, prescale_shift);
            break;
            
        default:
            assert(0);
            return CODEC_ERROR_UNEXPECTED;
    }
    
    return CODEC_ERROR_OKAY;
}

/*
    @brief Read the list of images into an image list data structure
 */
CODEC_ERROR ReadInputPathnameList(IMAGE_LIST *image_list, const PATHNAME_LIST *input_pathname_list)
{
    int input_pathname_index;
    CODEC_ERROR error = CODEC_ERROR_OKAY;

#if (0 && DEBUG)
    PrintPathnameListInfo(input_pathname_list);
#endif
    
    assert(image_list != NULL);
    if (! (image_list != NULL)) {
        return CODEC_ERROR_BAD_ARGUMENT;
    }
    
    // Read each image into the corresponding position in the image list
    for (input_pathname_index = 0; input_pathname_index < input_pathname_list->pathname_count; input_pathname_index++)
    {
        //IMAGE *image = image_list->image_list[input_pathname_index];
        const char *pathname = input_pathname_list->pathname_data[input_pathname_index].pathname;
        
        FILE_INFO info;
        
        // Get the pixel format and precision of the input image
        GetFileInfo(pathname, &info);
        
        if (info.type == FILE_TYPE_RAW)
        {
            DIMENSION image_width = input_pathname_list->pathname_data[input_pathname_index].image_width;
            DIMENSION image_height = input_pathname_list->pathname_data[input_pathname_index].image_height;
            PIXEL_FORMAT pixel_format = input_pathname_list->pathname_data[input_pathname_index].pixel_format;

            // The caller must provide the image dimensions and pixel format
            assert(image_width > 0 && image_height > 0 && pixel_format != PIXEL_FORMAT_UNKNOWN);
            
            // Allocate space for the next image in the list
            AllocListImage(NULL, image_list, input_pathname_index, image_width, image_height, pixel_format);
        }
        else
        {
            // Must be a DPX image in which case the file header specifies the dimensions and format
            assert(info.type == FILE_TYPE_DPX);
            if (! (info.type == FILE_TYPE_DPX)) {
                return CODEC_ERROR_BAD_ARGUMENT;
            }
        }
        
        // Read the input image (the call may reallocate the image)
        error = ReadImage(image_list->image_list[input_pathname_index], pathname);
        if (error != CODEC_ERROR_OKAY) {
            fprintf(stderr, "Could not read input image: %s\n", pathname);
        }
    }
    
    return CODEC_ERROR_OKAY;
}

#endif
