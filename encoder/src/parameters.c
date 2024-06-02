/*!	@file encoder/src/parameters.c

	Implementation of the data structure used to pass parameters
	to the encoder.

	The parameters data structure is currently a simple struct, but
	fields may be added, removed, or replaced.  A version number is
	included in the parameters data structure to allow decoders to
	adapt to changes.

	It is contemplated that future inplementations may use a dictionary
	of key-value pairs which would allow the decoder to determine whether
	a parameter is present.
    
	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#include "headers.h"

//! Current version number of the parameters data structure
#define PARAMETERS_VERSION		1

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
// Forward reference
CODEC_ERROR SetPathnameDataImageFormat(PATHNAME_DATA *pathname_data);
#endif


/*!
	@brief Initialize the parameters data structure

	The version number of the parameters data structure must be
	incremented whenever a change is made to the definition of
	the parameters data structure.

	@todo Special initialization required by the metadata?
*/
CODEC_ERROR InitParameters(PARAMETERS *parameters)
{
	memset(parameters, 0, sizeof(PARAMETERS));
	parameters->version = PARAMETERS_VERSION;

	// Set the default value for the number of bits per lowpass coefficient
	parameters->lowpass_precision = 16;

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
	// The maximum number of bits per component is the internal precision
	//parameters->max_bits_per_component  = internal_precision;
#endif
 
	// The elementary bitstream is always enabled
	parameters->enabled_parts = VC5_PART_MASK(VC5_PART_ELEMENTARY);

#if 0   //VC5_ENABLED_PART(VC5_PART_LAYERS)
	parameters->layer_count = 1;
    parameters->layer_flag = false;
	//memset(parameters->format_specification_string, 0, sizeof(parameters->format_specification_string));
#endif

	// Initialize the quantization table using the default values
	assert(sizeof(parameters->quant_table) == sizeof(quant_table));
	memcpy(parameters->quant_table, quant_table, sizeof(parameters->quant_table));

	return CODEC_ERROR_OKAY;
}

/*!
    @brief Deallocate memory allocated for the parameters data structure
 
    This routine assumes that the default memory allocator was used to allocate the pathname list.
*/
CODEC_ERROR ReleaseParameters(PARAMETERS *parameters, ALLOCATOR *allocator)
{
#if 0   //VC5_ENABLED_PART(VC5_PART_SECTIONS)
    int input_pathname_index;
    
    // Free each pathname on the pathname list
    for (input_pathname_index = 0; input_pathname_index < parameters->input_pathname_count; input_pathname_index++)
    {
        Free(NULL, parameters->input_pathname_list[input_pathname_index]);
    }
    
    // Release the memory used for the pathname list
    Free(NULL, parameters->input_pathname_list);
#endif

    return CODEC_ERROR_OKAY;
}

/*!
    @brief Initialize the pathname data structure
*/
CODEC_ERROR InitPathnameData(PATHNAME_DATA *pathname_data)
{
    memset(pathname_data, 0, sizeof(PATHNAME_DATA));
    return CODEC_ERROR_OKAY;
}

/*!
    @brief Safely copy a pathname
 
    The routine returns true if the output string is large enough for the input string and
    its terminating nul character.
 
    @todo Should use a standard safe string copy routine that is supported on all platforms
 */
CODEC_ERROR CopyPathname(char *output, const char *input, size_t length)
{
    strncpy(output, input, length);
    output[length - 1] = '\0';
    return strlen(input) < length;
}

/*!
    @brief Parse a pathname to obtain missing parameters that could not be read from the command line
 
    The pathname must conform to the filename conventions documented in VC-5 Part 2.
*/
CODEC_ERROR ParseImagePathnameData(const char *pathname, PATHNAME_DATA *pathname_data)
{
    CODEC_ERROR error = CODEC_ERROR_OKAY;
    FILE_INFO info;
    
    GetFileInfo(pathname, &info);
    
    if (info.type == FILE_TYPE_RAW)
    {
        // Try to determine the image dimensions from the pathname
        DIMENSION width = 0;
        DIMENSION height = 0;
        char *p1 = NULL;
        char *p2 = NULL;
        char *p3 = NULL;
        char *p;
        bool match_flag = false;    //!< True parsing found the image dimensions
        
        for (p1 = (char *)pathname; *p1 != '\0'; p1 = p2)
        {
            int digit_count = 0;
            
            // Find the hyphen that separates the test case from the image dimensions
            p1 = strchr(p1, '-');
            if (p1 == NULL) {
                break;
            }
            for (p2 = p1 + 1; isdigit(*p2); p2++) {
                digit_count++;
            }
            
            // Found a sequence of digits between the hyphen and the dimensions separator?
            if (*p2 == 'x' && digit_count > 0) {
                match_flag = true;
                break;
            }
        }
        
        if (match_flag)
        {
            // Find the character that separates the width from the height
            p2 = strchr(p1, 'x');
            if (p2 == NULL) {
                match_flag = false;
            }
        }
        
        if (match_flag)
        {
            // Find the last hyphen that separates the dimensions from the sequence number
            p3 = strchr(p2, '-');
            if (p3 == NULL) {
                match_flag = false;
            }
        }
        
        if (match_flag)
        {
            // Get the width and height
            width = (DIMENSION)strtol(p1 + 1, &p, 10);
            height = (DIMENSION)strtol(p2 + 1, &p, 10);
            
            // Store the dimensions and format in the pathname data structure
            pathname_data->image_width = width;
            pathname_data->image_height = height;
            pathname_data->pixel_format = info.format;
            pathname_data->precision = info.precision;

            // Set the image format using other information in the pathname data
            error = SetPathnameDataImageFormat(pathname_data);
            if (error != CODEC_ERROR_OKAY) {
                return error;
            }

            // Parsed the filename successfully and set all pathname data
            return CODEC_ERROR_OKAY;
        }
    }
    
    // Could not parse the filename to obtain the image parameters
    return CODEC_ERROR_COULD_NOT_PARSE_FILENAME;
}

/*!
    @brief Set the image format parameters based on the pixel type
 
    This routine fills missing parameter values for encoding VC-5 Part 3 bitstreams with
    values inferred from other parameter values.  The parameter values are used to set
    corresponding parameters in the pathname data.
 
    This routine should not be called if encoding image sections.
*/
CODEC_ERROR SetImageFormatParameters(PARAMETERS *parameters, PATHNAME_DATA *pathname_data)
{
    DIMENSION pattern_width;
    DIMENSION pattern_height;
    DIMENSION components_per_sample;
    
    // This routine should not be called if encoding image sections
    assert(!IsImageSectionEnabled(parameters->enabled_parts, parameters->enabled_sections));
    
    // Use the pixel format to set values for the pattern dimensions and components per sample
    switch (pathname_data->pixel_format)
    {
        case PIXEL_FORMAT_B64A:
            pattern_width = 1;
            pattern_height = 1;
            components_per_sample = 4;
            break;
            
        case PIXEL_FORMAT_RG48:
            pattern_width = 1;
            pattern_height = 1;
            components_per_sample = 3;
            break;
            
        case PIXEL_FORMAT_BYR4:
            pattern_width = 2;
            pattern_height = 2;
            components_per_sample = 1;
            break;
            
#if VC5_ENABLED_PART(VC5_PART_COLOR_SAMPLING)
        case PIXEL_FORMAT_NV12:
            pattern_width = 2;
            pattern_height = 2;
            components_per_sample = 0;      // Not applicable to images with subsampled color differences
            break;
#endif
        default:
            // Not able to set the command-line parameters from the input file format
            assert(0);
            return CODEC_ERROR_BAD_IMAGE_FORMAT;
            break;
    }
    
    if (parameters->image_format == IMAGE_FORMAT_UNKNOWN)
    {
        // Set the image format using the default value for the pixel format
        parameters->image_format = DefaultImageFormat(parameters->pixel_format);
    }
    
    assert(parameters->image_format != IMAGE_FORMAT_UNKNOWN);
    if (! (parameters->image_format != IMAGE_FORMAT_UNKNOWN)) {
        return CODEC_ERROR_BAD_IMAGE_FORMAT;
    }
    
    pathname_data->image_format = parameters->image_format;
    
    if (parameters->pattern_width == 0)
    {
        // Set the pattern width using the default value for the pixel format
        parameters->pattern_width = pattern_width;
    }
    
    assert(parameters->pattern_width != 0);
    if (! (parameters->pattern_width != 0)) {
        return CODEC_ERROR_PATTERN_DIMENSIONS;
    }
    
    pathname_data->pattern_width = parameters->pattern_width;
    
    if (parameters->pattern_height == 0)
    {
        // Set the pattern height using the default value for the pixel format
        parameters->pattern_height = pattern_height;
    }
    
    assert(parameters->pattern_height != 0);
    if (! (parameters->pattern_height != 0)) {
        return CODEC_ERROR_PATTERN_DIMENSIONS;
    }
    
    pathname_data->pattern_height = parameters->pattern_height;
    
    if (parameters->components_per_sample == 0)
    {
        // Set the number of components using the default value for the pixel format
        parameters->components_per_sample = components_per_sample;
    }
    
    assert(pathname_data->pixel_format == PIXEL_FORMAT_NV12 || parameters->components_per_sample != 0);
    if (! (pathname_data->pixel_format == PIXEL_FORMAT_NV12 || parameters->components_per_sample != 0)) {
        return CODEC_ERROR_COMPONENTS_PER_SAMPLE;
    }
    
    pathname_data->components_per_sample = parameters->components_per_sample;
    
    return CODEC_ERROR_OKAY;
}

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
/*!
    @brief Set the pathname data image format based on the pixel type
     
    This routine fills missing pathname data defined in VC-5 Part 3 with values that
    can be inferred from the pixel format.
*/
CODEC_ERROR SetPathnameDataImageFormat(PATHNAME_DATA *pathname_data)
{
    DIMENSION pattern_width;
    DIMENSION pattern_height;
    DIMENSION components_per_sample;
    
    // This routine should not be called before setting the pixel format
    assert(pathname_data->pixel_format != PIXEL_FORMAT_UNKNOWN);
    if (! (pathname_data->pixel_format != PIXEL_FORMAT_UNKNOWN)) {
        return CODEC_ERROR_UNEXPECTED;
    }
    
    // Use the pixel format to set values for the sample array structure
    switch (pathname_data->pixel_format)
    {
        case PIXEL_FORMAT_B64A:
            pattern_width = 1;
            pattern_height = 1;
            components_per_sample = 4;
            break;
            
        case PIXEL_FORMAT_RG48:
            pattern_width = 1;
            pattern_height = 1;
            components_per_sample = 3;
            break;
            
        case PIXEL_FORMAT_BYR4:
            pattern_width = 2;
            pattern_height = 2;
            components_per_sample = 1;
            break;
            
#if VC5_ENABLED_PART(VC5_PART_COLOR_SAMPLING)
        case PIXEL_FORMAT_NV12:
            pattern_width = 2;
            pattern_height = 2;
            components_per_sample = 0;      // Not applicable to images with subsampled color differences
            break;
#endif
        default:
            // Not able to set the sample array parameters from the pixel format
            assert(0);
            return CODEC_ERROR_BAD_IMAGE_FORMAT;
            break;
    }

    if (pathname_data->pattern_width == 0)
    {
        // Set the pattern width using the default value for the pixel format
        pathname_data->pattern_width = pattern_width;
    }
    
    assert(pathname_data->pattern_width > 0);
    if (! (pathname_data->pattern_width > 0)) {
        return CODEC_ERROR_PATTERN_DIMENSIONS;
    }

    if (pathname_data->pattern_height == 0)
    {
        // Set the pattern height using the default value for the pixel format
        pathname_data->pattern_height = pattern_height;
    }
    
    assert(pathname_data->pattern_height > 0);
    if (! (pathname_data->pattern_height > 0)) {
        return CODEC_ERROR_PATTERN_DIMENSIONS;
    }
    
    if (pathname_data->components_per_sample == 0)
    {
        // Set the number of components using the default value for the pixel format
        pathname_data->components_per_sample = components_per_sample;
    }
    
    assert(pathname_data->components_per_sample > 0);
    if (! (pathname_data->components_per_sample > 0)) {
        return CODEC_ERROR_COMPONENTS_PER_SAMPLE;
    }
    
    if (pathname_data->image_format == IMAGE_FORMAT_UNKNOWN)
    {
        // Set the image format using the default value for the pixel format
        pathname_data->image_format = DefaultImageFormat(pathname_data->pixel_format);
    }
    
    assert(pathname_data->image_format != IMAGE_FORMAT_UNKNOWN);
    if (! (pathname_data->image_format != IMAGE_FORMAT_UNKNOWN)) {
        return CODEC_ERROR_BAD_IMAGE_FORMAT;
    }
    
    return CODEC_ERROR_OKAY;
}
#endif

#if 0
/*!
    @brief Set the image parameters in a pathname data record
 
    The routine will parse the image filename to get the image dimensions and format if necessary,
    but first tries to obtain the image parameters using other means that do not depend on the
    filename conventions used by the codec test scripts.
 
    This routine depends on the file naming conventions described in VC-5 Part 2.
*/
CODEC_ERROR SetImageFileParameters(const char *pathname,            //!< Input image pathname
                                   PATHNAME_DATA *pathname_data,    //!< Input image data
                                   const PARAMETERS *parameters)    //!< Command-line parameters
{
    CODEC_ERROR error = CODEC_ERROR_OKAY;

    memset(pathname_data, 0, sizeof(PATHNAME_DATA));
    
#if VC5_ENABLED_PART(VC5_PART_SECTIONS)
    if (IsImageSectionEnabled(parameters->enabled_parts, parameters->enabled_sections))
    {
        error = ParseImageFilename(pathname, pathname_data);
        if (error != CODEC_ERROR_OKAY) {
            return error;
        }
    }
#endif

    // The command-line options override the file information

    if (parameters->width != 0) {
        pathname_data->image_width = parameters->width;
    }
    
    if (parameters->height != 0) {
        pathname_data->image_height = parameters->height;
    }
    
    if (parameters->pixel_format != PIXEL_FORMAT_UNKNOWN) {
        pathname_data->pixel_format = parameters->pixel_format;
    }
    
    // Pixel format still not defined?
    if (pathname_data->pixel_format == PIXEL_FORMAT_UNKNOWN)
    {
        // Get the pixel format from the pathname extension
        FILE_INFO info;
        GetFileInfo(pathname, &info);
        pathname_data->pixel_format = info.format;
    }
    
    if (parameters->bits_per_component != 0)
    {
        FILE_INFO info;
        GetFileInfo(pathname, &info);

        pathname_data->precision = info.precision;
    }
    
#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
    //if (IsPartEnabled(parameters->enabled_parts, VC5_PART_IMAGE_FORMATS))
    {
        SetImageFormatParameters(parameters, pathname_data);
    }
#endif

    // Copy the pathname into the pathname data
    CopyPathname(pathname_data->pathname, pathname, sizeof(pathname_data->pathname));
    
    return CODEC_ERROR_OKAY;
}
#endif

/*!
    @brief Print the pathname for each image file in the pathname list
 */
CODEC_ERROR PrintPathnameList(const PATHNAME_LIST *pathname_list, const char *label)
{
    int pathname_count = pathname_list->pathname_count;
    int pathname_index;

    for (pathname_index = 0; pathname_index < pathname_count; pathname_index++)
    {
        printf("%s: %s\n", label, pathname_list->pathname_data[pathname_index].pathname);
    }
    
    return CODEC_ERROR_OKAY;
}

/*!
 @brief Print information about each image file in the pathname list
 */
CODEC_ERROR PrintPathnameListInfo(const PATHNAME_LIST *pathname_list)
{
    int pathname_count = pathname_list->pathname_count;
    int pathname_index;
    
    //printf("\n");
    printf("Image pathname list count: %d\n", pathname_list->pathname_count);
    printf("\n");
    
    for (pathname_index = 0; pathname_index < pathname_count; pathname_index++)
    {
        DIMENSION width = pathname_list->pathname_data[pathname_index].image_width;
        DIMENSION height= pathname_list->pathname_data[pathname_index].image_height;
        PIXEL_FORMAT format = pathname_list->pathname_data[pathname_index].pixel_format;
        const char *pathname = pathname_list->pathname_data[pathname_index].pathname;
        
        printf("Image width: %d, height: %d, format: %s\n", width, height, PixelFormatName(format));
        printf("Image pathname: %s\n", pathname);
        printf("\n");
    }
    
    return CODEC_ERROR_OKAY;
}

/*!
    @brief Set default parameters in the data structure that represents the command-line arguments
 
    This routine should be called after calling @ref InitParameters and before calling @ref ParseParameters.
 
    Customize this routine to provide default values as necessary.  The routine @ref InitParameters clears
    the parameters data structure leaving default values of zero which indicates that no value has been
    obtained from the command line.  The routine @ref SetMissingParameters is called after @ref ParseParameters
    to replace zero values with suitable values inferred from the parameters that have been set.
*/
CODEC_ERROR SetDefaultParameters(PARAMETERS *parameters)
{
    //TODO: Custoize this routine as necessary
    return CODEC_ERROR_OKAY;
}

/*!
    @brief Fill in missing values in the parameters data structure
 
    This routine infers suitable values for missing command-line parameters from the other parameters.
    The behavior of this routine depends on whether the encoder is processing image sections or otherwise.

    If processing image sections, then the individual pathnames must be parsed to obtain the dimensions
    and pixel format.  The pathnames must follow the filename conventions specified in VC-5 Part 2.
 
    IF not processing image sections, then the values in the parameters data structure such as width and height
    apply to all pathnames in the input pathnamelist.  Missing values are inferred from the other parameters
    and then copied into the pathname data for each input file.
*/
CODEC_ERROR SetMissingParameters(PARAMETERS *parameters)
{
    CODEC_ERROR error = CODEC_ERROR_OKAY;
    int pathname_data_index;

#if VC5_ENABLED_PART(VC5_PART_SECTIONS)
    if (IsImageSectionEnabled(parameters->enabled_parts, parameters->enabled_sections))
    {
        for (pathname_data_index = 0;
             pathname_data_index < parameters->input_pathname_list.pathname_count;
             pathname_data_index++)
        {
            PATHNAME_DATA *pathname_data = &parameters->input_pathname_list.pathname_data[pathname_data_index];
            
            error = ParseImagePathnameData(pathname_data->pathname, pathname_data);
            if (error != CODEC_ERROR_OKAY) {
                return error;
            }
        }
        
        // Do not modify the parameters that apply to all input files
        return CODEC_ERROR_OKAY;
    }
#endif
    
    //TODO: Attempt to infer missing parameter values here
    
    // The command-line options override the pathname data for each input file
    
    for (pathname_data_index = 0;
         pathname_data_index < parameters->input_pathname_list.pathname_count;
         pathname_data_index++)
    {
        PATHNAME_DATA *pathname_data = &parameters->input_pathname_list.pathname_data[pathname_data_index];
        FILE_INFO info;

        // Some parameters can be inferred correctly from the filename extension
        GetFileInfo(pathname_data->pathname, &info);

        // Use parameters obtained from the command line and default values to set the pathname data
        if (parameters->width != 0) {
            pathname_data->image_width = parameters->width;
        }
        
        if (parameters->height != 0) {
            pathname_data->image_height = parameters->height;
        }
        
        if (parameters->pixel_format == PIXEL_FORMAT_UNKNOWN)
        {
            // Get the pixel format from the pathname extension
           parameters->pixel_format = info.format;
        }
        
        assert(parameters->pixel_format != PIXEL_FORMAT_UNKNOWN);
        if (! (parameters->pixel_format != PIXEL_FORMAT_UNKNOWN)) {
            return CODEC_ERROR_PIXEL_FORMAT;
        }
        
        pathname_data->pixel_format = parameters->pixel_format;
        
        if (parameters->bits_per_component == 0)
        {
            // Use the precision inferred from the filename extension
            parameters->bits_per_component = info.precision;
        }
        
        assert(parameters->bits_per_component != 0);
        if (! (parameters->bits_per_component != 0))
        {
            return CODEC_ERROR_BITS_PER_COMPONENT;
        }
        
        pathname_data->precision = parameters->bits_per_component;
        
#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
        SetImageFormatParameters(parameters, pathname_data);
#endif
    }
    
    return CODEC_ERROR_OKAY;
}
