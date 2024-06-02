/*! @file encoder/include/parameters.h

	Define a data structure for holding a table of parameters that is passed
	to the encoder during initialization.

	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _PARAMETERS_H
#define _PARAMETERS_H

#if 0
/*!
	@brief Function prototype for a decompositor

	A decompositor is the opposite of an image composition operator:
	It decomposes a frame into one or more frames.
	
	For example, an interlaced frame can be decomposed into fields or two frames
	arranged side-by-side within a single frame can be decomposed into individual
	frames.

	Each layer in an encoded sample may correspond to a separate input frame.
	For convenience, the reference codec stores the input to the encoder in a
	separate file with one frame per file if the encoded sample has a single layer.
	To allow the reference encoder to store all of the input frames that are
	encoded as separate layers in an encoded sample in a single file, multiple
	frames are stored in the file (often using over-under frame packing).  The
	decomposer unpacks multiple frames in a single frame into individual frames
	for encoding with one frame per layer.
*/
typedef CODEC_ERROR (* DECOMPOSITOR)(IMAGE *packed_image, IMAGE *image_array[], int frame_count);
#endif


//! Maximum number of pathnames in a pathname list
#define MAX_PATHNAME_COUNT 8


/*!
    @brief Data structure for representing an image file and metadata about the image
*/
typedef struct _pathname_data
{
    char pathname[PATH_MAX];            //!< Pathname to the image file

    DIMENSION image_width;              //!< Input image width
    DIMENSION image_height;             //!< Input image height
    PIXEL_FORMAT pixel_format;          //!< Input image pixel format
    PRECISION precision;                //!< Bits per pixel

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
    DIMENSION pattern_width;            //!< Input image pattern width
    DIMENSION pattern_height;           //!< Input image pattern height
    DIMENSION components_per_sample;    //!< Number of channels in the input image
    IMAGE_FORMAT image_format;          //!< Format used to encode the input image
#endif

} PATHNAME_DATA;


/*!
    @brief Data structure for representing a list of image files with image dimensions and format
 
    This data structure is used when sections are enabled and the command-line parameters contain
    a list of input image pathnames for encoding image sections into the bitstream.  Each input
    image has a pathname, image dimensions and image format which are passed on the commmand line.
    The options for width, height, and format are sticky.  If not specified for each input file,
    then the dimensions and format more recently passed on the command line are used.
*/
typedef struct _pathname_list
{
    COUNT pathname_count;
    PATHNAME_DATA pathname_data[MAX_PATHNAME_COUNT];

} PATHNAME_LIST;


/*!
	@brief Pathname for the band file and masks that specify which subbands to write

	@todo Add a lowpass mask indicating which lowpass bands in the intermediate
	wavelets are included in the bandfile.
*/
typedef struct bandfile_info
{
	uint32_t channel_mask;		//!< Mask that specifies which channels to write
	uint32_t subband_mask;		//!< Mask that specifies whcih subbands to write
	char pathname[PATH_MAX];	//! Pathname for the band file

} BANDFILE_INFO;


/*!
	@brief Declaration of a data structure for passing parameters to the encoder

	The code for parsing command-line options may not support all of the parameters
    defined in this data structure.  Some parameter values are inferred from other
    parameters.  For example, the pixel format is sufficient for setting the values
    of the pattern dimensions and number of components.
 
    The code that uses the parameters data structure is written to allow values to be
    explicitly specified on the command-line in the future and infers values that are
    not specified explicitly.
 
    Image dimensions and other parameters are defined in terms of the sample array as
    defined in VC-5 Part 1 Elementary Bitstream.  Image format and pattern dimensions
    are defined in VC-5 Part 3 Image Formats.
*/
typedef struct _parameters
{
	uint32_t version;                   //!< Version number for this definition of the parameters
    bool verbose_flag;                  //!< Control verbose output
    bool debug_flag;                    //!< Enable extra output for debugging
    bool quiet_flag;                    //!< Suppress all output to the terminal (overrides verbose and debug)
	ENABLED_PARTS enabled_parts;        //!< Parts of the VC-5 standard that are enabled
    
#if VC5_ENABLED_PART(VC5_PART_SECTIONS)
    ENABLED_SECTIONS enabled_sections;  //!< Sections defined in VC-5 Part 6 that are enabled
#endif
    
    /*** Parameters that may be passed on the command line ***/
    
    DIMENSION width;                    //!< Width of the input image (number of samples in each row of the sample array)
    DIMENSION height;                   //!< Height of the input image (number of rows in the sample array)
    PIXEL_FORMAT pixel_format;          //!< Pixel format
    PRECISION bits_per_component;       //!< Bits per component in the input image

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
    IMAGE_FORMAT image_format;          //!< Internal format of the encoded image
    DIMENSION pattern_width;            //!< Width of each pattern element in the sample array
    DIMENSION pattern_height;           //!< Height of each patterb element in the sample array
    DIMENSION components_per_sample;    //!< Number or components in each sample in the sample array
    PRECISION lowpass_precision;        //!< Number of bits used to encode lowpass coefficients
#endif

	//! Array of quantization values indexed by the subband number
    QUANT quant_table[MAX_SUBBAND_COUNT];

    //! Table for the order in which channels are encoded into the bitstream (for debugging)
    CHANNEL channel_order_table[MAX_CHANNEL_COUNT];
    
    //! Number of entries in the channel order table (may be less than the channel count)
    int channel_order_count;

    //! List of files to encode into the bitsteeam
    PATHNAME_LIST input_pathname_list;
    
    //! List of output files (typically only the encoded bitstream)
    //PATHNAME_LIST output_pathname_list;
    char output_pathname[PATH_MAX];
    
    //TODO: Consider changing the output pathname ro a pathname list

    BANDFILE_INFO bandfile;             //!< Information needed for writing the bandfile
    
#if VC5_ENABLED_PART(VC5_PART_SECTIONS) && VC5_ENABLED_PART(VC5_PART_LAYERS)
    //! Number of image sections and layers nexted within each image section
    COUNT image_section_count;
    COUNT section_layer_count[MAX_IMAGE_SECTIONS];
#endif
    
#if VC5_ENABLED_PART(VC5_PART_METADATA)
    //! Metadata that controls decoding (currently not used)
    //METADATA metadata;

    // XML file containing metadata to be injected into the bitstream
    char metadata_pathname[PATH_MAX];
#endif
	
} PARAMETERS;

CODEC_ERROR InitParameters(PARAMETERS *parameters);

CODEC_ERROR SetInputImageParameters(PARAMETERS *parameters,
									const IMAGE *image);

// Deallocate memory allocated in the parameters data structure
CODEC_ERROR ReleaseParameters(PARAMETERS *parameters, ALLOCATOR *allocator);

// Safely copy a pathname to a string
CODEC_ERROR CopyPathname(char *output, const char *input, size_t length);

// Parse the filename to obtain the image dimensions and format
//CODEC_ERROR SetImageFileParameters(const char *pathname, PATHNAME_DATA *pathname_data, const PARAMETERS *parameters);

//CODEC_ERROR CheckEnabledParts(PARAMETERS *parameters);

//bool SetImageFormatDefaults(PARAMETERS *parameters);

//bool CheckImageFormatParameters(PARAMETERS *parameters);

CODEC_ERROR InitPathnameData(PATHNAME_DATA *pathname_data);

CODEC_ERROR SetImageFormatParameters(PARAMETERS *parameters, PATHNAME_DATA *pathname_data);

CODEC_ERROR PrintPathnameList(const PATHNAME_LIST *pathname_list, const char *label);
CODEC_ERROR PrintPathnameListInfo(const PATHNAME_LIST *pathname_list);
CODEC_ERROR SetDefaultParameters(PARAMETERS *parameters);
CODEC_ERROR SetMissingParameters(PARAMETERS *parameters);

#endif
