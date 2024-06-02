/*!	@file decoder/src/main.c

	Main program for the reference decoder.

	This module is not part of the reference codec but is included
	to allow the codec to be tested.

	It is expected that an actual implementation of the codec will
	use a static or dynamic library for the decoder and a similar
	library for the encoder, but the reference decoder is a console
	application so that it can be easily tested without having to
	write a calling application.

	Encoded bitstreams are stored one image per file and decoded images
	are stored one image per file without any header or metadata.

	The program returns the error code from the decoder.

	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#include <string.h>
#include "headers.h"
#include "bandfile.h"
#include "dpxfile.h"
#include "fileinfo.h"
#include "convert.h"
#include "parseargs.h"

/*!
	@brief Write an image to a file without a file header

	This routine works correctly for Bayer images even though the image
	dimensions are twice the width and height of the rectangular grid of
	Bayer pattern elements because the pitch is the correct value for one
	row of samples and the height is the number of rows of samples in
	the image.
*/
CODEC_ERROR WriteImageRAW(const IMAGE *image, const char *pathname)
{
	FILE *file = fopen(pathname, "wb");
	if (file != NULL)
	{
		size_t image_size = image->height * image->pitch;
        size_t count;
        
        if (image->format == PIXEL_FORMAT_NV12)
        {
            // The image buffer contains the luma plane followed immediately by the chroma plane
            image_size += image->height * image->pitch / 2;
        }
        
        // Write the buffer of pixel data to the binary file
		count = fwrite(image->buffer, image_size, 1, file);
        
		fclose(file);
        
        if (count == 1)
        {
            // The output file was written successfully
            return CODEC_ERROR_OKAY;
        }
        else
        {
            // Could not write the output file
            return CODEC_ERROR_FILE_WRITE;
        }
	}
    
    // Could not create the output file
	return CODEC_ERROR_CREATE_FILE_FAILED;
}

/*!
	@brief Write an image to a file in DPX format

	The input image will be converted to the 10-bit RGB pixel format
	commonly used in DPX files if necessary.  Instead of applying a
	demosaic filter to Bayer images, this routine averages the green
	samples and writes a DPX image with half the width and height.
*/
CODEC_ERROR WriteImageDPX(const IMAGE *image, const char *pathname)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;

	if (image->format != PIXEL_FORMAT_DPX0)
	{
		IMAGE output;

		DIMENSION image_width = image->width;
		DIMENSION image_height = image->height;
		size_t image_pitch = image->pitch;

		if (IsBayerFormat(image->format))
		{
			// The dimensions must be in units of Bayer pattern elements
			image_width /= 2;
			image_height /= 2;
			image_pitch *= 2;
		}

		// Allocate the DPX output image
		AllocImage(NULL, &output, image_width, image_height, PIXEL_FORMAT_DPX_50);

		// Must signal whether pixels are byte swapping before calling the conversion routine below
		DPX_SetByteSwapFlag();

		switch (image->format)
		{
		case PIXEL_FORMAT_BYR3:
			ConvertBYR3ToDPX0(image->buffer, image_pitch, output.buffer, output.pitch, image_width, image_height);
			break;

		case PIXEL_FORMAT_BYR4:
			ConvertBYR4ToDPX0(image->buffer, image_pitch, output.buffer, output.pitch, image_width, image_height);
			break;

		case PIXEL_FORMAT_RG48:
			ConvertRG48ToDPX0(image->buffer, image_pitch, output.buffer, output.pitch, image_width, image_height);
			break;

		case PIXEL_FORMAT_B64A:
			ConvertB64AToDPX0(image->buffer, image_pitch, output.buffer, output.pitch, image_width, image_height);
			break;

		default:
			assert(0);
			break;
		}

		// Write the DPX image to the output file
		error = DPX_WriteImage(&output, pathname);

		ReleaseImage(NULL, &output);
	}
	else
	{
		// Write the DPX image to the output file
		error = DPX_WriteImage(image, pathname);
	}

	return error;
}

/*!
	@brief Convenience routine for writing a image to a file

	The file type is determined by the pathname extension and the
	image is converted to the output format if necessary.
*/
CODEC_ERROR WriteImage(const IMAGE *image, const char *pathname)
{
	int file_type = GetFileType(pathname);

	switch (file_type)
	{
	case FILE_TYPE_RAW:
		return WriteImageRAW(image, pathname);
		break;

	case FILE_TYPE_DPX:
		return WriteImageDPX(image, pathname);
		break;

	default:
		assert(0);
		break;
	}

	return CODEC_ERROR_UNSUPPORTED_FILE_TYPE;
}

/*!
	@brief Write an unpacked image to separate files of component arrays

	The output file pathname must be a string format specification that can be used to create
	a valid pathname with the channel number encoded in the directory path or filename so that
	each component array can be written to a separate file.
 */
CODEC_ERROR WriteComponentArrays(const UNPACKED_IMAGE *image,
								 FILELIST *output_filelist,
								 ALLOCATOR *allocator)
{
    CODEC_ERROR error = CODEC_ERROR_OKAY;
	char pathname[PATH_MAX];
	int channel_count = image->component_count;
	int channel_index;
	uint32_t *component_buffer = NULL;
	int component_index;

	for (channel_index = 0; channel_index < channel_count; channel_index++)
	{
		DIMENSION channel_width = image->component_array_list[channel_index].width;
		DIMENSION channel_height = image->component_array_list[channel_index].height;
		COMPONENT_VALUE *channel_data = image->component_array_list[channel_index].data;
		int component_count = channel_width * channel_height;
		size_t buffer_size = component_count * sizeof(uint32_t);
		FILE *file = NULL;

		// Allocate a buffer for the array of scaled component values
		component_buffer = Alloc(allocator, buffer_size);
		if (component_buffer == NULL) {
			return CODEC_ERROR_OUTOFMEMORY;
		}

		// Scale the component array values to 32 bit unsigned integers
		for (component_index = 0; component_index < component_count; component_index++)
		{
			component_buffer[component_index] = channel_data[component_index] << 20;
		}

		// Get the pathname to the file for this component array
        error = GetNextFileListPathname(output_filelist, pathname, sizeof(pathname));
        if (error != CODEC_ERROR_OKAY) {
            return CODEC_ERROR_UNEXPECTED;
        }

		file = fopen(pathname, "wb");
		if (file == NULL) {
			return CODEC_ERROR_CREATE_FILE_FAILED;
		}

		if (fwrite(component_buffer, buffer_size, 1, file) != 1) {
			return CODEC_ERROR_FILE_WRITE_FAILED;
		}

		Free(allocator, component_buffer);
	}

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Display the packed image output by the image repacking process

	This routine converts the packed image into a displayable picture by
	writing the image to a DPX file.
*/
CODEC_ERROR DisplayProcess(const PACKED_IMAGE *image,
						   const char *pathname,
						   const PARAMETERS *parameters)
{
	(void)parameters;
	return WriteImageDPX(image, pathname);
}

/*!
	@brief Main entry point for the reference decoder

	The program takes two arguments: the pathname to the file that contains a
	sample to decode and the pathname to the output file for the decoded image.

	The input file should contain a single encoded sample without any header.

	The output file can be a DPX file, otherwise the decoded image is written to the output
	file without a header.

	The image is decoded to the same dimensions as the encoded image and the decoded format
	is the same format as the original source image input to the encoder.
*/
int main(int argc, char *argv[])
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;
	PARAMETERS parameters;
    FILELIST input_filelist;
    FILELIST output_filelist;
    //FILE_INFO info;
    char input_pathname[PATH_MAX];
	STREAM input_stream;

	DATABASE *database = NULL;

	bool duplicates_flag = false;		/*****DEBUG*****/

	// Initialize the parameters and file lists
	InitParameters(&parameters);
    InitFileList(&input_filelist, NULL);
    InitFileList(&output_filelist, NULL);

	// Parse the command-line arguments
	error = ParseParameters(argc, argv, &parameters, &input_filelist, &output_filelist);
	if (error != CODEC_ERROR_OKAY) {
		return error;
	}

	// Must provide exactly one input bitstream file on the command line
    assert(FileListHasSinglePathname(&input_filelist));
	if (! (FileListHasSinglePathname(&input_filelist))) {
        fprintf(stderr, "Must provide one input file for the bitstream on the command line\n");
		return CODEC_ERROR_MISSING_ARGUMENT;
	}

    error = GetNextFileListPathname(&input_filelist, input_pathname, sizeof(input_pathname));
    if (error != CODEC_ERROR_OKAY) {
        return CODEC_ERROR_BAD_ARGUMENT;
    }
    
	// Check that the enabled parts are correct
	error =  CheckEnabledParts(&parameters.enabled_parts);
	if (error != CODEC_ERROR_OKAY) {
		return CODEC_ERROR_ENABLED_PARTS;
	}

#if 0
	// The output format should have been set when the command-line arguments were processed
	if (parameters.output.format == PIXEL_FORMAT_UNKNOWN)
	{
		// Get information about the output file format
		GetFileInfo(argv[2], &info);
		parameters.output.format = info.format;
	}
	assert(parameters.output.format != PIXEL_FORMAT_UNKNOWN);
#endif
    
#if VC5_ENABLED_PART(VC5_PART_SECTIONS)
    if (IsPartEnabled(parameters.enabled_parts, VC5_PART_SECTIONS) &&
        parameters.enabled_sections)
    {
#if 0
        if (FileListHasSinglePathname(&output_filelist))
        {
            // Get the pathname for the decoded sections log file
            SetSectionsLogfilePathname(&parameters, SingleFileListPathname(&output_filelist));
        }
        else
        {
            // Use one file to record section information for all output files
            SetSectionsLogfilePathname(&parameters, "sections.log");
        }
#else
        // Set the pathname for the decoded sections log file
        SetSectionsLogfilePathname(&parameters, SingleFileListPathname(&input_filelist));
#endif
    }
#endif

	if (parameters.verbose_flag)
	{
		printf("Codec parts: 0x%02X\n", parameters.enabled_parts);
		printf("Input bitstream: %s\n", input_pathname);
        
        if (output_filelist.pathname_count == 1 && !output_filelist.template_flag) {
            printf("Output pathname: %s\n", output_filelist.pathname_list[0]);
        }
		//if (argc > 3) {
		//	printf("Display file: %s\n", argv[3]);
		//}
		if (strlen(parameters.bandfile.pathname) > 0 &&
			parameters.bandfile.channel_mask != 0 &&
			parameters.bandfile.subband_mask != 0) {
				printf("Band file: %s\n", parameters.bandfile.pathname);
		}
#if VC5_ENABLED_PART(VC5_PART_SECTIONS)
        if (IsPartEnabled(parameters.enabled_parts, VC5_PART_SECTIONS) &&
            parameters.enabled_sections)
        {
            printf("Section logfile: %s\n", parameters.sections.logfile_pathname);
        }
#endif
#if VC5_ENABLED_PART(VC5_PART_METADATA)
        if (IsPartEnabled(parameters.enabled_parts, VC5_PART_METADATA) && parameters.metadata.output_flag)
        {
            printf("Metadata output: %s\n", parameters.metadata.output_pathname);
        }
#endif
	}

#if VC5_ENABLED_PART(VC5_PART_METADATA)
    if (IsPartEnabled(parameters.enabled_parts, VC5_PART_METADATA) && parameters.metadata.output_flag)
    {
    	error = CreateMetadataDatabase(&database, parameters.verbose_flag, parameters.debug_flag, duplicates_flag);
    	if (error != CODEC_ERROR_OKAY) {
    		return error;
    	}

    	// error = InitMetadataProcessing(database);
    	// if (error != CODEC_ERROR_OKAY) {
    	// 	return error;
    	// }
    }
#endif

	// Open a stream to the input file
	error = OpenStream(&input_stream, input_pathname);
	if (error != CODEC_ERROR_OKAY) {
		fprintf(stderr, "Could not open input file: %s\n", input_pathname);
		return error;
	}

#if VC5_ENABLED_PART(VC5_PART_SECTIONS)
    if (IsPartEnabled(parameters.enabled_parts, VC5_PART_SECTIONS))
    {
        DECODER decoder;
        BITSTREAM bitstream;
        TAGVALUE segment;
        //bool more_layers_flag = true;
        
        // Initialize the bitstream data structure
        InitBitstream(&bitstream);
        
        // Bind the bitstream to the byte stream
        error = AttachBitstream(&bitstream, &input_stream);
        if (error != CODEC_ERROR_OKAY) {
            return error;
        }

// #if VC5_ENABLED_PART(VC5_PART_METADATA)
//     if (IsPartEnabled(parameters.enabled_parts, VC5_PART_METADATA) && parameters.metadata.output_flag)
//     {
//     	// error = CreateMetadataDatabase(&database, parameters.verbose_flag, parameters.debug_flag, duplicates_flag);
//     	// if (error != CODEC_ERROR_OKAY) {
//     	// 	return error;
//     	// }

//     	// Initialize the metadata database
//         error = InitMetadataDatabase(&database, &parameters, duplicates_flag);
//         if (error != CODEC_ERROR_OKAY) {
//             return error;
//         }
//     }

//     // if (parameters.debug_flag) {
//     //     printf("Metadata database: %p\n", database);
//     // }
// #endif

        // Initialize the decoder with a default allocator
        PrepareDecoder(&decoder, NULL, database, &parameters);
        
        // Get the bitstream start marker
        segment = GetSegment(&bitstream);
        assert(segment.longword == StartMarkerSegment);
        if (! (segment.longword == StartMarkerSegment)) {
            return CODEC_ERROR_MISSING_START_MARKER;
        }
        
        /*
            Sections are enabled, but do not know whether the bitstream contains image sections.
            Assume that there are multiple image sections.  If the bitstream does not contain any
            image sections, then the single output image will be saved in the first pathname in
            the pathname list.  If the bitstream contains image sections, then each decoded image
            will be stored in a seperate output file obtained from the pathname list.
         */
        
        for (;;)
        {
            char pathname[PATH_MAX];
            PACKED_IMAGE output_image;
            bool missing_pathname_flag = false;

            // Get the next output pathname
            error = GetNextFileListPathname(&output_filelist, pathname, sizeof(pathname));
            if (error != CODEC_ERROR_OKAY)
            {
                if (error == CODEC_ERROR_FILELIST_MISSING_PATHNAME)
                {
                    //NOTE: There may be no more pathnames because the last image section has been decoded
                    missing_pathname_flag = true;
                }
                else
                {
                    return error;
                }
            }
            
            // The output format should have been set when the command-line arguments were processed
            if (parameters.output.format == PIXEL_FORMAT_UNKNOWN)
            {
                if (missing_pathname_flag)
                {
                    // Use the input pixel format
                    parameters.output.format = parameters.input.format;
                }
                
                if (parameters.output.format == PIXEL_FORMAT_UNKNOWN)
                {
                    // Get information about the output file format
                    FILE_INFO info;                
                    GetFileInfo(pathname, &info);
                    parameters.output.format = info.format;
                }
            }
            assert(parameters.output.format != PIXEL_FORMAT_UNKNOWN);

            // Decode the single image in the image section or the next layer if the image section contains nested layers
            error = DecodeImageSection(&decoder, &bitstream, &output_image, database, &parameters);
            if (error != CODEC_ERROR_OKAY) {
                break;
            }
            
            // Write the output image to the next output pathname
            error = WriteImage(&output_image, pathname);
            if (error != CODEC_ERROR_OKAY)
            {
                fprintf(stderr, "Could not write output image to file: %s\n", pathname);
                return error;
            }
            
            printf("Output pathname: %s\n", pathname);
            
            if (AllImageSectionsDecoded(&decoder)) {
                break;
            }
            
            // Decoding multiple layers in the bitstream?
            if (decoder.codec.layer_count > 0)
            {
                // Update decoder and codec state to indicate that the layer has been decoded
                UpdateLayerParameters(&decoder);
                
                // Reset the wavelet flags so that all wavelet bands in the next layer are decoded
                ResetWaveletDecodingFlags(&decoder);

                // Prepare for processing the next image section in case the bitstream contains multiple image sections
                if (AllLayersDecoded(&decoder))
                {
                    // Update decoder and codec state to indicate that the image section has been decoded
                    ResetDecoderImageSection(&decoder, NULL, &parameters);
                }
            }
            else
            {
                // Update decoder and codec state to indicate that the image section has been decoded
                ResetDecoderImageSection(&decoder, NULL, &parameters);
            }
       }
    }
    else
#endif
#if VC5_ENABLED_PART(VC5_PART_LAYERS)
    if (IsPartEnabled(parameters.enabled_parts, VC5_PART_LAYERS))
    {
        DECODER decoder;
        BITSTREAM bitstream;
        PACKED_IMAGE output_image;
        TAGVALUE segment;
        //bool more_layers_flag = true;

        // Initialize the bitstream data structure
        InitBitstream(&bitstream);
        
        // Bind the bitstream to the byte stream
        error = AttachBitstream(&bitstream, &input_stream);
        if (error != CODEC_ERROR_OKAY) {
            return error;
        }

// #if VC5_ENABLED_PART(VC5_PART_METADATA)

//     if (IsPartEnabled(parameters.enabled_parts, VC5_PART_METADATA) && parameters.metadata.output_flag)
//     {
//         // Initialize the metadata database
//         error = InitMetadataDatabase(&database, &parameters, duplicates_flag);
//         if (error != CODEC_ERROR_OKAY) {
//             return error;
//         }
//     }

//     // if (parameters.debug_flag) {
//     //     printf("Metadata database: %p\n", database);
//     // }

// #endif

        // Initialize the decoder with a default allocator
        PrepareDecoder(&decoder, NULL, database, &parameters);
        
        /*
            Set the output file format in the parameters.  This method only works for
            layers since each layer must have the same image dimensions and format.
         */
        if (parameters.output.width == 0) {
            FILE_INFO info;
            GetFileInfo(output_filelist.pathname_list[0], &info);
            parameters.output.format = info.format;
        }
        
        // Get the bitstream start marker
        segment = GetSegment(&bitstream);
        if (segment.longword != StartMarkerSegment)
        {
            return CODEC_ERROR_MISSING_START_MARKER;
        }
        
        //***** Need to parse the bitstream header *****//
        
        for (;;)
        {
            char pathname[PATH_MAX];
            
            error = DecodeLayer(&decoder, &bitstream, &output_image, database, &parameters);
            if (error != CODEC_ERROR_OKAY) {
                return error;
            }
            
            // Get the next output pathname
            error = GetNextFileListPathname(&output_filelist, pathname, sizeof(pathname));
            if (error != CODEC_ERROR_OKAY) {
                return CODEC_ERROR_BAD_LAYER_IMAGE_LIST;
            }
            
            printf("Output pathname: %s\n", pathname);
            
            // Write the output image to the next output pathname
            error = WriteImage(&output_image, pathname);
            if (error != CODEC_ERROR_OKAY)
            {
                fprintf(stderr, "Could not write output image to file: %s\n", pathname);
                return error;
            }
            
            // Update decoder and codec state to indicate that the layer has been decoded
            UpdateLayerParameters(&decoder);

            if (AllLayersDecoded(&decoder)) {
                break;
            }
            
            // Reset the wavelet flags so that all wavelet bands in the next layer are decoded
            ResetWaveletDecodingFlags(&decoder);
        }
    }
    else
#endif
	// Decode the bitstream into separate component arrays?
	if (parameters.output.format == PIXEL_FORMAT_CA32)
	{
		// The unpacked image will hold the component arrays decoded from the bitstream
		UNPACKED_IMAGE unpacked_image;

		// The component arrays will be allocated after the bitstream is decoded
		InitUnpackedImage(&unpacked_image);

		// Decode the stream into an unpacked image (separate component arrays) and process metadata chunk elements
		// Decode the stream into an unpacked image (separate component arrays)
		error = DecodeStream(&input_stream, &unpacked_image, NULL, &parameters);

		//TODO: 

		if (error != CODEC_ERROR_OKAY) {
			fprintf(stderr, "Error decoding bitstream: %s\n", argv[1]);
			return error;
		}

		// Write the unpacked image to separate files (one file per component array)
		error = WriteComponentArrays(&unpacked_image, &output_filelist, NULL);
		if (error != CODEC_ERROR_OKAY) {
            fprintf(stderr, "Could not write output image to file: %s\n", output_filelist.last_pathname);
			return error;
		}
#if 0
		//TODO: Add code to write a displayable image from the unpacked component arrays
		if (argc > 3)
		{
			// Write a displayable picture to a file
			error = DisplayProcess(&output_image, argv[3], &parameters);
			if (error != CODEC_ERROR_OKAY)
			{
				fprintf(stderr, "Could not write displayable image to file: %s\n", argv[3]);
				return error;
			}
		}
#endif
	}
	else
	{
        IMAGE output_image;
        char pathname[PATH_MAX];

#if VC5_ENABLED_PART(VC5_PART_METADATA)
    // if (IsPartEnabled(parameters.enabled_parts, VC5_PART_METADATA) && parameters.metadata.output_flag)
    // {
    //     // Initialize the metadata database
    //     error = InitMetadataDatabase(&database, &parameters, duplicates_flag);
    //     if (error != CODEC_ERROR_OKAY) {
    //         return error;
    //     }
    // }

    // if (parameters.debug_flag) {
    //     printf("Metadata database: %p\n", database);
    // }
#endif

		// Decode the stream into a packed output image
		error = DecodeImage(&input_stream, &output_image, database, &parameters);
		if (error != CODEC_ERROR_OKAY) {
			fprintf(stderr, "Error decoding bitstream: %s\n", input_pathname);
			return error;
		}
        
        // Get the next output pathname from the file list
        error = GetNextFileListPathname(&output_filelist, pathname, sizeof(pathname));
        if (error != CODEC_ERROR_OKAY) {
            return error;
        }

        // Write the output from the image repacking process to a file
		error = WriteImage(&output_image, pathname);
		if (error != CODEC_ERROR_OKAY)
		{
			fprintf(stderr, "Could not write output image to file: %s\n", argv[2]);
			return error;
		}
#if 0
		if (argc > 3)
		{
			// Write a displayable picture to a file
			error = DisplayProcess(&output_image, argv[3], &parameters);
			if (error != CODEC_ERROR_OKAY)
			{
				fprintf(stderr, "Could not write displayable image to file: %s\n", argv[3]);
				return error;
			}
		}
#endif
	}

#if VC5_ENABLED_PART(VC5_PART_METADATA)
    if (IsPartEnabled(parameters.enabled_parts, VC5_PART_METADATA) && parameters.metadata.output_flag)
    {
        //printf("Metadata file: %s\n", parameters.metadata.output_pathname);

        // Write the metadata database to the output file in XML format
        FILE *output_file = fopen(parameters.metadata.output_pathname, "w");
        assert(output_file != NULL);
        if (! (output_file != NULL)) {
            fprintf(stderr, "Could not open metadata output file: %s\n", parameters.metadata.output_pathname);
            return CODEC_ERROR_FILE_OPEN;
        }

        error = OutputMetadataDatabase(database, output_file);
        if (error != CODEC_ERROR_OKAY) {
            return error;
        }

        // Close the metadata output file
        fclose(output_file);

        // Free the metadata database
        if (database != NULL) {
            DestroyMetadataDatabase(database);
        }
    }
#endif

    CloseStream(&input_stream);

	return CODEC_ERROR_OKAY;
}
