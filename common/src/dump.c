/*!	@file common/src/dump.c
 
	Routines to write intermediate wavelet bands to image files
 
    The image files written by the routines in this module are unformatted files
    with pixel formats like the packed image files input to the image unpacking
    process prior to encoding or output by the image repacking process after
    decoding.
 
	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
 */

#include "headers.h"

static CODEC_ERROR DumpWaveletBands_NV12(WAVELET *wavelet_array[],
                                         int channel_count,
                                         int scale_shift,
                                         PACKED_IMAGE *output_image);

static CODEC_ERROR DumpComponentArrays_NV12(COMPONENT_ARRAY *component_array_list,
                                            PACKED_IMAGE *output_image);

static CODEC_ERROR WriteImageFile(const IMAGE *image, const char *pathname);


/*
    @brief Dump the lowpass band in all transform wavelets at the specified level
 
 */
CODEC_ERROR DumpTransformWavelets(TRANSFORM transform_array[MAX_CHANNEL_COUNT],
                                  int channel_count,
                                  int wavelet_level,
                                  const char *pathname)
{
    CODEC_ERROR error = CODEC_ERROR_OKAY;
    WAVELET *wavelet_array[MAX_CHANNEL_COUNT];
    int wavelet_index = wavelet_level - 1;
    int channel_index;
    int scale_shift;
    FILE_INFO info;
    PACKED_IMAGE output_image;
    DIMENSION width;
    DIMENSION height;

    assert(0 < channel_count && channel_count <= MAX_CHANNEL_COUNT);
    assert(0 <= wavelet_index && wavelet_index < MAX_WAVELET_COUNT);
    
    memset(wavelet_array, 0, sizeof(wavelet_array));

    // Get the pixel format for the unformatted file type
    error = GetFileInfo(pathname, &info);
    if (error != CODEC_ERROR_OKAY) {
        return error;
    }
    
    // This routine only writes unformatted files
    assert(info.type == FILE_TYPE_RAW);
    if (! (info.type == FILE_TYPE_RAW)) {
        return CODEC_ERROR_UNSUPPORTED_FILE_TYPE;
    }
    
    // Fill an array with pointers to the transform wavelets at the specified level
    for (channel_index = 0; channel_index < channel_count; channel_index++)
    {
        wavelet_array[channel_index] = transform_array[channel_index].wavelet[wavelet_index];
    }

    // The image dimensions are the dimensions of the luma channel
    width = wavelet_array[0]->width;
    height = wavelet_array[0]->height;
    
    // Allocate a packed image for the output image
    error = AllocImage(NULL, &output_image, width, height, info.format);
    if (error != CODEC_ERROR_OKAY) {
        return error;
    }
    
    // Set the amount of shift performed during image unpacking
    int unpacking_shift = 4;
    
    // Set the amount of prescale shift
    int prescale_shift = (wavelet_level > 1) ? 2 : 0;
    
    // The component values are scaled horizontally and vertically at each level in the wavelet
    scale_shift = (wavelet_level + wavelet_level) + unpacking_shift - prescale_shift;
    
    switch (info.format)
    {
        case PIXEL_FORMAT_NV12:
            DumpWaveletBands_NV12(wavelet_array, channel_count, scale_shift, &output_image);
            break;
            
        default:
            assert(0);
            break;
    }
    
    // Write the image to the output file
    WriteImageFile(&output_image, pathname);
    
    // Free the packed image
    ReleaseImage(NULL, &output_image);
    
    return error;
}

/*
    @brief Dump an unpacked image to an unformatted image file
 
    An unpacked image might be the output from the image unpacking process that precedes
    the encoding process or the input to the image repacking process that follows the
    decoding process.
 */
CODEC_ERROR DumpUnpackedImage(UNPACKED_IMAGE *image, const char *pathname)
{
    CODEC_ERROR error = CODEC_ERROR_OKAY;
    PACKED_IMAGE output_image;
    DIMENSION width;
    DIMENSION height;
    FILE_INFO info;
    
    // The image dimensions are determined by the first channel
    width = image->component_array_list[0].width;
    height = image->component_array_list[0].height;
    
    // Get the pixel format for the unformatted file type
    error = GetFileInfo(pathname, &info);
    if (error != CODEC_ERROR_OKAY) {
        return error;
    }

    // Allocate a packed image for the output image
    error = AllocImage(NULL, &output_image, width, height, info.format);
    if (error != CODEC_ERROR_OKAY) {
        return error;
    }
    
    switch (info.format)
    {
        case PIXEL_FORMAT_NV12:
            DumpComponentArrays_NV12(image->component_array_list, &output_image);
            break;
            
        default:
            assert(0);
            break;
    }

    // Write the image to the output file
    WriteImageFile(&output_image, pathname);
    
    // Free the packed image
    ReleaseImage(NULL, &output_image);
    
    return error;
}
 
/*
    @brief Dump the lowpass band in each wavelet to a file in NV12 format
 */
static CODEC_ERROR DumpWaveletBands_NV12(WAVELET *wavelet_array[],
                                         int channel_count,
                                         int scale_shift,
                                         PACKED_IMAGE *output_image)
{
    PIXEL *Y_lowpass_band = wavelet_array[0]->data[0];
    PIXEL *U_lowpass_band = wavelet_array[1]->data[0];
    PIXEL *V_lowpass_band = wavelet_array[2]->data[0];
    
    DIMENSION width = output_image->width;
    DIMENSION height = output_image->height;

    uint8_t *upper_output_plane = (uint8_t *)output_image->buffer;
    uint8_t *lower_output_plane = upper_output_plane + (width * height);
    
    int luma_row;
    
    for (luma_row = 0; luma_row < height; luma_row++)
    {
        PIXEL *Y_input_row = Y_lowpass_band + luma_row * wavelet_array[0]->pitch/sizeof(PIXEL);
        uint8_t *upper_output_row = upper_output_plane + luma_row * width;
        int column;
        
        // Pack the row of luma values into the upper half of the output buffer
        for (column = 0; column < width; column++)
        {
            PIXEL Y = Y_input_row[column];
            upper_output_row[column] = (Y >> scale_shift);
        }
        
        // Pack a row of color difference components for every second row of luma components
        if ((luma_row % 2) == 0)
        {
            // Pack a row of color difference components
            int chroma_row = luma_row/2;
            PIXEL *U_input_row = U_lowpass_band + chroma_row * wavelet_array[1]->pitch/sizeof(PIXEL);
            PIXEL *V_input_row = V_lowpass_band + chroma_row * wavelet_array[2]->pitch/sizeof(PIXEL);
            uint8_t *lower_output_row = lower_output_plane + chroma_row * width;
            int column;
            
            for (column = 0; column < width/2; column++)
            {
                PIXEL U = U_input_row[column];
                PIXEL V = V_input_row[column];
                
                lower_output_row[2 * column + 0] = (U >> scale_shift);
                lower_output_row[2 * column + 1] = (V >> scale_shift);
            }
        }
    }
    
    return CODEC_ERROR_OKAY;
}

/*!
    @brief Dump the component arrays to an unformatted image file
 */
static CODEC_ERROR DumpComponentArrays_NV12(COMPONENT_ARRAY *component_array_list,
                                            PACKED_IMAGE *output_image)
{
    COMPONENT_VALUE *Y_lowpass_band = component_array_list[0].data;
    COMPONENT_VALUE *U_lowpass_band = component_array_list[1].data;
    COMPONENT_VALUE *V_lowpass_band = component_array_list[2].data;
    
    DIMENSION width = output_image->width;
    DIMENSION height = output_image->height;
    
    uint8_t *upper_output_plane = (uint8_t *)output_image->buffer;
    uint8_t *lower_output_plane = upper_output_plane + (width * height);
    
    // Set the amount of shift performed during image unpacking
    int unpacking_shift = 4;
    
    int luma_row;
    
    for (luma_row = 0; luma_row < height; luma_row++)
    {
        COMPONENT_VALUE *Y_input_row = Y_lowpass_band + luma_row * component_array_list[0].pitch/sizeof(COMPONENT_VALUE);
        uint8_t *upper_output_row = upper_output_plane + luma_row * width;
        int column;
        
        // Pack the row of luma values into the upper half of the output buffer
        for (column = 0; column < width; column++)
        {
            COMPONENT_VALUE Y = Y_input_row[column];
            upper_output_row[column] = (Y >> unpacking_shift);
        }
        
        // Pack a row of color difference components for every second row of luma components
        if ((luma_row % 2) == 0)
        {
            // Pack a row of color difference components
            int chroma_row = luma_row/2;
            COMPONENT_VALUE *U_input_row = U_lowpass_band + chroma_row * component_array_list[1].pitch/sizeof(PIXEL);
            COMPONENT_VALUE *V_input_row = V_lowpass_band + chroma_row * component_array_list[2].pitch/sizeof(PIXEL);
            uint8_t *lower_output_row = lower_output_plane + chroma_row * width;
            int column;
            
            for (column = 0; column < width/2; column++)
            {
                COMPONENT_VALUE U = U_input_row[column];
                COMPONENT_VALUE V = V_input_row[column];
                
                lower_output_row[2 * column + 0] = (U >> unpacking_shift);
                lower_output_row[2 * column + 1] = (V >> unpacking_shift);
            }
        }
    }
    
    return CODEC_ERROR_OKAY;
}

/*!
    @brief Write a packed image to a unformatted image file
 
    Note: This routine duplicates a routine in the decoder
 
    @todo Share the code for writing images between the encoder and decoder
 */
static CODEC_ERROR WriteImageFile(const IMAGE *image, const char *pathname)
{
    FILE *file = fopen(pathname, "wb");
    if (file != NULL)
    {
        size_t image_size = image->height * image->pitch;
        
        if (image->format == PIXEL_FORMAT_NV12)
        {
            // The image buffer contains the luma plane followed immediately by the chroma plane
            image_size += image->height * image->pitch / 2;
        }
        
        // Write the buffer of pixel data to the binary file
        fwrite(image->buffer, image_size, 1, file);
        
        fclose(file);
        
        return CODEC_ERROR_OKAY;
    }
    
    return CODEC_ERROR_CREATE_FILE_FAILED;
}
             
