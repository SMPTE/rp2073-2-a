/*!	@file encoder/src/layers.c
 
	Implementation of code for encoding layers
 
	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
 */

#include "headers.h"


#if 0   //VC5_ENABLED_PART(VC5_PART_LAYERS)

/*!
    @brief Read a list of image files specified by a format specification string
*/
CODEC_ERROR ReadImageList(IMAGE_LIST *image_list, PARAMETERS *parameters, const char *format_specification_string)
{
    CODEC_ERROR error = CODEC_ERROR_OKAY;
    int image_index;
    
    // Initialize the image list
    InitImageList(image_list, parameters->layer_count);
    
    for (image_index = 0; image_index < parameters->layer_count; image_index++)
    {
        char pathname[PATH_MAX];
        FILE_INFO info;
        
        // Compute the actual pathname from the format specification string
        sprintf(pathname, format_specification_string, image_index);
        
        // Get the pixel format and precision of the input image
        GetFileInfo(pathname, &info);
        
        if (info.type == FILE_TYPE_RAW)
        {
            DIMENSION image_width = parameters->input.width;
            DIMENSION image_height = parameters->input.height;
            PIXEL_FORMAT pixel_format = parameters->input.format;
            
            // The caller must provide the image dimensions and pixel format
            assert(image_width > 0 && image_height > 0 && pixel_format != PIXEL_FORMAT_UNKNOWN);
            
            // Allocate space for the next image in the list
            AllocListImage(NULL, image_list, image_index, image_width, image_height, pixel_format);
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
        error = ReadListImage(image_list, image_index, pathname);
        if (error != CODEC_ERROR_OKAY) {
            fprintf(stderr, "Could not read input image: %s\n", pathname);
        }
    }
    
    return error;
}

/*!
    @brief Read the next image in a list of images

    The image list must be iniitalized before reading any images into the list
*/
CODEC_ERROR ReadListImage(IMAGE_LIST *image_list, int image_index, const char *pathname)
{
    assert(image_list != NULL);
    if (! (image_list != NULL)) {
        return CODEC_ERROR_BAD_ARGUMENT;
    }
    
    if (image_index < image_list->image_count) {
        return ReadImage(image_list->image_list[image_index], pathname);
    }
    
    return CODEC_ERROR_UNEXPECTED;
}

#endif
