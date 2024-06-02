/*! @file decoder/include/parameters.h

	Define a data structure for holding a table of parameters used
	during decoding to override the default decoding behavior.

	The decoder can be initialized using the dimensions of the encoded frame
	obtained from an external source such as a media container and the pixel
	format of the decoded frame.  The encoded sample will be decoded to the
	dimensions of the encoded frame without at the full encoded resolution
	without scaling.  The decoded frames will have the specified pixel format,
	but this assumes that the encoded dimensions used during initialization
	are the same as the actual encoded dimensions and that the pixel format of
	the decoded frames is a valid pixel format.

	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _PARAMETERS_H
#define _PARAMETERS_H

#ifndef PATH_MAX
#define PATH_MAX _MAX_PATH
#endif


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
	@brief Declaration of a data structure for passing decoding parameters to the decoder
*/
typedef struct _parameters
{
	uint32_t version;               //!< Version number for this definition of the parameters

	ENABLED_PARTS enabled_parts;	//!< Parts of the VC-5 standard that are enabled

#if VC5_ENABLED_PART(VC5_PART_LAYERS)
    bool layer_flag;            //!< True if decoding all layers in the bitstream
#endif

#if VC5_ENABLED_PART(VC5_PART_SECTIONS)
    //bool section_flag;          //!< True if decoding section elements in the bitstream
    //bool filenames_flag;        //!< True if parsing the bitstream to suggest output filenames

    //! Bit mask that indicates which sections are enabled for processing
    ENABLED_SECTIONS enabled_sections;

    struct _sections_parameters
    {
        // Pathname for the decoded sections log file
        char logfile_pathname[PATH_MAX];
    } sections;     //!< Information about sections encountered in the bitstream
#endif

	//! Dimensions and format of the output of the image unpacking process
	struct _input_parameters
	{
		DIMENSION width;
		DIMENSION height;
		PIXEL_FORMAT format;
	} input;		//!< Dimensions and format of the unpacked image

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
	//! Data structure for representing the encoded dimensions of the sample
	struct _encoded_parameters
	{
		DIMENSION width;
		DIMENSION height;
		IMAGE_FORMAT format;
	} encoded;		//!< Encoded image dimensions and pixel format
#endif

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
	//! Dimensions and format of the output of the decoding process
	struct _decoded_parameters
	{
		DIMENSION width;
		DIMENSION height;
		PIXEL_FORMAT format;
	} decoded;		//! Decoded image dimensions and pixel format
#endif

#if VC5_ENABLED_PART(VC5_PART_ELEMENTARY)
	//! Dimensions and format of the output of the image repacking process
	struct _output_parameters
	{
		DIMENSION width;
		DIMENSION height;
		PIXEL_FORMAT format;
	} output;
#endif

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
	//! Dimensions and format of the displayable image
	struct _display_parameters
	{
		DIMENSION width;
		DIMENSION height;
		PIXEL_FORMAT format;
	} display;
#endif

#if VC5_ENABLED_PART(VC5_PART_METADATA)
	//! Metadata that controls decoding
	//METADATA metadata;
	// Pathname for the decoded metadata in XML format
	struct _metadata_parameters
	{
		bool output_flag;
		bool duplicates_flag;
    	char output_pathname[PATH_MAX];
    } metadata;
#endif

	//! Flag that controls verbose output
	bool verbose_flag;

	//! Flag the control extra output for debugging
	bool debug_flag;

	//! Suppress all output to the terminal
	bool quiet_flag;

	//! Information for writing the bandfile
	BANDFILE_INFO bandfile;

} PARAMETERS;

CODEC_ERROR InitParameters(PARAMETERS *parameters);

CODEC_ERROR SetSectionsLogfilePathname(PARAMETERS *parameters, const char *output_pathname);

#endif
