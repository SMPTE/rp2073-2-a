/*!	@file main.c

	This module is not part of the reference codec but is included
	to allow the codec to be tested.

	It is expected that an actual implementation of the codec will
	use a static or dynamic library for the encoder and a similar
	library for the decoder, but the reference encoder is a console
	application so that it can be easily tested without having to
	write a calling application.

	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#include "headers.h"


/*!
	@brief Main entry point for the reference encoder

	Usage: encoder [options] input output
	
	The input argument is the pathname to a file that contains a single image that
	is the input the image unpacking process (see @ref ImageUnpackingProcess).  The
	input file can be a DPX file or an unformatted file that contains the input image
	without a header, in which case the filename extension indicates the pixel format
	of the input image.  The output argument is the pathname to a file that will contain
	the encoded bitstream.  Media containers are not currently supported by the reference
	encoder.  The command-line options are described in @ref ParseParameters.
*/
int main(int argc, const char *argv[])
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;
	STREAM output;
	PARAMETERS parameters;

	// Performance timer
	TIMER timer;
	InitTimer(&timer);

	// Initialize the data structure for passing parameters to the encoder
	InitParameters(&parameters);
    SetDefaultParameters(&parameters);

    // Obtain parameters by parsing the command line
	error = ParseParameters(argc, argv, &parameters);
	if (error != CODEC_ERROR_OKAY) {
		return error;
	}

    // The quiet flag overrides the verbose and debug flags
    if (parameters.quiet_flag) {
        parameters.verbose_flag = false;
        parameters.debug_flag = false;
    }

    // Fill missing values in the parameters obtained from the command line
    SetMissingParameters(&parameters);
    
	// Check that the enabled parts are correct
	error = CheckEnabledParts(&parameters.enabled_parts);
	if (error != CODEC_ERROR_OKAY) {
		return error;
	}

    if (parameters.verbose_flag)
    {
        // Print the flags indicating which parts are enabled for this encoder
        printf("Codec parts: 0x%02X\n", parameters.enabled_parts);

        // Print the list of input images passed on the command line
        PrintPathnameList(&parameters.input_pathname_list, "Input image");
        
        // Print the output pathname for the encoded bitstream
        printf("Output file: %s\n", parameters.output_pathname);
        //printf("\n");
    }
    
    // Open a stream to the output file
    error = CreateStream(&output, parameters.output_pathname);
    if (error != CODEC_ERROR_OKAY) {
        fprintf(stderr, "Could not create output file: %s\n", parameters.output_pathname);
        return error;
    }
    
#if VC5_ENABLED_PART(VC5_PART_LAYERS) && VC5_ENABLED_PART(VC5_PART_SECTIONS)
    if (IsPartEnabled(parameters.enabled_parts, VC5_PART_LAYERS) &&
        IsPartEnabled(parameters.enabled_parts, VC5_PART_SECTIONS) &&
        IsSectionEnabled(parameters.enabled_sections, SECTION_NUMBER_IMAGE))
    {
        // Encode the input images as layers within image sections in a single bitstream
        if (parameters.input_pathname_list.pathname_count > 0)
        {
            IMAGE_LIST image_list;
            
            // Initialize the image list
            InitImageList(&image_list, parameters.input_pathname_list.pathname_count);
            
            // Allocate the list of packed images based on the parameters and read the image files
            error = ReadInputPathnameList(&image_list, &parameters.input_pathname_list);
            if (error != CODEC_ERROR_OKAY) {
                //fprintf(stderr, "Could not read image file list: %s\n", argv[1]);
                return error;
            }
            
            if (image_list.image_count > 0)
            {
#if (0 && DEBUG)
                // Report the actual dimensions and pixel format of each image in the list
                if (parameters.verbose_flag) {
                    PrintImageList(&image_list);
                }
#endif
                StartTimer(&timer);
                
                // Encode the list of images into the byte stream
                error = EncodeImageSectionLayers(&image_list, &output, &parameters);
                if (error != CODEC_ERROR_OKAY) {
                    //fprintf(stderr, "Error encoding image list: %s\n", argv[1]);
                    return error;
                }
                
                StopTimer(&timer);
            }
        }
    }
    else
#endif
#if VC5_ENABLED_PART(VC5_PART_LAYERS)
    if (IsPartEnabled(parameters.enabled_parts, VC5_PART_LAYERS))
    {
        // Encode each input image as a layer in a single bitstream
        if (parameters.input_pathname_list.pathname_count > 0)
        {
            IMAGE_LIST image_list;
            
            // Initialize the image list
            InitImageList(&image_list, parameters.input_pathname_list.pathname_count);
            
            // Allocate the list of packed images based on the parameters and read the image files
            error = ReadInputPathnameList(&image_list, &parameters.input_pathname_list);
            if (error != CODEC_ERROR_OKAY) {
                //fprintf(stderr, "Could not read image file list: %s\n", argv[1]);
                return error;
            }
            
            if (image_list.image_count > 0)
            {
#if (0 && DEBUG)
                // Report the actual dimensions and pixel format of each image in the list
                if (parameters.verbose_flag) {
                    PrintImageList(&image_list);
                }
#endif
                StartTimer(&timer);
                
                // Encode the list of images into the byte stream
                error = EncodeImageList(&image_list, &output, &parameters);
                if (error != CODEC_ERROR_OKAY) {
                    //fprintf(stderr, "Error encoding image list: %s\n", argv[1]);
                    return error;
                }
                
                StopTimer(&timer);
            }
        }
    }
    else
#endif
#if VC5_ENABLED_PART(VC5_PART_SECTIONS)
    if (IsPartEnabled(parameters.enabled_parts, VC5_PART_SECTIONS) &&
        IsSectionEnabled(parameters.enabled_sections, SECTION_NUMBER_IMAGE))
    {
        // Encode each input image as an image section in a single bitstream
        if (parameters.input_pathname_list.pathname_count > 0)
        {
            IMAGE_LIST image_list;
            
            // Initialize the image list
            InitImageList(&image_list, parameters.input_pathname_list.pathname_count);

            // Allocate the list of packed input images in the parameters and read the image files
            error = ReadInputPathnameList(&image_list, &parameters.input_pathname_list);
            if (error != CODEC_ERROR_OKAY) {
                fprintf(stderr, "Could not read input image file list\n");
                return error;
            }

            if (image_list.image_count > 0)
            {
#if (0 && DEBUG)
                // Report the actual dimensions and pixel format of each image in the list
                if (parameters.verbose_flag) {
                    PrintImageList(&image_list);
                }
#endif
                StartTimer(&timer);
                 
                // Encode the list of images into the byte stream
                error = EncodeImageList(&image_list, &output, &parameters);
                if (error != CODEC_ERROR_OKAY) {
                    //fprintf(stderr, "Error encoding image list: %s\n", argv[1]);
                    return error;
                }
                 
                StopTimer(&timer);
            }
        }
    }
    else
#endif
#if VC5_ENABLED_PART(VC5_PART_METADATA)
    if (IsPartEnabled(parameters.enabled_parts, VC5_PART_METADATA) && parameters.verbose_flag)
    {
        printf("\n");
        printf("Inject data: %s\n", strlen(parameters.metadata_pathname) > 0 ? parameters.metadata_pathname : "(none)");
    }
#endif

    if (parameters.verbose_flag) {
        printf("\n");
    }

    {
        // Encode one input image without layers or image sections in a single bitstream
       	IMAGE image;
        DIMENSION width;
        DIMENSION height;
        PIXEL_FORMAT format;
        const char *pathname;
        
        // There should have been exactly one input image pathname on the command line
        assert(parameters.input_pathname_list.pathname_count == 1);
        if ( !(parameters.input_pathname_list.pathname_count == 1)) {
            return CODEC_ERROR_MISSING_ARGUMENT;
        }
        
        width = parameters.width;
        height = parameters.height;
        format = parameters.pixel_format;
        pathname = parameters.input_pathname_list.pathname_data[0].pathname;

        // The image dimensions and pixel format should have been determined already
        assert(width > 0 && height > 0 && format != PIXEL_FORMAT_UNKNOWN);
        if (! (width > 0 && height > 0 && format != PIXEL_FORMAT_UNKNOWN)) {
            return CODEC_ERROR_UNEXPECTED;
        }
                
        // Allocate the packed input image based on the parameters and read the image file
        error = ReadImageFile(&image, width, height, format, pathname);
        if (error != CODEC_ERROR_OKAY) {
            fprintf(stderr, "Could not read input file: %s\n", pathname);
            return error;
        }
        
        if (parameters.verbose_flag) {
            // Report the actual dimensions and pixel format of the image read from the file
            printf("Input image width: %d, height: %d, format: %s\n\n", width, height, PixelFormatName(format));
        }

        // Set the dimensions and pixel format of the packed input image
        //SetInputImageParameters(&parameters, &image);
        
        StartTimer(&timer);
        
        // Encode the image into the byte stream
        error = EncodeImage(&image, &output, &parameters);
        if (error != CODEC_ERROR_OKAY) {
            fprintf(stderr, "Error encoding image: %s (%d)\n", parameters.output_pathname, error);
            return error;
        }
        
        StopTimer(&timer);
    }

#if _TIMING
	printf("Encoding time: %.3f ms\n", TimeMS(&timer));
	printf("\n");
#endif
    
    // Close the output stream
    CloseStream(&output);
    
    // Cleanup all memory allocated by the program
    ReleaseParameters(&parameters, NULL);

	return CODEC_ERROR_OKAY;
}
