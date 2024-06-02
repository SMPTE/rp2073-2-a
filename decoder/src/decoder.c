/*!	@file decoder/src/decoder.c

	Implementation of functions for decoding samples.

	Encoded samples must be aligned on a four byte boundary.
	Any constraints on the alignment of data within the sample
	are handled by padding the sample to the correct alignment.

	TODO: Is this restriction still true?

	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

//! Control debug statements in this module
#ifndef DEBUG
#define DEBUG (1 && _DEBUG)
#endif

#include "headers.h"
#include "bandfile.h"


/*!
	@brief Initialize the decoder data structure

	This routine performs the same function as a C++ constructor.
	The decoder is initialized with default values that are replaced
	by the parameters used to prepare the decoder (see @ref PrepareDecoder).

	This routine does not perform all of the initializations required
	to prepare the decoder data structure for decoding a sample.
*/
CODEC_ERROR InitDecoder(DECODER *decoder, ALLOCATOR *allocator)
{
	assert(decoder != NULL);
	if (! (decoder != NULL)) {
		return CODEC_ERROR_NULLPTR;
	}

	memset(decoder, 0, sizeof(DECODER));

	// Assign a memory allocator to the decoder
	decoder->allocator = allocator;

	return CODEC_ERROR_OKAY;
}

#if VC5_ENABLED_PART(VC5_PART_METADATA)

/*!
    @brief Create and initialize an instance of a metadata database
*/
CODEC_ERROR InitMetadataDatabase(DATABASE **database_out, const PARAMETERS *parameters, bool duplicates_flag)
{
    CODEC_ERROR error = CODEC_ERROR_OKAY;

    // Create fake command-line arguments for routines in the XML dumper
    //ARGUMENTS args;
    // memset(&args, 0, sizeof(args));
    // args.verbose_flag = parameters->verbose_flag;
    // args.debug_flag = parameters->debug_flag;

    //bool duplicates_flag = false;       /****DEBUG****/

    DATABASE *database = NULL;
    error = CreateMetadataDatabase(&database, parameters->verbose_flag, parameters->debug_flag, duplicates_flag);
    assert(error == CODEC_ERROR_OKAY);
    if (! (error == CODEC_ERROR_OKAY)) {
        return error;
    }

    // Return the new database instance
    *database_out = database;

    return error;
}

#endif

#if _DEBUG
/*!
	@brief Set the logfile for debug output
*/
CODEC_ERROR SetDecoderLogfile(DECODER *decoder, FILE *logfile)
{
	assert(decoder != NULL);
	if (! (decoder != NULL)) {
		return CODEC_ERROR_NULLPTR;
	}

	decoder->logfile = logfile;

	return CODEC_ERROR_OKAY;
}
#endif

/*!
	@brief Release resources allocated by the decoder

	Note that this routine does not close the logfile.
*/
CODEC_ERROR ReleaseDecoder(DECODER *decoder)
{
	// Free the wavelet transforms and decoding buffers
	ReleaseDecoderTransforms(decoder);
	ReleaseDecoderBuffers(decoder);

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Decode the bitstream encoded into the byte stream into separate component arrays

	This is a convenience routine for applications that use the byte stream data structure
	for bitstreams stored in a file or memory buffer.

	The main entry point for decoding a bitstream is @ref DecodingProcess.

	The parameters data structure is intended to simulate information that may be available
	to the decoder from the media container or an external application.

	This routine assumes that the unpacked image has already been initialized.
*/
CODEC_ERROR DecodeStream(STREAM *stream, UNPACKED_IMAGE *unpacked_image, DATABASE *database, const PARAMETERS *parameters)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;
	BITSTREAM bitstream;
	DECODER decoder;

    //printf("Decode stream database: %p\n", database);

#if (0 && DEBUG)
	//! See the module @ref bandfile.c
	uint32_t channel_mask = parameters->bandfile.channel_mask;
	uint32_t subband_mask = parameters->bandfile.subband_mask;
	const char *pathname = parameters->bandfile.pathname;
#endif

#if VC5_ENABLED_PART(VC5_PART_METADATA)
    // Save the metadata database in the decoder data structure
    decoder.metadata.database = database;
#endif

	// Initialize the bitstream data structure
	InitBitstream(&bitstream);

	// Bind the bitstream to the byte stream
	error = AttachBitstream(&bitstream, stream);
	if (error != CODEC_ERROR_OKAY) {
		return error;
	}

#if _DEBUG
	// Provide a file for debug output
	SetDecoderLogfile(&decoder, stdout);
#endif

	// Decode the bitstream sample into a image buffer
	error = DecodingProcess(&decoder, &bitstream, unpacked_image, database, parameters);

#if (0 && DEBUG)
	// Print the quantization values used for decoding (for debugging)
	PrintDecoderQuantization(&decoder);
#endif
#if (0 && DEBUG)
	// Write the decoded component arrays (for debugging)
	WriteUnpackedImage(&unpacked_image, EncodedPixelFormat(&decoder, parameters), parameters-> enabled_parts, "output.dpx");
#endif
#if (0 && DEBUG)
	DumpTransformSubbands(&decoder, channel_mask, subband_mask, pathname);
#endif

	// Release any resources allocated by the decoder
	ReleaseDecoder(&decoder);

	// Release any resources allocated by the bitstream
	ReleaseBitstream(&bitstream);

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Decode the bitstream encoded into the byte stream

	This is a convenience routine for applications that use the byte
	stream data structure for samples stored in a file or memory buffer.
	The main entry point for decoding a bitstream is @ref DecodingProcess.

	The parameters data structure is intended to simulate information that
	may be available to the decoder from the media container or an external
	application.
*/
CODEC_ERROR DecodeImage(STREAM *stream, IMAGE *packed_image, DATABASE *database, const PARAMETERS *parameters)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;
	BITSTREAM bitstream;
	DECODER decoder;
	DIMENSION packed_width;
	DIMENSION packed_height;
	PIXEL_FORMAT packed_format;

    //if (parameters->debug_flag) printf("DecodeImage database: %p\n", database);

	// The unpacked image will hold the component arrays decoded from the bitstream
	UNPACKED_IMAGE unpacked_image;

    // Metadata database for VC-5 Part 7
    // DATABASE *database = NULL;
    // bool duplicates_flag = false;       /*****DEBUG*****/

#if (0 && DEBUG)
	//! See the module @ref bandfile.c
	uint32_t channel_mask = parameters->bandfile.channel_mask;
	uint32_t subband_mask = parameters->bandfile.subband_mask;
	const char *pathname = parameters->bandfile.pathname;
#endif

	// Initialize the bitstream data structure
	InitBitstream(&bitstream);

	// Bind the bitstream to the byte stream
	error = AttachBitstream(&bitstream, stream);
	if (error != CODEC_ERROR_OKAY) {
		return error;
	}

#if _DEBUG
	// Provide a file for debug output
	SetDecoderLogfile(&decoder, stdout);
#endif
    
    // The component arrays will be allocated after the bitstream is decoded
    InitUnpackedImage(&unpacked_image);

// #if VC5_ENABLED_PART(VC5_PART_METADATA)

    // if (IsPartEnabled(parameters->enabled_parts, VC5_PART_METADATA) && parameters->metadata.output_flag)
    // {
    //     // Initialize the metadata database
    //     error = InitMetadataDatabase(&database, parameters, duplicates_flag);
    //     if (error != CODEC_ERROR_OKAY) {
    //         return error;
    //     }
    // }

    // if (parameters->debug_flag) {
    //     printf("Metadata database: %p\n", database);
    // }

// #endif

	// Decode the bitstream sample into a image buffer
	error = DecodingProcess(&decoder, &bitstream, &unpacked_image, database, parameters);

#if (0 && DEBUG)
	// Print the quantization values used for decoding (for debugging)
	PrintDecoderQuantization(&decoder);
#endif
#if (0 && DEBUG)
	// Write the decoded component arrays (for debugging)
	WriteUnpackedImage(&unpacked_image, EncodedPixelFormat(&decoder, parameters), parameters->enabled_parts, "output.dpx");
#endif

	// The dimensions and format for the output of the image packing process
	SetOutputImageFormat(&decoder, parameters, &packed_width, &packed_height, &packed_format);

	// Allocate the image buffer for output of the image packing process
	AllocImage(decoder.allocator, packed_image, packed_width, packed_height, packed_format);

	// Pack the component arrays into the output image
	ImageRepackingProcess(&unpacked_image, packed_image, database, parameters);

#if (0 && DEBUG)
	DumpTransformSubbands(&decoder, channel_mask, subband_mask, pathname);
#endif

// #if VC5_ENABLED_PART(VC5_PART_METADATA)
//     if (IsPartEnabled(parameters->enabled_parts, VC5_PART_METADATA) && parameters->metadata.output_flag)
//     {
//         printf("Metadata file: %s\n", parameters->metadata.output_pathname);

//         // Write the metadata database to the output file in XML format
//         FILE *output_file = fopen(parameters->metadata.output_pathname, "w");
//         assert(output_file != NULL);
//         if (! (output_file != NULL)) {
//             fprintf(stderr, "Could not open metadata output file: %s\n", parameters->metadata.output_pathname);
//             return CODEC_ERROR_FILE_OPEN;
//         }

//         error = OutputMetadataDatabase(database, output_file, NULL);
//         if (error != CODEC_ERROR_OKAY) {
//             return error;
//         }

//         // Close the metadata output file
//         fclose(output_file);

//         // Free the metadata database
//         if (database != NULL) {
//             DestroyMetadataDatabase(database);
//         }
//     }
// #endif

	// Release any resources allocated by the decoder
	ReleaseDecoder(&decoder);

	// Release any resources allocated by the bitstream
	ReleaseBitstream(&bitstream);

	return CODEC_ERROR_OKAY;
}


#if VC5_ENABLED_PART(VC5_PART_LAYERS)

/*!
    @brief Decode the next layer in the bitstream
 
    The decoder and bitstream are not reinitialized so that each layer can be decoded from the bitsteam
    using the same decoding parameters.  The codec state is preserved between layers.
*/
CODEC_ERROR DecodeLayer(DECODER *decoder, BITSTREAM *input, PACKED_IMAGE *output, DATABASE *database, const PARAMETERS *parameters)
{
   	CODEC_ERROR error = CODEC_ERROR_OKAY;
    DIMENSION packed_width;
    DIMENSION packed_height;
    PIXEL_FORMAT packed_format;
    
    // The unpacked image will hold the component arrays decoded for each layer in the bitstream
    UNPACKED_IMAGE unpacked_image;
   
    // The component arrays will be allocated after the bitstream is decoded
    InitUnpackedImage(&unpacked_image);

    // DATABASE *database = NULL;
    // bool duplicates_flag = false;       /*****DEBUG*****/

// #if VC5_ENABLED_PART(VC5_PART_METADATA)

//     if (IsPartEnabled(parameters->enabled_parts, VC5_PART_METADATA) && parameters->metadata.output_flag)
//     {
//         // Initialize the metadata database
//         error = InitMetadataDatabase(&database, parameters, duplicates_flag);
//         if (error != CODEC_ERROR_OKAY) {
//             return error;
//         }
//     }

//     // if (parameters->debug_flag) {
//     //     printf("Metadata database: %p\n", database);
//     // }

// #endif

    //*****

    //error = DecodeSingleImage(decoder, stream, image);
    // Process tag value pairs until the layer has been decoded
    for (;;)
    {
        TAGVALUE segment;
        
        // Read the next tag value pair from the bitstream
        segment = GetSegment(input);
        assert(input->error == BITSTREAM_ERROR_OKAY);
        if (input->error != BITSTREAM_ERROR_OKAY) {
            decoder->error = CodecErrorBitstream(input->error);
            return decoder->error;
            break;
        }
        
        error = UpdateCodecState(decoder, input, segment);
        if (error != CODEC_ERROR_OKAY) {
            return error;
        }
        
        // Processed all wavelet bands in all channels?
        if (IsDecodingComplete(decoder)) {
            break;
        }
    }

#if VC5_ENABLED_PART(VC5_PART_METADATA)
    if (IsPartEnabled(decoder->enabled_parts, VC5_PART_METADATA))
    {
        // Does the remainder of the bitstream contain a metadata chunk element?
        TAGVALUE segment = GetSegment(input);
        while (input->error == BITSTREAM_ERROR_OKAY)
        {
            error = UpdateCodecState(decoder, input, segment);
            if (error != CODEC_ERROR_OKAY) {
                return error;
            }

            // More data in the bitstream?
            segment = GetSegment(input);
        }

        // Output the metadata database in XML format
        // FILE *output_file = fopen(decoder->metadata.output_pathname, "w");
        // if (output_file == NULL) {
        //     return CODEC_ERROR_FILE_CREATE;
        // }

        // error = OutputMetadataDatabase(decoder->metadata.database, output_file, NULL);

        // fclose(output_file);

        // if (error != CODEC_ERROR_OKAY) {
        //     return error;
        // }

        //TODO: Need to free the items in the metadata database
    }
#endif
    
    // Parsed the bitstream without errors?
    if (error != CODEC_ERROR_OKAY) {
        return error;
    }
    
#if (0 && DEBUG)
    printf("Encoded precision: %d\n\n", decoder->codec.bits_per_component);
#endif
    
#if (0 && DEBUG)
    // Output the lowpass bands at the highest wavelet level (for debugging)
    WriteLowpassBands(decoder, 3, "lowpass%d.dpx");
#endif
    
    // Reconstruct the output image using the last decoded wavelet in each channel
    error = ReconstructUnpackedImage(decoder, &unpacked_image);
    if (error != CODEC_ERROR_OKAY) {
        return error;
    }
    
    //*****
    
#if (0 && DEBUG)
    // Print the quantization values used for decoding (for debugging)
    PrintDecoderQuantization(&decoder);
#endif
#if (1 && DEBUG)
    // Write the decoded component arrays (for debugging)
    WriteUnpackedImage(&unpacked_image, EncodedPixelFormat(decoder, parameters), parameters->enabled_parts, "output.dpx");
#endif
    
    // The dimensions and format for the output of the image packing process
    SetOutputImageFormat(decoder, parameters, &packed_width, &packed_height, &packed_format);
    
    // Allocate the image buffer for output of the image packing process
    AllocImage(decoder->allocator, output, packed_width, packed_height, packed_format);
    
    // Pack the component arrays into the output image
    ImageRepackingProcess(&unpacked_image, output, database, parameters);
    
#if (0 && DEBUG)
    DumpTransformSubbands(&decoder, channel_mask, subband_mask, pathname);
#endif

    return error;
}

/*!
    @brief Update layer parameters in the decoder and codec state after decoding a layer
*/
CODEC_ERROR UpdateLayerParameters(DECODER *decoder)
{
    // Indicate that another layer has been decoded
    decoder->decoded_layer_count++;
    
    // Set the default number of the next layer in the bitstream
    decoder->codec.layer_number++;

    return CODEC_ERROR_OKAY;
}

/*!
 @brief Reset the valid band mask in all wavelets in all channels
 
 Must reset the flags that indicate that the wavelets have been decoded so that
 the wavelet bands for the next layer will be decoded.
 */
CODEC_ERROR ResetWaveletDecodingFlags(DECODER *decoder)
{
    int channel_count = decoder->codec.channel_count;
    int channel_index;
    
    for (channel_index = 0; channel_index < channel_count; channel_index++)
    {
        int wavelet_count = decoder->wavelet_count;
        int wavelet_index;
        
        for (wavelet_index = 0; wavelet_index < wavelet_count; wavelet_index++)
        {
            WAVELET *wavelet = decoder->transform[channel_index].wavelet[wavelet_index];
        
            // The valid band mask will be cleared when the wavelet is allocated
            if (wavelet == NULL) continue;
        
            // Reset the valid band mask to force decoding all wavelet bands
            ResetWaveletValidBandMask(wavelet);
        }
    }
    
    // The valid band mask in all wavelets have been cleared
    return CODEC_ERROR_OKAY;
}

/*!
    @brief Return true of all layers that should be decoded have been decoded
 
    Depending on the command-line arguments, the decoder will either process only
    the first layer in the bitstream or process all layers in the bitstream.
*/
bool AllLayersDecoded(DECODER *decoder)
{
    // Decoding all layers in the bitstream?
    if (decoder->decode_all_layers_flag)
    {
        // Indicate whether all layers have been decoded
        return (decoder->decoded_layer_count == decoder->codec.layer_count);
    }
    
    // Indicate whether the first layer has been decoded
    return (decoder->decoded_layer_count == 1);
}

#endif



#if VC5_ENABLED_PART(VC5_PART_SECTIONS)

/*!
     @brief Decode the next image section in the bitstream
     
     This routine assumes that there are multiple image sections in the bitstream.  If the bitstream does
     not contain image sections, then this routine will decode the single image encoded in the bitstream.
 
    If the image section contains layers, then this routine is called multiple times to decoder each layer.
*/
CODEC_ERROR DecodeImageSection(DECODER *decoder, BITSTREAM *input, PACKED_IMAGE *output, DATABASE *database, const PARAMETERS *parameters)
{
   	CODEC_ERROR error = CODEC_ERROR_OKAY;
    DIMENSION packed_width;
    DIMENSION packed_height;
    PIXEL_FORMAT packed_format;
    
    // The unpacked image will hold the component arrays decoded for each layer in the bitstream
    UNPACKED_IMAGE unpacked_image;
    
    // The component arrays will be allocated after the bitstream is decoded
    InitUnpackedImage(&unpacked_image);
    
    // DATABASE *database = NULL;
    // bool duplicates_flag = false;       /*****DEBUG*****/

// #if VC5_ENABLED_PART(VC5_PART_METADATA)

//     if (IsPartEnabled(parameters->enabled_parts, VC5_PART_METADATA) && parameters->metadata.output_flag)
//     {
//         // Initialize the metadata database
//         error = InitMetadataDatabase(&database, parameters, duplicates_flag);
//         if (error != CODEC_ERROR_OKAY) {
//             return error;
//         }
//     }

//     // if (parameters->debug_flag) {
//     //     printf("Metadata database: %p\n", database);
//     // }

// #endif

    for (;;)
    {
        TAGVALUE segment;
        
        // Read the next tag value pair from the bitstream
        segment = GetSegment(input);
        if (input->error == BITSTREAM_ERROR_EOF) {
            return CodecErrorBitstream(input->error);
        }
        assert(input->error == BITSTREAM_ERROR_OKAY);
        if (input->error != BITSTREAM_ERROR_OKAY) {
            decoder->error = CodecErrorBitstream(input->error);
            return decoder->error;
            break;
        }
        
        error = UpdateCodecState(decoder, input, segment);
        if (error != CODEC_ERROR_OKAY) {
            return error;
        }
        
        // Processed all wavelet bands in all channels?
        if (IsDecodingComplete(decoder)) {
            break;
        }
    }

#if VC5_ENABLED_PART(VC5_PART_METADATA)
    if (IsPartEnabled(decoder->enabled_parts, VC5_PART_METADATA))
    {
        // Does the remainder of the bitstream contain a metadata chunk element?
        TAGVALUE segment = GetSegment(input);
        while (input->error == BITSTREAM_ERROR_OKAY)
        {
            error = UpdateCodecState(decoder, input, segment);
            if (error != CODEC_ERROR_OKAY) {
                return error;
            }

            // More data in the bitstream?
            segment = GetSegment(input);
        }

        // Output the metadata database in XML format
        // FILE *output_file = fopen(decoder->metadata.output_pathname, "w");
        // if (output_file == NULL) {
        //     return CODEC_ERROR_FILE_CREATE;
        // }

        // error = OutputMetadataDatabase(decoder->metadata.database, output_file, NULL);

        // fclose(output_file);

        // if (error != CODEC_ERROR_OKAY) {
        //     return error;
        // }

        //TODO: Need to free the items in the metadata database
    }
#endif
    
    // Parsed the bitstream without errors?
    if (error != CODEC_ERROR_OKAY) {
        return error;
    }
    
    // Leave the bitstream aligned to a segment boundary
    AlignBitsSegment(input);
    
#if (0 && DEBUG)
    printf("Encoded precision: %d\n\n", decoder->codec.bits_per_component);
#endif
    
#if (0 && DEBUG)
    // Output the lowpass bands at the highest wavelet level (for debugging)
    WriteLowpassBands(decoder, 3, "lowpass%d.dpx");
#endif
    
    // Reconstruct the output image using the last decoded wavelet in each channel
    error = ReconstructUnpackedImage(decoder, &unpacked_image);
    if (error != CODEC_ERROR_OKAY) {
        return error;
    }
    
    //*****
    
#if (0 && DEBUG)
    // Print the quantization values used for decoding (for debugging)
    PrintDecoderQuantization(&decoder);
#endif
#if (0 && DEBUG)
    // Write the decoded component arrays (for debugging)
    WriteUnpackedImage(&unpacked_image, EncodedPixelFormat(decoder, parameters), parameters->enabled_parts, "output.dpx");
#endif
    
    // The dimensions and format for the output of the image packing process
    SetOutputImageFormat(decoder, parameters, &packed_width, &packed_height, &packed_format);
    
    // Allocate the image buffer for output of the image packing process
    AllocImage(decoder->allocator, output, packed_width, packed_height, packed_format);
    
    // Pack the component arrays into the output image
    ImageRepackingProcess(&unpacked_image, output, database, parameters);
    
#if (0 && DEBUG)
    DumpTransformSubbands(&decoder, channel_mask, subband_mask, pathname);
#endif

// #if VC5_ENABLED_PART(VC5_PART_METADATA)
//     if (IsPartEnabled(parameters->enabled_parts, VC5_PART_METADATA) && parameters->metadata.output_flag)
//     {
//         // Write the metadata database to the output file in XML format
//         FILE *output_file = fopen(parameters->metadata.output_pathname, "w");
//         assert(output_file != NULL);
//         if (! (output_file != NULL)) {
//             fprintf(stderr, "Could not open metadata output file: %s\n", parameters->metadata.output_pathname);
//             return CODEC_ERROR_FILE_OPEN;
//         }

//         error = OutputMetadataDatabase(database, output_file, NULL);
//         if (error != CODEC_ERROR_OKAY) {
//             return error;
//         }

//         // Close the metadata output file
//         fclose(output_file);

//         // Free the metadata database
//         if (database != NULL) {
//             DestroyMetadataDatabase(database);
//         }
//     }
// #endif

    return error;
}

/*!
 @brief Update decoding parameters and the codec state after decoding an image
 
 Assume that the next image section will have different dimensions and format.
 Deallocate decoder memory so that it can be reallocated with the correct dimensions.
 */
CODEC_ERROR ResetDecoderImageSection(DECODER *decoder, ALLOCATOR *allocator, PARAMETERS *parameters)
{
    assert(decoder != NULL);
    if (! (decoder != NULL)) {
        return CODEC_ERROR_UNEXPECTED;
    }
    
    ReleaseDecoderTransforms(decoder);
    ReleaseDecoderBuffers(decoder);
    
    // Partially reinitialize the decoder
    decoder->memory_allocated = false;
    decoder->header_finished = false;
    decoder->header_mask = 0;
    
    //TODO: Should tag-value pairs to reset the channel and subband numbers be encoded in the bitstream?
    decoder->codec.channel_number = 0;
    decoder->codec.subband_number = 0;
    
    // Clear the table of information about each decoded channel
    memset(decoder->channel, 0, sizeof(decoder->channel));
    
    if (parameters != NULL)
    {
#if VC5_ENABLED_PART(VC5_PART_LAYERS)
        if (IsPartEnabled(decoder->enabled_parts, VC5_PART_LAYERS))
        {
            decoder->decoded_layer_count = 0;
            //decoder->decode_all_layers_flag = parameters->layer_flag;
            decoder->decode_all_layers_flag = true;
        }
#endif
        
        // Reset the output image file format
        parameters->output.format = PIXEL_FORMAT_UNKNOWN;
    }

    return CODEC_ERROR_OKAY;
}

/*!
 @brief Return true there are no more image sections in the bitstream
 
 Depending on the command-line arguments, the decoder will either process only the
 first image section in the bitstream or process all image sections in the bitstream.
 */
bool AllImageSectionsDecoded(DECODER *decoder)
{
    // There may be more image sections in the bitstream if an image section was found
    return !decoder->image_section_flag;
}

#endif


/*!
	@brief Initialize the decoder using the specified parameters

	@todo Add more error checking to this top-level routine
*/
CODEC_ERROR PrepareDecoder(DECODER *decoder, ALLOCATOR *allocator, DATABASE *database, const PARAMETERS *parameters)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;

	// Initialize the decoder data structure
	error = InitDecoder(decoder, allocator);
	if (error != CODEC_ERROR_OKAY) {
		return error;
	}

	// Set the mask that specifies which parts of the VC-5 standard are supported
	decoder->enabled_parts = parameters->enabled_parts;

	// Verify that the enabled parts are correct
	error = VerifyEnabledParts(decoder->enabled_parts);
	assert(error == CODEC_ERROR_OKAY);
	if (! (error == CODEC_ERROR_OKAY)) {
		return error;
	}

	// Initialize the codec state (allocation routines use the codec state)
	error = PrepareDecoderState(decoder, parameters);
	if (error != CODEC_ERROR_OKAY) {
		return error;
	}

	//TODO: Do the parameters need to be copied to the decoder in this routine?

	if (parameters != NULL)
	{
#if 0	//VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
		// The encoded format should have been provided
		if (decoder->encoded.format == ENCODED_FORMAT_UNKNOWN) {
			decoder->encoded.format = DefaultEncodedFormat();
		}
#endif

        // Set the flags that control output to the terminal
        decoder->verbose_flag = parameters->verbose_flag;
        decoder->debug_flag = parameters->debug_flag;
        decoder->quiet_flag = parameters->quiet_flag;

#if VC5_ENABLED_PART(VC5_PART_LAYERS)
        if (IsPartEnabled(parameters->enabled_parts, VC5_PART_LAYERS))
        {
            // Decode all layers in the bitstream
            //decoder->decode_all_layers_flag = parameters->layer_flag;
            decoder->decode_all_layers_flag = true;
            
            // Reset the count of layers that have been decoded
            decoder->decoded_layer_count = 0;
        }
#endif
#if VC5_ENABLED_PART(VC5_PART_SECTIONS)
      if (IsPartEnabled(decoder->enabled_parts, VC5_PART_SECTIONS))
      {
            decoder->enabled_sections = parameters->enabled_sections;

            if (parameters->enabled_sections)
            {
              // Open a log file for writing information about sections encountered in the bitstream
              decoder->section_logfile = fopen(parameters->sections.logfile_pathname, "w");
              assert(decoder->section_logfile != NULL);
              if (! (decoder->section_logfile != NULL)) {
                  return CODEC_ERROR_OPEN_FILE_FAILED;
              }
            }
      }
#endif
#if VC5_ENABLED_PART(VC5_PART_METADATA)
        //printf("PrepareDecoder database: %p\n", decoder->metadata.database);
        if (IsPartEnabled(decoder->enabled_parts, VC5_PART_METADATA))
        {
            if (parameters->metadata.output_flag && database != NULL)
            {
                decoder->metadata.database = database;

                // decoder->metadata.verbose_flag = database->verbose_flag;
                // decoder->metadata.debug_flag = database->debug_flag;
                // decoder->metadata.quiet_flag = database->quiet_flag;

                // No parameters provided for setting the remove duplicates flag
                //decoder->metadata.duplicates_flag = true;
                // decoder->metadata.duplicates_flag = database->duplicates_flag;

                strcpy(decoder->metadata.output_pathname, parameters->metadata.output_pathname);
            }
        }
#endif

		//TODO: Check the combination of encoded and decoded parameters
	}

	return error;
}

/*!
	@brief Decode a VC-5 bitstream to an ordered set of component arrays

	This is the main entry point for decoding a sample.  The decoder must
	have been initialized by a call to @ref PrepareDecoder.

	The bitstream must be initialized and bound to a byte stream before
	calling this routine.  The unpacked output image will be initialized by this
	routine to hold the decoded component arrays represented in the bitstream.

	@todo When the VC-5 part for layers is defined, should be able to pass a mask
	indicating which layers must be decoded
*/
CODEC_ERROR DecodingProcess(DECODER *decoder, BITSTREAM *stream, UNPACKED_IMAGE *image, DATABASE *database, const PARAMETERS *parameters)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;
	TAGVALUE segment;

    //if (parameters->debug_flag) printf("DecodingProcess database: %p\n", database);

	// Initialize the decoder with a default allocator
	PrepareDecoder(decoder, NULL, database, parameters);

	// Get the bitstream start marker
	segment = GetSegment(stream);
	if (segment.longword != StartMarkerSegment)
	{
		return CODEC_ERROR_MISSING_START_MARKER;
	}

	// A VC-5 Part 1 bitstream can only contain a single layer (encoded image)
	error = DecodeSingleImage(decoder, stream, image);

	// Done decoding all layers in the sample and computing the output image
	return error;
}

/*!
	@brief Decode the bitstream into a list of component arrays
*/
CODEC_ERROR DecodeSingleImage(DECODER *decoder, BITSTREAM *input, UNPACKED_IMAGE *image)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;

	// Process tag value pairs until the layer has been decoded
	for (;;)
	{
		TAGVALUE segment;

		// Read the next tag value pair from the bitstream
		segment = GetSegment(input);
		assert(input->error == BITSTREAM_ERROR_OKAY);
		if (input->error != BITSTREAM_ERROR_OKAY) {
			decoder->error = CodecErrorBitstream(input->error);
			return decoder->error;
			break;
		}

		error = UpdateCodecState(decoder, input, segment);
		if (error != CODEC_ERROR_OKAY) {
			return error;
		}

		// Processed all wavelet bands in all channels?
		if (IsDecodingComplete(decoder)) {
			break;
		}
	}

#if VC5_ENABLED_PART(VC5_PART_METADATA)
    if (IsPartEnabled(decoder->enabled_parts, VC5_PART_METADATA))
    {
        // Does the remainder of the bitstream contain a metadata chunk element?
        TAGVALUE segment = GetSegment(input);
        while (input->error == BITSTREAM_ERROR_OKAY)
        {
            error = UpdateCodecState(decoder, input, segment);
            if (error != CODEC_ERROR_OKAY) {
                return error;
            }

            // More data in the bitstream?
            segment = GetSegment(input);
        }

        // Output the metadata database in XML format
        // FILE *output_file = fopen(decoder->metadata.output_pathname, "w");
        // if (output_file == NULL) {
        //     return CODEC_ERROR_FILE_CREATE;
        // }

        // error = OutputMetadataDatabase(decoder->metadata.database, output_file, NULL);

        // fclose(output_file);

        // if (error != CODEC_ERROR_OKAY) {
        //     return error;
        // }

        //TODO: Need to free the items in the metadata database
    }
#endif

	// Parsed the bitstream without errors?
	if (error != CODEC_ERROR_OKAY) {
		return error;
	}

#if (0 && DEBUG)
	printf("Encoded precision: %d\n\n", decoder->codec.bits_per_component);
#endif

#if (0 && DEBUG)
	// Output the lowpass bands at the highest wavelet level (for debugging)
	WriteLowpassBands(decoder, 3, "lowpass%d.dpx");
#endif

	// Reconstruct the output image using the last decoded wavelet in each channel
	return ReconstructUnpackedImage(decoder, image);
}

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
    @brief Return the bitstream error code embedded in the codec error code.
*/
BITSTREAM_ERROR CodecBitstreamError(CODEC_ERROR error)
{
    // Is this a bitstream error?
    if (error & CODEC_ERROR_BITSTREAM)
    {
        BITWORD bitstream_error_mask = BitMask(CODEC_ERROR_SUBSYSTEM_SHIFT);
        return (error & bitstream_error_mask);
    }
    
    // This routine should not be called unless the error is associate with a bitstream
    return BITSTREAM_ERROR_UNEXPECTED;
}

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
/*!
	@brief Set the channel dimensions from the image dimensions and format

	This routine is used set the channel dimensions and other channel-specific
	parameters after the bitstream header has been parsed.  The dimensions of
	each channel can only be set using the parameters present in the bitstream
	header if the bitstream conforms to VC-5 Part 3.
*/
CODEC_ERROR SetImageChannelParameters(DECODER *decoder, int channel_number)
{
	CODEC_STATE *codec = &decoder->codec;
	IMAGE_FORMAT image_format = codec->image_format;
	DIMENSION image_width = codec->image_width;
	DIMENSION image_height = codec->image_height;
	DIMENSION pattern_width = codec->pattern_width;
	DIMENSION pattern_height = codec->pattern_height;
    DIMENSION channel_width;
    DIMENSION channel_height;
    
	// Are the image dimensions valid?
	if (image_width == 0 || image_height == 0)
	{
		// Cannot set the channel dimensions without valid image dimensions
		return CODEC_ERROR_IMAGE_DIMENSIONS;
	}

	// Are the pattern dimensions valid?
	if (pattern_width == 0 || pattern_height == 0)
	{
		// The channel dimensions may depend on the pattern dimensions
		return CODEC_ERROR_PATTERN_DIMENSIONS;
	}

	switch (image_format)
	{
	case IMAGE_FORMAT_RGBA:
        // The pattern width and height must be one
		assert(pattern_width == 1 && pattern_height == 1);

		// All channels have the same dimensions as the image
		decoder->channel[channel_number].width = image_width;
		decoder->channel[channel_number].height = image_height;
		break;
            
    case IMAGE_FORMAT_YCbCrA:

        channel_width = image_width;
        channel_height = image_height;
            
#if VC5_ENABLED_PART(VC5_PART_COLOR_SAMPLING)
        if (IsPartEnabled(decoder->enabled_parts, VC5_PART_COLOR_SAMPLING))
        {
            // Must have valid pattern width and pattern height
            assert(pattern_width > 0 && pattern_height > 0);

            if (channel_number > 0)
            {
                // The color difference components are subsampled
                channel_width = image_width / pattern_width;
                channel_height = image_height / pattern_height;
            }
        }
        else
        {
            // The pattern width and height must be one
            assert(pattern_width == 1 && pattern_height == 1);
        }
#else
        // The pattern width and height must be one
        assert(pattern_width == 1 && pattern_height == 1);
#endif
        decoder->channel[channel_number].width = channel_width;
        decoder->channel[channel_number].height = channel_height;
        break;

    case IMAGE_FORMAT_BAYER:
		// The pattern width and height must be two
		assert(pattern_width == 2 && pattern_height == 2);

		// The image dimensions must be divisible by the pattern dimensions
		//assert((image_width % 2) == 0 && (image_height % 2) == 0);

		decoder->channel[channel_number].width = image_width / 2;
		decoder->channel[channel_number].height = image_height / 2;
		break;

	case IMAGE_FORMAT_CFA:
		// The pattern dimensions must have been set to positive values
		assert(pattern_width > 0 && pattern_height > 0);

		// The image dimensions must be divisible by the pattern dimensions
		//assert((image_width % pattern_width) == 0 && (image_height % pattern_height) == 0);

		decoder->channel[channel_number].width = image_width / pattern_width;
		decoder->channel[channel_number].height = image_height / pattern_height;

		break;

	default:
		// Cannot set the channel dimensions without a valid image format
		return CODEC_ERROR_BAD_IMAGE_FORMAT;
		break;
	}

	//TODO: Is the default bits per component the correct value to use?
	decoder->channel[channel_number].bits_per_component = codec->bits_per_component;
	decoder->channel[channel_number].initialized = true;

	return CODEC_ERROR_OKAY;
}
#endif

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
/*!
	@brief Allocate all of the wavelets used during decoding

	This routine allocates all of the wavelets in the wavelet tree that
	may be used during decoding.

	This routine is used to preallocate the wavelets before decoding begins.
	If the wavelet bands are allocated on demand if not preallocated.

	By default, the wavelet bands are encoded into the bitstream with the bands
	from the wavelet at the highest level (smallest wavelet) first so that the
	bands can be processed by the decoder in the order as the sample is decoded.
*/
CODEC_ERROR AllocDecoderTransforms(DECODER *decoder)
{
	// Use the default allocator for the decoder

	ALLOCATOR *allocator = decoder->allocator;
	int channel_number;
	int wavelet_index;

	int channel_count;
	int wavelet_count;

	assert(decoder != NULL);
	if (! (decoder != NULL)) {
		return CODEC_ERROR_NULLPTR;
	}

	channel_count = decoder->codec.channel_count;
	wavelet_count = decoder->wavelet_count;

	for (channel_number = 0; channel_number < channel_count; channel_number++)
	{
		DIMENSION wavelet_width;
		DIMENSION wavelet_height;

		// Set the channel dimensions using the information obtained from the bitstream header
		SetImageChannelParameters(decoder, channel_number);

		// Check that the channel dimensions and other parameters have been set
		assert(decoder->channel[channel_number].initialized);

		// The dimensions of the wavelet at level zero are equal to the channel dimensions
		wavelet_width = decoder->channel[channel_number].width;
		wavelet_height = decoder->channel[channel_number].height;

		for (wavelet_index = 0; wavelet_index < wavelet_count; wavelet_index++)
		{
			WAVELET *wavelet;

			// Pad the wavelet width if necessary
			if ((wavelet_width % 2) != 0) {
				wavelet_width++;
			}

			// Pad the wavelet height if necessary
			if ((wavelet_height % 2) != 0) {
				wavelet_height++;
			}

			// Dimensions of the current wavelet must be divisible by two
			assert((wavelet_width % 2) == 0 && (wavelet_height % 2) == 0);

			// Reduce the dimensions of the next wavelet by half
			wavelet_width /= 2;
			wavelet_height /= 2;

			wavelet = CreateWavelet(allocator, wavelet_width, wavelet_height);
			decoder->transform[channel_number].wavelet[wavelet_index] = wavelet;
		}
	}

	return CODEC_ERROR_OKAY;
}
#endif

/*!
	@brief Free the wavelet transforms allocated by the decoder
*/
CODEC_ERROR ReleaseDecoderTransforms(DECODER *decoder)
{
	int channel_count = decoder->codec.channel_count;
	int channel_index;

	for (channel_index = 0; channel_index < channel_count; channel_index++)
	{
		int wavelet_index;

		for (wavelet_index = 0; wavelet_index < decoder->wavelet_count; wavelet_index++)
		{
			WAVELET *wavelet = decoder->transform[channel_index].wavelet[wavelet_index];
			DeleteWavelet(decoder->allocator, wavelet);
            decoder->transform[channel_index].wavelet[wavelet_index] = NULL;
		}
	}

	return CODEC_ERROR_OKAY;
}

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
/*!
	@brief Allocate all of the buffers required for decoding

	This routine allocates buffers required for decoding, not including
	the wavelet images in the wavelet tree which are allocated by
	@ref AllocDecoderTransforms

	This routine is used to preallocate buffers before decoding begins.
	Decoding buffers are allocated on demand if not preallocated.

	Currently, the reference decoder allocates scratch buffers as required
	by each routine that needs scratch space and the scratch buffers are
	deallocated at the end each routine that allocates scratch space.

	@todo Should it be an error if the buffers are not preallocated?
*/
CODEC_ERROR AllocDecoderBuffers(DECODER *decoder)
{
	(void)decoder;
	return CODEC_ERROR_UNIMPLEMENTED;
}
#endif

/*!
	@brief Free any buffers allocated by the decoder
*/
CODEC_ERROR ReleaseDecoderBuffers(DECODER *decoder)
{
	(void)decoder;
	return CODEC_ERROR_UNIMPLEMENTED;
}

/*!
	@brief Allocate the wavelets for the specified channel
*/
CODEC_ERROR AllocateChannelWavelets(DECODER *decoder, int channel_number)
{
	// Use the default allocator for the decoder
	ALLOCATOR *allocator = decoder->allocator;
	int wavelet_index;

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
	// Use the channel dimensions computed from the image dimension and image format
	DIMENSION channel_width = decoder->channel[channel_number].width;
	DIMENSION channel_height = decoder->channel[channel_number].height;
#else
	// Use the channel dimensions from the current codec state
	DIMENSION channel_width = decoder->codec.channel_width;
	DIMENSION channel_height = decoder->codec.channel_height;
#endif

	// Round up the wavelet dimensions to an even number
	DIMENSION wavelet_width = ((channel_width % 2) == 0) ? channel_width / 2 : (channel_width + 1) / 2;
	DIMENSION wavelet_height = ((channel_height % 2) == 0) ? channel_height / 2 : (channel_height + 1) / 2;

	//TODO: Check for errors before the code that initializes the local variables
	assert(decoder != NULL);
	if (! (decoder != NULL)) {
		return CODEC_ERROR_NULLPTR;
	}

	for (wavelet_index = 0; wavelet_index < decoder->wavelet_count; wavelet_index++)
	{
		WAVELET *wavelet = decoder->transform[channel_number].wavelet[wavelet_index];

		// Has a wavelet already been created?
		if (wavelet != NULL)
		{
			// Is the wavelet the correct size?
			if (wavelet_width != wavelet->width ||
				wavelet_height != wavelet->height)
			{
				// Deallocate the wavelet
				DeleteWavelet(allocator, wavelet);

				wavelet = NULL;
			}
		}

		if (wavelet == NULL)
		{
			wavelet = CreateWavelet(allocator, wavelet_width, wavelet_height);
			assert(wavelet != NULL);

			decoder->transform[channel_number].wavelet[wavelet_index] = wavelet;
		}

		// Pad the wavelet width if necessary
		if ((wavelet_width % 2) != 0) {
			wavelet_width++;
		}

		// Pad the wavelet height if necessary
		if ((wavelet_height % 2) != 0) {
			wavelet_height++;
		}

		// Dimensions of the current wavelet must be divisible by two
		assert((wavelet_width % 2) == 0 && (wavelet_height % 2) == 0);

		// Reduce the dimensions of the next wavelet by half
		wavelet_width /= 2;
		wavelet_height /= 2;
	}

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Initialize the codec state before starting to decode a bitstream
*/
CODEC_ERROR PrepareDecoderState(DECODER *decoder, const PARAMETERS *parameters)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;
	CODEC_STATE *codec = &decoder->codec;

	// Set the parameters that control the decoding process
	decoder->wavelet_count = 3;

	// The wavelets and decoding buffers have not been allocated
	decoder->memory_allocated = false;

	// Clear the table of information about each decoded channel
	memset(decoder->channel, 0, sizeof(decoder->channel));

	// Set the codebook
	decoder->codebook = (CODEBOOK *)cs17.codebook;

#if _DEBUG
	// Record the pixel format of the image input to the encoder (for debugging)
	decoder->input.format = parameters->input.format;
#endif

	// Initialize the codec state with the default parameter values
	error = PrepareCodecState(codec);
	if (error != CODEC_ERROR_OKAY) {
		return error;
	}

	// Initialize the codec state with the external parameter values
	codec->image_width = parameters->input.width;
	codec->image_height = parameters->input.height;

	//TODO: Initialize other parameters with external values?

	// The default channel dimensions are the image dimensions
	codec->channel_width = codec->image_width;
	codec->channel_height = codec->image_height;

	return error;
}

/*!
	@brief Prepare the decoder transforms for the next layer

	Each wavelet in the decoder transforms contain flags that indicate
	whether the wavelet bands must be decoded.  These flags must be reset
	before decoding the next layer.
*/
CODEC_ERROR PrepareDecoderTransforms(DECODER *decoder)
{
	int channel_count = decoder->codec.channel_count;
	int channel_index;

	for (channel_index = 0; channel_index < channel_count; channel_index++)
	{
		int wavelet_count = decoder->wavelet_count;
		int wavelet_index;

		for (wavelet_index = 0; wavelet_index < wavelet_count; wavelet_index++)
		{
			WAVELET *wavelet = decoder->transform[channel_index].wavelet[wavelet_index];
			wavelet->valid_band_mask = 0;
		}
	}

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Pack the component arrays into the output image
	
	The decoding process outputs a set of component arrays that does not correspond
	to any common image format.  The image repacking process converts the ordered
	set of component arrays output by the decoding processing into a packed image.

	The image repacking process is not normative in VC-5 Part 1.
*/
CODEC_ERROR ImageRepackingProcess(const UNPACKED_IMAGE *unpacked_image,
								  PACKED_IMAGE *packed_image,
                                  DATABASE *database,
								  const PARAMETERS *parameters)
{
	DIMENSION output_width = packed_image->width;
	DIMENSION output_height = packed_image->height;
	size_t output_pitch = packed_image->pitch;
	PIXEL_FORMAT output_format = packed_image->format;
	PIXEL *output_buffer = packed_image->buffer;
    ENABLED_PARTS enabled_parts = parameters->enabled_parts;

	(void)parameters;

	// Is the format of the output image Bayer?
	if (IsBayerFormat(output_format))
	{
		// The dimensions must be in units of Bayer pattern elements
		output_width /= 2;
		output_height /= 2;
		output_pitch *= 2;
	}

	switch (output_format)
	{
	case PIXEL_FORMAT_BYR4:
		return PackComponentsToBYR4(unpacked_image, output_buffer, output_pitch,
                                    output_width, output_height, enabled_parts);
		break;

	case PIXEL_FORMAT_RG48:
		return PackComponentsToRG48(unpacked_image, output_buffer, output_pitch,
                                    output_width, output_height, enabled_parts);
		break;

	case PIXEL_FORMAT_B64A:
		return PackComponentsToB64A(unpacked_image, output_buffer, output_pitch,
                                    output_width, output_height, enabled_parts);
		break;

	case PIXEL_FORMAT_DPX0:
		return PackComponentsToDPX0(unpacked_image, output_buffer, output_pitch,
                                    output_width, output_height, enabled_parts);
		break;
            
    case PIXEL_FORMAT_NV12:
        return PackComponentsToNV12(unpacked_image, output_buffer, output_pitch,
                                    output_width, output_height, enabled_parts);
        break;

	default:
		assert(0);
		break;
	}

	// Unsupported output image format
	return CODEC_ERROR_UNSUPPORTED_FORMAT;
}

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
/*!
	@brief Compute default parameters for the repacked image
*/
CODEC_ERROR SetOutputImageFormat(DECODER *decoder,
								 const PARAMETERS *parameters,
								 DIMENSION *width_out,
								 DIMENSION *height_out,
								 PIXEL_FORMAT *format_out)
{
	// The image dimensions are in units of samples
	DIMENSION output_width = decoder->codec.image_width;
	DIMENSION output_height = decoder->codec.image_height;

#if (0 && DEBUG)
	// Decode to the pixel format of the image input to the unpacking process
	PIXEL_FORMAT output_format = decoder->codec.input.format;
#else
	PIXEL_FORMAT output_format = PIXEL_FORMAT_UNKNOWN;
#endif

	// Override the pixel format with the format passed as a parameter
	if (parameters->output.format != PIXEL_FORMAT_UNKNOWN) {
		output_format = parameters->output.format;
	}
    
    // Assume that the output pixel format is the same as the input to the encoder
    if (output_format == PIXEL_FORMAT_UNKNOWN) {
        output_format = parameters->input.format;
    }

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
    if (IsPartEnabled(decoder->enabled_parts, VC5_PART_IMAGE_FORMATS))
    {
        // Pixel format still unknown?
        if (output_format == PIXEL_FORMAT_UNKNOWN)
        {
            // Use information obtained from the bitstream to set the pixel format
            IMAGE_FORMAT image_format = decoder->codec.image_format;
            COUNT channel_count = decoder->codec.channel_count;
            
            switch (image_format)
            {
                case IMAGE_FORMAT_RGBA:
                    output_format = (channel_count == 3) ? PIXEL_FORMAT_RG48 : PIXEL_FORMAT_B64A;
                    break;
                    
                case IMAGE_FORMAT_YCbCrA:
                    output_format = PIXEL_FORMAT_NV12;
                    break;
                    
                case IMAGE_FORMAT_BAYER:
                    output_format = PIXEL_FORMAT_BYR4;
                    break;
                    
                default:
                    output_format = PIXEL_FORMAT_B64A;
                    break;
            }
        }
    }
#endif
    
	assert(output_format != PIXEL_FORMAT_UNKNOWN);

	if (width_out != NULL) {
		*width_out = output_width;
	}

	if (height_out != NULL) {
		*height_out = output_height;
	}

	if (format_out != NULL) {
		*format_out = output_format;
	}

	return CODEC_ERROR_OKAY;
}
#else
/*!
	@brief Compute default parameters for the repacked image
*/
CODEC_ERROR SetOutputImageFormat(DECODER *decoder,
								 const PARAMETERS *parameters,
								 DIMENSION *width_out,
								 DIMENSION *height_out,
								 PIXEL_FORMAT *format_out)
{
	// The decoded dimensions are in units of Bayer pattern elements
	DIMENSION output_width = decoder->codec.image_width;
	DIMENSION output_height = decoder->codec.image_height;

	//TODO: Should set the output dimensions based on the decoded channel dimensions

#if (0 && DEBUG)
	// Decode to the pixel format of the image input to the unpacking process
	PIXEL_FORMAT output_format = decoder->codec.input.format;
#else
	PIXEL_FORMAT output_format = PIXEL_FORMAT_UNKNOWN;
#endif

	// Override the pixel format with the format passed as a parameter
	if (parameters->output.format != PIXEL_FORMAT_UNKNOWN) {
		output_format = parameters->output.format;
	}
	assert(output_format != PIXEL_FORMAT_UNKNOWN);

	// Is the output image a grid of Bayer pattern elements?
	if (IsBayerFormat(output_format))
	{
		// The image dimensions are in units of samples
		output_width *= 2;
		output_height *= 2;
	}

	if (width_out != NULL) {
		*width_out = output_width;
	}

	if (height_out != NULL) {
		*height_out = output_height;
	}

	if (format_out != NULL) {
		*format_out = output_format;
	}

	return CODEC_ERROR_OKAY;
}
#endif

#if 0
/*!
	@brief Set the image dimensions and pixel format for the packed image

	The dimensions of the image returned by the image repacking process
	follow industry standard practice, so the dimensions of a Bayer image
	are twice the dimensions of the grid of Bayer pattern elements.
*/
CODEC_ERROR SetDisplayImageFormat(DECODER *decoder,
								  const PARAMETERS *parameters,
								  DIMENSION *width_out,
								  DIMENSION *height_out,
								  PIXEL_FORMAT *format_out)
{
	DIMENSION display_width = decoder->codec.image_width;
	DIMENSION display_height = decoder->codec.image_height;
	PIXEL_FORMAT display_format = decoder->codec.input.format;

	// Was the pixel format of the image input to the encoder found in the bitstream?
	if (display_format == PIXEL_FORMAT_UNKNOWN)
	{
		// The output format must be provided as an external parameter
		display_format = parameters->display.format;
	}
	assert(display_format != PIXEL_FORMAT_UNKNOWN);

	// Is the output image a grid of Bayer pattern elements?
	if (IsBayerFormat(display_format))
	{
		// Adjust the dimensions of Bayer image
		display_format *= 2;
		display_format *= 2;
	}

	if (width_out != NULL) {
		*width_out = display_width;
	}

	if (height_out != NULL) {
		*height_out = display_height;
	}

	if (format_out != NULL) {
		*format_out = display_format;
	}

	return CODEC_ERROR_OKAY;
}
#endif

/*!
	@brief Return true if the lowpass bands in all channels are valid
*/
bool ChannelLowpassBandsAllValid(const DECODER *decoder, int index)
{
	int channel_count = decoder->codec.channel_count;
	int channel;
	for (channel = 0; channel < channel_count; channel++)
	{
		WAVELET *wavelet = decoder->transform[channel].wavelet[index];
		if ((wavelet->valid_band_mask & BandValidMask(0)) == 0) {
			return false;
		}
	}

	// All channels have valid lowpass bands at the specified level
	return true;
}

#if _DEBUG
/*!
	@brief Return the pixel format of the input to the image unpacking process

	The pixel format of the input image may have been specified as an external
	parameter or may have been present in the bitstream as an optional tag-value
	pair.  The value obtained from the bitstream takes precedence over any
	external parameters.
*/
PIXEL_FORMAT EncodedPixelFormat(const DECODER *decoder, const PARAMETERS *parameters)
{
	PIXEL_FORMAT format = parameters->output.format;
	if (decoder->codec.input.format != PIXEL_FORMAT_UNKNOWN) {
		format = decoder->codec.input.format;
	}
	return format;
}
#endif

#if 0
/*!
    @brief Set the number of bits per component in the decoder
 
    This routine sets the number of bits per component in all decoder channels
*/
CODEC_ERROR SetDecoderBitsPerComponent(DECODER *decoder, int channel_number, PRECISION bits_per_component)
{
    // Already found the first codeblock in the channel?
    if (!decoder->channel[channel_number].found_first_codeblock)
    {
        // Remember the number of bits per component in this and higher numberedchannel
        decoder->channel[codec->channel_number].bits_per_component = codec->bits_per_component;
    }
    else
    {
        // Should not encounter a BitsPerComponent tag-value pair after the first codeblock
        return CODEC_ERROR_SYNTAX_ERROR;
    }
    
    return CODEC_ERROR_SYNTAX_ERROR;
}
#endif

#if VC5_ENABLED_PART(VC5_PART_SECTIONS)

/*
    @brief Return true if the tag identifies a section header
 */
bool IsSectionHeader(TAGWORD tag)
{
    switch (tag)
    {
        case CODEC_TAG_ImageSectionTag:
        case CODEC_TAG_HeaderSectionTag:
        case CODEC_TAG_LayerSectionTag:
        case CODEC_TAG_ChannelSectionTag:
        case CODEC_TAG_WaveletSectionTag:
        case CODEC_TAG_SubbandSectionTag:
            return true;

        default:
            return false;
    }
    
    return false;
}

/*
    @brief Map the tag for a section header to the section number
 */
CODEC_ERROR GetSectionNumber(TAGWORD tag, int *section_number_out)
{
    int section_number = 0;
    
    switch (tag)
    {
        case CODEC_TAG_ImageSectionTag:
            section_number = 1;
            break;
            
        case CODEC_TAG_HeaderSectionTag:
            section_number = 2;
            break;
            
        case CODEC_TAG_LayerSectionTag:
            section_number = 3;
            break;
            
        case CODEC_TAG_ChannelSectionTag:
            section_number = 4;
            break;
            
        case CODEC_TAG_WaveletSectionTag:
            section_number = 5;
            break;
            
        case CODEC_TAG_SubbandSectionTag:
            section_number = 6;
            break;
            
        default:
            assert(0);
            break;
    }

    if (section_number_out != NULL) {
        *section_number_out = section_number;
    }
    
    if (section_number > 0) {
        return CODEC_ERROR_OKAY;
    }
    
    return CODEC_ERROR_BAD_SECTION_TAG;
}

/*!
    @brief Write section information to the section log file
 */
CODEC_ERROR WriteSectionInformation(FILE *logfile, int section_number, int section_length)
{
    fprintf(logfile, "Section: %d, length: %d\n", section_number, section_length);
    return CODEC_ERROR_OKAY;
}

#endif


/*!
	@brief Update the codec state with the specified tag value pair

	When a segment (tag value pair) is encountered in the bitstream of an
	encoded sample, it may imply some change in the codec state.  For example,
	when a tag for the encoded format is read from the bitstream, the encoded
	format entry in the codec state may change.

	Some tags require that additional information must be read from the
	bitstream and more segments may be encountered, leading to additional
	changes in the codec state.

	A tag may identify a single parameter and the parameter value must be updated
	in the codec state with the new value specified in the segment, but a tag may
	also imply that other pparameter values must be updated.  For example, the tag
	that marks the first encounter with a wavelet at a lower level in the wavelet
	tree implies that the width and height of wavelet bands that may be encoded in
	the remainder of the sample must be doubled.
	
	It is not necessary for the encoder to insert segments into the bitstream if the
	codec state change represented by an encoded tag and value can be deduced from
	earlier segments in the bitstream and the codec state can be changed at a time
	during decoding that is functionally the same as when the state change would have
	been performed by an explicitly encoded tag and value.

	@todo Need to check that parameters found in the sample are consistent with
	the decoding parameters used to initialize the codec state.
*/
CODEC_ERROR UpdateCodecState(DECODER *decoder, BITSTREAM *stream, TAGVALUE segment)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;
	CODEC_STATE *codec = &decoder->codec;
	ENABLED_PARTS enabled_parts = decoder->enabled_parts;
	bool optional = false;
	int32_t chunk_size = 0;
	TAGWORD tag = segment.tuple.tag;
	TAGWORD value = segment.tuple.value;

	// The enabled parts variable may not be used depending on the compile-time options
	(void)enabled_parts;

	// Assume that the next syntax element is not a tag-value pair for a header parameter
	codec->header = false;

	// Assume that the next syntax element is not a codeblock (large chunk element)
	codec->codeblock = false;

	// Is this an optional tag?
	if (tag < 0) {
		tag = RequiredTag(tag);
		optional = true;
	}

#if (0 && DEBUG)
	if (logfile) {
		fprintf(logfile, "UpdateCodecState tag: %d, value: %d, optional: %d\n",
				tag, value, optional);
	}
#endif

	switch (tag)
	{
	case CODEC_TAG_ChannelCount:		// Number of channels in the transform
		assert(0 < value && value <= MAX_CHANNEL_COUNT);
		codec->channel_count = (uint_least8_t)value;
		codec->header = true;
		break;

	case CODEC_TAG_ImageWidth:			// Width of the image
		codec->image_width = value;
		codec->header = true;

		// The image width is the default width of the next channel in the bitstream
		codec->channel_width = value;
		break;

	case CODEC_TAG_ImageHeight:			// Height of the image
		codec->image_height = value;
		codec->header = true;

		// The image height is the default height of the next channel in the bitstream
		codec->channel_height = value;
		break;
#if _DEBUG
	case CODEC_TAG_PixelFormat:			// Pixel format of the packed image input to the encoder
		codec->input.format = value;
		//codec->header = true;
		break;
#endif

	case CODEC_TAG_SubbandNumber:		// Subband number of this wavelet band
		codec->subband_number = value;
		break;

	case CODEC_TAG_Quantization:		// Quantization applied to band
		codec->band.quantization = value;
		break;

	case CODEC_TAG_LowpassPrecision:	// Number of bits per lowpass coefficient
		if (! (PRECISION_MIN <= value && value <= PRECISION_MAX)) {
			return CODEC_ERROR_LOWPASS_PRECISION;
		}
		codec->lowpass_precision = (PRECISION)value;
		break;

	case CODEC_TAG_ChannelNumber:		// Channel number
		codec->channel_number = value;
		break;

#if 0   //VC5_ENABLED_PART(VC5_PART_COLOR_SAMPLING)
	case CODEC_TAG_INTERLACED_FLAGS:	// Interlaced structure of the video stream
		assert(0);
		break;
#endif

	case CODEC_TAG_BitsPerComponent:	// Number of bits in the video source
        codec->bits_per_component = (PRECISION)value;
        //error = SetDecoderBitsPerComponent(decoder, codec->channel_number, codec->bits_per_component);
		break;

	case CODEC_TAG_PrescaleShift:
		UpdatePrescaleTable(codec, value);
		break;

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
	case CODEC_TAG_ImageFormat:
		if (IsPartEnabled(enabled_parts, VC5_PART_IMAGE_FORMATS))
		{
			codec->image_format = (IMAGE_FORMAT)value;
			codec->header = true;
		}
		else
		{
			// The image format shall not be present in the bitstream
			assert(0);
			error = CODEC_ERROR_BITSTREAM_SYNTAX;
		}
		break;

	case CODEC_TAG_PatternWidth:
		if (IsPartEnabled(enabled_parts, VC5_PART_IMAGE_FORMATS))
		{
			codec->pattern_width = (DIMENSION)value;
			codec->header = true;
		}
		else
		{
			// The pattern width shall not be present in the bitstream
			assert(0);
			error = CODEC_ERROR_BITSTREAM_SYNTAX;
		}
		break;

	case CODEC_TAG_PatternHeight:
		if (IsPartEnabled(enabled_parts, VC5_PART_IMAGE_FORMATS))
		{
			codec->pattern_height = (DIMENSION)value;
			codec->header = true;
		}
		else
		{
			// The pattern height shall not be present in the bitstream
			assert(0);
			error = CODEC_ERROR_BITSTREAM_SYNTAX;
		}
		break;

	case CODEC_TAG_ComponentsPerSample:
		if (IsPartEnabled(enabled_parts, VC5_PART_IMAGE_FORMATS))
		{
			codec->components_per_sample = (DIMENSION)value;
			codec->header = true;
		}
		else
		{
			// The components per sample shall not be present in the bitstream
			assert(0);
			error = CODEC_ERROR_BITSTREAM_SYNTAX;
		}
		break;

	case CODEC_TAG_MaxBitsPerComponent:
		if (IsPartEnabled(enabled_parts, VC5_PART_IMAGE_FORMATS))
		{
			codec->max_bits_per_component = (PRECISION)value;
			codec->header = true;
		}
		else
		{
			// The components per sample shall not be present in the bitstream
			assert(0);
			error = CODEC_ERROR_BITSTREAM_SYNTAX;
		}
		break;
#endif

	case CODEC_TAG_ChannelWidth:
#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
		if (IsPartEnabled(enabled_parts, VC5_PART_IMAGE_FORMATS))
		{
			// The channel width shall not be present in the bitstream
			assert(0);
			error = CODEC_ERROR_BITSTREAM_SYNTAX;
		}
		else
#endif
		{
			// The channel width may be present in the bitstream
			codec->channel_width = (DIMENSION)value;
		}
		break;

	case CODEC_TAG_ChannelHeight:
#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
		if (IsPartEnabled(enabled_parts, VC5_PART_IMAGE_FORMATS))
		{
			// The channel height shall not be present in the bitstream
			assert(0);
			error = CODEC_ERROR_BITSTREAM_SYNTAX;
		}
		else
#endif
		{
			// The channel height may be present in the bitstream
			codec->channel_height = (DIMENSION)value;
		}
		break;
            
#if VC5_ENABLED_PART(VC5_PART_LAYERS)
    case CODEC_TAG_LayerCount:
         if (IsPartEnabled(enabled_parts, VC5_PART_LAYERS))
         {
             codec->layer_count = (COUNT)value;
             codec->header = true;
         }
        break;
            
    case CODEC_TAG_LayerNumber:
        if (IsPartEnabled(enabled_parts, VC5_PART_LAYERS))
        {
            codec->layer_number = (COUNT)value;
        }
        break;

    case CODEC_TAG_LayerPattern:
        if (IsPartEnabled(enabled_parts, VC5_PART_LAYERS))
        {
            codec->layer_pattern = (uint16_t)value;
            codec->header = true;
        }
        break;
#endif
            
	default:		// Unknown tag or the tag identifies a chunk

		//TODO: Check for chunk tags that are not defined in VC-5 Part 1

        if (decoder->debug_flag) {
            fprintf(stderr, "Possible chunk element: 0x%04X\n", tag);
            //printf(stderr, "Metadata small tag: 0x%04X, large tag: 0x%04X\n", CODEC_TAG_SmallMetadata, CODEC_TAG_LargeMetadata);
        }

		// Does this tag indicate a chunk of data?
		if (tag & CODEC_TAG_CHUNK_MASK)
		{
			// Does this chunk have a 24-bit size?
			if (tag & CODEC_TAG_LARGE_CHUNK)
			{
				// The chunk size includes the low byte in the tag
				chunk_size = (value & 0xFFFF);
				chunk_size += ((tag & 0xFF) << 16);

                // Clear the low byte in the chunk tag
                tag &= 0xFF00;
			}
			else
			{
				// The chunk size is specified by the value
				chunk_size = (value & 0xFFFF);
			}
		}

        if (decoder->debug_flag) {
            fprintf(stderr, "Chunk element tag: 0x%04X, size: %d\n", tag, chunk_size);
        }

		// Is this a codeblock?
		if (tag == CODEC_TAG_LargeCodeblock)
		{
			codec->codeblock = true;
		}
        
        // Is this chunk a unique image identifier?
        else if (tag == CODEC_TAG_UniqueImageIdentifier)
        {
            // The unique image identifier should be optional
            assert(optional);
            if (! optional) {
                return CODEC_ERROR_SYNTAX_ERROR;
            }
            
            // Parse the unique image identifier
            error = ParseUniqueImageIdentifier(decoder, stream, chunk_size);
        }
            
        // Is this chunk an inverse component transform?
        else if (tag == CODEC_TAG_InverseTransform)
        {
            // The inverse component transform should not be optional
            assert(!optional);
            if (optional) {
                return CODEC_ERROR_SYNTAX_ERROR;
            }
            
            // Parse the inverse component transform
            error = ParseInverseComponentTransform(decoder, stream, chunk_size);
        }
            
        // Is this chunk an inverse component permutation?
        else if (tag == CODEC_TAG_InversePermutation)
        {
            // The inverse component permutation should not be optional
            assert(!optional);
            if (optional) {
                return CODEC_ERROR_SYNTAX_ERROR;
            }

            // Parse the inverse component permutation
            error = ParseInverseComponentPermutation(decoder, stream, chunk_size);
        }
            
        // Is this chunk a 16-bit inverse component transform?
        else if (tag == CODEC_TAG_InverseTransform16)
        {
            // The 16-bit inverse component transform is not supported
            assert(0);
            return CODEC_ERROR_UNIMPLEMENTED;
        }

#if VC5_ENABLED_PART(VC5_PART_SECTIONS)
        // Is this a section header?
        else if (IsPartEnabled(enabled_parts, VC5_PART_SECTIONS) &&
                 IsSectionHeader(tag) &&
                 decoder->enabled_sections)
        {
            int section_number;
            
            // Section headers are optional tag-value pairs
            assert(optional);
            
            // Is this a bitstream header section?
            if (tag == CODEC_TAG_HeaderSectionTag)
            {
                // Handle this tag-value pair as if it was a bitstream header parameter
                codec->header = true;
            }
            
            // Convert the tag to a section number
            GetSectionNumber(tag, &section_number);
            
            //printf("Section number: %d\n", section_number);

            // Record the section number and length (in segments)
            codec->section_number = section_number;
            codec->section_length = chunk_size;
            
            // Encountered an image section in the bitstream?
            if (section_number == SECTION_NUMBER_IMAGE)
            {
                // Indicate that there may be more images encoded in the bitstream
                decoder->image_section_flag = true;
            }
            
            // Write the section information to the log file
            WriteSectionInformation(decoder->section_logfile, section_number, chunk_size);
        }
#endif
#if VC5_ENABLED_PART(VC5_PART_METADATA)

        // if (decoder->debug_flag) {
        //     fprintf(stderr, "Possible metadata chunk element: 0x%04X\n", tag);
        // }

        // Is this a metadata chunk?
        else if (tag == CODEC_TAG_SmallMetadata || tag == CODEC_TAG_LargeMetadata)
        {
            // if (decoder->debug_flag) {
            //     fprintf(stderr, "Found metadata chunk element: 0x%04X\n", tag);
            // }

            // Decode the metadata tuples in this chunk element
            DecodeMetadataChunk(decoder, stream, tag, chunk_size);
        }
#endif
		else
		{
			// Does this chunk have a 24-bit chunk payload size?
			if (tag & CODEC_TAG_LARGE_CHUNK)
			{
				optional = true;
				chunk_size = 0;
			}

			assert(optional);
			if (! (optional))
			{
				error = CODEC_ERROR_BITSTREAM_SYNTAX;
			}
			else if (chunk_size > 0)
			{
				// Skip processing the payload of this optional chunk element
				SkipPayload(stream, chunk_size);
			}
		}
		break;
	}

	// Encountered an error while processing the tag?
	if (error != CODEC_ERROR_OKAY)
	{
		return error;
	}

	//TODO: Check that bitstreams with missplaced header parameters fail to decode

	//if (IsHeaderParameter(tag))
	if (codec->header)
	{
		if (optional)
		{
            // Okay for a header parameter to be optional in some situations
            switch (tag)
            {
#if VC5_ENABLED_PART(VC5_PART_SECTIONS)
            case CODEC_TAG_HeaderSectionTag:
                break;
#endif
#if VC5_ENABLED_PART(VC5_PART_LAYERS)
            case CODEC_TAG_LayerCount:
                break;
#endif
            default:
                error =  CODEC_ERROR_REQUIRED_PARAMETER;
                break;
            }
		}
		else if (decoder->header_finished)
		{
			// Should not encounter a header parameter after the header has been parsed
			error = CODEC_ERROR_BITSTREAM_SYNTAX;
		}
		else
		{
			// Record that this header parameter has been decoded
			error = UpdateHeaderParameter(decoder, tag);
		}
	}
	else if (!decoder->header_finished && !optional)
	{
		// There should be no more header parameters in the bitstream
		decoder->header_finished = true;
	}

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
	// The wavelets and buffers can be allocated after the bitstream header has been parsed
	if (IsPartEnabled(enabled_parts, VC5_PART_IMAGE_FORMATS) &&
		decoder->header_finished &&
		!decoder->memory_allocated)
	{
		// Allocate space for the wavelet transforms
		AllocDecoderTransforms(decoder);

		// Allocate all buffers required for decoding
		AllocDecoderBuffers(decoder);

		// Reset the flags in the wavelet transforms
		PrepareDecoderTransforms(decoder);

		// The wavelet transforms and decoding buffers have been allocated
		decoder->memory_allocated = true;
	}
#endif

	// Found a codeblock element?
	if (codec->codeblock)
	{
		const int channel_number = codec->channel_number;

		// Have the channel dimensions been initialized?
		if (!decoder->channel[channel_number].initialized)
		{
			// Record the channel dimensions and component precision
			decoder->channel[channel_number].width = codec->channel_width;
			decoder->channel[channel_number].height = codec->channel_height;

			// Initialize the dimensions of this channel
			decoder->channel[channel_number].initialized = true;

			//TODO: Allocate space for the wavelet transforms and decoding buffers
		}

        // Is this the first codeblock encountered in the bitstream for this channel?
        if (!decoder->channel[channel_number].found_first_codeblock)
        {
            // Remember the number of bits per component in this and higher numbered channel
            decoder->channel[codec->channel_number].bits_per_component = codec->bits_per_component;
            
            // Found the first codeblock in the channel
            decoder->channel[channel_number].found_first_codeblock = true;
        }
        
		// Decode the subband into its wavelet band
		error = DecodeChannelSubband(decoder, stream, chunk_size);
	}

	return error;
}

/*!
	@brief Return true if the tag corresponds to a bitstream header parameter
*/
bool IsHeaderParameter(TAGWORD tag)
{
	switch (tag)
	{
	case CODEC_TAG_ImageWidth:
	case CODEC_TAG_ImageHeight:
	case CODEC_TAG_ChannelCount:
	case CODEC_TAG_SubbandCount:

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
	case CODEC_TAG_ImageFormat:
	case CODEC_TAG_PatternWidth:
	case CODEC_TAG_PatternHeight:
	case CODEC_TAG_ComponentsPerSample:
	case CODEC_TAG_MaxBitsPerComponent:
#endif
		return true;

	default:
		return false;
	}
}

/*!
	@brief Return the header mask that corresponds to the header tag
*/
uint16_t GetHeaderMask(TAGWORD tag)
{
	uint16_t header_mask = 0;

	switch (tag)
	{
	case CODEC_TAG_ImageWidth:
		header_mask = BITSTREAM_HEADER_FLAGS_IMAGE_WIDTH;
		break;

	case CODEC_TAG_ImageHeight:
		header_mask = BITSTREAM_HEADER_FLAGS_IMAGE_HEIGHT;
		break;

	case CODEC_TAG_ChannelCount:
		header_mask = BITSTREAM_HEADER_FLAGS_CHANNEL_COUNT;
		break;

	case CODEC_TAG_SubbandCount:
		header_mask = BITSTREAM_HEADER_FLAGS_SUBBAND_COUNT;
		break;

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
	case CODEC_TAG_ImageFormat:
		header_mask = BITSTREAM_HEADER_FLAGS_IMAGE_FORMAT;
		break;
#endif
#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
	case CODEC_TAG_PatternWidth:
		header_mask = BITSTREAM_HEADER_FLAGS_PATTERN_WIDTH;
		break;
#endif
#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
	case CODEC_TAG_PatternHeight:
		header_mask = BITSTREAM_HEADER_FLAGS_PATTERN_HEIGHT;
		break;
#endif
#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
	case CODEC_TAG_ComponentsPerSample:
		header_mask = BITSTREAM_HEADER_FLAGS_COMPONENTS_PER_SAMPLE;
		break;
#endif
#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
	case CODEC_TAG_MaxBitsPerComponent:
		header_mask = BITSTREAM_HEADER_FLAGS_MAX_BITS_PER_COMPONENT;
		break;
#endif

	default:
		assert(0);
		break;
	}

	return header_mask;
}

/*!
	@brief Record that a header parameter was found in the bitstream.

	The tag-value pair that corresponds to a header parameters must occur
	in the bitstream header and must occur at most once in the bitstream.
*/
CODEC_ERROR UpdateHeaderParameter(DECODER *decoder, TAGWORD tag)
{
	uint16_t header_mask = 0;

	if (!IsHeaderParameter(tag)) {
		return CODEC_ERROR_UNEXPECTED;
	}

	header_mask = GetHeaderMask(tag);

	if (header_mask == 0) {
		return CODEC_ERROR_UNEXPECTED;
	}

	if (decoder->header_mask & header_mask) {
		// The header parameter should occur at most once
		return CODEC_ERROR_DUPLICATE_HEADER_PARAMETER;
	}

	// Record this encounter with the header parameter
	decoder->header_mask |= header_mask;

	return CODEC_ERROR_OKAY;
}


#if 0

//! Largest tag number for a bitstream header parameter
#define MAX_HEADER_PARAMETER_TAG CODEC_TAG_ImageHeight

/*!
	@brief Parse the sample to obtain information for initializing the decoder

	The decoder will self-initialize while decoding the first sample and will
	reallocate memory used for decoding if subsequent samples have different
	parameters.

	The routine @ref PrepareDecoder can be used to initialize the decoder with
	known parameters that may be obtained, for example, from the container or
	stream in which the encoded samples are embedded.  It may be desireable,
	but not necessary, to initialize the decoder before beginning to decode
	any samples to avoid taking too long to decode the first sample.  If the
	necessary parameters are not available from an external source, this routine
	can be used to parse a sample that is representative of the samples to be
	decoded in order to obtain the parameters required for decoder initialization.

	This routine decodes the sample header to determine the type of sample and
	other parameters needed to initialize the decoder.
*/
CODEC_ERROR ParseBitstreamHeader(BITSTREAM *input, BITSTREAM_HEADER *header)
{
	TAGVALUE segment;

	// Record parameters that are present in the bitsream header
	bool header_parameter_count[MAX_HEADER_PARAMETER_TAG + 1];
	memset(header_parameter_count, 0, sizeof(header_parameter_count));

	if (header == NULL) {
		return CODEC_ERROR_NULLPTR;
	}

	// Clear the entire sample header to prevent early return from this routine
	memset(header, 0, sizeof(BITSTREAM_HEADER));

	// Initialize the frame dimensions to unknown
	header->encoded_width = 0;
	header->encoded_height = 0;

#if _BITSTREAM_UNALIGNED
	// Record the alignment of the bitstream within the sample
	SetBitstreamAlignment(input, 0);
#endif

	// Get the bitstream start marker
	segment = GetSegment(input);
	if (segment.longword != StartMarkerSegment)
	{
		return CODEC_ERROR_MISSING_START_MARKER;
	}

	// Continue parsing the sample header until all of the information has been found
	for (;;)
	{
		// Get the next tag value pair from the bitstream
		segment = GetSegment(input);

		// Did the bitstream end before the last tag was found?
		if (input->error == BITSTREAM_ERROR_UNDERFLOW) {
			break;
		}

		// Did an error occur while reading the bitstream?
		if (input->error != BITSTREAM_ERROR_OKAY) {
			return BitstreamCodecError(input);
		}

		// Convert the tag into a required tag
		segment.tuple.tag = RequiredTag(segment.tuple.tag);

		// Stop processing the header if this is not a header parameter
		if (!IsHeaderParameter(segment.tuple.tag)) break;

		// The tag number should be a required tag in the range that includes all header parameters
		assert(0 < segment.tuple.tag && segment.tuple.tag <= MAX_HEADER_PARAMETER_TAG);
		if (! (0 < segment.tuple.tag && segment.tuple.tag <= MAX_HEADER_PARAMETER_TAG)) {
			return CODEC_ERROR_UNEXPECTED;
		}

		// Record the occurance of each header parameter
		header_parameter_count[segment.tuple.tag]++;

		// Check for duplicate header parameters
		if (header_parameter_count[segment.tuple.tag] > 1) {
			return CODEC_ERROR_DUPLICATE_HEADER_PARAMETER;
		}

		switch (segment.tuple.tag)
		{
#if VC5_ENABLED_PART(VC5_PART_LAYERS)
		 case CODEC_TAG_LAYER_COUNT:
			 // Get the number of layers in the sample
			 header->layer_count = segment.tuple.value;
			 break;
#endif
		case CODEC_TAG_ChannelCount:
			// Record the number of channels in the bitstream header
			header->channel_count = segment.tuple.value;
			break;

		case CODEC_TAG_ImageWidth:
			// Record the frame width in the bitstream header
			header->encoded_width = segment.tuple.value;
			break;

		case CODEC_TAG_ImageHeight:
			// Record the frame height in the bitstream header
			header->encoded_height = segment.tuple.value;
			break;

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
		case CODEC_TAG_ImageFormat:
			// Record the encoded format (internal representation)
			header->encoded_format = (IMAGE_FORMAT)segment.tuple.value;
			if (header->encoded_format == ENCODED_FORMAT_RGBA_4444 && channel_count == 3) {
				header->encoded_format = ENCODED_FORMAT_RGB_444;
			}
			break;
#endif
		}
	}

	// Should have discovered the maximum dimensions of the component arrays
	assert(header->encoded_width > 0 && header->encoded_height > 0);

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
	if (header->encoded_format == ENCODED_FORMAT_BAYER)
	{
		// Adjust the display height to remove padding
		if (display_height == 0 && header->encoded_height == 544) {
			header->display_height = 540;
		}

		// Adjust the display dimensions to the full frame after demosaic processing
		header->display_width *= 2;
		header->display_height *= 2;
	}
#endif

	// Return true if all of the manadatory header parameters were found
	if (header_parameter_count[CODEC_TAG_ImageWidth] == 1 &&
		header_parameter_count[CODEC_TAG_ImageHeight] == 1) {
			return CODEC_ERROR_OKAY;
	}

	// Missing or duplicate header parameters
	return CODEC_ERROR_BITSTREAM_SYNTAX;
}

#endif

#if 0
/*!
	@brief Parse the channel index table in the bitstream

	The channel index table is a table of the size of each channel
	and is used to compute the byte offset of a channel in the sample.

	The channel index table is used for decoding a sample to less than
	full resolution.  The portion of each encoded channel that is not
	required can be skipped.

	@todo Check that the channel index is in units of bytes
*/
CODEC_ERROR ParseChannelIndex(BITSTREAM *stream, uint32_t *channel_size_table, int channel_count)
{
	//TODO: Should each channel size entry be more than 32 bits?
	const BITCOUNT channel_size_bit_count = 32;

	int channel_index;
	for (channel_index = 0; channel_index < channel_count; channel_index++)
	{
		if (channel_size_table == NULL)
		{
			// Skip the channel index
			(void)GetBits(stream, channel_size_bit_count);
		}
		else
		{
			// Record the size of the channel (in bytes)
			channel_size_table[channel_index] = GetBits(stream, channel_size_bit_count);
		}
	}

	return CODEC_ERROR_OKAY;
}
#endif

#if VC5_ENABLED_PART(VC5_PART_COLOR_SAMPLING)
/*!
	@brief Adjust the width of a channel to account for chroma sampling
*/
DIMENSION ChannelWidth(DECODER *decoder, int channel_index, DIMENSION width)
{
	switch (decoder->encoded.format)
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

#if VC5_ENABLED_PART(VC5_PART_COLOR_SAMPLING)
/*!
	@brief Adjust the width of the layer (if necessary)

	Note that all layers have the same dimensions so the layer index is not
	passed as an argument to this routine.

	All layers have the same width as the encoded width.
*/
DIMENSION LayerWidth(DECODER *decoder, DIMENSION width)
{
	//CODEC_STATE *codec = &decoder->codec;
	(void)decoder;
	return width;
}
#endif

#if VC5_ENABLED_PART(VC5_PART_COLOR_SAMPLING)
/*!
	@brief Adjust the height of the layer to account for interlaced frames

	Note that all layers have the same dimensions so the layer index is not
	passed as an argument to this routine.
*/
DIMENSION LayerHeight(DECODER *decoder, DIMENSION height)
{
	CODEC_STATE *codec = &decoder->codec;

	if (codec->progressive == 0)
	{
		height /= 2;
	}

	return height;
}
#endif

/*!
	@brief Decode the specified wavelet subband

	After decoded the specified subband, the routine checks whether all bands
	in the current wavelet have been decoded and if so the inverse transform is
	applied to the wavelet to reconstruct the lowpass band in the wavelet at the
	next lower level.
*/
CODEC_ERROR DecodeChannelSubband(DECODER *decoder, BITSTREAM *input, size_t chunk_size)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;
	CODEC_STATE *codec = &decoder->codec;

	const int channel_number = codec->channel_number;
	const int subband_number = codec->subband_number;

	// Get the index to the wavelet corresponding to this subband
	const int index = SubbandWaveletIndex(subband_number);

	// Get the index of the wavelet band corresponding to this subband
	const int band = SubbandBandIndex(subband_number);

	// Wavelet containing the band to decode
	WAVELET *wavelet = NULL;

	//TODO: Need to check that the codeblock matches the chunk size
	(void)chunk_size;

	// Allocate the wavelets for this channel if not already allocated
	AllocateChannelWavelets(decoder, channel_number);

	// Is this a highpass band?
	if (subband_number > 0)
	{
		// Decode a highpass band

		// Get the wavelet that contains the highpass band
		wavelet = decoder->transform[channel_number].wavelet[index];

		// The wavelets are preallocated
		assert(wavelet != NULL);

		error = DecodeHighpassBand(decoder, input, wavelet, band);
		if (error == CODEC_ERROR_OKAY)
		{
			// Update the wavelet band valid flags
			UpdateWaveletValidBandMask(wavelet, band);
		}

		// Save the quantization factor
		wavelet->quant[band] = codec->band.quantization;
	}
	else
	{
		// Decode a lowpass band

		// Get the wavelet that contains the lowpass band
		wavelet = decoder->transform[channel_number].wavelet[index];

		// The lowpass band must be subband zero
		assert(subband_number == 0);

		// The lowpass data is always stored in wavelet band zero
		assert(band == 0);

		// The wavelets are preallocated
		assert(wavelet != NULL);

		error = DecodeLowpassBand(decoder, input, wavelet);
		if (error == CODEC_ERROR_OKAY)
		{
			// Update the wavelet band valid flags
			UpdateWaveletValidBandMask(wavelet, band);
		}
	}

	// Set the subband number for the next band expected in the bitstream
	codec->subband_number++;

	// Was the subband successfully decoded?
	if (error == CODEC_ERROR_OKAY)
	{
		// Record that this subband has been decoded successfully
		SetDecodedBandMask(codec, subband_number);
	}
    
    //printf("Decoded subband number: %d\n", subband_number);

	// Ready to invert this wavelet to get the lowpass band in the lower wavelet?
	if (BandsAllValid(wavelet))
	{
		// Apply the inverse wavelet transform to reconstruct the lower level wavelet
		error = ReconstructWaveletBand(decoder, channel_number, wavelet, index);

#if (0 && DEBUG)
		if (ChannelLowpassBandsAllValid(decoder, index))
		{
			int wavelet_level = index + 1;

			// Output the lowpass bands at this wavelet level (for debugging)
			WriteLowpassBands(decoder, wavelet_level, "lowpass%d.dpx");
		}
#endif
	}

	// Done decoding all subbands in this channel?
	if (codec->subband_number == codec->subband_count)
	{
		// Advance to the next channel
		codec->channel_number++;

		// Reset the subband number
		codec->subband_number = 0;
	}

	return error;
}

/*!
	@brief Invert the wavelet to reconstruct a lowpass band

	The bands in the wavelet at one level are used to compute the lowpass
	band in the wavelet at the next lower level in the transform.  Wavelet
	levels are numbered starting at zero for the original image.  The
	reference codec for the baseline profile uses the classic wavelet
	tree where each wavelet at a high level depends only on the wavelet
	at the next lower level and each wavelet is a spatial wavelet with
	four bands.

	This routine is called during decoding after all bands in a wavelet
	have been decoded and the lowpass band in the wavelet at the next
	lower level can be computed by applying the inverse wavelet transform.

	This routine is not called for the wavelet at level one to reconstruct the
	decoded component arrays.  Special routines are used to compute each component
	array using the wavelet at level one in each channel.
	
	See @ref ReconstructUnpackedImage.
*/
CODEC_ERROR ReconstructWaveletBand(DECODER *decoder, int channel, WAVELET *wavelet, int index)
{
	PRESCALE prescale = decoder->codec.prescale_table[index];
    
#if (0 && DEBUG)
    // Dump the lowpass bands in the wavelets at the current level
    int wavelet_level = index + 1;
    char pathname[PATH_MAX];
    sprintf(pathname, "decoded-%d.nv12", wavelet_level);
    CODEC_STATE *codec = &decoder->codec;
    DumpTransformWavelets(decoder->transform, codec->channel_count, wavelet_level, pathname);
#endif

	// Is the current wavelet at a higher level than wavelet level one?
	if (index > 0)
	{
		// Reconstruct the lowpass band in the lower wavelet
		const int lowpass_index = index - 1;
		WAVELET *lowpass;
		int lowpass_width;
		int lowpass_height;

		lowpass = decoder->transform[channel].wavelet[lowpass_index];
		assert(lowpass != NULL);
		if (! (lowpass != NULL)) {
			return CODEC_ERROR_UNEXPECTED;
		}

		lowpass_width = lowpass->width;
		lowpass_height = lowpass->height;

		// Check that the reconstructed wavelet is valid
		assert(lowpass_width > 0 && lowpass_height > 0);

		// Check that the lowpass band has not already been reconstructed
		assert((lowpass->valid_band_mask & BandValidMask(0)) == 0);

		// Check that all of the wavelet bands have been decoded
		assert(BandsAllValid(wavelet));

		// Decode the lowpass band in the wavelet one lower level than the input wavelet
		TransformInverseSpatialQuantLowpass(decoder->allocator, wavelet, lowpass, prescale);

		// Update the band valid flags
		UpdateWaveletValidBandMask(lowpass, 0);
	}

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Set the bit for the specified subband in the decoded band mask

	The decoded subband mask is used to track which subbands have been
	decoded in the current channel.  It is reset at the start of each
	channel.
*/
CODEC_ERROR SetDecodedBandMask(CODEC_STATE *codec, int subband)
{
	if (0 <= subband && subband < MAX_SUBBAND_COUNT) {
		codec->decoded_subband_mask |= (1 << subband);
	}
	return CODEC_ERROR_OKAY;
}

/*!
	@brief Decoded the lowpass band from the bitstream

	The wavelet at the highest level is passes as an argument.
	This routine decodes lowpass band in the bitstream into the
	lowpass band of the wavelet.
*/
CODEC_ERROR DecodeLowpassBand(DECODER *decoder, BITSTREAM *stream, WAVELET *wavelet)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;
	CODEC_STATE *codec = &decoder->codec;

	int lowpass_band_width;			// Lowpass band dimensions
	int lowpass_band_height;
	int lowpass_band_pitch;
	PIXEL *lowpass_band_ptr;		// Pointer into the lowpass band

	PRECISION lowpass_precision;	// Number of bits per lowpass coefficient

	int channel_offset;
	int row, column;

	lowpass_band_width = wavelet->width;
	lowpass_band_height = wavelet->height;
	lowpass_band_pitch = wavelet->pitch/sizeof(PIXEL);
	lowpass_band_ptr = wavelet->data[0];

	lowpass_precision = codec->lowpass_precision;

	// Set the channel offset to compensate for rounding errors
	channel_offset = 0;

	// Decode each row in the lowpass image
	for (row = 0; row < lowpass_band_height; row++)
	{
		for (column = 0; column < lowpass_band_width; column++)
		{
			COEFFICIENT lowpass_value = (COEFFICIENT)GetBits(stream, lowpass_precision);
			//assert(0 <= lowpass_value && lowpass_value <= COEFFICIENT_MAX);

			//if (lowpass_value > COEFFICIENT_MAX) {
			//	lowpass_value = COEFFICIENT_MAX;
			//}

			lowpass_band_ptr[column] = lowpass_value;
		}

		// Advance to the next row in the lowpass image
		lowpass_band_ptr += lowpass_band_pitch;
	}
	// Align the bitstream to the next tag value pair
	AlignBitsSegment(stream);

	// Return indication of lowpass decoding success
	return error;
}

/*!
	@brief Decode the highpass band from the bitstream

	The specified wavelet band is decoded from the bitstream
	using the codebook and encoding method specified in the
	bitstream.
*/
CODEC_ERROR DecodeHighpassBand(DECODER *decoder, BITSTREAM *stream, WAVELET *wavelet, int band)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;

	// Get the highpass band dimensions
	DIMENSION width = wavelet->width;	//codec->band.width;
	DIMENSION height = wavelet->height;	//codec->band.height;

	// Check that the band index is in range
	assert(0 <= band && band < wavelet->band_count);

	// Encoded coefficients start on a tag boundary
	AlignBitsSegment(stream);

		// Decode this subband
		error = DecodeBandRuns(stream, decoder->codebook, wavelet->data[band], width, height, wavelet->pitch);
		assert(error == CODEC_ERROR_OKAY);

	// Return failure if a problem was encountered while reading the band coefficients
	if (error != CODEC_ERROR_OKAY) {
		return error;
	}

	// The encoded band coefficients end on a bitstream word boundary
	// to avoid interference with the marker for the coefficient band trailer
	AlignBitsWord(stream);

	// Decode the band trailer
	error = DecodeBandTrailer(stream);
	decoder->error = error;
	assert(error == CODEC_ERROR_OKAY);
	return error;
}

/*!
	@brief Decode the highpass band from the bitstream

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
CODEC_ERROR DecodeBandRuns(BITSTREAM *stream, CODEBOOK *codebook, PIXEL *data,
						   DIMENSION width, DIMENSION height, DIMENSION pitch)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;
	size_t data_count;
	size_t row_padding;
	int row = 0;
	int column = 0;
	int index = 0;
	//BITWORD special;
	RUN run = RUN_INITIALIZER;

	// Convert the pitch to units of pixels
	pitch /= sizeof(PIXEL);

	// Check that the band dimensions are reasonable
	assert(width <= pitch);

	// Compute the number of pixels encoded into the band
	data_count = height * width;
	row_padding = pitch - width;

	while (data_count > 0)
	{
		// Get the next run length and value
		error = GetRun(stream, codebook, &run);
		if (error != CODEC_ERROR_OKAY) {
			return error;
		}

		// Check that the run does not extend past the end of the band
		assert(run.count <= data_count);

		// Copy the value into the specified number of pixels in the band
		while (run.count > 0)
		{
			// Reached the end of the column?
			if (column == width)
			{
				// Need to pad the end of the row?
				if (row_padding > 0)
				{
					int count;
					for (count = 0; (size_t)count < row_padding; count++) {
						data[index++] = 0;
					}
				}

				// Advance to the next row
				row++;
				column = 0;
			}

			data[index++] = (PIXEL)run.value;
			column++;
			run.count--;
			data_count--;
		}
	}

	// The last run should have ended at the end of the band
	assert(data_count == 0 && run.count == 0);

	// Check for the special codeword that marks the end of the highpass band
	error = GetRlv(stream, codebook, &run);
	if (error == CODEC_ERROR_OKAY) {
		if (! (run.count == 0 || run.value == SPECIAL_MARKER_BAND_END)) {
			error = CODEC_ERROR_BAND_END_MARKER;
		}
	}

	return error;
}

/*!
	@brief Decode the band trailer that follows a highpass band

	This routine aligns the bitstream to a tag value boundary.
	Currently the band trailer does not perform any function beyond
	preparing the bitstream for reading the next tag value pair.
*/
CODEC_ERROR DecodeBandTrailer(BITSTREAM *stream)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;

	// Advance the bitstream to a tag boundary
	AlignBitsSegment(stream);

	return error;
}

/*!
	@brief Return true if the bitstream has been completely decoded

	The end of sample flag is set to true when enough of the sample has been read
	from the bitstream to allow the output frame to be fully reconstructed.  Any
	remaining bits in the sample can be ignored and it may be the case that further
	reads from the bitstream will result in an error.

	The end of sample flag is set when the tag for the frame trailer is found, but
	may be set when sufficient subbands have been decoded to allow the frame to be
	reconstructed at the desired resolution.  For example, it is not an error if
	bands at level one in the wavelet tree are not present in the bitstream when
	decoding to half resolution.  The decoder should set the end of sample flag as
	soon as it is no longer necessary to read further information from the sample.
	
	@todo Rename this routine to end of image or end of bitstream?
*/
bool EndOfSample(DECODER *decoder)
{
	return decoder->codec.end_of_sample;
}

/*!
	@brief Return true if the entire bitstream header has been decoded

	The bitstream header has been completely decoded when at least one
	non-header parameter has been encountered in the bitstream and all
	of the required header parameters have been decoded.
	
	@todo Create a bitstream that can be used to test this predicate.
*/
bool IsHeaderComplete(DECODER *decoder)
{
	return (decoder->header_finished &&
			((decoder->header_mask & BITSTREAM_HEADER_FLAGS_REQUIRED) == BITSTREAM_HEADER_FLAGS_REQUIRED));
}

/*!
	@brief Return true if all channels in the bitstream have been processed

	It is only necessary to test the bands in the largest wavelet in each
	channel since its lowpass band would not be finished if the wavelets
	at the higher levels were incomplete.
*/
bool IsDecodingComplete(DECODER *decoder)
{
    // Still processing the bitstream header?
    if (!decoder->header_finished)
    {
        // Decoding cannot be complete
        return false;
    }
    else
    {
        int channel_count = decoder->codec.channel_count;
        int channel_index;

        for (channel_index = 0; channel_index < channel_count; channel_index++)
        {
            WAVELET *wavelet = decoder->transform[channel_index].wavelet[0];

            // Processing is not complete if the wavelet has not been allocated
            if (wavelet == NULL) return false;

            // Processing is not complete unless all bands have been processed
            if (!AllBandsValid(wavelet)) return false;
        }
        
        // All bands in all wavelets in all channels are done
        return true;
    }
}

/*!
	@brief Perform the final wavelet transform in each channel to compute the component arrays

	Each channel is decoded and the lowpass and highpass bands are used to reconstruct the
	lowpass band in the wavelet at the next lower level by applying the inverse wavelet filter.
	Highpass band decoding and computation of the inverse wavelet transform in each channel
	stops when the wavelet at the level immediately above the output frame is computed.

	This routine performs the final wavelet transform in each channel and combines the channels
	into a single output frame.  Note that this routine is called for each layer in a sample,
	producing an output frame for each layer.  The output frames for each layer must be combine
	by an image compositing operation into a single output frame for the fully decoded sample.
*/
CODEC_ERROR ReconstructUnpackedImage(DECODER *decoder, UNPACKED_IMAGE *image)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;
	ALLOCATOR *allocator = decoder->allocator;

	int channel_count = decoder->codec.channel_count;
	int channel_number;

	// Check for enough space in the local array allocations
	//assert(channel_count <= MAX_CHANNEL_COUNT);

	// Allocate the vector of component arrays
	size_t size = channel_count * sizeof(COMPONENT_ARRAY);
	image->component_array_list = Alloc(allocator, size);
	if (image->component_array_list == NULL) {
		return CODEC_ERROR_OUTOFMEMORY;
	}

	// Clear the component array information so that the state is consistent
	image->component_count = 0;
	memset(image->component_array_list, 0, size);

	for (channel_number = 0; channel_number < channel_count; channel_number++)
	{
		// Get the dimensions of this channel
		DIMENSION channel_width = decoder->channel[channel_number].width;
		DIMENSION channel_height = decoder->channel[channel_number].height;
		PRECISION bits_per_component = decoder->channel[channel_number].bits_per_component;

		// Amount of prescaling applied to the component array values before encoding
		PRESCALE prescale = decoder->codec.prescale_table[0];

		// Allocate the component array for this channel
		error = AllocateComponentArray(allocator,
                                       &image->component_array_list[channel_number],
                                       channel_width,
                                       channel_height,
                                       bits_per_component);
        if (error != CODEC_ERROR_OKAY) {
            return error;
        }
        
		error = TransformInverseSpatialQuantArray(allocator,
                                                  decoder->transform[channel_number].wavelet[0],
                                                  image->component_array_list[channel_number].data,
                                                  channel_width,
                                                  channel_height,
                                                  image->component_array_list[channel_number].pitch,
                                                  prescale);
        if (error != CODEC_ERROR_OKAY) {
            return error;
        }
	}

	// One component array is output by the decoding process per channel in the bitstream
	image->component_count = channel_count;
    
#if (0 && DEBUG)
    DumpUnpackedImage(image, "decoded-0.nv12");
#endif

	return error;
}

#if 0   //VC5_ENABLED_PART(VC5_PART_LAYERS)
/*!
	@brief Perform the final wavelet transform in each channel to compute the output frame

	Each channel is decoded and the lowpass and highpass bands are used to reconstruct the
	lowpass band in the wavelet at the next lower level by applying the inverse wavelet filter.
	Highpass band decoding and computation of the inverse wavelet transform in each channel
	stops when the wavelet at the level immediately above the output frame is computed.

	This routine performs the final wavelet transform in each channel and combines the channels
	into a single output frame.  Note that this routine is called for each layer in a sample,
	producing an output frame for each layer.  The output frames for each layer must be combine
	by an image compositing operation into a single output frame for the fully decoded sample.

	Refer to @ref ReconstructSampleFrame for the details of how the frames from each layer
	are combined to produce the output frame for the decoded sample.
*/
CODEC_ERROR ReconstructLayerImage(DECODER *decoder, IMAGE *image)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;

	DIMENSION decoded_width = decoder->decoded.width;
	DIMENSION decoded_height = decoder->decoded.height;
	int channel_count = decoder->codec.channel_count;

	//DIMENSION layer_width = LayerWidth(decoder, decoded_width);
	//DIMENSION layer_height = LayerHeight(decoder, decoded_height);
	DIMENSION layer_width = decoded_width;
	DIMENSION layer_height = decoded_height;

	//TODO: Adjust the layer width to account for chroma sampling

	// Allocate a buffer for the intermediate output from each wavelet transform
	size_t decoded_frame_pitch = layer_width * channel_count * sizeof(PIXEL);
	size_t decoded_frame_size = layer_height * decoded_frame_pitch;
	PIXEL *decoded_frame_buffer = (PIXEL *)Alloc(decoder->allocator, decoded_frame_size);
	if (decoded_frame_buffer == NULL) {
		return CODEC_ERROR_OUTOFMEMORY;
	}

	error = TransformInverseSpatialQuantBuffer(decoder, decoded_frame_buffer, (DIMENSION)decoded_frame_pitch);
	if (error == CODEC_ERROR_OKAY)
	{
		// Pack the decoded frame into the output format
		error = PackOutputImage(decoded_frame_buffer, decoded_frame_pitch, decoder->encoded.format, image);
	}

	// Free the buffer for the decoded frame
	Free(decoder->allocator, decoded_frame_buffer);

	return error;
}
#endif

#if VC5_ENABLED_PART(VC5_PART_LAYERS)
/*!
	@brief Combine multiple decoded frames from each layer into a single output frame

	An encoded sample may contain multiple sub-samples called layers.  For example,
	there may be two sub-samples (layers) for the left and right frames in a stereo pair.

	Note that the baseline profile supports only one layer per sample.

	Each layer is decoded independently to produce an output frame for that layer.  The
	CineForm codec does not support dependent sub-samples in any of the existing profiles.
	
	This routine forms a composite frame for the output of the completely decoded sample
	from the individual frames obtained by decoding each layer.  It is contemplated that
	any image compositing algorithm could be used to combine decoded layers, although the
	most sophisticated algorithms might be reserved for the most advanced profiles.

	The dimensions of the output frame could be much larger than the dimensions of any
	of the frames decoded from individual layers.  Compositing could overlay the frames
	from the individual layers with an arbitrary spatial offset applied to the frame from
	each layer, creating a collage from frames decoded from the individual layers.  Typical
	applications may use only the most elementary compositing operations.
*/
CODEC_ERROR ReconstructSampleFrame(DECODER *decoder, IMAGE image_array[], int frame_count, IMAGE *output_image)
{
	DIMENSION frame_width = image_array[0].width;
	DIMENSION field_height = image_array[0].height;
	DIMENSION frame_height = 2 * field_height;
	PIXEL_FORMAT frame_format = image_array[0].format;

	AllocImage(decoder->allocator, output_image, frame_width, frame_height, frame_format);

	return ComposeFields(image_array, frame_count, output_image);
}
#endif


// Set the global debug flag used by the metadata database routines
bool debug_flag = true;

// Define the log file used by the metadata database routines
FILE *logfile = NULL;


#if VC5_ENABLED_PART(VC5_PART_METADATA)


static inline uint32_t GetUint32(BITSTREAM *bitstream)
{
    assert(IsAlignedSegment(bitstream));
    //return Swap32(GetBits(bitstream, 32));
    return GetBits(bitstream, 32);
}

/*!
    @brief Decode the payload of a metadata chunk element
*/
CODEC_ERROR DecodeMetadataChunk(DECODER *decoder, BITSTREAM *bitstream, TAGWORD chunk_tag, int32_t chunk_size)
{
    CODEC_ERROR error = CODEC_ERROR_OKAY;
    DATABASE *database = decoder->metadata.database;
    assert(database != NULL);

    //logfile = decoder->logfile;
    logfile = stderr;

    // Size of a metadata tuple header (in bytes)
    const size_t metadata_tuple_header_size = 8;

    // if (decoder->debug_flag) {
    //     fprintf(stderr, "Decoding metadata chunk tag: 0x%04X, size: %u\n", chunk_tag, chunk_size);
    // }

    //bool verbose_flag = database->verbose_flag;
    //bool debug_flag = database->debug_flag;

    // error = InitializeMetadataDatabase(database);
    // if (error != CODEC_ERROR_OKAY) {
    //     return error;
    // }

    //printf("Metadata chunk element tag: 0x%04X, size: %d\n", chunk_tag, chunk_size);

    error = InsertDatabaseChunk(database, chunk_tag, chunk_size);
    if (error != CODEC_ERROR_OKAY) {
        return error;
    }

    // Push the node for the chunk element onto the node stack
    //PushNode(database->chunk);

    //SkipPayload(stream, chunk_size);
    // printf("Skiped metadata chunk payload\n");
    // Process the chunk payload at the next level in the tuple hierarchy
    //current_level++;

    // The bitstream should be aligned to a segment boundary
    assert(IsAlignedSegment(bitstream));

    // Read metadata tuples from the chunk payload
    size_t payload_size_remaining = chunk_size * sizeof(SEGMENT);
    while (payload_size_remaining > 0 && !EndOfBitsteam(bitstream) && bitstream->error == BITSTREAM_ERROR_OKAY)
    {
        FOURCC tuple_tag = Swap32(GetUint32(bitstream));
        if (bitstream->error != BITSTREAM_ERROR_OKAY) {
            return BitstreamCodecError(bitstream);
        }

        // Read the next segment containing the data type and the tuple size and repeat count
        SEGMENT type_size_count = GetUint32(bitstream);
        if (bitstream->error != BITSTREAM_ERROR_OKAY) {
            return BitstreamCodecError(bitstream);
        }

        // Swap the segment into network (big endian) order
        // type_size_count = Swap32(type_size_count);

        // The first byte is the tuple data type
        char type = (type_size_count >> 24);
        size_t count;
        size_t size;

        if (HasRepeatCount(type))
        {
            // One byte size and two byte count and two byte repeast count
            size = (type_size_count >> 16) & 0xFF;
            count = (type_size_count & 0xFFFF);
        }
        else
        {
            // Three byte size with no repeat count
            size = type_size_count & 0xFFFFFF;
            count = 0;
        }

        // if (decoder->debug_flag) {
        //     fprintf(stderr, "Tuple tag: %c%c%c%c, type: %c, size: %zu, count: %zu\n", FOURCC_CHARS(tuple_tag), type, size, count);
        // }

        // Update the nesting level based on the new tuple tag and the current nested tag and level
        //UpdateNestingLevel(tuple_tag, type, &database->current_level, &database->next_level);
        UpdateDatabaseLevel(database, tuple_tag, type);

        if (decoder->verbose_flag)
        {
            printf("%sTuple tag: %c%c%c%c, type: %c, size: %zu, count: %zu\n",
                CurrentLevelIndentation(database), FOURCC_CHARS(tuple_tag), type, size, count);
        }

        //if (tuple_tag == TupleTag("CFHD"))
        if (IsClassInstance(tuple_tag, type))
        {
            size_t padding = TuplePadding(size, 0);

            TUPLE_HEADER tuple_header = {tuple_tag, type, size, 0, padding};
            error = InsertDatabaseClass(database, &tuple_header);
            if (error != CODEC_ERROR_OKAY) {
                return error;
            }

            //TODO: Check that the current node at the top of the stack is a metadata chunk element

            // Push the node for the metadata class instance onto the node stack
            // PushNode(class);

            // Subtract the size of the class instance header from the payload size
            payload_size_remaining -= metadata_tuple_header_size;
        }
        else
        {
            // if (debug_flag && (tuple_tag == FOURCC_VALUE("GYRO") || tuple_tag == FOURCC_VALUE("LAYD")))
            // {
            //  fprintf(stderr, "Tuple: %c%c%c%c, size: %zd, count: %zd, total_size: %zd, padding: %zd, payload_size: %zd\n",
            //      FOURCC_CHARS(tuple_tag), size, count, total_size, padding, payload_size);
            // }

            // Round up the size to a segment boundary
            // size_t segment_count = (size + 3)/4;
            // size_t padding = 4 * segment_count - size;

            size_t total_size = ((count > 0) ? count : 1) * size;
            //fprintf(stderr, "%zd\n", total_size);

            size_t segment_count = (total_size + 3) / 4;
            size_t payload_size = 4 * segment_count;
            size_t padding = payload_size - total_size;

            // Define the tuple header (set the payload after the payload is read from the file)
            TUPLE tuple = {
                {tuple_tag, type, size, count, padding},
                NULL,
                0
            };

            // Tuple must have a payload unless it is a nested tuple or an encoding curve
            assert(tuple.header.type == 0 || tuple.header.type == 'P' || total_size > 0);

            // Output the tuple value unless this is a nested tuple
            if (! IsNestedTuple(tuple.header.type))
            {
                // Read the payload (metadata value and padding)
                //size_t padding = sizeof(SEGMENT) - (total_size % sizeof(SEGMENT));
                //if (padding == sizeof(SEGMENT)) padding = 0;
                //size_t payload_size = total_size + padding;
                ALLOC_BUFFER(payload, payload_size);
                // int file_result = fread(payload, sizeof(payload), 1, input_file);
                // if (file_result != 1) {
                //     return CODEC_ERROR_FILE_READ;
                // }
                error = GetByteArray(bitstream, payload, payload_size);
                if (error != CODEC_ERROR_OKAY) {
                    return error;
                }

                tuple.payload = payload;
                tuple.payload_size = payload_size;

                error = InsertDatabaseTuple(database, &tuple);
                if (error != CODEC_ERROR_OKAY) {
                    return error;
                }

                // Subtract the size of this metadata tuple from the payload size
                payload_size_remaining -= (metadata_tuple_header_size + payload_size);
            }
            else
            {
                error = InsertDatabaseTuple(database, &tuple);
                if (error != CODEC_ERROR_OKAY) {
                    return error;
                }

                // Subtract the size of the nested tuple header from the payload size
                payload_size_remaining -= metadata_tuple_header_size;
            }

            // if (debug_flag) {
            //     fprintf(stderr, "Tuple: %c%c%c%c, size: %zd, count: %zd, total_size: %zd, padding: %zd, payload_size: %zd\n",
            //         FOURCC_CHARS(tuple_tag), size, count, total_size, padding, payload_size);
            // }
            //DumpNodeInfo(tuple, "New node");

            //printf("Tuple: %c%c%c%c, type: %c (%X)\n", FOURCC_CHARS(tuple_tag), ((type == 0) ? '0' : type), type);
        }

        // Update the nesting level for the next tuple in the bitstream
        //database->current_level = database->next_level;
        SetDatabaseNextLevel(database);

        if (decoder->debug_flag) {
            fprintf(stderr, "Payload size remaining: %zu\n", payload_size_remaining);
        }
    }

    // if (verbose_flag) fprintf(stderr, "\n");

    if (bitstream->error != BITSTREAM_ERROR_OKAY) {
        if (decoder->debug_flag) fprintf(stderr, "Bitstream read error: %d\n", bitstream->error);
        return CODEC_ERROR_FILE_READ;
    }

    // if (database->duplicates_flag)
    // {
        // Prune duplicate entries from the database
        error = PruneDatabaseDuplicateTuples(database);
        assert(error == CODEC_ERROR_OKAY);
        if (! (error == CODEC_ERROR_OKAY)) {
            return error;
        // }
    }

    return error;
}

#endif 

#if _DEBUG

/*!
	@brief Print the values used by teh decoder for dequantization.
*/
CODEC_ERROR PrintDecoderQuantization(const DECODER *decoder)
{
	//FILE *logfile = decoder->logfile;
	FILE *logfile = stdout;

	if (logfile)
	{
		int channel_count = decoder->codec.channel_count;
		int channel;

		for (channel = 0; channel < channel_count; channel++)
		{
			int wavelet_count = decoder->wavelet_count;

#if (1 && DEBUG)
			fprintf(logfile, "Quantization for channel: %d\n", channel);
			PrintTransformQuantization(&decoder->transform[channel], wavelet_count, logfile);
			fprintf(logfile, "\n");
#endif
		}
	}

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Write selected subbands to a wavelet band file

	Each bit in the subband mask argument specifies whether the corresponding
	subband should be written to the wavelet band file.

	Note that the band file can contain reconstructed lowpass bands, but this
	routine only write decoded subbands to the wavelet band file.

	@todo Change the channel index into a channel mask
*/
CODEC_ERROR DumpDecodedSubbands(DECODER *decoder,
								int channel_index,
								uint32_t subband_mask,
								const char *pathname)
{
	BANDFILE file;
	int max_band_width;
	int max_band_height;
	int subband;

	//TODO: Modify this routine to take the frame index as an argument
	const int frame_index = 0;

	// Compute the maximum dimensions of each subband
	max_band_width = decoder->codec.image_width;
	max_band_height = decoder->codec.image_height;

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
			WAVELET *wavelet = decoder->transform[channel_index].wavelet[wavelet_index];
			int width = wavelet->width;
			int height = wavelet->height;
			void *data = wavelet->data[band_index];
			size_t size = wavelet->width * wavelet->height * sizeof(PIXEL);

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
CODEC_ERROR DumpWaveletBands(DECODER *decoder,
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

	//TODO: Modify this routine to take the image index as an argument
	const int frame_index = 0;

	if (decoder == NULL) {
		return CODEC_ERROR_NULLPTR;
	}

	if (! (0 <= channel_index && channel_index < decoder->codec.channel_count)) {
		return CODEC_ERROR_BAD_ARGUMENT;
	}

	if (! (0 <= wavelet_index && wavelet_index < decoder->wavelet_count)) {
		return CODEC_ERROR_BAD_ARGUMENT;
	}

	// Get the specified wavelet from the transform
	wavelet = decoder->transform[channel_index].wavelet[wavelet_index];
	assert(wavelet != NULL);

	// Compute the maximum dimensions of each subband
	max_band_width = decoder->codec.image_width;
	max_band_height = decoder->codec.image_height;

	// Create the band file
	CreateBandFile(&file, pathname);

	// Write the band file header
	WriteFileHeader(&file, max_band_width, max_band_height);

	for (band_index = 0; band_mask != 0; band_mask >>= 1, band_index++)
	{
		// Write this subband?
		if ((band_mask & 0x01) != 0)
		{
			int width = wavelet->width;
			int height = wavelet->height;
			void *data = wavelet->data[band_index];
			size_t size = wavelet->width * wavelet->height * sizeof(PIXEL);

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
CODEC_ERROR DumpTransformBands(DECODER *decoder,
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

	// Get the number of channels in the decoder wavelet transform
	channel_count = decoder->codec.channel_count;

	// Compute the maximum dimensions of each subband
	max_band_width = decoder->codec.image_width;
	max_band_height = decoder->codec.image_height;

	// Create the band file
	CreateBandFile(&file, pathname);

	// Write the band file header
	WriteFileHeader(&file, max_band_width, max_band_height);

	for (channel_index = 0;
		 channel_index < channel_count && channel_mask != 0;
		 channel_mask >>= 1, channel_index++)
	{
		if ((channel_mask & 0x01) != 0)
		{
			uint32_t wavelet_mask = channel_wavelet_mask;
			int wavelet_count = decoder->wavelet_count;
			int wavelet_index;

			for (wavelet_index = 0;
				 wavelet_index < wavelet_count && wavelet_mask != 0;
				 wavelet_mask >>= 1, wavelet_index++)
			{
				// Write bands in this wavelet?
				if ((wavelet_mask & 0x01) != 0)
				{
					WAVELET *wavelet = decoder->transform[channel_index].wavelet[wavelet_index];
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
							int width = wavelet->width;
							int height = wavelet->height;
							void *data = wavelet->data[band_index];
							size_t size = wavelet->width * wavelet->height * sizeof(PIXEL);

							WriteWaveletBand(&file, frame_index, channel_index, wavelet_index,
											 band_index, BAND_TYPE_SINT16, width, height, data, size);
						}
					}
				}
			}
		}
	}

	CloseBandFile(&file);

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Write selected wavelet subbands to a band file
*/
CODEC_ERROR DumpTransformSubbands(DECODER *decoder,
								  uint32_t channel_mask,
								  uint32_t subband_mask,
								  const char *pathname)
{
	BANDFILE file;
	int max_band_width;
	int max_band_height;
	int channel_count;
	int channel_index;
	int result;

	//TODO: Modify this routine to take the frame index as an argument
	const int frame_index = 0;

	// Get the number of channels in the decoder wavelet transform
	channel_count = decoder->codec.channel_count;

	// Compute the maximum dimensions of each subband
	max_band_width = decoder->codec.image_width;
	max_band_height = decoder->codec.image_height;

	// Create the band file
	result = CreateBandFile(&file, pathname);
	if (result != BANDFILE_ERROR_OKAY) {
		return CODEC_ERROR_BANDFILE_FAILED;
	}

	// Write the band file header
	result = WriteFileHeader(&file, max_band_width, max_band_height);
	if (result != BANDFILE_ERROR_OKAY) {
		return CODEC_ERROR_BANDFILE_FAILED;
	}

	for (channel_index = 0;
		 channel_index < channel_count && channel_mask != 0;
		 channel_mask >>= 1, channel_index++)
	{
		if ((channel_mask & 0x01) != 0)
		{
			uint32_t channel_subband_mask = subband_mask;
			int subband_count = MAX_SUBBAND_COUNT;
			int subband_index;

			for (subband_index = 0;
				 subband_index < subband_count && channel_subband_mask != 0;
				 channel_subband_mask >>= 1, subband_index++)
			{
				// Write this subband to the band file?
				if ((channel_subband_mask & 0x01) != 0)
				{
					int wavelet_index = SubbandWaveletIndex(subband_index);
					int band_index = SubbandBandIndex(subband_index);
					WAVELET *wavelet = decoder->transform[channel_index].wavelet[wavelet_index];
					int width = wavelet->width;
					int height = wavelet->height;
					void *data = wavelet->data[band_index];
					size_t size = wavelet->width * wavelet->height * sizeof(PIXEL);

					WriteWaveletBand(&file, frame_index, channel_index, wavelet_index,
									 band_index, BAND_TYPE_SINT16, width, height, data, size);
				}
			}
		}
	}

	CloseBandFile(&file);

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Sum the prescale shifts up to the specified wavelet level
*/
int PrescaleSum(const PRESCALE *prescale_table, int wavelet_level)
{
	int sum = 0;
	int index;
	for (index = 0; index < wavelet_level; index++)
	{
		sum += prescale_table[index];
	}
	return sum;
}

#include "dpxfile.h"
#include "fileinfo.h"

/*!
	@brief Write the lowpass bands at the specified wavelet level to a DPX file.

	Note that the index of the wavelet in the wavelet tree is one less than the
	wavelet level because level zero coresponds to the original image.

	This routine only works if the component arrays constitute an RGB image.
*/
CODEC_ERROR WriteLowpassBands(const DECODER *decoder,
							  int wavelet_level,
							  const char *basename)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;

	const TRANSFORM *transform = decoder->transform;
	int channel_count = decoder->codec.channel_count;
	int wavelet_index = wavelet_level - 1;

	char pathname[PATH_MAX];

	IMAGE output;

	DIMENSION width;
	DIMENSION height;
	PIXEL_FORMAT format;

	void *lowpass_buffer_array[MAX_CHANNEL_COUNT];
	size_t lowpass_pitch_array[MAX_CHANNEL_COUNT];

	int channel_number;

	int file_type = FILE_TYPE_UNKNOWN;

	const int prescale_shift = PrescaleSum(decoder->codec.prescale_table, wavelet_level);
	const int shift = 4 - (wavelet_level * 2) + prescale_shift;

	if (decoder->codec.channel_count != 3)
	{
		// The component arrays do not correspond to an RGB image
		return CODEC_ERROR_UNSUPPORTED_FORMAT;
	}

	assert(0 <= wavelet_index && wavelet_index < decoder->wavelet_count);

	// Get the dimensions and format of the lowpass image
	width = transform[0].wavelet[wavelet_index]->width;
	height = transform[0].wavelet[wavelet_index]->height;

	// Use the pixel format obtained from the biitstream
	format = decoder->codec.input.format;

	// Use the pixel format from the external parameters
	if (format == PIXEL_FORMAT_UNKNOWN) {
		format = decoder->input.format;
	}

	// Cannot continue if the pixel format is unknown
	if (format == PIXEL_FORMAT_UNKNOWN) {
		return CODEC_ERROR_PIXEL_FORMAT;
	}

	// Allocate the DPX output image
	AllocImage(NULL, &output, width, height, PIXEL_FORMAT_DPX_50);

	for (channel_number = 0; channel_number < channel_count; channel_number++)
	{
		WAVELET *wavelet = transform[channel_number].wavelet[wavelet_index];
		lowpass_buffer_array[channel_number] = wavelet->data[0];
		lowpass_pitch_array[channel_number] = wavelet->pitch;
	}

	// Pack the components into an image of DPX pixels
	PackLowpassBandsToDPX0(lowpass_buffer_array, lowpass_pitch_array, width, height, format, shift, &output);

	// Compute the output pathname from the pathname template
	sprintf(pathname, basename, wavelet_level);

	file_type = GetFileType(basename);
	assert(file_type == FILE_TYPE_DPX);
	if (! (file_type == FILE_TYPE_DPX)) {
		return CODEC_ERROR_UNSUPPORTED_FILE_TYPE;
	}

	// Write the DPX image to the output file
	error = DPX_WriteImage(&output, pathname);

	ReleaseImage(NULL, &output);

	return error;
}

#endif
