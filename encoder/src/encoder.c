/*!	@file encoder/src/encoder.c

	Implementation of functions for encoding samples.

	Encoded samples must be aligned on a four byte boundary.
	Any constraints on the alignment of data within the sample
	are handled by padding the sample to the correct alignment.

	Note that the encoded dimensions are the actual dimensions of each channel
	(or the first channel in the case of 4:2:2 sampling) in the encoded sample.
	The display offsets and dimensions specify the portion of the encoded frame
	that should be displayed, but in the case of a Bayer image the display
	dimensions are doubled to account for the effects of the demosaic filter.
	If a Bayer image is encoded to Bayer format (no demosaic filter applied),
	then the encoded dimensions will be the same as grid of Bayer quads, less
	any padding required during encoding, but the display dimensions and
	offset will be reported as if a demosiac filter were applied to scale the
	encoded frame to the display dimensions (doubling the width and height).
	
	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef DEBUG
#define DEBUG (1 && _DEBUG)
#endif

#include "headers.h"
#include "bandfile.h"

#ifndef PATH_MAX
//! Maximum length of a pathname (in characters)
#define PATH_MAX 256
#endif


//! Number of rows of intermediate horizontal transform results
#define ROW_BUFFER_COUNT	6

#if 0   //VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
/*!
	@brief Set default values for the pattern element structure

	Some image formats imply specific parameters for the dimensions of the
	pattern elements and the number of components per sample.  If the pattern
	element structure has not been fully specified by the command-line
	arguments, then missing values can be filled in from the default values
	for the image format.
 */
bool SetImageFormatDefaults(ENCODER *encoder)
{
	switch (encoder->image_format)
	{
	case IMAGE_FORMAT_RGBA:
        if (encoder->pattern_width == 0) {
            encoder->pattern_width = 1;
        }
        
        if (encoder->pattern_height == 0) {
            encoder->pattern_height = 1;
        }

        // Set the default components per sample assuming no alpha channel
        if (encoder->components_per_sample == 0) {
            encoder->components_per_sample = 3;
        }
        
        return true;
            
            
	case IMAGE_FORMAT_YCbCrA:
		if (encoder->pattern_width == 0) {
			encoder->pattern_width = 1;
		}

		if (encoder->pattern_height == 0) {
			encoder->pattern_height = 1;
		}

#if VC5_ENABLED_PART(VC5_PART_COLOR_SAMPLING)
        if (IsPartEnabled(encoder->enabled_parts, VC5_PART_COLOR_SAMPLING))
        {
            // The components per sample parameter is not applicable to VC-5 Part 4 bitstreams
            assert(encoder->components_per_sample == 0);
            encoder->components_per_sample = 0;
        }
        else
        {
            // Set the default components per sample assuming no alpha channel
            if (encoder->components_per_sample == 0) {
                encoder->components_per_sample = 3;
            }
        }
#else
		// Set the default components per sample assuming no alpha channel
		if (encoder->components_per_sample == 0) {
			encoder->components_per_sample = 3;
		}
#endif
		return true;

	case IMAGE_FORMAT_BAYER:
		if (encoder->pattern_width == 0) {
			encoder->pattern_width = 2;
		}

		if (encoder->pattern_height == 0) {
			encoder->pattern_height = 2;
		}

		if (encoder->components_per_sample == 0) {
			encoder->components_per_sample = 1;
		}

		return true;

	default:
		// Unable to set default values for the pattern elements
		return false;
	}
}
#endif

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
/*!
	@brief Check for inconsistent values for the parameters specified on the command-line

	This routine looks for inconsistencies between the image format, the dimensions of the
	pattern elements, and the number of components per sample.
 */
bool CheckImageFormatParameters(ENCODER *encoder)
{
	switch (encoder->image_format)
	{
	case IMAGE_FORMAT_RGBA:
        if (encoder->pattern_width != 1) {
            return false;
        }
        
        if (encoder->pattern_height != 1) {
            return false;
        }
        
        if (! (3 <= encoder->components_per_sample && encoder->components_per_sample <= 4)) {
            return false;
        }
        
        // The parameters for the RGB(A) image format are correct
        
        return true;
        
	case IMAGE_FORMAT_YCbCrA:
#if VC5_ENABLED_PART(VC5_PART_COLOR_SAMPLING)
        if (IsPartEnabled(encoder->enabled_parts, VC5_PART_COLOR_SAMPLING))
        {
            assert(encoder->pattern_width > 0);
            if (! (encoder->pattern_width > 0)) {
                return false;
            }
            
            assert(encoder->pattern_height > 0);
            if (! (encoder->pattern_height > 0)) {
                return false;
            }
            
            assert(encoder->components_per_sample == 0);
            if (! (encoder->components_per_sample == 0)) {
                return false;
            }
            
            // The parameters for a YCbCr(A) image with subsampled color difference components are correct
        }
        else
        {
            if (encoder->pattern_width != 1) {
                return false;
            }
            
            if (encoder->pattern_height != 1) {
                return false;
            }
            
            if (! (3 <= encoder->components_per_sample && encoder->components_per_sample <= 4)) {
                return false;
            }
            
            // The parameters for the YCbCr(A) image format are correct
        }
#else
		if (encoder->pattern_width != 1) {
			return false;
		}

		if (encoder->pattern_height != 1) {
			return false;
		}

		if (! (3 <= encoder->components_per_sample && encoder->components_per_sample <= 4)) {
			return false;
		}

		// The parameters for the YCbCr(A) image format are correct
#endif
		return true;

	case IMAGE_FORMAT_BAYER:
		if (encoder->pattern_width != 2) {
			return false;
		}

		if (encoder->pattern_height != 2) {
			return false;
		}

		if (encoder->components_per_sample != 1) {
			return false;
		}

		// The parameters for the Bayer image format are correct
		return true;

	case IMAGE_FORMAT_CFA:
		if (! (encoder->pattern_width > 0)) {
			return false;
		}

		if (! (encoder->pattern_height > 0)) {
			return false;
		}
		if (encoder->components_per_sample != 1) {
			return false;
		}

		// The parameters for the CFA image format are correct
		return true;


	default:
		// Cannot verify the parameters for an unknown image format
		return false;

	}
}
#endif

/*!
	@brief Prepare the encoder state
 
    This routine works for single images and works for layers since each layer has the same
    number of channels and the same information per channel, but image sections do not have
    the same image dimensions and pixel format for each image.
 
    The values set using the first pathname will not work for image sections after the first
    image section, but the values set by this routine are overridden by a call to the routine
    @ref InitializeImageSectionEncoder from inside @ref PrepareEncoderImageSection immediately
    after this routine is called.
*/
CODEC_ERROR PrepareEncoderState(ENCODER *encoder,
								const UNPACKED_IMAGE *image,
								const PARAMETERS *parameters,
                                int input_image_index)
{
	CODEC_STATE *codec = &encoder->codec;
	int channel_count = image->component_count;
	int channel_number;
    
    PATHNAME_DATA *pathname_data;
    
    assert(parameters->input_pathname_list.pathname_count != 0);
    if (! (parameters->input_pathname_list.pathname_count != 0)) {
        // This routine should not be called if there are no input images to encode
        return CODEC_ERROR_UNEXPECTED;
    }
    
    // Use the parameter values for the current input pathname to initialize the encoder
    pathname_data = (PATHNAME_DATA *)&parameters->input_pathname_list.pathname_data[input_image_index];
   
	// Set the default value for the number of bits per lowpass coefficient
	PRECISION lowpass_precision = 16;
    
	if (parameters->lowpass_precision > 0) {
		lowpass_precision = parameters->lowpass_precision;
	}

	for (channel_number = 0; channel_number < channel_count; channel_number++)
	{
		DIMENSION width = image->component_array_list[channel_number].width;
		DIMENSION height = image->component_array_list[channel_number].height;
		PRECISION bits_per_component = image->component_array_list[channel_number].bits_per_component;

		// Copy the component array parameters into the encoder state
		encoder->channel[channel_number].width = width;
		encoder->channel[channel_number].height = height;
		encoder->channel[channel_number].bits_per_component = bits_per_component;

		// The lowpass bands in all channels are encoded with the same precision
		encoder->channel[channel_number].lowpass_precision = lowpass_precision;
	}

	// Record the number of channels in the encoder state
	encoder->channel_count = channel_count;

	// The encoder uses three wavelet transform levels for each channel
	encoder->wavelet_count = 3;

	// Set the channel encoding order
	if (parameters->channel_order_count > 0)
	{
		// Use the channel order specified by the encoding parameters
		encoder->channel_order_count = parameters->channel_order_count;
		memcpy(encoder->channel_order_table, parameters->channel_order_table, sizeof(encoder->channel_order_table));
	}
	else
	{
		// Use the default channel encoding order
		for (channel_number = 0; channel_number < channel_count; channel_number++)
		{
			encoder->channel_order_table[channel_number] = channel_number;
		}
		encoder->channel_order_count = channel_count;
	}

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
	encoder->image_width = pathname_data->image_width;
	encoder->image_height = pathname_data->image_height;
	encoder->pattern_width = pathname_data->pattern_width;
	encoder->pattern_height = pathname_data->pattern_height;
	encoder->components_per_sample = pathname_data->components_per_sample;
	encoder->image_format = pathname_data->image_format;
    encoder->max_bits_per_component = MaxBitsPerComponent(image);

	// Set default parameters for the image format
	//SetImageFormatDefaults(encoder);

	if (!CheckImageFormatParameters(encoder)) {
		return CODEC_ERROR_BAD_IMAGE_FORMAT;
	}
#else
	// The dimensions of the image is the maximum of the channel dimensions (VC-5 Part 1)
	GetMaximumChannelDimensions(image, &encoder->image_width, &encoder->image_height);
#endif

#if 0   //VC5_ENABLED_PART(VC5_PART_LAYERS)
    encoder->layer_count = parameters->layer_count;
    encoder->layer_flag = parameters->layer_flag;
#endif
    
#if VC5_ENABLED_PART(VC5_PART_SECTIONS)
    encoder->enabled_sections = parameters->enabled_sections;
#endif

	// Initialize the codec state with the default parameters used by the decoding process
	return PrepareCodecState(codec);
}

/*!
	@brief Initialize the encoder data structure

	This routine performs the same function as a C++ constructor.
	The encoder is initialized with default values that are replaced
	by the parameters used to prepare the encoder (see @ref PrepareEncoder).

	This routine does not perform all of the initializations required
	to prepare the encoder data structure for decoding a sample.
*/
CODEC_ERROR InitEncoder(ENCODER *encoder, const ALLOCATOR *allocator, const VERSION *version)
{
	assert(encoder != NULL);
	if (! (encoder != NULL)) {
		return CODEC_ERROR_NULLPTR;
	}

	memset(encoder, 0, sizeof(ENCODER));

	// Assign a memory allocator to the encoder
	encoder->allocator = (ALLOCATOR *)allocator;

	// Write debugging information to standard output
	encoder->logfile = stdout;

#if (1 && _DEBUG)
	// Buffer used to debug entropy coding of highpass bands
	encoder->encoded_band_bitstream = NULL;
#endif

	// Initialize the performance measurements
	InitTimer(&encoder->timing.transform);
	InitTimer(&encoder->timing.encoding);

	if (version)
	{
		// Store the version number in the encoder
		memcpy(&encoder->version, version, sizeof(encoder->version));
	}
	else
	{
		// Clear the version number in the encoder
		memset(&encoder->version, 0, sizeof(encoder->version));
	}

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Encode the image into the output stream

	This is a convenience routine for applications that use a byte stream to
	represent a memory buffer or binary file that will store the encoded image.

	The image is unpacked into a set of component arrays by the image unpacking
	process invoked by calling the routine @ref ImageUnpackingProcess.  The image
	unpacking process is informative and is not part of the VC-5 standard.

	The main entry point for encoding the component arrays output by the image
	unpacking process is @ref EncodingProcess.
*/
CODEC_ERROR EncodeImage(IMAGE *image, STREAM *stream, const PARAMETERS *parameters)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;

	// Allocate data structures for the encoder state and the bitstream
	ENCODER encoder;
	BITSTREAM bitstream;

#if (0 && DEBUG)
	int channel;
	uint32_t subband_mask;
	uint32_t channel_mask;
	uint32_t wavelet_mask;
	uint32_t wavelet_band_mask;
	const char *pathname = "C:/Users/bschunck/Temp/encoder1.dat";
#endif

	UNPACKED_IMAGE unpacked_image;
    
	// Unpack the image into a set of component arrays
	error = ImageUnpackingProcess(image, &unpacked_image, parameters, NULL);
	if (error != CODEC_ERROR_OKAY) {
		return error;
	}

#if (0 && DEBUG)
	// Write the unpacked component arrays to a file (for debugging)
	WriteUnpackedImage(&unpacked_image, image->format, parameters->enabled_parts, "input.dpx");
#endif

	// Initialize the bitstream data structure
	InitBitstream(&bitstream);

	// Bind the bitstream to the byte stream
	error = AttachBitstream(&bitstream, stream);
	if (error != CODEC_ERROR_OKAY) {
		return error;
	}

	// Encode the component arrays into the bitstream
	error = EncodingProcess(&encoder, &unpacked_image, &bitstream, parameters);
	if (error != CODEC_ERROR_OKAY) {
		return error;
	}

    error = ReleaseComponentArrays(NULL, &unpacked_image, unpacked_image.component_count);
    if (error != CODEC_ERROR_OKAY) {
        return error;
    }
    
	// Release any resources allocated by the bitstream
	ReleaseBitstream(&bitstream);

	// Release any resources allocated by the encoder
	ReleaseEncoder(&encoder);

	return error;
}

#if VC5_ENABLED_PART(VC5_PART_LAYERS) || VC5_ENABLED_PART(VC5_PART_SECTIONS)

/*!
    @brief Encode the list of images into the bitstream as layers or sections
 
    The layer flag in the encoding parameters determines whether the images are encoded as
    layers or sections.  If the images are encoded as layers, then each image must have the
    same dimensions and pixel format.
 
    @todo Finish handling the encoding of sections.
 */
CODEC_ERROR EncodeImageList(IMAGE_LIST *image_list, STREAM *stream, const PARAMETERS *parameters)
{
    CODEC_ERROR error = CODEC_ERROR_OKAY;
    
    // Allocate data structures for the encoder state and the bitstream
    ENCODER encoder;
    BITSTREAM bitstream;
    
#if (0 && DEBUG)
    int channel;
    uint32_t subband_mask;
    uint32_t channel_mask;
    uint32_t wavelet_mask;
    uint32_t wavelet_band_mask;
    const char *pathname = "C:/Users/bschunck/Temp/encoder1.dat";
#endif
    
    UNPACKED_IMAGE_LIST unpacked_image_list;
    
#if VC5_ENABLED_PART(VC5_PART_LAYERS)
    if (IsPartEnabled(parameters->enabled_parts, VC5_PART_LAYERS))
    {
        // Each layer image must have the same image dimensions and pixel format
        if (!CheckLayerImageList(image_list)) {
            return CODEC_ERROR_BAD_LAYER_IMAGE_LIST;
        };
    }
#endif

    // Initialize the list of unpacked images
    InitUnpackedImageList(&unpacked_image_list, image_list->image_count);
    
    // Unpack the list of packed images into a list of unpacked images (component array list)
    error = ImageListUnpackingProcess(image_list, &unpacked_image_list, parameters, NULL);
    if (error != CODEC_ERROR_OKAY) {
        return error;
    }
    
#if (0 && DEBUG)
    // Write the unpacked component arrays to a file (for debugging)
    WriteUnpackedImageList(&unpacked_image_list, parameters->enabled_parts, "input%04d.dpx");
#endif
    
    // Initialize the bitstream data structure
    InitBitstream(&bitstream);
    
    // Bind the bitstream to the byte stream
    error = AttachBitstream(&bitstream, stream);
    if (error != CODEC_ERROR_OKAY) {
        return error;
    }
    
    // Encode the component arrays into the bitstream
    error = ImageListEncodingProcess(&encoder, &unpacked_image_list, &bitstream, parameters);
    if (error != CODEC_ERROR_OKAY) {
        return error;
    }
    
    error = ReleaseUnpackedImageList(NULL, &unpacked_image_list);
    if (error != CODEC_ERROR_OKAY) {
        return error;
    }
    
    // Release any resources allocated by the bitstream
    ReleaseBitstream(&bitstream);
    
    // Release any resources allocated by the encoder
    ReleaseEncoder(&encoder);
    
    return error;
}

/*!
     @brief Apply the encoding process to each image in the unpacked image list
     
     The images are encoded as layers or image sections depending on whether layers or
    image sections are enabled.
 */
CODEC_ERROR ImageListEncodingProcess(ENCODER *encoder,
                                     UNPACKED_IMAGE_LIST *unpacked_image_list,
                                     BITSTREAM *bitstream,
                                     const PARAMETERS *parameters)
{
    CODEC_ERROR error = CODEC_ERROR_OKAY;
    
    assert(unpacked_image_list != NULL && unpacked_image_list->image_count > 0);
    if (! (unpacked_image_list != NULL && unpacked_image_list->image_count > 0)) {
        return CODEC_ERROR_UNEXPECTED;
    }

#if VC5_ENABLED_PART(VC5_PART_LAYERS)
    if (IsPartEnabled(parameters->enabled_parts, VC5_PART_LAYERS))
    {
        //TODO: Modkfy rhis routine to accept other allocators
        ALLOCATOR *allocator = NULL;
        
        // Each image in a list of unpacked images has the same dimensions and number of channels
        UNPACKED_IMAGE *image = unpacked_image_list->image_list[0];
        
        assert(image != NULL);
        if (! (image != NULL)) {
            return CODEC_ERROR_UNEXPECTED;
        }
        
        // Initialize the encoder using the parameters provided by the application
        error = PrepareEncoder(encoder, image, allocator, parameters, 0);
        assert(error == CODEC_ERROR_OKAY);
        if (! (error == CODEC_ERROR_OKAY)) {
            return error;
        }
        
#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
        if (encoder->image_format == IMAGE_FORMAT_UNKNOWN) {
            return CODEC_ERROR_BAD_IMAGE_FORMAT;
        }
        printf("Image format: %s\n", ImageFormatString(encoder->image_format));
        printf("Pattern width: %d\n", encoder->pattern_width);
        printf("Pattern height: %d\n", encoder->pattern_height);
        if (!IsPartEnabled(encoder->enabled_parts, VC5_PART_COLOR_SAMPLING)) {
            printf("Components per sample: %d\n", encoder->components_per_sample);
        }
        printf("Internal precision: %d\n", encoder->internal_precision);
        printf("\n");
#endif

        // Write the bitstream start marker
        PutBitstreamStartMarker(bitstream);

        // Encode each image as a separate layer in the bitstream
        error = EncodeImageLayers(encoder, unpacked_image_list, bitstream);
    }
    else
#endif
#if VC5_ENABLED_PART(VC5_PART_SECTIONS)
    if (IsSectionEnabled(parameters->enabled_sections, SECTION_NUMBER_IMAGE))
    {
        // Write the bitstream start marker
        PutBitstreamStartMarker(bitstream);
        
        // Encode each image as a separate image section in the bitstream
        error = EncodeImageSections(encoder, unpacked_image_list, bitstream, parameters);
    }
    else
#endif
    {
        // Should not reach this point in the code with both layers and sections not enabled
        assert(0);
        return CODEC_ERROR_UNEXPECTED;
    }

#if (0 && DEBUG)
    // Dump selected wavelet bands to a file (for debugging)
    (void)channel;
    (void)subband_mask;
    channel_mask = 0x01;
    wavelet_mask = 0x06;
    wavelet_band_mask = 0x0F;
    DumpTransformBands(&encoder, channel_mask, wavelet_mask, wavelet_band_mask, pathname);
#endif

#if _TIMING
    // Print out performance measurements before releasing the encoder
    printf("Transform time: %.3f ms\n", TimeMS(&encoder->timing.transform));
    printf("Entropy coding: %.3f ms\n", TimeMS(&encoder->timing.encoding));
#endif
    
    return error;
}

#endif

#if VC5_ENABLED_PART(VC5_PART_LAYERS)
/*!
    @brief Encode a list of unpacked images as layers
*/
CODEC_ERROR EncodeImageLayers(ENCODER *encoder, const UNPACKED_IMAGE_LIST *image_list, BITSTREAM *stream)
{
    CODEC_ERROR error = CODEC_ERROR_OKAY;
    int layer_count = encoder->layer_count;
    int layer_index;
    
    assert(layer_count == image_list->image_count);
    
    // Write the sample header that is common to all layers
    error = EncodeBitstreamHeader(encoder, stream);
    assert(error == CODEC_ERROR_OKAY);
    if (! (error == CODEC_ERROR_OKAY)) {
        return error;
    }
    
    // Write the sample extension header to the bitstream
    error = EncodeExtensionHeader(encoder, stream);
    assert(error == CODEC_ERROR_OKAY);
    if (! (error == CODEC_ERROR_OKAY)) {
        return error;
    }
    
    for (layer_index = 0; layer_index < layer_count; layer_index++)
    {
        UNPACKED_IMAGE *image = image_list->image_list[layer_index];
       
        // Write the layer header
        COUNT layer_number = layer_index;
        error = EncodeLayerHeader(encoder, stream, layer_number);
        assert(error == CODEC_ERROR_OKAY);
        if (! (error == CODEC_ERROR_OKAY)) {
            return error;
        }
        
        // Encode each component array as a separate channel in the bitstream
        error = EncodeMultipleChannels(encoder, image, stream);
        assert(error == CODEC_ERROR_OKAY);
        if (! (error == CODEC_ERROR_OKAY)) {
            return error;
        }
        
        // Write the layer trailer
        error = EncodeLayerTrailer(encoder, stream);
        assert(error == CODEC_ERROR_OKAY);
        if (! (error == CODEC_ERROR_OKAY)) {
            return error;
        }
    }

#if 0
    error = EncodeExtensionTrailer(encoder, stream);
    assert(error == CODEC_ERROR_OKAY);
    if (! (error == CODEC_ERROR_OKAY)) {
        return error;
    }
#endif
    
    // Finish the encoded sample after the last layer
    error = EncodeBitstreamTrailer(encoder, stream);
    assert(error == CODEC_ERROR_OKAY);
    if (! (error == CODEC_ERROR_OKAY)) {
        return error;
    }
    
    // Force any data remaining in the bitstream to be written into the sample
    FlushBitstream(stream);
    
    // Check that the sample offset stack has been emptied
    assert(stream->sample_offset_count == 0);
    
    //TODO: Any resources need to be released?
    
    return error;
}
#endif

#if VC5_ENABLED_PART(VC5_PART_SECTIONS)

/*!
	@brief Set the encoding parameters from the information in the pathname data for the encoded image
 */
CODEC_ERROR SetEncodingParameters(ENCODER *encoder, PATHNAME_DATA *pathname_data)
{
    encoder->image_width = pathname_data->image_width;
    encoder->image_height = pathname_data->image_height;
    
#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
    if (IsPartEnabled(encoder->enabled_parts, VC5_PART_IMAGE_FORMATS))
    {
        encoder->image_format = pathname_data->image_format;
        encoder->pattern_width = pathname_data->pattern_width;
        encoder->pattern_height = pathname_data->pattern_height;
        encoder->components_per_sample = pathname_data->components_per_sample;

#if 0
        switch (format)
        {
        case PIXEL_FORMAT_B64A:
            encoder->pattern_width = 1;
            encoder->pattern_height = 1;
            encoder->components_per_sample = 4;
            break;
            
        case PIXEL_FORMAT_RG48:
            encoder->pattern_width = 1;
            encoder->pattern_height = 1;
            encoder->components_per_sample = 3;
            break;
            
        case PIXEL_FORMAT_BYR4:
            encoder->pattern_width = 2;
            encoder->pattern_height = 2;
            encoder->components_per_sample = 1;
            break;
            
#if VC5_ENABLED_PART(VC5_PART_COLOR_SAMPLING)
        case PIXEL_FORMAT_NV12:
            encoder->pattern_width = 2;
            encoder->pattern_height = 2;
            encoder->components_per_sample = 0;      // Not applicable to images with subsampled color differences
            break;
#endif
        default:
            // Unable to set the encoding parameters from the image dimensions and pixel format
            assert(0);
            return CODEC_ERROR_BAD_IMAGE_FORMAT;
            break;
        }
#endif
    }
#endif
    
    return CODEC_ERROR_OKAY;
}

/*!
    @brief Initialize the encoding parameters for an image section
*/
CODEC_ERROR InitializeImageSectionEncoder(ENCODER *encoder, int section_index)
{
    CODEC_ERROR error = CODEC_ERROR_OKAY;
    PATHNAME_DATA *pathname_data;
    
    assert(encoder != NULL);
    
    assert(0 <= section_index && section_index < encoder->input_pathname_list.pathname_count);
    if (! (0 <= section_index && section_index < encoder->input_pathname_list.pathname_count)) {
        return CODEC_ERROR_UNEXPECTED;
    }
    
    pathname_data = &encoder->input_pathname_list.pathname_data[section_index];
    
//    width = pathname_data->image_width;
//    height = pathname_data->image_height;
//    format = pathname_data->pixel_format;
    
    error = SetEncodingParameters(encoder, pathname_data);
    assert(error == CODEC_ERROR_OKAY);
    if (! (error == CODEC_ERROR_OKAY)) {
        return error;
    }
    
//    encoder->image_width = width;
//    encoder->image_height = height;
    
    return CODEC_ERROR_OKAY;
}

/*!
 @brief Prepare the encoder to encoder the next image section
 */
CODEC_ERROR PrepareEncoderImageSection(ENCODER *encoder,
                                       const UNPACKED_IMAGE *image,
                                       int section_index,
                                       ALLOCATOR *allocator,
                                       const PARAMETERS *parameters)
{
    CODEC_ERROR error = CODEC_ERROR_OKAY;
    VERSION version = VERSION_INITIALIZER(VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION, 0);
    PRECISION max_bits_per_component = MaxBitsPerComponent(image);
    
#if VC5_ENABLED_PART(VC5_PART_COLOR_SAMPLING)
    int component_count = image->component_count;
#endif
    
    // Initialize the encoder data structure
    InitEncoder(encoder, allocator, &version);

    // Record the enabled parts and sections
    encoder->enabled_parts = parameters->enabled_parts;
    encoder->enabled_sections = parameters->enabled_sections;
    
    // Copy the pathname list into the encoding parameters
    memcpy(&encoder->input_pathname_list, &parameters->input_pathname_list, sizeof(encoder->input_pathname_list));
    
    // Set the internal precision used by the encoder for the wavelet transforms
    encoder->internal_precision = min(max_bits_per_component, default_internal_precision);
    
    // Initialize the encoding parameters and the codec state
    PrepareEncoderState(encoder, image, parameters, section_index);
    
    // Initialize the encoding parameters for this image section
    InitializeImageSectionEncoder(encoder, section_index);
    
    // Allocate the wavelet transforms
    AllocEncoderTransforms(encoder);
    
    // Initialize the quantizer
    SetEncoderQuantization(encoder, parameters);
    
    // Initialize the wavelet transforms
    PrepareEncoderTransforms(encoder);
    
    // Allocate the scratch buffers used for encoding
    AllocEncoderBuffers(encoder);
    
    // Initialize the encoding tables for magnitudes and runs of zeros
    error = PrepareCodebooks(allocator, &cs17);
    if (error != CODEC_ERROR_OKAY) {
        return error;
    }
    
    // Select the codebook for encoding
    encoder->codeset = &cs17;
    
#if (0 && DEBUG)
    if (encoder->logfile) {
        PrintQuantizer(&encoder->q, encoder->logfile);
    }
#endif
    
#if (1 && DEBUG)
    // Record the pixel format of the input image (for debugging)
    encoder->pixel_format = parameters->pixel_format;
#endif
    
#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
#if VC5_ENABLED_PART(VC5_PART_COLOR_SAMPLING)
    if (IsPartEnabled(encoder->enabled_parts, VC5_PART_COLOR_SAMPLING))
    {
        // The number of components per pattern element depends on the color difference component sampling
        component_count = encoder->pattern_width * encoder->pattern_width + 2;
    }
#endif
#if (0 && DEBUG)
    // Fill the unique image identifier and sequence number with known values for debugging
    SetUniqueImageIdentifierTesting(encoder);
#endif
#if (0 && DEBUG)
    // Fill the component transform with known values for debugging
    if (encoder->component_transform == NULL) {
        encoder->component_transform = Alloc(allocator, sizeof(COMPONENT_TRANSFORM));
    }
    InitComponentTransformTesting(encoder->component_transform, component_count, allocator);
#endif
#if (0 && DEBUG)
    // Fill the component permutation with known values for debugging
    if (encoder->component_permutation == NULL) {
        encoder->component_permutation = Alloc(allocator, sizeof(COMPONENT_PERMUTATION));
    }
    InitComponentPermutationTesting(encoder->component_permutation, component_count, allocator);
#endif
#endif
    
    // The encoder is ready to decode a sample
    return CODEC_ERROR_OKAY;
}

/*!
    @brief Encode a list of unpacked images as layers
*/
CODEC_ERROR EncodeImageSections(ENCODER *encoder,
                                const UNPACKED_IMAGE_LIST *image_list,
                                BITSTREAM *stream,
                                const PARAMETERS *parameters)
{
    CODEC_ERROR error = CODEC_ERROR_OKAY;
    ALLOCATOR *allocator = NULL;                    //TODO: Need to pass the allocator into this routine
    int section_count = image_list->image_count;
    int section_index;

    for (section_index = 0; section_index < section_count; section_index++)
    {
        UNPACKED_IMAGE *image = image_list->image_list[section_index];
        
        // Initialize the encoder using the parameters provided by the application
        error = PrepareEncoderImageSection(encoder, image, section_index, allocator, parameters);
        assert(error == CODEC_ERROR_OKAY);
        if (! (error == CODEC_ERROR_OKAY)) {
            return error;
        }

        // Write the image section header into the bitstream
        BeginImageSection(encoder, stream);
        
        // Write the bitstream header for each image section into the bitstream
        error = EncodeBitstreamHeader(encoder, stream);
        assert(error == CODEC_ERROR_OKAY);
        if (! (error == CODEC_ERROR_OKAY)) {
            return error;
        }
        
        // Write the extension header for each image section into the bitstream
        error = EncodeExtensionHeader(encoder, stream);
        assert(error == CODEC_ERROR_OKAY);
        if (! (error == CODEC_ERROR_OKAY)) {
            return error;
        }
       
        // Encode each component array as a separate channel in the bitstream
        error = EncodeMultipleChannels(encoder, image, stream);
        assert(error == CODEC_ERROR_OKAY);
        if (! (error == CODEC_ERROR_OKAY)) {
            return error;
        }
#if 0
        error = EncodeExtensionTrailer(encoder, stream);
        assert(error == CODEC_ERROR_OKAY);
        if (! (error == CODEC_ERROR_OKAY)) {
            return error;
        }
#endif
        // Make sure that the bitstream is aligned to a segment boundary
        AlignBitsSegment(stream);

        // Update the section header with the actual size of the image section
        EndSection(stream);
        
        // Prepare the encoder for the next image section?
        if (section_index < (section_count - 1))
        {
            // Free the memory allocated by the encoder
            ReleaseEncoderTransforms(encoder, allocator);
        }
    }
    
    // Finish the encoded bitstream after the last image section
    error = EncodeBitstreamTrailer(encoder, stream);
    assert(error == CODEC_ERROR_OKAY);
    if (! (error == CODEC_ERROR_OKAY)) {
        return error;
    }
    
    // Force any data remaining in the bitstream to be written into the sample
    FlushBitstream(stream);
    
    // Check that the sample offset stack has been emptied
    assert(stream->sample_offset_count == 0);
    
    //TODO: Any resources need to be released?
    
    return error;
}

#endif

#if VC5_ENABLED_PART(VC5_PART_LAYERS) && VC5_ENABLED_PART(VC5_PART_SECTIONS)

/*!
    @brief Encode the input images as image sections with nested layers
 
    This routine unpacks the list of input images and binds a bitstream to the byte stream
    passed as an argument.  Most of the encoding work is performed by @ref ImageSectionLayersEncodingProcess.
*/
CODEC_ERROR EncodeImageSectionLayers(IMAGE_LIST *image_list, STREAM *stream, const PARAMETERS *parameters)
{
    CODEC_ERROR error = CODEC_ERROR_OKAY;
    
    // Allocate data structures for the encoder state and the bitstream
    ENCODER encoder;
    BITSTREAM bitstream;
    
#if (0 && DEBUG)
    int channel;
    uint32_t subband_mask;
    uint32_t channel_mask;
    uint32_t wavelet_mask;
    uint32_t wavelet_band_mask;
    const char *pathname = "C:/Users/bschunck/Temp/encoder1.dat";
#endif
    
    UNPACKED_IMAGE_LIST unpacked_image_list;
    
    // Initialize the list of unpacked images
    InitUnpackedImageList(&unpacked_image_list, image_list->image_count);
    
    // Unpack the list of packed images into a list of unpacked images (component array list)
    error = ImageListUnpackingProcess(image_list, &unpacked_image_list, parameters, NULL);
    if (error != CODEC_ERROR_OKAY) {
        return error;
    }
    
#if (0 && DEBUG)
    // Write the unpacked component arrays to a file (for debugging)
    WriteUnpackedImageList(&unpacked_image_list, parameters->enabled_parts, "input%04d.dpx");
#endif
    
    // Initialize the bitstream data structure
    InitBitstream(&bitstream);
    
    // Bind the bitstream to the byte stream
    error = AttachBitstream(&bitstream, stream);
    if (error != CODEC_ERROR_OKAY) {
        return error;
    }
    
    // Encode the component arrays into the bitstream
    error = ImageSectionLayersEncodingProcess(&encoder, &unpacked_image_list, &bitstream, parameters);
    if (error != CODEC_ERROR_OKAY) {
        return error;
    }
    
    error = ReleaseUnpackedImageList(NULL, &unpacked_image_list);
    if (error != CODEC_ERROR_OKAY) {
        return error;
    }
    
    // Release any resources allocated by the bitstream
    ReleaseBitstream(&bitstream);
    
    // Release any resources allocated by the encoder
    ReleaseEncoder(&encoder);
    
    return error;
}

/*!
    @brief Apply the encoding process to each image in the unpacked image list
 
    The images are encoded as image sections with nested layers according to the pattern
    specified by the layers command-line option.
 
    This routine is only called with both layers and image sections are enabled.
 
    If the layers command-line option is not provided, then this routine encodes the input images
    as image sections with one image per section witout using layer syntax.
 
    This routine is an adaption of @ref ImageListEncodingProcess, @ref EncodeImageLayers, and @ref EncodeImageSections.
 */
CODEC_ERROR ImageSectionLayersEncodingProcess(ENCODER *encoder,
                                              UNPACKED_IMAGE_LIST *image_list,
                                              BITSTREAM *bitstream,
                                              const PARAMETERS *parameters)
{
    CODEC_ERROR error = CODEC_ERROR_OKAY;
    
    //TODO: Modkfy rhis routine to accept other allocators
    ALLOCATOR *allocator = NULL;

    int section_count;      //!< Number of image sections to encode into the bitstream
    int section_index;      //!< Index of the image section that is being encoded
    int layer_index;        //!< Index of the layer within the image section
    int image_index;        //!< Index of the input image
    int image_count;        //!< Number of images to encode as nested layers
    
    // Must provide a non-empty list of images to encode
    assert(image_list != NULL && image_list->image_count > 0);
    if (! (image_list != NULL && image_list->image_count > 0)) {
        return CODEC_ERROR_UNEXPECTED;
    }
    
    // This routine is only called when encopding image sections wth nexted layers
    assert(IsImageSectionEnabled(parameters->enabled_parts, parameters->enabled_sections) &&
           IsPartEnabled(parameters->enabled_parts, VC5_PART_LAYERS));
    if (! (IsImageSectionEnabled(parameters->enabled_parts, parameters->enabled_sections) &&
           IsPartEnabled(parameters->enabled_parts, VC5_PART_LAYERS))) {
        return CODEC_ERROR_UNEXPECTED;
    }
    
    // This routine is only called if the structure of sections and nested layers is provided on the commahd line
    section_count = parameters->image_section_count;
    assert(section_count > 0);
    if (! (section_count > 0)) {
        return CODEC_ERROR_UNEXPECTED;
    }
    
    // The number of images and the characteristics of each image are obtained from the commamnd-line arguments
    image_count = parameters->input_pathname_list.pathname_count;
    
    // Check for consistency between the command-line arguments and the unpacked images
    assert(image_count == image_list->image_count);
    if (! (image_count == image_list->image_count)) {
        return CODEC_ERROR_UNEXPECTED;
    }

    // Write the bitstream start marker
    PutBitstreamStartMarker(bitstream);
    
    for (section_index = 0, image_index = 0;
         section_index < section_count && image_index < image_count;
         section_index++)
    {
        // Initialize the number of layers nested within the current image section
        int layer_count = parameters->section_layer_count[section_index];

        // Save the image dimenskions and pixel format to check for consistency of each layer in an image section
        DIMENSION image_width = parameters->input_pathname_list.pathname_data[image_index].image_width;
        DIMENSION image_height = parameters->input_pathname_list.pathname_data[image_index].image_height;
        PIXEL_FORMAT pixel_format = parameters->input_pathname_list.pathname_data[image_index].pixel_format;
        
#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
        // Save the image format and pattern dimensions to check for consistency of each layer in an image section
        IMAGE_FORMAT image_format = parameters->input_pathname_list.pathname_data[image_index].image_format;
        DIMENSION pattern_width = parameters->input_pathname_list.pathname_data[image_index].pattern_width;
        DIMENSION pattern_height = parameters->input_pathname_list.pathname_data[image_index].pattern_height;
#endif
        
        // Initialize the encoder using the parameters provided by the application
        error = PrepareEncoder(encoder, image_list->image_list[image_index], allocator, parameters, image_index);
        assert(error == CODEC_ERROR_OKAY);
        if (! (error == CODEC_ERROR_OKAY)) {
            return error;
        }
        
        // Set the number of layers in this image section
        encoder->layer_count = layer_count;
        encoder->layer_flag = true;
        
        // Write the image section header into the bitstream
        BeginImageSection(encoder, bitstream);
        
        // Write the bitstream header that is common to all layers
        error = EncodeBitstreamHeader(encoder, bitstream);
        assert(error == CODEC_ERROR_OKAY);
        if (! (error == CODEC_ERROR_OKAY)) {
            return error;
        }
 
        // Write the extension header to the bitstream
        error = EncodeExtensionHeader(encoder, bitstream);
        assert(error == CODEC_ERROR_OKAY);
        if (! (error == CODEC_ERROR_OKAY)) {
            return error;
        }
        
        for (layer_index = 0; layer_index < layer_count; layer_index++, image_index++)
        {
            if (image_width != parameters->input_pathname_list.pathname_data[image_index].image_width ||
                image_height != parameters->input_pathname_list.pathname_data[image_index].image_height ||
                pixel_format != parameters->input_pathname_list.pathname_data[image_index].pixel_format
#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
                ||
                image_format != parameters->input_pathname_list.pathname_data[image_index].image_format ||
                pattern_width != parameters->input_pathname_list.pathname_data[image_index].pattern_width ||
                pattern_height != parameters->input_pathname_list.pathname_data[image_index].pattern_height
#endif
                )
            {
                // All images must have the same dimensions and format
                return CODEC_ERROR_BAD_LAYER_IMAGE_LIST;
            }

            UNPACKED_IMAGE *image = image_list->image_list[image_index];
            
            // Write the layer header
            COUNT layer_number = layer_index;
            error = EncodeLayerHeader(encoder, bitstream, layer_number);
            assert(error == CODEC_ERROR_OKAY);
            if (! (error == CODEC_ERROR_OKAY)) {
                return error;
            }
            
            // Encode each component array as a separate channel in the bitstream
            error = EncodeMultipleChannels(encoder, image, bitstream);
            assert(error == CODEC_ERROR_OKAY);
            if (! (error == CODEC_ERROR_OKAY)) {
                return error;
            }
            
            // Write the layer trailer
            error = EncodeLayerTrailer(encoder, bitstream);
            assert(error == CODEC_ERROR_OKAY);
            if (! (error == CODEC_ERROR_OKAY)) {
                return error;
            }
        }
        
        // Make sure that the bitstream is aligned to a segment boundary
        AlignBitsSegment(bitstream);
        
        // Update the section header with the actual size of the image section
        EndSection(bitstream);
        
        // Prepare the encoder for the next image section?
        if (section_index < (section_count - 1))
        {
            // Free the memory allocated by the encoder
            ReleaseEncoderTransforms(encoder, allocator);
        }
    }
    
#if 0
    error = EncodeExtensionTrailer(encoder, bitstream);
    assert(error == CODEC_ERROR_OKAY);
    if (! (error == CODEC_ERROR_OKAY)) {
        return error;
    }
#endif
    
    // Finish the bitstream after the last section
    error = EncodeBitstreamTrailer(encoder, bitstream);
    assert(error == CODEC_ERROR_OKAY);
    if (! (error == CODEC_ERROR_OKAY)) {
        return error;
    }
    
    // Force any data remaining in the bitstream to be written into the sample
    FlushBitstream(bitstream);
    
    // Check that the sample offset stack has been emptied
    assert(bitstream->sample_offset_count == 0);
    
    //TODO: Any resources need to be released?
    
    // Check that all input images were encoded into the bitstream as nested layers
    assert(image_index == image_count && section_index == section_count);
    
    
    return error;
}

#endif


/*!
	@brief Reference implementation of the VC-5 encoding process.

	The encoder takes input image in the form of a list of component arrays
	produced by the image unpacking process and encodes the image into the
	bitstream.

	External parameters are used to initialize the encoder state.

	The encoder state determines how the image is encoded int the bitstream.
*/
CODEC_ERROR EncodingProcess(ENCODER *encoder,
							const UNPACKED_IMAGE *image,
							BITSTREAM *bitstream,
							const PARAMETERS *parameters)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;
	ALLOCATOR *allocator = NULL;

	// Initialize the encoder using the parameters provided by the application
	error = PrepareEncoder(encoder, image, allocator, parameters, 0);
	assert(error == CODEC_ERROR_OKAY);
	if (! (error == CODEC_ERROR_OKAY)) {
		return error;
	}

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
	if (encoder->image_format == IMAGE_FORMAT_UNKNOWN) {
		return CODEC_ERROR_BAD_IMAGE_FORMAT;
	}

	if (parameters->verbose_flag)
	{
		printf("Image format: %s\n", ImageFormatString(encoder->image_format));
		printf("Pattern width: %d\n", encoder->pattern_width);
		printf("Pattern height: %d\n", encoder->pattern_height);
	    if (!IsPartEnabled(encoder->enabled_parts, VC5_PART_COLOR_SAMPLING)) {
	        printf("Components per sample: %d\n", encoder->components_per_sample);
	    }
	    printf("Internal precision: %d\n", encoder->internal_precision);
		printf("\n");

		//printf("Enabled parts: %X\n", encoder->enabled_parts);
		PrintEnabledParts(encoder->enabled_parts);
		printf("\n");
	}
#endif

#if VC5_ENABLED_PART(VC5_PART_LAYERS)
	if (IsPartEnabled(encoder->enabled_parts, VC5_PART_LAYERS) && encoder->layer_count > 1)
	{
        // This routine should not be called to encode multiple images as layers
        return CODEC_ERROR_UNEXPECTED;
    }
#endif

// #if VC5_ENABLED_PART(VC5_PART_METADATA)
// 	if (IsPartEnabled(encoder->enabled_parts, VC5_PART_METADATA)) {
// 		encoder->metadata_flag = true;
// 	}
// #endif

	// Write the bitstream start marker
	PutBitstreamStartMarker(bitstream);

    // Encode one image into the bitstream
	error = EncodeSingleImage(encoder, image, bitstream);
	assert(error == CODEC_ERROR_OKAY);
	if (! (error == CODEC_ERROR_OKAY)) {
		return error;
	}

#if VC5_ENABLED_PART(VC5_PART_METADATA)
	if (IsPartEnabled(encoder->enabled_parts, VC5_PART_METADATA))
	{
		// Write the metadata chunk after the encoded image but before the bitstream trailer
		error = EncodeMetadataChunk(encoder, bitstream, parameters);
		//assert(error == CODEC_ERROR_OKAY);
		if (! (error == CODEC_ERROR_OKAY)) {
			return error;
		}
	}
#endif

	// Finish the encoded bitstream
	error = EncodeBitstreamTrailer(encoder, bitstream);
	assert(error == CODEC_ERROR_OKAY);
	if (! (error == CODEC_ERROR_OKAY)) {
		return error;
	}

	// Force any data remaining in the bitstream to be written into the sample
	FlushBitstream(bitstream);

	// Check that the sample offset stack has been emptied
	assert(bitstream->sample_offset_count == 0);

#if (0 && DEBUG)
	// Dump selected wavelet bands to a file (for debugging)
	(void)channel;
	(void)subband_mask;
	channel_mask = 0x01;
	wavelet_mask = 0x06;
	wavelet_band_mask = 0x0F;
	DumpTransformBands(&encoder, channel_mask, wavelet_mask, wavelet_band_mask, pathname);
#endif
#if _TIMING
	// Print out performance measurements before releasing the encoder
	printf("Transform time: %.3f ms\n", TimeMS(&encoder->timing.transform));
	printf("Entropy coding: %.3f ms\n", TimeMS(&encoder->timing.encoding));
#endif

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Initialize the encoder using the specified parameters

	It is important to use the correct encoded image dimensions (including padding)
	and the correct encoded format to initialize the encoder.  The decoded image
	dimensions must be adjusted to account for a lower decoded resolution if applicable.

	It is expected that the parameters data structure may change over time
	with additional or different fields, depending on the codec profile or
	changes made to the codec during further development.  The parameters
	data structure may have a version number or may evolve into a dictionary
	of key-value pairs with missing keys indicating that a default value
	should be used.

	@todo Add more error checking to this top-level routine
*/
CODEC_ERROR PrepareEncoder(ENCODER *encoder,
						   const UNPACKED_IMAGE *image,
						   ALLOCATOR *allocator,
						   const PARAMETERS *parameters,
                           int input_image_index)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;
	VERSION version = VERSION_INITIALIZER(VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION, 0);
    PRECISION max_bits_per_component = MaxBitsPerComponent(image);
    
#if VC5_ENABLED_PART(VC5_PART_COLOR_SAMPLING)
    int component_count = image->component_count;
#endif
    
	// Initialize the encoder data structure
	InitEncoder(encoder, allocator, &version);

	// Set the mask that specifies which parts of the VC-5 standard are supported
	encoder->enabled_parts = parameters->enabled_parts;

	// Verify that the enabled parts are correct
	error = VerifyEnabledParts(encoder->enabled_parts);
	assert(error == CODEC_ERROR_OKAY);
	if (! (error == CODEC_ERROR_OKAY)) {
		return error;
	}
    
    encoder->image_format = parameters->image_format;
    if (encoder->image_format == IMAGE_FORMAT_UNKNOWN) {
        encoder->image_format = DefaultImageFormat(parameters->pixel_format);
    }
    
	// Set the internal precision used by the encoder for the wavelet transforms
	encoder->internal_precision = min(max_bits_per_component, default_internal_precision);

	// Initialize the encoding parameters and the codec state
	PrepareEncoderState(encoder, image, parameters, input_image_index);

	// Allocate the wavelet transforms
	AllocEncoderTransforms(encoder);

	// Initialize the quantizer
	SetEncoderQuantization(encoder, parameters);

	// Initialize the wavelet transforms
	PrepareEncoderTransforms(encoder);

	// Allocate the scratch buffers used for encoding
	AllocEncoderBuffers(encoder);

	// Initialize the encoding tables for magnitudes and runs of zeros
	error = PrepareCodebooks(allocator, &cs17);
	if (error != CODEC_ERROR_OKAY) {
		return error;
	}

	// Select the codebook for encoding
	encoder->codeset = &cs17;

#if (0 && DEBUG)
	if (encoder->logfile) {
		PrintQuantizer(&encoder->q, encoder->logfile);
	}
#endif

#if (1 && DEBUG)
	// Record the pixel format of the input image (for debugging)
	encoder->pixel_format = parameters->pixel_format;
#endif

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
#if VC5_ENABLED_PART(VC5_PART_COLOR_SAMPLING)
    if (IsPartEnabled(encoder->enabled_parts, VC5_PART_COLOR_SAMPLING))
    {
        // The number of components per pattern element depends on the color difference component sampling
        component_count = encoder->pattern_width * encoder->pattern_width + 2;
    }
#endif
#if (0 && DEBUG)
    // Fill the unique image identifier and sequence number with known values for debugging
    SetUniqueImageIdentifierTesting(encoder);
#endif
#if (0 && DEBUG)
    // Fill the component transform with known values for debugging
    if (encoder->component_transform == NULL) {
        encoder->component_transform = Alloc(allocator, sizeof(COMPONENT_TRANSFORM));
    }
    InitComponentTransformTesting(encoder->component_transform, component_count, allocator);
#endif
#if (0 && DEBUG)
    // Fill the component permutation with known values for debugging
    if (encoder->component_permutation == NULL) {
        encoder->component_permutation = Alloc(allocator, sizeof(COMPONENT_PERMUTATION));
    }
    InitComponentPermutationTesting(encoder->component_permutation, component_count, allocator);
#endif
#endif
    
#if VC5_ENABLED_PART(VC5_PART_LAYERS)
    if (IsPartEnabled(encoder->enabled_parts, VC5_PART_LAYERS))
    {
        // Encode each input image as a layer (even if there is only one input image)
        encoder->layer_count = parameters->input_pathname_list.pathname_count;
        encoder->layer_flag = true;
        
        //NOTE: The layer count includes all layers in each image section
    }
#endif

	// The encoder is ready to decode a sample
	return CODEC_ERROR_OKAY;
}

/*!
    @brief Free the wavelet transforms allocated for the encoder
*/
CODEC_ERROR ReleaseEncoderTransforms(ENCODER *encoder, ALLOCATOR *allocator)
{
    int channel;
    
    // Free the wavelet tree for each channel
    for (channel = 0; channel < MAX_CHANNEL_COUNT; channel++)
    {
        ReleaseTransform(allocator, &encoder->transform[channel]);
    }

    return CODEC_ERROR_OKAY;
}

/*!
	@brief Free all resources allocated by the encoder
*/
CODEC_ERROR ReleaseEncoder(ENCODER *encoder)
{
	if (encoder != NULL)
	{
		ALLOCATOR *allocator = encoder->allocator;

		// Free the encoding tables
		ReleaseCodebooks(allocator, encoder->codeset);

		// Free the wavelet tree for each channel
        ReleaseEncoderTransforms(encoder, allocator);

		//TODO: Free the encoding buffers
	}

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Encode a single image into the bitstream

	This is the main entry point for encoding a single image into the bitstream.
	The encoder must have been initialized by a call to @ref PrepareEncoder.

	The unpacked image is the set of component arrays output by the image unpacking
	process.  The bitstream must be initialized and bound to a byte stream before
	calling this routine.
*/
CODEC_ERROR EncodeSingleImage(ENCODER *encoder, const UNPACKED_IMAGE *image, BITSTREAM *stream)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;

	// Write the bitstream header into the bitstream
	error = EncodeBitstreamHeader(encoder, stream);
	assert(error == CODEC_ERROR_OKAY);
	if (! (error == CODEC_ERROR_OKAY)) {
		return error;
	}

	// Write the bitstream extension header into the bitstream
	error = EncodeExtensionHeader(encoder, stream);
	assert(error == CODEC_ERROR_OKAY);
	if (! (error == CODEC_ERROR_OKAY)) {
		return error;
	}

	// Encode each component array as a separate channel in the bitstream
	error = EncodeMultipleChannels(encoder, image, stream);
	assert(error == CODEC_ERROR_OKAY);
	if (! (error == CODEC_ERROR_OKAY)) {
		return error;
	}

#if 0
	error = EncodeExtensionTrailer(encoder, stream);
	assert(error == CODEC_ERROR_OKAY);
	if (! (error == CODEC_ERROR_OKAY)) {
		return error;
	}
#endif

	//TODO: Any resources need to be released?

	return error;
}

#if 0   //VC5_ENABLED_PART(VC5_PART_LAYERS)
/*!
	@brief Encode multiple frames as separate layers in a sample

	The encoder must have been initialized by a call to @ref PrepareEncoder.

	The bitstream must be initialized and bound to a byte stream before
	calling this routine.
*/
CODEC_ERROR EncodeMultipleFrames(ENCODER *encoder, IMAGE *image_array[], int frame_count, BITSTREAM *stream)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;
	//CODEC_STATE *codec = &encoder->codec;

	int layer_index;

	// The number of frames must match the number of layers in the sample
	assert(frame_count == encoder->layer_count);

	// Initialize the codec state
	PrepareEncoderState(encoder);

	// Write the bitstream start marker
	error = PutBitstreamStartMarker(stream);
	assert(error == CODEC_ERROR_OKAY);
	if (! (error == CODEC_ERROR_OKAY)) {
		return error;
	}

	// Write the bitstream header that is common to all layers
	error = EncodeBitstreamHeader(encoder, stream);
	assert(error == CODEC_ERROR_OKAY);
	if (! (error == CODEC_ERROR_OKAY)) {
		return error;
	}

	// Write the extension header to the bitstream
	error = EncodeExtensionHeader(encoder, stream);
	assert(error == CODEC_ERROR_OKAY);
	if (! (error == CODEC_ERROR_OKAY)) {
		return error;
	}

	// Encode each frame in a separate layer in the sample
	for (layer_index = 0; layer_index < frame_count; layer_index++)
	{
		error = EncodeLayer(encoder, image_array[layer_index]->buffer, image_array[layer_index]->pitch, stream);
		assert(error == CODEC_ERROR_OKAY);
		if (! (error == CODEC_ERROR_OKAY)) {
			return error;
		}
	}

	error = EncodeSampleExtensionTrailer(encoder, stream);
	assert(error == CODEC_ERROR_OKAY);
	if (! (error == CODEC_ERROR_OKAY)) {
		return error;
	}

	// Finish the encoded sample after the last layer
	error = EncodeSampleTrailer(encoder, stream);
	assert(error == CODEC_ERROR_OKAY);
	if (! (error == CODEC_ERROR_OKAY)) {
		return error;
	}

	// Force any data remaining in the bitstream to be written into the sample
	FlushBitstream(stream);

	// Check that the sample offset stack has been emptied
	assert(stream->sample_offset_count == 0);

	//TODO: Any resources need to be released?

	// Done encoding all layers in the sample
	return error;
}
#endif

/*!
	@brief Initialize the wavelet transforms for encoding
*/
CODEC_ERROR PrepareEncoderTransforms(ENCODER *encoder)
{
	//int channel_count = encoder->channel_count;
	int channel_number;

	// Set the prescale and quantization in each wavelet transform
	for (channel_number = 0; channel_number < encoder->channel_count; channel_number++)
	{
		TRANSFORM *transform = &encoder->transform[channel_number];

		// Set the prescaling (may be used in setting the quantization)
		int bits_per_component = encoder->channel[channel_number].bits_per_component;
		SetTransformPrescale(transform, bits_per_component);

		//TODO: Are the wavelet scale factors still used?

		// Must set the transform scale if not calling SetTransformQuantization
		SetTransformScale(transform);
	}

#if (0 && DEBUG)
	if (encoder->logfile)
	{
		FILE *logfile = encoder->logfile;

		fprintf(logfile, "Midpoint prequant parameter: %d\n\n", encoder->midpoint_prequant);

		for (channel_number = 0; channel_number < encoder->channel_count; channel_number++)
		{
			const int wavelet_count = 3;

			fprintf(logfile, "Prescale for channel: %d\n", channel_number);
			PrintTransformPrescale(&encoder->transform[channel_number], wavelet_count, logfile);
			fprintf(logfile, "\n");

			fprintf(logfile, "Quantization for channel: %d\n", channel_number);
			PrintTransformQuantization(&encoder->transform[channel_number], wavelet_count, logfile);
			fprintf(logfile, "\n");
		}
	}
#endif

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Unpack the image into component arrays for encoding
*/
CODEC_ERROR ImageUnpackingProcess(const PACKED_IMAGE *input,
								  UNPACKED_IMAGE *output,
								  const PARAMETERS *parameters,
								  ALLOCATOR *allocator)
{
	ENABLED_PARTS enabled_parts = parameters->enabled_parts;
	int channel_count;
	DIMENSION max_channel_width;
	DIMENSION max_channel_height;
	int bits_per_component;

	// The configuration of component arrays is determined by the image format
	switch (input->format)
	{
	case PIXEL_FORMAT_BYR4:
		channel_count = 4;
		max_channel_width = input->width / 2;
		max_channel_height = input->height / 2;
		bits_per_component = 12;
		break;

	case PIXEL_FORMAT_RG48:
		channel_count = 3;
		max_channel_width = input->width;
		max_channel_height = input->height;
		bits_per_component = 12;
		break;

	case PIXEL_FORMAT_DPX0:
		channel_count = 3;
		max_channel_width = input->width;
		max_channel_height = input->height;
		//bits_per_component = 10;
		bits_per_component = 12;
		break;

	case PIXEL_FORMAT_B64A:
		channel_count = 4;
		max_channel_width = input->width;
		max_channel_height = input->height;
		bits_per_component = 12;
		break;

	case PIXEL_FORMAT_NV12:
		channel_count = 3;
		max_channel_width = input->width;
		max_channel_height = input->height;
		//bits_per_component = 8;
        bits_per_component = 12;
		break;

	default:
		assert(0);
		return CODEC_ERROR_PIXEL_FORMAT;
		break;
	}

	// Allocate space for the component arrays
	AllocateComponentArrays(allocator, output, channel_count, max_channel_width, max_channel_height,
		input->format, bits_per_component);

	// Unpack the image into component arrays
	UnpackImage(input, output, enabled_parts);

	return CODEC_ERROR_OKAY;
}


#if VC5_ENABLED_PART(VC5_PART_LAYERS) || VC5_ENABLED_PART(VC5_PART_SECTIONS)

/*
    @brief Unpack each image in the list into an unpacked image (component array list)
 */
CODEC_ERROR ImageListUnpackingProcess(PACKED_IMAGE_LIST *packed_image_list,
                                      UNPACKED_IMAGE_LIST *unpacked_image_list,
                                      const PARAMETERS *parameters,
                                      ALLOCATOR *allocator)
{
    CODEC_ERROR error = CODEC_ERROR_OKAY;
    int image_index;
    
    assert(packed_image_list != NULL);
    assert(unpacked_image_list != NULL);
    
    for (image_index = 0; image_index < packed_image_list->image_count; image_index++)
    {
        PACKED_IMAGE *packed_image = packed_image_list->image_list[image_index];
        UNPACKED_IMAGE *unpacked_image = unpacked_image_list->image_list[image_index];
        
        assert(packed_image != NULL);
        
        if (unpacked_image != NULL)
        {
            // Free the component arrays without deallocating the unpacked image data structure
            ReleaseComponentArrays(allocator, unpacked_image, unpacked_image->component_count);
        }
        else
        {
            // Allocate the data structure for this unpacked image
            unpacked_image = Alloc(allocator, sizeof(UNPACKED_IMAGE));
            unpacked_image_list->image_list[image_index] = unpacked_image;
        }

        assert(unpacked_image != NULL);
        
        // Unpacked the image into component arrays
        error = ImageUnpackingProcess(packed_image, unpacked_image, parameters, allocator);
        if (error != CODEC_ERROR_OKAY) {
            break;
        }
    }
    
    return error;
}

#endif


/*!
	@brief Unpack the image into component arrays
*/
CODEC_ERROR UnpackImage(const PACKED_IMAGE *input, UNPACKED_IMAGE *output, ENABLED_PARTS enabled_parts)
{
    CODEC_ERROR codec_error = CODEC_ERROR_OKAY;
	uint8_t *input_buffer = (uint8_t *)input->buffer + input->offset;
	DIMENSION input_width = input->width;
	DIMENSION input_height = input->height;
	size_t input_pitch = input->pitch;
	int row;
    
    // Handle the NV12 image format as a special case
    if (input->format == PIXEL_FORMAT_NV12)
    {
        PIXEL *output_buffer_list[3];
        int component_index;
        
        assert(output->component_count == 3);
        if (! (output->component_count == 3)) {
            return CODEC_ERROR_UNEXPECTED;
        }
        
        // Initialize an array of pointers to the buffers in the output image
        for (component_index = 0; component_index < output->component_count; component_index++)
        {
            output_buffer_list[component_index] = (PIXEL *)output->component_array_list[component_index].data;
        }
        
        codec_error = UnpackImageNV12(input_buffer, input_width, input_height, output_buffer_list);
        
        return codec_error;
    }

	if (IsBayerFormat(input->format))
	{
		// Adjust the image dimensions to match the grid of Bayer pixels
		input_width /= 2;
		input_height /= 2;
		input_pitch *= 2;
	}

	for (row = 0; row < input_height; row++)
	{
		uint8_t *input_row_ptr = input_buffer + row * input_pitch;
		PIXEL *output_row_ptr_array[MAX_CHANNEL_COUNT];
		PRECISION bits_per_component_array[MAX_CHANNEL_COUNT];
		int channel_count = output->component_count;
		int channel_number;

		for (channel_number = 0; channel_number < channel_count; channel_number++)
		{
			uint8_t *buffer = (uint8_t *)output->component_array_list[channel_number].data;
			size_t pitch = output->component_array_list[channel_number].pitch;
			PRECISION bits_per_component = output->component_array_list[channel_number].bits_per_component;
			output_row_ptr_array[channel_number] = (PIXEL *)(buffer + row * pitch);
			bits_per_component_array[channel_number] = bits_per_component;
		}

		codec_error = UnpackImageRow(input_row_ptr, input_width, input->format,
                                     output_row_ptr_array, bits_per_component_array,
                                     channel_count, enabled_parts);

        if (codec_error != CODEC_ERROR_OKAY) {
            break;
        }
	}

	return codec_error;
}

CODEC_ERROR UnpackImageRow(uint8_t *input_row_ptr,
						   DIMENSION image_width,
						   PIXEL_FORMAT pixel_format,
						   PIXEL *output_row_ptr[],
						   PRECISION bits_per_component[],
						   int channel_count,
						   ENABLED_PARTS enabled_parts)
{
	switch (pixel_format)
	{
	case PIXEL_FORMAT_BYR3:
		return UnpackImageRowBYR3(input_row_ptr, image_width, output_row_ptr,
			bits_per_component, channel_count, enabled_parts);
		break;

	case PIXEL_FORMAT_BYR4:
		return UnpackImageRowBYR4(input_row_ptr, image_width, output_row_ptr,
			bits_per_component, channel_count, enabled_parts);
		break;

	case PIXEL_FORMAT_DPX0:
		return UnpackImageRowDPX0(input_row_ptr, image_width, output_row_ptr,
			bits_per_component, channel_count, enabled_parts);
		break;

	case PIXEL_FORMAT_YUY2:
		return UnpackImageRowYUY2(input_row_ptr, image_width, output_row_ptr,
			bits_per_component, channel_count, enabled_parts);
		break;

	case PIXEL_FORMAT_RG48:
		return UnpackImageRowRG48(input_row_ptr, image_width, output_row_ptr,
			bits_per_component, channel_count, enabled_parts);
		break;

	case PIXEL_FORMAT_B64A:
		return UnpackImageRowB64A(input_row_ptr, image_width, output_row_ptr,
			bits_per_component, channel_count, enabled_parts);
		break;
            
#if 0   //VC5_ENABLED_PART(VC5_PART_COLOR_SAMPLING)
    case PIXEL_FORMAT_NV12:
        return UnpackImageRowNV12(input_row_ptr, image_width, output_row_ptr,
            bits_per_component, channel_count, enabled_parts);
        break;
#endif

	default:
		assert(0);
		break;
	}

	// Cannot unpack the spsecified pixel format
	return CODEC_ERROR_PIXEL_FORMAT;
}

/*!
	@brief Insert the header segments that are common to all samples

	This code was derived from PutVideoIntraFrameHeader in the current codec.

	@todo Need to output the channel size table.
*/
CODEC_ERROR EncodeBitstreamHeader(ENCODER *encoder, BITSTREAM *stream)
{
	CODEC_STATE *codec = &encoder->codec;

	//TAGWORD subband_count = 10;
	TAGWORD image_width = encoder->image_width;
	TAGWORD image_height = encoder->image_height;

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
	TAGWORD image_format = encoder->image_format;
	TAGWORD pattern_width = encoder->pattern_width;
	TAGWORD pattern_height = encoder->pattern_height;
	TAGWORD components_per_sample = encoder->components_per_sample;
	TAGWORD max_bits_per_component = encoder->max_bits_per_component;
	//TAGWORD default_bits_per_component = max_bits_per_component;
#endif

	// Align the start of the header on a segment boundary
	AlignBitsSegment(stream);

	// The bitstream should be aligned to a segment boundary
	assert(IsAlignedSegment(stream));

#if VC5_ENABLED_PART(VC5_PART_SECTIONS)
    if (IsEncoderSectionEnabled(encoder, SECTION_NUMBER_HEADER))
    {
        // Write the section header for the bitstream header into the bitstream
        BeginHeaderSection(encoder, stream);
    }
#endif

	// Output the number of channels
	if (encoder->channel_count != codec->channel_count) {
		PutTagPair(stream, CODEC_TAG_ChannelCount, encoder->channel_count);
		codec->channel_count = encoder->channel_count;
	}

	// Inform the decoder of the maximum component array dimensions
	PutTagPair(stream, CODEC_TAG_ImageWidth, image_width);
	PutTagPair(stream, CODEC_TAG_ImageHeight, image_height);

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
	if (IsPartEnabled(encoder->enabled_parts, VC5_PART_IMAGE_FORMATS))
	{
		PutTagPair(stream, CODEC_TAG_ImageFormat, image_format);
		PutTagPair(stream, CODEC_TAG_PatternWidth, pattern_width);
		PutTagPair(stream, CODEC_TAG_PatternHeight, pattern_height);
		PutTagPair(stream, CODEC_TAG_ComponentsPerSample, components_per_sample);
		PutTagPair(stream, CODEC_TAG_MaxBitsPerComponent, max_bits_per_component);
	}
#endif

#if VC5_ENABLED_PART(VC5_PART_LAYERS)
	if (IsPartEnabled(encoder->enabled_parts, VC5_PART_LAYERS))
	{
		// Output the number of layers in the sample (optional for backward compatibility)
		//PutTagPairOptional(stream, CODEC_TAG_LAYER_COUNT, layer_count);
	}
#endif

#if 0   //VC5_ENABLED_PART(VC5_PART_COLOR_SAMPLING)
	if (IsPartEnabled(encoder->enabled_parts, VC5_PART_COLOR_SAMPLING))
	{
		// Output the flags that describe the frame structure
		PutFrameStructureFlags(encoder, stream);
	}
#endif

	// Record the image dimensions in the codec state
	codec->image_width = image_width;
	codec->image_height = image_height;
	
	// The image dimensions determine the default channel dimensions
	codec->channel_width = image_width;
	codec->channel_height = image_height;

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
	if (IsPartEnabled(encoder->enabled_parts, VC5_PART_IMAGE_FORMATS))
	{
		// Record the pattern element parameters in the codec state
		codec->image_format = image_format;
		codec->pattern_width = pattern_width;
		codec->pattern_height = pattern_height;
		codec->components_per_sample = components_per_sample;
		codec->max_bits_per_component = (PRECISION)max_bits_per_component;
	}
#endif

	// This parameter is the default precision for each channel
	codec->bits_per_component = default_internal_precision;

#if VC5_ENABLED_PART(VC5_PART_SECTIONS)
    if (IsEncoderSectionEnabled(encoder, SECTION_NUMBER_HEADER))
    {
        // Make sure that the bitstream is aligned to a segment boundary
        AlignBitsSegment(stream);
        
        // Update the section header with the actual size of the bitstream header section
        EndSection(stream);
    }
#endif

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Write the trailer at the end of the encoded sample

	This routine updates the sample size segment in the sample extension header
	with the actual size of the encoded sample.  The size of the encoded sample
	does not include the size of the sample header or trailer.

	Note that the trailer may not be necessary as the decoder may stop
	reading from the sample after it has decoded all of the information
	required to reconstruct the frame.

	This code was derived from PutVideoIntraFrameTrailer in the current codec.
*/
CODEC_ERROR EncodeBitstreamTrailer(ENCODER *encoder, BITSTREAM *stream)
{
#if 0   //VC5_ENABLED_PART(VC5_PART_SECTIONS)
	if (IsPartEnabled(encoder->enabled_parts, VC5_PART_SECTIONS))
	{
		//TODO: Need to compute the checksum and write it into the bitstream
		uint16_t checksum = 0;
	}
#endif

	AlignBitsSegment(stream);

#if 0   //VC5_ENABLED_PART(VC5_PART_SECTIONS)
	if (IsPartEnabled(encoder->enabled_parts, VC5_PART_SECTIONS))
	{
		PopSampleSize(stream);
	}
#endif

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Write extra information that follows the sample header into the bitstream

	This routine writes metadata into the sample header extension.

	Metadata includes the unique GUID for each video clip, the number of each video frame,
	and the timecode (if available).  The GUID and frame number pair uniquely identify each
	frame in the encoded clip.

	This routine also outputs additional information that describes the characterstics of
    the encoded video in the GOP extension and sample flags.

	The size of the sample extension header is provided by the sample size segment.
*/
CODEC_ERROR EncodeExtensionHeader(ENCODER *encoder, BITSTREAM *stream)
{
    ENABLED_PARTS enabled_parts = encoder->enabled_parts;

    // Encode the transform prescale for the first channel (assume all channels are the same)
	TAGWORD prescale_shift = PackTransformPrescale(&encoder->transform[0]);

	// The tag-value pair is required if the encoder is not using the default values
	//if (IsTransformPrescaleDefault(&encoder->transform[0], TRANSFORM_TYPE_SPATIAL, encoder->encoded.precision))
	if (IsTransformPrescaleDefault(&encoder->transform[0], encoder->internal_precision))
	{
		PutTagPairOptional(stream, CODEC_TAG_PrescaleShift, prescale_shift);
	}
	else
	{
		PutTagPair(stream, CODEC_TAG_PrescaleShift, prescale_shift);
	}

#if (1 && DEBUG)
	// Output an optional tag-value pair for the input pixel format (for debugging)
	PutTagPairOptional(stream, CODEC_TAG_PixelFormat, encoder->pixel_format);
#endif

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
    if (IsPartEnabled(enabled_parts, VC5_PART_IMAGE_FORMATS))
    {
        WriteUniqueImageIdentifier(encoder, stream);
    }
#endif

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
    if (IsPartEnabled(enabled_parts, VC5_PART_IMAGE_FORMATS) &&
        !IsComponentTransformIdentity(encoder->component_transform))
    {
        WriteComponentTransform(encoder->component_transform, stream);
    }
#endif

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
    if (IsPartEnabled(enabled_parts, VC5_PART_IMAGE_FORMATS) &&
        !IsComponentPermutationIdentity(encoder->component_permutation))
    {
        WriteComponentPermutation(encoder->component_permutation, stream);
    }
#endif
    
#if VC5_ENABLED_PART(VC5_PART_LAYERS)
    if (IsPartEnabled(enabled_parts, VC5_PART_LAYERS) && encoder->layer_flag)
    {
        PutTagPairOptional(stream, CODEC_TAG_LayerCount, encoder->layer_count);
    }
#endif

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Write the sample extension trailer into the bitstream

	This routine must be called after encoding the sample and before writing the
	sample trailer, but must only be called if the sample extension header was
	written into the bitstream.
*/
CODEC_ERROR EncodeExtensionTrailer(ENCODER *encoder, BITSTREAM *stream)
{
	return CODEC_ERROR_OKAY;
}

/*!
	@brief Encode the portion of a sample that corresponds to a single layer

	Samples can be contain multiple subsamples.  Each subsample may correspond to
	a different view.  For example, an encoded video sample may contain both the
	left and right subsamples in a stereo pair.

	Subsamples have been called tracks or channels, but this terminology can be
	confused with separate video tracks in a multimedia container or the color
	planes that are called channels elsewhere in this codec.

	The subsamples are decoded seperately and composited to form a single frame
	that is the output of the complete process of decoding a single video sample.
	For this reason, the subsamples are called layers.

	@todo Need to reset the codec state for each layer?
*/
//CODEC_ERROR EncodeLayer(ENCODER *encoder, void *buffer, size_t pitch, BITSTREAM *stream)
CODEC_ERROR EncodeMultipleChannels(ENCODER *encoder, const UNPACKED_IMAGE *image, BITSTREAM *stream)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;

	int channel_count;
	int wavelet_count;
	int channel_index;
	int wavelet_index;

	channel_count = encoder->channel_count;
	wavelet_count = encoder->wavelet_count;

	// Start computing the wavelet transform for each channel
	StartTimer(&encoder->timing.transform);

	// Compute the wavelet transform tree for each channel
	for (channel_index = 0; channel_index < channel_count; channel_index++)
	{
		// Compute the number of transforms in the rest of the wavelet tree
		int transform_count = wavelet_count - 1;

#if (0 && DEBUG)
		printf("Wavelet transforms for channel: %d\n", channel_index);
#endif

		// Apply the first wavelet transform to the component array
		error = TransformForwardSpatialChannel(encoder, image, channel_index);
		if (error != CODEC_ERROR_OKAY) {
			return error;
		}

		// Compute the remaining wavelet transforms for this channel
		for (wavelet_index = 0; wavelet_index < transform_count; wavelet_index++)
		{
			// Transform the current wavelet to the next wavelet in the tree
			int output_index = wavelet_index + 1;

			// Get the input and output wavelets for the next transform
			WAVELET *input = encoder->transform[channel_index].wavelet[wavelet_index];
			WAVELET *output = encoder->transform[channel_index].wavelet[output_index];
            
#if (0 && DEBUG)
            int wavelet_level = wavelet_index + 1;
            char pathname[PATH_MAX];
            sprintf(pathname, "encoded-%d.nv12", wavelet_level);
            DumpTransformWavelets(encoder->transform, channel_count, wavelet_level, pathname);
#endif
			// The prescale is applied to the input values before the wavelet transform
			int prescale = encoder->transform[channel_index].prescale[output_index];

			// The wavelet should have already been created
			assert(input != NULL && output != NULL);

#if (0 && DEBUG)
			printf("Wavelet transform channel: %d, wavelet: %d\n", channel_index, wavelet_index);
#endif
			// Apply the forward wavelet transform to the lowpass band in the wavelet at this level
			error = TransformForwardSpatialLowpass(encoder, input, output, prescale);
			if (error != CODEC_ERROR_OKAY) {
				return error;
			}
		}
	}

	// Finished computing the wavelet transform for each channel
	StopTimer(&encoder->timing.transform);

	// Start encoding the wavelets in each channel
	StartTimer(&encoder->timing.encoding);

	// Output the encoded wavelet tree in each channel to the bitstream
	//error = EncodeLayerChannels(encoder, stream);
	error = EncodeChannelWavelets(encoder, stream);
	
	if (error != CODEC_ERROR_OKAY) {
		return error;
	}

	// Finished encoding the wavelets in each channel
	StopTimer(&encoder->timing.encoding);

	return CODEC_ERROR_OKAY;
}

#if VC5_ENABLED_PART(VC5_PART_LAYERS)
/*!
	@brief Write the sample layer header

	The baseline profile only supports a single layer so the layer header
	and trailer are not required.
*/
CODEC_ERROR EncodeLayerHeader(ENCODER *encoder, BITSTREAM *stream, COUNT layer_number)
{
	// Write the tag-value pair for the layer number
    PutTagPairOptional(stream, CODEC_TAG_LayerNumber, layer_number);

	return CODEC_ERROR_OKAY;
}
#endif
#if VC5_ENABLED_PART(VC5_PART_LAYERS)
/*!
	@brief Write the sample layer trailer

	The baseline profile only supports a single layer so the layer header
	and trailer are not required.

	If more than one layer is present, the layers must be terminated by a
	layer trailer.  Otherwise, the decoder will continue to parse tag-value
	pairs that belong to the next layer.
*/
CODEC_ERROR EncodeLayerTrailer(ENCODER *encoder, BITSTREAM *stream)
{
    // Align the bitstream to a segment boundary
    AlignBitsSegment(stream);
    
	// The value in the layer trailer tag-value pair is not used
	//PutTagPairOptional(stream, CODEC_TAG_LAYER_TRAILER, 0);

	return CODEC_ERROR_OKAY;
}
#endif

/*!
	@brief Encode the channel into the bistream

	This routine encodes all of the subbands (lowpass and highpass) in the
	wavelet tree for the specified channel into the bitstream.
*/
CODEC_ERROR EncodeChannelWavelets(ENCODER *encoder, BITSTREAM *stream)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;
	CODEC_STATE *codec = &encoder->codec;

	int channel_count;
	int channel_index;
	int channel_encoding_count;

#if (0 && DEBUG)
	const char *pathname = "C:/Users/bschunck/Temp/entropy2.dat";
	CreateEncodedBandFile(encoder, pathname);
#endif

	// Get the number of channels in the encoder wavelet transform
	channel_count = encoder->channel_count;

	// Get the number of channels to encode into the bitstream
	channel_encoding_count = encoder->channel_order_count;

	// Compute the remaining wavelet transforms for each channel
	//for (channel_index = 0; channel_index < channel_count; channel_index++)
	for (channel_index = 0; channel_index < channel_count; channel_index++)
	{
		int channel_number = encoder->channel_order_table[channel_index];

		// Encode the tag value pairs in the header for this channel
		error = EncodeChannelHeader(encoder, channel_number, stream);
		if (error != CODEC_ERROR_OKAY) {
			return error;
		}

		// Encode the lowpass and highpass bands in the wavelet tree for this channel
		error = EncodeChannelSubbands(encoder, channel_number, stream);
		if (error != CODEC_ERROR_OKAY) {
			return error;
		}

		// Encode the tag value pairs in the trailer for this channel
		error = EncodeChannelTrailer(encoder, channel_number, stream);
		if (error != CODEC_ERROR_OKAY) {
			return error;
		}

		// Check that the bitstream is aligned to a segment boundary
		assert(IsAlignedSegment(stream));

		// Update the codec state for the next channel in the bitstream
		codec->channel_number = (channel_number + 1);
		codec->subband_number = 0;
	}

#if (0 && DEBUG)
	CloseEncodedBandFile(encoder);
#endif

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Write the channel header into the bitstream
	
	The channel header separates channels in the encoded layer.  The channel header
	is not required before the first encoded channel because the codec state is
	initialized for decoding the first channel.
	
	The first channel is channel number zero.
*/
CODEC_ERROR EncodeChannelHeader(ENCODER *encoder,
								int channel_number,
								BITSTREAM *stream)
{
	CODEC_STATE *codec = &encoder->codec;
	DIMENSION channel_width = encoder->channel[channel_number].width;
	DIMENSION channel_height = encoder->channel[channel_number].height;
	int bits_per_component = encoder->channel[channel_number].bits_per_component;

	AlignBitsSegment(stream);
    
#if VC5_ENABLED_PART(VC5_PART_SECTIONS)
    if (IsEncoderSectionEnabled(encoder, SECTION_NUMBER_CHANNEL))
    {
        // Write the channel section header into the bitstream
        BeginChannelSection(encoder, stream);
    }
#endif

	// Write the channel number if it does not match the codec state
	if (channel_number != codec->channel_number)
	{
		PutTagPair(stream, CODEC_TAG_ChannelNumber, channel_number);
		codec->channel_number = channel_number;
	}

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
	if (IsPartEnabled(encoder->enabled_parts, VC5_PART_IMAGE_FORMATS))
	{
		// The decoder will derive the channel width and height from the image dimensions and format
		codec->channel_width = channel_width;
		codec->channel_height = channel_height;
	}
	else
#endif
	{
		// Write the component array width if it does not match the codec state
		if (channel_width != codec->channel_width)
		{
			PutTagPair(stream, CODEC_TAG_ChannelWidth, channel_width);
			codec->channel_width = channel_width;
		}

		// Write the component array height if it does not match the codec state
		if (channel_height != codec->channel_height)
		{
			PutTagPair(stream, CODEC_TAG_ChannelHeight, channel_height);
			codec->channel_height = channel_height;
		}
	}

	// Write the component array precision if it does not match the codec state
	if (bits_per_component != codec->bits_per_component)
	{
		PutTagPair(stream, CODEC_TAG_BitsPerComponent, bits_per_component);
		codec->bits_per_component = bits_per_component;
	}

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Write the encoded subbands for this channel into the bitstream

	This routine writes the encoded subbands in the wavelet tree for this channel
	into the bitstream, including both the lowpass band and all of the highpass
	bands in each wavelet in this channel.
*/
CODEC_ERROR EncodeChannelSubbands(ENCODER *encoder, int channel_number, BITSTREAM *stream)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;
	//CODEC_STATE *codec = &encoder->codec;

	int wavelet_count = encoder->wavelet_count;
	int last_wavelet_index = wavelet_count - 1;
	int wavelet_index;

	int subband = 0;

	// Start with the lowpass band in the wavelet at the highest level
	WAVELET *wavelet = encoder->transform[channel_number].wavelet[last_wavelet_index];

	// Check that the bitstream is aligned on a segment boundary
	assert(IsAlignedSegment(stream));

#if VC5_ENABLED_PART(VC5_PART_SECTIONS)
    if (IsEncoderSectionEnabled(encoder, SECTION_NUMBER_WAVELET))
    {
        // Write the wavelet section header into the bitstream
        BeginWaveletSection(encoder, stream);
    }
#endif

	// Encode the lowpass subband in this channel
	error = EncodeLowpassBand(encoder, wavelet, channel_number, stream);
	if (error != CODEC_ERROR_OKAY) {
		return error;
	}

	// Advance to the first highpass subband
	subband++;

	// Encode the highpass bands in order of subband number
	for (wavelet_index = last_wavelet_index; wavelet_index >= 0; wavelet_index--)
	{
		//int wavelet_type = WAVELET_TYPE_SPATIAL;
		//int wavelet_level = wavelet_index + 1;
		DIMENSION band_width;
		DIMENSION band_height;
		int band_count;
		int band_index;

		//int lowpass_scale = 0;
		//int lowpass_divisor = 0;

		wavelet = encoder->transform[channel_number].wavelet[wavelet_index];

		band_width = wavelet->width;
		band_height = wavelet->height;
		band_count = wavelet->band_count;

#if VC5_ENABLED_PART(VC5_PART_SECTIONS)
        if (IsEncoderSectionEnabled(encoder, SECTION_NUMBER_WAVELET))
        {
            // Was the wavelet section header already written into the bitstream?
            if (wavelet_index < last_wavelet_index)
            {
                // Write the wavelet section header into the bitstream
                BeginWaveletSection(encoder, stream);
            }
        }
#endif
		// Encode the highpass bands in this wavelet
		for (band_index = 1; band_index < wavelet->band_count; band_index++)
		{
#if (1 && DEBUG)
			if (encoder->encoded_band_bitstream != NULL)
			{
				BITSTREAM *bitstream = encoder->encoded_band_bitstream;
				const int frame = (int)encoder->frame_number;
				const int width = wavelet->width;
				const int height = wavelet->height;
				void *data;
				size_t size;

				uint32_t channel_mask = 0x01;
				uint32_t wavelet_mask = 0x04;
				uint32_t band_mask = 0x08;

				error = EncodeHighpassBand(encoder, wavelet, band_index, subband, bitstream);
				if (error != CODEC_ERROR_OKAY) {
					return error;
				}

				if ((((1 << channel_number) & channel_mask) != 0) &&
					(((1 << wavelet_index) & wavelet_mask) != 0) &&
					(((1 << band_index) & band_mask) != 0))
				{

					GetStreamBuffer(bitstream->stream, &data, &size);

					WriteWaveletBand(&encoder->encoded_band_file, frame, channel_number, wavelet_index,
						band_index, BAND_TYPE_ENCODED_RUNLENGTHS, width, height, data, size);
				}

				RewindBitstream(bitstream);

				continue;
			}
#endif
			error = EncodeHighpassBand(encoder, wavelet, band_index, subband, stream);
			if (error != CODEC_ERROR_OKAY) {
				return error;
			}

			// Advance to the next subband
			subband++;
		}

#if VC5_ENABLED_PART(VC5_PART_SECTIONS)
        if (IsEncoderSectionEnabled(encoder, SECTION_NUMBER_WAVELET))
        {
            // Make sure that the bitstream is aligned to a segment boundary
            AlignBitsSegment(stream);
            
            // Update the section header with the actual size of the wavelet section
            EndSection(stream);
        }
#endif
	}

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Write the channel trailer into the bitstream

	A channel trailer is not required as the channel header functions as a marker
	between channels in the bitstream.
	
	It may be necessary to update the channel size in a channel section header that was
    written into the channel header if channel sections are enabled.
*/
CODEC_ERROR EncodeChannelTrailer(ENCODER *encoder, int channel, BITSTREAM *stream)
{
#if VC5_ENABLED_PART(VC5_PART_SECTIONS)
    if (IsEncoderSectionEnabled(encoder, SECTION_NUMBER_CHANNEL))
    {
        // Make sure that the bitstream is aligned to a segment boundary
        AlignBitsSegment(stream);

        // Update the section header with the actual size of the channel section
        EndSection(stream);
    }
#endif
    
	return CODEC_ERROR_OKAY;
}

/*!
	@brief Allocate intermediate buffers for the horizontal transform results

	@todo Need to return an error code if allocation fails
*/
CODEC_ERROR AllocateEncoderHorizontalBuffers(ENCODER *encoder, int buffer_width)
{
	ALLOCATOR *allocator = encoder->allocator;

	int channel_count = encoder->channel_count;
	int channel_index;

	for (channel_index = 0; channel_index < channel_count; channel_index++)
	{
		size_t row_buffer_size;
		int row;

//#if VC5_ENABLED_PART(VC5_PART_COLOR_SAMPLING)
		//int channel_width = ChannelWidth(encoder, channel_index, buffer_width);
//#else
		int channel_width = buffer_width;
//#endif
		row_buffer_size = channel_width * sizeof(PIXEL);

		for (row = 0; row < ROW_BUFFER_COUNT; row++)
		{
			encoder->lowpass_buffer[channel_index][row] = Alloc(allocator, row_buffer_size);
			encoder->highpass_buffer[channel_index][row] = Alloc(allocator, row_buffer_size);

			// Check that the memory allocation was successful
			assert(encoder->lowpass_buffer[channel_index][row] != NULL);
			if (! (encoder->lowpass_buffer[channel_index][row] != NULL)) {
				return CODEC_ERROR_OUTOFMEMORY;
			}
			assert(encoder->highpass_buffer[channel_index][row] != NULL);
			if (! (encoder->highpass_buffer[channel_index][row] != NULL)) {
				return CODEC_ERROR_OUTOFMEMORY;
			}
		}
	}

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Deallocate the intermediate buffers for the horizontal transform results

	It is possible to avoid reallocating the buffers for the horizontal transform
	results if the buffers were not deallocated between encoded frames.  In this case,
	it would be necessary to call this routine inside @ref ReleaseEncoder and it would
	also be necessary to modify @ref AllocateEncoderHorizontalBuffers to not allocate
	the buffers if they are already allocated.
*/
CODEC_ERROR DeallocateEncoderHorizontalBuffers(ENCODER *encoder)
{
	ALLOCATOR *allocator = encoder->allocator;

	int channel_count = encoder->channel_count;
	int channel_index;

	for (channel_index = 0; channel_index < channel_count; channel_index++)
	{
		int row;

		for (row = 0; row < ROW_BUFFER_COUNT; row++)
		{
			Free(allocator, encoder->lowpass_buffer[channel_index][row]);
			Free(allocator, encoder->highpass_buffer[channel_index][row]);
		}
	}

	return CODEC_ERROR_OKAY;
}

#if 0
/*!
	@brief Allocate buffers for unpacking rows of the input frame

	The unpacking buffers are used to unpack the input frame into separate
	channels one row at a time for more efficient memory usage.
*/
CODEC_ERROR AllocateEncoderUnpackingBuffers(ENCODER *encoder, int frame_width)
{
	ALLOCATOR *allocator = encoder->allocator;

	int channel_count = encoder->channel_count;
	int channel_index;

	for (channel_index = 0; channel_index < channel_count; channel_index++)
	{
		// Compute the actual buffer size for each channel
		int channel_width = ChannelWidth(encoder, channel_index, frame_width);
		size_t row_buffer_size = channel_width * sizeof(PIXEL);
		assert(row_buffer_size > 0);

		// Allocate an unpacking buffer for this channel
		encoder->unpacked_buffer[channel_index] = Alloc(allocator, row_buffer_size);

		// Check that the memory allocation was successful
		assert(encoder->unpacked_buffer[channel_index] != NULL);
		if (! (encoder->unpacked_buffer[channel_index] != NULL)) {
			return CODEC_ERROR_OUTOFMEMORY;
		}
	}

	return CODEC_ERROR_OKAY;
}
#endif

/*!
	@brief Allocate buffers used for computing the forward wavelet transform
*/
CODEC_ERROR AllocateHorizontalBuffers(ALLOCATOR *allocator,
									  PIXEL *lowpass_buffer[],
									  PIXEL *highpass_buffer[],
									  int buffer_width)
{
	const size_t row_buffer_size = buffer_width * sizeof(PIXEL);

	int row;

	for (row = 0; row < ROW_BUFFER_COUNT; row++)
	{
		lowpass_buffer[row] = Alloc(allocator, row_buffer_size);
		highpass_buffer[row] = Alloc(allocator, row_buffer_size);

		// Check that the memory allocation was successful
		assert(lowpass_buffer[row] != NULL);
		if (! (lowpass_buffer[row] != NULL)) {
			return CODEC_ERROR_OUTOFMEMORY;
		}
		assert(highpass_buffer[row] != NULL);
		if (! (highpass_buffer[row] != NULL)) {
			return CODEC_ERROR_OUTOFMEMORY;
		}
	}

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Deallocate buffers used for computing the forward wavelet transform
*/
CODEC_ERROR DeallocateHorizontalBuffers(ALLOCATOR *allocator,
										PIXEL *lowpass_buffer[],
										PIXEL *highpass_buffer[])
{
	int row;

	for (row = 0; row < ROW_BUFFER_COUNT; row++)
	{
		Free(allocator, lowpass_buffer[row]);
		Free(allocator, highpass_buffer[row]);
	}

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Shift the buffers of horizontal highpass results

	The encoder contains six rows of horizontal lowpass and highpass results
	for each channel.  This routine shifts the buffers by two rows to make
	room for two more rows of horizontal results for each channel.
*/
CODEC_ERROR ShiftHorizontalResultBuffers(ENCODER *encoder)
{
	int channel;

	for (channel = 0; channel < encoder->channel_count; channel++)
	{
		PIXEL *buffer0;
		PIXEL *buffer1;

		// Shift the rows of lowpass results
		buffer0 = encoder->lowpass_buffer[channel][0];
		buffer1 = encoder->lowpass_buffer[channel][1];
		encoder->lowpass_buffer[channel][0] = encoder->lowpass_buffer[channel][2];
		encoder->lowpass_buffer[channel][1] = encoder->lowpass_buffer[channel][3];
		encoder->lowpass_buffer[channel][2] = encoder->lowpass_buffer[channel][4];
		encoder->lowpass_buffer[channel][3] = encoder->lowpass_buffer[channel][5];
		encoder->lowpass_buffer[channel][4] = buffer0;
		encoder->lowpass_buffer[channel][5] = buffer1;

		// Shift the rows of highpass results
		buffer0 = encoder->highpass_buffer[channel][0];
		buffer1 = encoder->highpass_buffer[channel][1];
		encoder->highpass_buffer[channel][0] = encoder->highpass_buffer[channel][2];
		encoder->highpass_buffer[channel][1] = encoder->highpass_buffer[channel][3];
		encoder->highpass_buffer[channel][2] = encoder->highpass_buffer[channel][4];
		encoder->highpass_buffer[channel][3] = encoder->highpass_buffer[channel][5];
		encoder->highpass_buffer[channel][4] = buffer0;
		encoder->highpass_buffer[channel][5] = buffer1;
	}

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Shift the array of buffers of horizontal highpass results

	The buffers are used to store siz rows of horizontal lowpass and highpass results
	while applying the forward wavelet transform.
*/
CODEC_ERROR ShiftHorizontalBuffers(PIXEL *lowpass_buffer[], PIXEL *highpass_buffer[])
{
	PIXEL *buffer0;
	PIXEL *buffer1;

	// Shift the rows of lowpass results
	buffer0 = lowpass_buffer[0];
	buffer1 = lowpass_buffer[1];
	lowpass_buffer[0] = lowpass_buffer[2];
	lowpass_buffer[1] = lowpass_buffer[3];
	lowpass_buffer[2] = lowpass_buffer[4];
	lowpass_buffer[3] = lowpass_buffer[5];
	lowpass_buffer[4] = buffer0;
	lowpass_buffer[5] = buffer1;

	// Shift the rows of highpass results
	buffer0 = highpass_buffer[0];
	buffer1 = highpass_buffer[1];
	highpass_buffer[0] = highpass_buffer[2];
	highpass_buffer[1] = highpass_buffer[3];
	highpass_buffer[2] = highpass_buffer[4];
	highpass_buffer[3] = highpass_buffer[5];
	highpass_buffer[4] = buffer0;
	highpass_buffer[5] = buffer1;

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Apply the forward spatial wavelet transform to the component array

	Each channel in the bitstream represents a single component array.
	
	This routine applies the forward spatial wavelet transform to the
	component array for the specified channel.
*/
CODEC_ERROR TransformForwardSpatialChannel(ENCODER *encoder,
										   const UNPACKED_IMAGE *image,
										   int channel_number)
{
	//ALLOCATOR *allocator = encoder->allocator;

	DIMENSION input_width = encoder->channel[channel_number].width;
	DIMENSION input_height = encoder->channel[channel_number].height;
	DIMENSION output_width = ((input_width % 2) == 0) ? input_width / 2 : (input_width + 1) / 2;

	size_t input_pitch = image->component_array_list[channel_number].pitch;
	void *buffer = image->component_array_list[channel_number].data;

	// The prescale is applied to the input values before the wavelet transform
	int prescale = encoder->transform[channel_number].prescale[0];

	// Last row of the wavelet result
	//int bottom_input_row = input_height - 2;
	int bottom_input_row = ((input_height % 2) == 0) ? input_height - 2 : input_height - 1;

	// Calculate the last row for unpacking more rows from the input frame
	int last_unpacked_row = bottom_input_row - 2;

	// The midpoint prequant offset is added during quantization
	int midpoint_prequant = encoder->midpoint_prequant;

	int input_row;

	int unpacked_buffer_row;
	//PIXEL *unpacked_buffer_row_ptr;

	// Allocate six pairs of lowpass and highpass buffers for each channel
	AllocateEncoderHorizontalBuffers(encoder, output_width);

	// Allocate buffers for unpacking a row of packed pixels from each channel
	//AllocateEncoderUnpackingBuffers(encoder, input_width);

	// Compute six pairs of horizontal transform results for each channel
	for (unpacked_buffer_row = 0; unpacked_buffer_row < ROW_BUFFER_COUNT; unpacked_buffer_row++)
	{
		PIXEL *unpacked_buffer_row_ptr = (PIXEL *)((uintptr_t)buffer + unpacked_buffer_row * input_pitch);

		// The width of each input row may depend on the channel
		//int channel_width = ChannelWidth(encoder, channel_index, input_width);

		FilterHorizontalRow(unpacked_buffer_row_ptr,
							encoder->lowpass_buffer[channel_number][unpacked_buffer_row],
							encoder->highpass_buffer[channel_number][unpacked_buffer_row],
							input_width,
							prescale);
	}

	// Start applying the vertical transform to the first row
	input_row = 0;

	// Process all channels in the first row as a special case
	//for (channel_index = 0; channel_index < channel_count; channel_index++)
	{
		// The output wavelet is at the first level in the wavelet tree
		WAVELET *wavelet = encoder->transform[channel_number].wavelet[0];

		// The width of each output row may depend on the channel
		int wavelet_width = wavelet->width;

#if (0 && DEBUG)
		printf("TransformForwardSpatialQuantFrame output width: %d, quant: %d %d %d %d\n",
			wavelet_width, wavelet->quant[0], wavelet->quant[1], wavelet->quant[2], wavelet->quant[3]);
#endif
		// Process the first row as a special case for the boundary condition
		FilterVerticalTopRow(encoder->lowpass_buffer[channel_number],
							 encoder->highpass_buffer[channel_number],
							 wavelet->data,
							 wavelet->pitch,
							 wavelet->band_count,
							 input_row,
							 wavelet_width,
							 wavelet->quant,
							 midpoint_prequant);
	}

	// Advance to the second pair of input rows and use the first six horizontal results
	input_row += 2;

	// Process the middle rows
	for (; input_row < bottom_input_row; input_row += 2)
	{
		//printf("Processing middle rows, input_row: %d\n", input_row);

		// Check for errors in the row calculation
		assert((input_row % 2) == 0);

		//for (channel_index = 0; channel_index < channel_count; channel_index++)
		{
			// The output wavelet is at the first level in the wavelet tree
			WAVELET *wavelet = encoder->transform[channel_number].wavelet[0];

			// The width of each output row may depend on the channel
			int wavelet_width = wavelet->width;

			FilterVerticalMiddleRow(encoder->lowpass_buffer[channel_number],
									encoder->highpass_buffer[channel_number],
									wavelet->data,
									wavelet->pitch,
									wavelet->band_count,
									input_row,
									wavelet_width,
									wavelet->quant,
									midpoint_prequant);
		}

		if (input_row < last_unpacked_row)
		{
			//printf("Unpacking two more rows, input_row: %d, last_unpacked_row: %d\n", input_row, last_unpacked_row);

			// Shift the intermediate horizontal results to make room for the next two rows
			ShiftHorizontalResultBuffers(encoder);

			// Get two more rows of horizontal lowpass and highpass results
			for (unpacked_buffer_row = 4; unpacked_buffer_row < ROW_BUFFER_COUNT; unpacked_buffer_row++)
			{
				int component_array_row = input_row + unpacked_buffer_row;
				PIXEL *component_array_row_ptr;

				if (component_array_row >= input_height)
				{
					//printf("Duplicating last row\n");

					// Duplicate the last input row
					component_array_row = input_height - 1;
				}

				component_array_row_ptr = (PIXEL *)((uintptr_t)buffer + component_array_row * input_pitch);

				// Unpack the row in the frame buffer
				//UnpackFrameRow(encoder, unpacked_frame_row_ptr);

				// Apply the horizontal wavelet transform to each unpacked component
				//for (channel_index = 0; channel_index < channel_count; channel_index++)
				{
					// The width of each input row may depend on the channel
					//int channel_width = ChannelWidth(encoder, channel_index, input_width);

					// The prescale is applied to the input values before the wavelet transform
					int prescale = encoder->transform[channel_number].prescale[0];

					FilterHorizontalRow(component_array_row_ptr,
										encoder->lowpass_buffer[channel_number][unpacked_buffer_row],
										encoder->highpass_buffer[channel_number][unpacked_buffer_row],
										input_width,
										prescale);
				}
			}
		}
	}

	// Should have exited the loop at the last row
	assert(input_row == bottom_input_row);

	// Process the last row as a special case for the boundary conditions
	//for (channel_index = 0; channel_index < channel_count; channel_index++)
	{
		// The output wavelet is at the first level in the wavelet tree
		WAVELET *wavelet = encoder->transform[channel_number].wavelet[0];

		// The width of each output row may depend on the channel
		int wavelet_width = wavelet->width;

		FilterVerticalBottomRow(encoder->lowpass_buffer[channel_number],
								encoder->highpass_buffer[channel_number],
								wavelet->data,
								wavelet->pitch,
								wavelet->band_count,
								input_row,
								wavelet_width,
								wavelet->quant,
								midpoint_prequant);

		// Does the encoded frame including padding?
		//PadWaveletBands(encoder, wavelet);

		//TODO: Set the valid band mask for all bands in the wavelet in each channel
	}

	// Deallocate the buffers for horizontal results
	DeallocateEncoderHorizontalBuffers(encoder);

	// Deallocate the buffers for unpacking input rows
	//DeallocateEncoderUnpackingBuffers(encoder);

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Apply the forward spatial wavelet transform to the lowpass band

	The forward spatial wavelet transform is applied to the lowpass band in the
	input wavelet producing four bands of results in the output wavelet.
	
	The input values are prescaled to avoid overflow.  The quantization values
	stored in the output wavelet are applied to the result.
*/
CODEC_ERROR TransformForwardSpatialLowpass(ENCODER *encoder, WAVELET *input, WAVELET *output, int prescale)
{
	ALLOCATOR *allocator = encoder->allocator;

	DIMENSION input_width = input->width;
	DIMENSION input_height = input->height;

	DIMENSION output_width = ((input_width % 2) == 0) ? input_width / 2 : (input_width + 1) / 2;

	//TODO: Need to calculate the actual bottom row without padding

	int horizontal_buffer_row;

	// The last row of the transform is calculated using the display height
	int bottom_input_row = input_height - 2;

	// Calculate the last row for reading more rows from the input wavelet
	//int last_input_row = bottom_input_row - 2;
	//int last_input_row = input_height - 2;
	int last_input_row;

	int input_row = 0;

	//int midpoint_prequant = encoder->q.midpoint_prequant;
	int midpoint_prequant = encoder->midpoint_prequant;

	PIXEL *lowpass_buffer[ROW_BUFFER_COUNT];
	PIXEL *highpass_buffer[ROW_BUFFER_COUNT];

	PIXEL *prescaled_buffer;
	size_t prescaled_size;

	int row;

#if (0 && DEBUG)
	printf("TransformForwardSpatialLowpass output width: %d, quant: %d %d %d %d\n",
		output_width, output->quant[0], output->quant[1], output->quant[2], output->quant[3]);
#endif

	// Adjust the bottom input row if the wavelet height is odd
	if ((input_height % 2) != 0) {
		bottom_input_row++;
	}

	// The bottom input row must be even
	assert((bottom_input_row % 2) == 0);

	// Calculate the last row for reading more rows from the input wavelet
	last_input_row = bottom_input_row - 2;

	// The last input row must be even
	assert((last_input_row % 2) == 0);

	// Allocate six pairs of lowpass and highpass buffers
	AllocateHorizontalBuffers(allocator, lowpass_buffer, highpass_buffer, output_width);

	// Allocate a buffer for the prescaled input values
	prescaled_size = input_width * sizeof(PIXEL);
	prescaled_buffer = Alloc(allocator, prescaled_size);

	// Compute six pairs of horizontal transform results
	for (row = 0; row < ROW_BUFFER_COUNT; row++)
	{
		PIXEL *input_row_ptr = WaveletRowAddress(input, LL_BAND, row);

		//PrescaleInputRow(input_row_ptr, prescaled_buffer, input_width, prescale);

		// Apply the horizontal wavelet transform to each input row
		//FilterHorizontalRow(prescaled_buffer,
		FilterHorizontalRow(input_row_ptr,
							lowpass_buffer[row],
							highpass_buffer[row],
							input_width,
							prescale);
	}

	// Process the first row as a special case
	input_row = 0;

	FilterVerticalTopRow(lowpass_buffer,
						 highpass_buffer,
						 output->data,
						 output->pitch,
						 output->band_count,
						 input_row,
						 output_width,
						 output->quant,
						 midpoint_prequant);

	// Advance to the second pair of rows and use the first six horizontal results
	input_row += 2;

	// Process the middle rows
	for (; input_row < bottom_input_row; input_row += 2)
	//for (; input_row < last_input_row; input_row += 2)
	{
		// Check for errors in the row calculation
		assert((input_row % 2) == 0);

		FilterVerticalMiddleRow(lowpass_buffer,
								highpass_buffer,
								output->data,
								output->pitch,
								output->band_count,
								input_row,
								output_width,
								output->quant,
								midpoint_prequant);

		if (input_row < last_input_row)
		{
			// Shift the intermediate horizontal results to make room for the next two rows
			ShiftHorizontalBuffers(lowpass_buffer, highpass_buffer);

			// Get two more rows of horizontal lowpass and highpass results
			for (horizontal_buffer_row = 4; horizontal_buffer_row < ROW_BUFFER_COUNT; horizontal_buffer_row++)
			{
				int next_input_row = input_row + horizontal_buffer_row;
				PIXEL *input_row_ptr;

				if (next_input_row >= input_height)
				{
					// Duplicate the last input row
					next_input_row = input_height - 1;
				}

				input_row_ptr = WaveletRowAddress(input, LL_BAND, next_input_row);

				//PrescaleInputRow(input_row_ptr, prescaled_buffer, input_width, prescale);

				//FilterHorizontalRow(prescaled_buffer,
				FilterHorizontalRow(input_row_ptr,
									lowpass_buffer[horizontal_buffer_row],
									highpass_buffer[horizontal_buffer_row],
									input_width,
									prescale);
			}
		}
	}

	// Should have exited the loop at the last row
	assert(input_row == bottom_input_row);
	//assert(input_row == last_input_row);

	// Process the last row as a special case
	FilterVerticalBottomRow(lowpass_buffer,
							highpass_buffer,
							output->data,
							output->pitch,
							output->band_count,
							input_row,
							output_width,
							output->quant,
							midpoint_prequant);

	// Fill the unused rows in the wavelet bands
	//PadWaveletBands(output, actual_height);

	//TODO: Set the valid band mask for all bands in the wavelet in each channel

	DeallocateHorizontalBuffers(allocator, lowpass_buffer, highpass_buffer);

	Free(allocator, prescaled_buffer);

	return CODEC_ERROR_OKAY;
}

#if 0
/*!
	@brief Zero the padding at the bottom of the wavelet bands
*/
CODEC_ERROR PadWaveletBands(ENCODER *encoder, WAVELET *wavelet)
{
	DIMENSION input_height = EncodedLayerHeight(encoder, encoder->input.height);
	DIMENSION encoded_height = EncodedLayerHeight(encoder, encoder->encoded.height);

	int last_row = input_height - 2;

	// Does the encoded frame including padding?
	if (input_height < encoded_height)
	{
		// Compute the number of rows of padding in the wavelet bands
		int padding_height = wavelet->height - (input_height / 2);

		// Compute the first row of padding in the wavelet bands
		int padding_row = (last_row / 2);

		size_t padding_size = padding_height * wavelet->pitch;

		int band;

		// Zero the wavelet band rows used for padding
		for (band = 0; band < wavelet->band_count; band++)
		{
			// Compute the starting address of the padding in the band
			uint8_t *padding_ptr = (uint8_t *)wavelet->data[band] + padding_row * wavelet->pitch;
			
			// Zero the padding area at the bottom of the band
			memset(padding_ptr, 0, padding_size);
		}
	}

	return CODEC_ERROR_OKAY;
}
#endif

/*!
	@brief Convert a bitstream error code to a codec error codec

	The bitstream and byte stream modules use a separate enumeration
	for error codes since these modules are used in other applications.
	The bistream error code is embedded in a range of codec error codes
	that are reserved for bitstream errors.
*/
CODEC_ERROR CodecErrorBitstream(BITSTREAM_ERROR error)
{
	uint32_t codec_error = CODEC_ERROR_BITSTREAM;
	codec_error |= (uint32_t)error;
	return (CODEC_ERROR)codec_error;
}

/*!
	@brief Allocate all of the wavelets used during encoding

	This routine allocates all of the wavelets in the wavelet tree that
	may be used during encoding.

	This routine is used to preallocate the wavelets before encoding begins.
	If the wavelet bands are allocated on demand if not preallocated.

	By default, the wavelet bands are encoded into the bitstream with the bands
	from the wavelet at the highest level (smallest wavelet) first so that the
	bands can be processed by the encoder in the order as the sample is decoded.

	@todo Do not allocate wavelets for resolutions that are larger then the
	decoded resolution.  At lower resolutions, the depth of the wavelet tree
	can be reduced and the highpass bands in the unused wavelets to not have
	to be decoded.

	@todo Should it be an error if the wavelets are not preallocated?
*/
CODEC_ERROR AllocEncoderTransforms(ENCODER *encoder)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;

	// Use the default allocator for the encoder
	ALLOCATOR *allocator = encoder->allocator;
	int channel_index;
	int wavelet_index;

	assert(encoder != NULL);
	if (! (encoder != NULL)) {
		return CODEC_ERROR_NULLPTR;
	}

	// Initialize the wavelet tree based on the encoded format
	//SetTransformParameters(encoder);

	// Check that the encoded dimensions are valid
	//assert((encoder->encoded.width % (1 << encoder->wavelet_count)) == 0);

	for (channel_index = 0; channel_index < encoder->channel_count; channel_index++)
	{
		// The wavelet at level zero has the same dimensions as the encoded frame
		DIMENSION wavelet_width = 0;
		DIMENSION wavelet_height = 0;
		error = GetChannelDimensions(encoder, channel_index, &wavelet_width, &wavelet_height);
		assert(wavelet_width > 0 && wavelet_height > 0);
		if (error != CODEC_ERROR_OKAY) {
			return error;
		}

		for (wavelet_index = 0; wavelet_index < encoder->wavelet_count; wavelet_index++)
		{
			WAVELET *wavelet = NULL;

			// Pad the wavelet width if not divisible by two
			if ((wavelet_width % 2) != 0) {
				wavelet_width++;
			}

			// Pad the wavelet height if not divisible by two
			if ((wavelet_height % 2) != 0) {
				wavelet_height++;
			}

			// Reduce the dimensions of the next wavelet by half
			wavelet_width /= 2;
			wavelet_height /= 2;

			// Dimensions of the current wavelet must be divisible by two
			//assert((wavelet_width % 2) == 0 && (wavelet_height % 2) == 0);

			// The wavelet width must be divisible by two
			//assert((wavelet_width % 2) == 0);

			// Allocate the wavelet
			wavelet = CreateWavelet(allocator, wavelet_width, wavelet_height);
			if (wavelet == NULL) {
				return CODEC_ERROR_OUTOFMEMORY;
			}

			// Add the wavelet to the transform
			encoder->transform[channel_index].wavelet[wavelet_index] = wavelet;
		}
	}

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Allocate all of the buffers required for encoding

	This routine allocates buffers required for encoding, not including
	the wavelet images in the wavelet tree which are allocated by
	@ref AllocEncoderTransforms

	This routine is used to preallocate buffers before encoding begins.
	If the buffers are allocated on demand if not preallocated.

	The encoding parameters, including the encoded frame dimensions,
	resolution of the decoded frame, and the decoded pixel format, are
	taken into account when the buffers are allocated.  For example,
	buffer space that is only used when encoding to full resolution will
	not be allocated if the frame is decoded to a smaller size.

	Note that it is not an error to preallocate more buffer space than
	what is strictly required for encoding.  For example, it is okay to
	allocate buffer space required for full frame encoding even if the
	encoded sample will be decoded at lower resolution.  In many applications,
	it is simpler to preallocate the maximum buffer space that may be needed.

	Currently, the reference encoder allocates scratch buffers as required
	by each routine that needs scratch space and the scratch buffers are
	deallocated at the end each routine that allocates scratch space.
	A custom memory allocator can make this scheme efficient.  See comments
	in the documentation for the memory allocator module.

	@todo Should it be an error if the buffers are not preallocated?
*/
CODEC_ERROR AllocEncoderBuffers(ENCODER *encoder)
{
	(void)encoder;
	return CODEC_ERROR_UNIMPLEMENTED;
}

#if 0
/*!
	@brief Set the configuration of the wavelet tree

	The parameters that determine the configuration of the wavelet tree
	can be computed from the encoded format.
*/
CODEC_ERROR SetTransformParameters(ENCODER *encoder)
{
	switch (encoder->encoded.format)
	{
	case ENCODED_FORMAT_UNKNOWN:
	default:
		assert(0);
		break;

	case ENCODED_FORMAT_BAYER:
		encoder->channel_count = 4;
		encoder->wavelet_count = 3;
		break;

#if (CODEC_PROFILE > CODEC_PROFILE_BASELINE)
	case ENCODED_FORMAT_RGB_444:
		encoder->channel_count = 3;
		encoder->wavelet_count = 3;
		break;
#endif
#if (CODEC_PROFILE > CODEC_PROFILE_MAIN)
	case ENCODED_FORMAT_YUV_422:
		encoder->channel_count = 3;
		encoder->wavelet_count = 3;
		break;
#endif
#if 0
	case ENCODED_FORMAT_RGBA_4444:
		encoder->channel_count = 4;
		encoder->wavelet_count = 3;
		break;
#endif
#if 0
	case ENCODED_FORMAT_YUVA_4444:
		//TODO: Set different dimensions in each channel
		assert(0);
		break;
#endif
	}

	return CODEC_ERROR_OKAY;
}
#endif

/*!
	@brief Set the quantization parameters in the encoder

	This routine computes the parameters in the quantizer used by
	the encoder based based on the quality setting and the desired
	bitrate.  The quantization parameters are adjsuted to compensate
	for the precision of the input pixels.
	
	Note that the baseline profile does not support quantization to
	achieve a desired bitrate.

*/
CODEC_ERROR SetEncoderQuantization(ENCODER *encoder,
								   const PARAMETERS *parameters)
{
	int channel_count = encoder->channel_count;
	int channel_number;

	const int quant_table_length = sizeof(parameters->quant_table)/sizeof(parameters->quant_table[0]);

	// Set the quantization table in each channel
	for (channel_number = 0; channel_number < channel_count; channel_number++)
	{
		SetTransformQuantTable(encoder, channel_number, parameters->quant_table, quant_table_length);
	}

	// Set the midpoint prequant parameter
	encoder->midpoint_prequant = 2;

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Copy the quantization table into the wavelet bands
*/
CODEC_ERROR SetTransformQuantTable(ENCODER *encoder, int channel, const QUANT table[], int table_length)
{
	int wavelet_count = encoder->wavelet_count;
	int wavelet_index;
	int subband;

	// All lowpass bands use the quantization for subband zero
	for (wavelet_index = 0; wavelet_index < wavelet_count; wavelet_index++)
	{
		WAVELET *wavelet = encoder->transform[channel].wavelet[wavelet_index];
		wavelet->quant[0] = table[0];
	}

	// Store the quantization values for the highpass bands in each wavelet
	for (subband = 1; subband < table_length; subband++)
	{
		int wavelet_index = SubbandWaveletIndex(subband);
		int band_index = SubbandBandIndex(subband);
		WAVELET *wavelet;

		assert(0 <= wavelet_index && wavelet_index < wavelet_count);
		assert(0 <= band_index && band_index <= MAX_BAND_COUNT);

		// Store the quantization value for this subband
		wavelet = encoder->transform[channel].wavelet[wavelet_index];
		wavelet->quant[band_index] = table[subband];
	}

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Return the encoded dimensions for the specified channel

	The encoded dimensions for each channel may differ due to color
	difference component sampling.
*/
CODEC_ERROR GetChannelDimensions(ENCODER *encoder,
								 int channel_number,
								 DIMENSION *channel_width_out,
								 DIMENSION *channel_height_out)
{
	DIMENSION channel_width = 0;
	DIMENSION channel_height = 0;

	assert(encoder != NULL && channel_width_out != NULL && channel_height_out != NULL);
	if (! (encoder != NULL && channel_width_out != NULL && channel_height_out != NULL)) {
		return CODEC_ERROR_NULLPTR;
	}

	assert(0 <= channel_number && channel_number < encoder->channel_count);
	if (! (0 <= channel_number && channel_number < encoder->channel_count)) {
		return CODEC_ERROR_UNEXPECTED;
	}

	// Clear the output dimensions in case this routine terminates early
	*channel_width_out = 0;
	*channel_height_out = 0;

	channel_width = encoder->channel[channel_number].width;
	channel_height = encoder->channel[channel_number].height;

#if 0
	switch (encoder->encoded.format)
	{
#if VC5_ENABLED_PART(VC5_PART_COLOR_SAMPLING)
	case ENCODED_FORMAT_YUV_422:
		if (channel_number > 0)
		{
			// Chroma channels are half as wide due to 4:2:2 sampling
			encoded_width /= 2;
		}
		if (encoder->progressive == 0)
		{
			// Each field is encoded as a separate layer (half height)
			encoded_height /= 2;
		}
		break;
#endif
#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
	case ENCODED_FORMAT_RGB_444:
		// All channels have the same dimensions
		break;
#endif
#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
	case ENCODED_FORMAT_BAYER:
		// All channels have the same dimensions
		break;
#endif
	default:
		assert(0);
		return CODEC_ERROR_UNSUPPORTED_FORMAT;
		break;
	}
#endif

	*channel_width_out = channel_width;
	*channel_height_out = channel_height;

	return CODEC_ERROR_OKAY;
}

#if VC5_ENABLED_PART(VC5_PART_COLOR_SAMPLING)
/*!
	@brief Adjust the width of a channel to account for chroma sampling
*/
DIMENSION ChannelWidth(ENCODER *encoder, int channel_index, DIMENSION width)
{
	switch (encoder->image_format)
	{
	case IMAGE_FORMAT_YCbCrA:
		if (channel_index > 0) {
			width /= 2;
		}
		break;

	default:
		// Assume that the width does not have to be adjusted
		break;
	}

	return width;
}
#endif

/*!
	@brief Adjust the height of encoded layer

	Interleaved frames are encoded as separate layers with half the height.
*/
DIMENSION EncodedLayerHeight(ENCODER *encoder, DIMENSION height)
{
#if 0   //VC5_ENABLED_PART(VC5_PART_LAYERS)
	assert(encoder != NULL);
	if (encoder->progressive == 0) {
		height /= 2;
	}
#endif

	return height;
}

/*!
	@brief Compute the dimensions of the image as reported by the ImageWidth and ImageHeight parameters

	The image width is the maximum width of all component arrays and the image height is the maximum height
	of all component arrays.
*/
CODEC_ERROR GetMaximumChannelDimensions(const UNPACKED_IMAGE *image, DIMENSION *width_out, DIMENSION *height_out)
{
	DIMENSION width = 0;
	DIMENSION height = 0;
	int channel_number;

	if (image == NULL) {
		return CODEC_ERROR_UNEXPECTED;
	}

	for (channel_number = 0; channel_number < image->component_count; channel_number++)
	{
		if (width < image->component_array_list[channel_number].width) {
			width = image->component_array_list[channel_number].width;
		}

		if (height < image->component_array_list[channel_number].height) {
			height = image->component_array_list[channel_number].height;
		}
	}

	if (width_out != NULL) {
		*width_out = width;
	}

	if (height_out != NULL) {
		*height_out = height;
	}

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Set the bit for the specified subband in the decoded band mask

	The decoded subband mask is used to track which subbands have been
	decoded in teh current channel.  It is reset at the start of each
	channel.

	The decoded subband mask is used when decoding a sample at less
	than full resolution.  The mask indicates when enough subbands
	have been decoded for a channel and that remaining portion of the
	encoded sample for the current channel may be skipped.
*/
CODEC_ERROR SetEncodedBandMask(CODEC_STATE *codec, int subband)
{
	if (0 <= subband && subband < MAX_SUBBAND_COUNT) {
		codec->decoded_subband_mask |= (1 << subband);
	}
	return CODEC_ERROR_OKAY;
}


/*!
	@brief Encoded the lowpass band from the bitstream

	The wavelet at the highest level is passes as an argument.
	This routine decodes lowpass band in the bitstream into the
	lowpass band of the wavelet.
*/
CODEC_ERROR EncodeLowpassBand(ENCODER *encoder, WAVELET *wavelet, int channel_number, BITSTREAM *stream)
{
	CODEC_STATE *codec = &encoder->codec;
	//FILE *logfile = encoder->logfile;
	//int subband = 0;
	//int level = encoder->wavelet_count;
	int width = wavelet->width;
	int height = wavelet->height;
	uint8_t *lowpass_row_ptr;
	int lowpass_pitch;
	int row;

	PRECISION lowpass_precision = encoder->channel[channel_number].lowpass_precision;

	lowpass_row_ptr = (uint8_t *)wavelet->data[LL_BAND];
	lowpass_pitch = wavelet->pitch;
    
#if VC5_ENABLED_PART(VC5_PART_SECTIONS)
    if (IsEncoderSectionEnabled(encoder, SECTION_NUMBER_SUBBAND))
    {
        // Make sure that the bitstream is aligned to a segment boundary
        AlignBitsSegment(stream);
        
        // Write the channel section header into the bitstream
        BeginSubbandSection(encoder, stream);
    }
#endif

	// Write the tag-value pairs for the lowpass band to the bitstream
	PutVideoLowpassHeader(encoder, channel_number, stream);

	// Check that the bitstream is tag aligned before writing the pixels
	assert(IsAlignedSegment(stream));

	for (row = 0; row < height; row++)
	{
		uint16_t *lowpass = (uint16_t *)lowpass_row_ptr;
		int column;

		for (column = 0; column < width; column++)
		{
			BITWORD coefficient = lowpass[column];
			//assert(0 <= lowpass[column] && lowpass[column] <= COEFFICIENT_MAX);
			assert(lowpass[column] <= COEFFICIENT_MAX);
			assert(coefficient <= COEFFICIENT_MAX);
			PutBits(stream, coefficient, lowpass_precision);
		}

		lowpass_row_ptr += lowpass_pitch;
	}

	// Align the bitstream to a segment boundary
	AlignBitsSegment(stream);

	PutVideoLowpassTrailer(stream);

	// Update the subband number in the codec state
	codec->subband_number++;

#if VC5_ENABLED_PART(VC5_PART_SECTIONS)
    if (IsEncoderSectionEnabled(encoder, SECTION_NUMBER_SUBBAND))
    {
        // Make sure that the bitstream is aligned to a segment boundary
        AlignBitsSegment(stream);
        
        // Update the section header with the actual size of the subband section
        EndSection(stream);
    }
#endif

    return CODEC_ERROR_OKAY;
}

/*!
	@brief Encode the highpass band into the bitstream

	The specified wavelet band is decoded from the bitstream
	using the codebook and encoding method specified in the
	bitstream.
*/
CODEC_ERROR EncodeHighpassBand(ENCODER *encoder, WAVELET *wavelet, int band, int subband, BITSTREAM *stream)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;
	CODEC_STATE *codec = &encoder->codec;

	DIMENSION band_width = wavelet->width;
	DIMENSION band_height = wavelet->height;

	void *band_data = wavelet->data[band];
	DIMENSION band_pitch = wavelet->pitch;

	QUANT quantization = wavelet->quant[band];
	//uint16_t scale = wavelet->scale[band];

	//int divisor = 0;
	//int peaks_coding = 0;

	CODESET *codeset = encoder->codeset;

	//int encoding_method = BAND_ENCODING_RUNLENGTHS;

	// Check that the band header starts on a tag boundary
	assert(IsAlignedTag(stream));
    
#if VC5_ENABLED_PART(VC5_PART_SECTIONS)
    if (IsEncoderSectionEnabled(encoder, SECTION_NUMBER_SUBBAND))
    {
        // Make sure that the bitstream is aligned to a segment boundary
        AlignBitsSegment(stream);
        
        // Write the channel section header into the bitstream
        BeginSubbandSection(encoder, stream);
    }
#endif

    // Output the tag-value pairs for this subband
	PutVideoSubbandHeader(encoder, subband, quantization, stream);

	// Encode the highpass coefficients for this subband into the bitstream
	error = EncodeHighpassBandRowRuns(stream, codeset, band_data, band_width, band_height, band_pitch);
	if (error != CODEC_ERROR_OKAY) {
		return error;
	}

	// Align the bitstream to a segment boundary
	AlignBitsSegment(stream);

	// Output the band trailer
	PutVideoSubbandTrailer(encoder, stream);

	// Update the subband number in the codec state
	codec->subband_number++;

#if VC5_ENABLED_PART(VC5_PART_SECTIONS)
    if (IsEncoderSectionEnabled(encoder, SECTION_NUMBER_SUBBAND))
    {
        // Make sure that the bitstream is aligned to a segment boundary
        AlignBitsSegment(stream);
        
        // Update the section header with the actual size of the subband section
        EndSection(stream);
    }
#endif

    return CODEC_ERROR_OKAY;
}

#if 0
/*!
	@brief Encode the highpass band into the bitstream

	The highpass band in the bitstream is decoded using the specified
	codebook.  This routine assumes that the highpass band was encoded
	using the run lengths encoding method which is the default for all
	current codec implementations.

	The encoded highpass band consists of signed values and runs of zeros.
	Each codebook entry specifies either an unsigned magnitude with a run
	length of one or a run of zeros.  The unsigned magnitude is immediately
	followed by the sign bit.

	Unsigned magnitudes always have a run length of one.

	Note that runs of zeros can straddle end of line boundaries.

	The end of the highpass band is marked by a special codeword.
	Special codewords in the codebook have a run length of zero.
	The value indicates the type or purpose of the special codeword.
*/
CODEC_ERROR EncodeHighpassBandLongRuns(BITSTREAM *stream, CODESET *codeset, PIXEL *data,
									   DIMENSION width, DIMENSION height, DIMENSION pitch)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;
	size_t data_count;
	size_t row_padding;
	int row = 0;
	int column = 0;
	size_t index = 0;
	//BITWORD special;

	// The encoder uses the codebooks for magnitudes and runs of zeros
	const MAGS_TABLE *mags_table = codeset->mags_table;
	const RUNS_TABLE *runs_table = codeset->runs_table;

	// The band is terminated by the band end codeword in the codebook
	const CODEBOOK *codebook = codeset->codebook;

	// Initialize the run length and value to zero
	RUN run = RUN_INITIALIZER;

	// Convert the pitch to units of pixels
	assert((pitch % sizeof(PIXEL)) == 0);
	pitch /= sizeof(PIXEL);

	// Check that the band dimensions are reasonable
	assert(width <= pitch);

	// Compute the number of values in the band to be encoded
	data_count = height * width;
	row_padding = pitch - width;

	while (data_count > 0)
	{
		// Read the next value to encode
		run.value = data[index++];
		run.count = 1;

		// Reduce the number of values to encode
		data_count--;

		// Advance to the next column in the band
		column++;

		// At the end of the row?
		if (column == width)
		{
			// Skip the padding at the end of the row
			index += row_padding;

			// Advance to the start of the next row in the band
			row++;
			column = 0;
		}

		if (run.value == 0)
		{
			PIXEL value = 0;

			// Count the number of zeros in the run
			while (value == 0 && data_count > 0)
			{
				// Get the next value
				value = data[index++];

				// Reduce the number of values to encode
				data_count--;

				// Advance to the next column
				column++;

				// At the end of the row?
				if (column == width)
				{
					// Skip the padding at the end of the row
					index += row_padding;

					// Advance to the start of the next row in the band
					row++;
					column = 0;
				}

				if (value == 0)
				{
					// Increment the number of zeros
					run.count++;
				}
			}

			//PutRun(stream, codebook, &run);
			//PutZeros(stream, codebook, run.count);
			error = PutZeros(stream, runs_table, run.count);
			if (error != CODEC_ERROR_OKAY) {
				return error;
			}

			// Did the loop terminate at a non-zero value?
			if (value == 0)
			{
				// Save the non-zero value to encode into the bitstream later
				run.value = value;
				run.count = 1;
			}
			else
			{
				// Should have encoded all of the data in the band
				assert(data_count == 0);
				break;
			}
		}

		// Should have a single non-zero value
		assert(run.value != 0 && run.count == 1);

		// Write a signed coefficient
		//PutValue(stream, codebook, run.value);
		PutValue(stream, mags_table, run.value);
		if (error != CODEC_ERROR_OKAY) {
			return error;
		}
	}

	// The last run should have ended at the end of the band
	assert(data_count == 0);

	// Insert the special codeword that marks the end of the highpass band
	error = PutSpecial(stream, codebook, SPECIAL_MARKER_BAND_END);

	return error;
}
#endif

/*!
	@brief Encode the highpass band from the bitstream

	This routine does not encode runs of zeros across row boundaries.
*/
CODEC_ERROR EncodeHighpassBandRowRuns(BITSTREAM *stream, CODESET *codeset, PIXEL *data,
									  DIMENSION width, DIMENSION height, DIMENSION pitch)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;
	size_t data_count;
	int row_padding;
	int row = 0;
	//int column = 0;
	//size_t index = 0;

	// The encoder uses the codebooks for magnitudes and runs of zeros
	const MAGS_TABLE *mags_table = codeset->mags_table;
	const RUNS_TABLE *runs_table = codeset->runs_table;

	// The band is terminated by the band end codeword in the codebook
	const CODEBOOK *codebook = codeset->codebook;

#if (0 && DEBUG)
	static int file_count = 0;
	char pathname[PATH_MAX];
	FILE *file = NULL;
#endif

	PIXEL *rowptr = data;
	int count = 0;

#if (0 && DEBUG)
	sprintf(pathname, "C:/Users/bschunck/Temp/Encode2/encode-%02d.dat", ++file_count);
	file = fopen(pathname, "w");
	stream->logfile = file;
	stream->putbits_flag = true;
#endif

	// Convert the pitch to units of pixels
	assert((pitch % sizeof(PIXEL)) == 0);
	pitch /= sizeof(PIXEL);

	// Check that the band dimensions are reasonable
	assert(width <= pitch);

	// Compute the number of values in the band to be encoded
	data_count = height * width;

	// Compute the number of values of padding at the end of each row
	row_padding = pitch - width;

	for (row = 0; row < height; row++)
	{
		int index = 0;			// Start at the beginning of the row

		// Search the row for runs of zeros and nonzero values
		while (index < width)
		{
			// Loop invariant
			assert(0 <= index && index < width);

			// Search the rest of the row for a nonzero value
			for (; index < width; index++) {
				if (rowptr[index] == 0) count++;
				else break;
			}

			// Need to output a value?
			if (index < width)
			{
				PIXEL value = rowptr[index];
				assert(value != 0);

				// Need to output a run of zeros before this value?
				if (count > 0)
				{
					error = PutZeros(stream, runs_table, count);
					if (error != CODEC_ERROR_OKAY) {
						return error;
					}

					// Reduce the number of values to encode (for debugging)
					data_count -= count;

					count = 0;
				}

				error = PutValue(stream, mags_table, value);
				if (error != CODEC_ERROR_OKAY) {
					return error;
				}

				// Reduce the number of values to encode (for debugging)
				data_count--;

				// Advance to the next column
				index++;
			}

			// Add the end of row padding to the encoded length
			if (index == width) count += row_padding;
		}

		// Should have processed the entire row
		assert(index == width);

		// Advance to the next row
		rowptr += pitch;
	}

	// Need to output a pending run of zeros?
	if (count > 0)
	{
		error = PutZeros(stream, runs_table, count);
		if (error != CODEC_ERROR_OKAY) {
			return error;
		}

		// Reduce the number of values to encode (for debugging)
		data_count -= count;

		count = 0;
	}
	
	// The last run should have ended at the end of the band
	assert(data_count == 0);

	// Insert the special codeword that marks the end of the highpass band
	error = PutSpecial(stream, codebook, SPECIAL_MARKER_BAND_END);

#if (0 && DEBUG)
	fclose(file);
	stream->putbits_flag = false;
	stream->logfile = NULL;
#endif

	return CODEC_ERROR_OKAY;
}

#if 0
/*!
	@brief Encode the band trailer that follows a highpass band

	This routine aligns the bitstream to a tag value boundary.
	Currently the band trailer does not perform any function beyond
	preparing the bitstream for reading the next tag value pair.
*/
CODEC_ERROR EncodeBandTrailer(BITSTREAM *stream)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;
	TAGVALUE segment;

	// Advance the bitstream to a tag boundary
	AlignBitsSegment(stream);

	// Write the debugging marker
	segment.tuple.tag = CODEC_TAG_BAND_TRAILER;
	segment.tuple.value = 0;
	return PutTagValue(stream, segment);
}
#endif


#if VC5_ENABLED_PART(VC5_PART_METADATA)

/*!
	@brief Encode metadata contained in the metadata file passed in the parameters into the bitstream

	@todo Modify the XML parser to output to a stream (binary file or memory buffer).
*/
CODEC_ERROR EncodeMetadataChunk(ENCODER *encoder, BITSTREAM *bitstream, const PARAMETERS *parameters)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;

	if (IsPartEnabled(encoder->enabled_parts, VC5_PART_METADATA))
	{
		// Was a metadata file provided on the command line?
		if (parameters->metadata_pathname[0] != '\0')
		{
			// FILE *metadata_file = fopen(parameters->metadata_pathname, "rb");
			// assert (metadata_file != NULL);
			// if (metadata_file == NULL) {
			// 	return CODEC_ERROR_FILE_OPEN;
			// }

			// Initialize the data passed to the XML element handlers
			PARSER_DATA parser_data;
			memset(&parser_data, 0, sizeof(parser_data));

			// Align the bitstream to a segment boundary
			AlignBitsSegment(bitstream);

			// The metadata parser needs a file pointer (not a memory buffer)
			assert(bitstream->stream->type == STREAM_TYPE_FILE);
			if (! (bitstream->stream->type == STREAM_TYPE_FILE)) {
				return CODEC_ERROR_UNEXPECTED;
			}

			// Flush the bitstream so that the metadata parser can write directly to the output file
			FlushBitstream(bitstream);

			// Write the metadata into the output stream (binary file or memory buffer)
			parser_data.output = bitstream->stream->location.file.iobuf;		
			parser_data.verbose = parameters->verbose_flag;
			parser_data.debug = parameters->debug_flag;

			// Parse the XML representation of the metadata and write the metadata chunk to the bitstream
			error = ParseMetadataFile(&parser_data, parameters->metadata_pathname);

			// Flush the output buffer
			FlushBitstream(bitstream);		
		}
		else
		{
			fprintf(stderr, "No metadata file provided on the command line\n");
			return CODEC_ERROR_FILE_OPEN;
		}
	}

	return error;
}


#if 0
/*!
	@brief Parse the XML representation of a metadata test case and write the metadata into the bitstream

	The XML representation of the metadata is described in ST 2073-7 Annex A.
*/
CODEC_ERROR WriteMetadataFile(ENCODER *encoder, BITSTREAM *bitstream, FILE *metadata_file)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;

	// Initialize the data passed to the XML element handlers
	PARSER_DATA parser_data;
	memset(&parser_data, 0, sizeof(parser_data));

	// Allocate a buffer for reading blocks of text from the metadata file
	char buffer[BUFSIZ];

	//XML_Parser parser = XML_ParserCreate("UTF-8");
	XML_Parser parser = XML_ParserCreate(NULL);

	// Initialize the XML parser with user data and element handlers
	XML_SetUserData(parser, &parser_data);
	XML_SetElementHandler(parser, ElementStartHandler, ElementEndHandler);
	XML_SetCharacterDataHandler(parser, CharacterDataHandler);
	XML_SetCdataSectionHandler(parser, CDATA_SectionStart, CDATA_SectionEnd);

	// Open the input file that contains the XML representation of metadata
	FILE *input = fopen(pathname, "r");
	if (input == NULL) {
		fprintf(stderr, "Could not open input filename: %s\n", pathname);
		error = CODEC_ERROR_FILE_OPEN;
	}
	else
	{
		// Process the XML file one buffer of data at a time

		bool done;

		do {
			size_t length = fread(buffer, 1, sizeof(buffer), input);
			done = length < sizeof(buffer);
			if (XML_Parse(parser, buffer, (int)length, done) == XML_STATUS_ERROR) {
				fprintf(stderr,
					"%" XML_FMT_STR " at line %" XML_FMT_INT_MOD "u\n",
					XML_ErrorString(XML_GetErrorCode(parser)),
					XML_GetCurrentLineNumber(parser));
				error = CODEC_ERROR_XML_PARSER;
				break;
			}
		} while (!done);

		// Close the input file and the XML parser
		fclose(input);
	}

	XML_ParserFree(parser);

	return error;
}
#endif

#endif


#if _DEBUG

/*!
	@brief Write selected subbands to a wavelet band file

	Each bit in the subband mask argument specifies whether the corresponding
	subband should be written to the wavelet band file.

	Note that the band file can contain reconstructed lowpass bands, but this
	routine only write decoded subbands to the wavelet band file.

	@todo Change the channel index into a channel mask
*/
CODEC_ERROR DumpEncodedSubbands(ENCODER *encoder,
								int channel_index,
								uint32_t subband_mask,
								const char *pathname)
{
	BANDFILE file;
	int max_band_width;
	int max_band_height;
	int subband;

	//TODO: Modify this routine to take the image index as an argument
	const int frame_index = 0;

	// Compute the maximum dimensions of each subband
	max_band_width = encoder->image_width;
	max_band_height = encoder->image_height;

	// Create the band file
	CreateBandFile(&file, pathname);

	// Write the band file header
	WriteFileHeader(&file, max_band_width, max_band_height);

	for (subband = 0; subband_mask != 0; subband_mask >>= 1, subband++)
	{
		// Write this subband?
		if ((subband_mask & 0x01) != 0)
		{
			int wavelet_index = SubbandWaveletIndex(subband);
			int band_index = SubbandBandIndex(subband);
			WAVELET *wavelet = encoder->transform[channel_index].wavelet[wavelet_index];
			DIMENSION width = wavelet->width;
			DIMENSION height = wavelet->height;
			void *data = wavelet->data[band_index];
			size_t size = width * height * sizeof(PIXEL);

			WriteWaveletBand(&file, frame_index, channel_index, wavelet_index,
				band_index, BAND_TYPE_SINT16, width, height, data, size);
		}
	}

	CloseBandFile(&file);

	return CODEC_ERROR_OKAY;
}



/*!
	@brief Write selected bands in a wavelet to a band file
*/
CODEC_ERROR DumpWaveletBands(ENCODER *encoder,
							 int channel_index,
							 int wavelet_index,
							 uint32_t band_mask,
							 const char *pathname)
{
	BANDFILE file;
	WAVELET *wavelet;
	int max_band_width;
	int max_band_height;
	int band_index;

	//TODO: Modify this routine to take the frame index as an argument
	const int frame_index = 0;

	if (encoder == NULL) {
		return CODEC_ERROR_NULLPTR;
	}

	if (! (0 <= channel_index && channel_index < encoder->channel_count)) {
		return CODEC_ERROR_BAD_ARGUMENT;
	}

	if (! (0 <= wavelet_index && wavelet_index < encoder->wavelet_count)) {
		return CODEC_ERROR_BAD_ARGUMENT;
	}

	// Get the specified wavelet from the transform
	wavelet = encoder->transform[channel_index].wavelet[wavelet_index];
	assert(wavelet != NULL);

	// Compute the maximum dimensions of each subband
	max_band_width = encoder->image_width;
	max_band_height = encoder->image_height;

	// Create the band file
	CreateBandFile(&file, pathname);

	// Write the band file header
	WriteFileHeader(&file, max_band_width, max_band_height);

	for (band_index = 0; band_mask != 0; band_mask >>= 1, band_index++)
	{
		// Write this subband?
		if ((band_mask & 0x01) != 0)
		{
			DIMENSION width = wavelet->width;
			DIMENSION height = wavelet->height;
			void *data = wavelet->data[band_index];
			size_t size = width * height * sizeof(PIXEL);

			WriteWaveletBand(&file, frame_index, channel_index, wavelet_index,
				band_index, BAND_TYPE_SINT16, width, height, data, size);
		}
	}

	CloseBandFile(&file);

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Write selected wavelet bands to a band file
*/
CODEC_ERROR DumpTransformBands(ENCODER *encoder,
							   //int channel_index,
							   uint32_t channel_mask,
							   uint32_t channel_wavelet_mask,
							   uint32_t wavelet_band_mask,
							   const char *pathname)
{
	BANDFILE file;
	int max_band_width;
	int max_band_height;
	int channel_count;
	int channel_index;

	//TODO: Modify this routine to take the frame index as an argument
	const int frame_index = 0;

	// Get the number of channels in the encoder wavelet transform
	channel_count = encoder->channel_count;

	// Compute the maximum dimensions of each subband
	max_band_width = encoder->image_width;
	max_band_height = encoder->image_height;

	// Create the band file
	CreateBandFile(&file, pathname);

	// Write the band file header
	WriteFileHeader(&file, max_band_width, max_band_height);

	for (channel_index = 0;
		 channel_index < channel_count && channel_mask != 0;
		 channel_mask >>= 1, channel_index++)
	{
		uint32_t wavelet_mask = channel_wavelet_mask;
		int wavelet_count = encoder->wavelet_count;
		int wavelet_index;

		for (wavelet_index = 0;
			 wavelet_index < wavelet_count && wavelet_mask != 0;
			 wavelet_mask >>= 1, wavelet_index++)
		{
			// Write bands in this wavelet?
			if ((wavelet_mask & 0x01) != 0)
			{
				WAVELET *wavelet = encoder->transform[channel_index].wavelet[wavelet_index];
				uint32_t band_mask = wavelet_band_mask;
				int band_count = wavelet->band_count;
				int band_index;

				for (band_index = 0;
					 band_index < band_count && band_mask != 0;
					 band_mask >>= 1, band_index++)
				{
					// Write this band in the wavelet?
					if ((band_mask & 0x01) != 0)
					{
						DIMENSION width = wavelet->width;
						DIMENSION height = wavelet->height;
						void *data = wavelet->data[band_index];
						size_t size = wavelet->width * wavelet->height * sizeof(PIXEL);

						WriteWaveletBand(&file, frame_index, channel_index, wavelet_index,
							band_index, BAND_TYPE_SINT16, width, height, data, size);
					}
				}
			}
		}
	}

	CloseBandFile(&file);

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Create a band data file for the encoded highpass bands
*/
CODEC_ERROR CreateEncodedBandFile(ENCODER *encoder, const char *pathname)
{
	ALLOCATOR *allocator = encoder->allocator;
	BITSTREAM *encoded_bitstream;
	STREAM *encoded_stream;
	int max_band_width;
	int max_band_height;
	void *encoded_band_buffer;
	size_t encoded_band_size;

	// Use the encoded dimensions to maximum dimensions of each subband
	max_band_width = encoder->image_width;
	max_band_height = encoder->image_height;
	encoded_band_size = max_band_width * max_band_height;

	// Create the band file
	CreateBandFile(&encoder->encoded_band_file, pathname);

	// Write the band file header
	WriteFileHeader(&encoder->encoded_band_file, max_band_width, max_band_height);

	// Allocate a buffer for the encoded band data
	encoded_band_buffer = Alloc(allocator, encoded_band_size);

	// Create a new bitstream for the encoded band data
	encoded_bitstream = Alloc(allocator, sizeof(BITSTREAM));
	InitBitstream(encoded_bitstream);

	// Redirect the encoded bitstream to a buffer
	encoded_stream = Alloc(allocator, sizeof(STREAM));
	CreateStreamBuffer(encoded_stream, encoded_band_buffer, encoded_band_size);
	AttachBitstream(encoded_bitstream, encoded_stream);
	encoder->encoded_band_bitstream = encoded_bitstream;

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Close the bitstream used for encoded band data
*/
CODEC_ERROR CloseEncodedBandFile(ENCODER *encoder)
{
	CloseBandFile(&encoder->encoded_band_file);
	return CODEC_ERROR_OKAY;
}

#endif
