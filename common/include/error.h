/*! @file common/include/error.h

	Definitions of the error codes reported by this codec implementation

	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _ERROR_H
#define _ERROR_H

//! Reserve the upper bits to designate the subsystem associated with the error
#define CODEC_ERROR_SUBSYSTEM_SHIFT 10

/*!
	@brief Codec error codes
*/
typedef enum _codec_error
{
	CODEC_ERROR_OKAY = 0,					//!< No error
	CODEC_ERROR_UNEXPECTED,					//!< Encountered an unexpected condition
	CODEC_ERROR_OUTOFMEMORY,				//!< Memory allocation failed
	CODEC_ERROR_UNIMPLEMENTED,				//!< Function has not been implemented
	CODEC_ERROR_NULLPTR,					//!< Data structure or argument pointer was null
	CODEC_ERROR_BITSTREAM_SYNTAX,			//!< Error in the sequence of tag value pairs
	CODEC_ERROR_IMAGE_DIMENSIONS,			//!< Wrong or unknown image dimensions
	CODEC_ERROR_INVALID_TAG,				//!< Found a tag that should not be present
	CODEC_ERROR_INVALID_BAND,				//!< Wavelet band index is out of range
	CODEC_ERROR_DECODING_SUBBAND,			//!< Error decoding a wavetet subband
	CODEC_ERROR_NOTFOUND,					//!< Did not find a value codeword
	CODEC_ERROR_BAND_END_MARKER,			//!< Could not find special codeword after end of band
	CODEC_ERROR_BAND_END_TRAILER,			//!< Could not find start of highpass band trailer
	CODEC_ERROR_PIXEL_FORMAT,				//!< Unsupported or unknown pixel format
    CODEC_ERROR_BITS_PER_COMPONENT,         //!< Could not determine the number of bits per component
	CODEC_ERROR_INVALID_MARKER,				//!< Bitstream marker was not found in the codebook
	CODEC_ERROR_FILE_CREATE,				//!< Could not open pathname for writing
	CODEC_ERROR_FILE_OPEN,					//!< Could not open pathname for reading
	CODEC_ERROR_FILE_GET_POSITION,			//!< Could not get position of the file stream
	CODEC_ERROR_FILE_SEEK,					//!< Could not seek to a position in a file stream
	CODEC_ERROR_FILE_READ,					//!< Read from a file stream failed
	CODEC_ERROR_FILE_WRITE,					//!< Write to a file stream failed
	CODEC_ERROR_CHANNEL_SIZE_TABLE,			//!< Could not write the channel size table
	CODEC_ERROR_UNSUPPORTED_FORMAT,			//!< Pixel or encoded format is not supported
	CODEC_ERROR_MISSING_START_MARKER,		//!< Bitstream does not begin with the start marker
	CODEC_ERROR_DUPLICATE_HEADER_PARAMETER,	//!< Header parameter occurs more than once
	CODEC_ERROR_REQUIRED_PARAMETER,			//!< Optional tag-value pair for a required parameter
	CODEC_ERROR_LOWPASS_PRECISION,			//!< Number of bits per lowpass coefficient out of range
	CODEC_ERROR_LOWPASS_VALUE,				//!< Lowpass coefficient value is out of range
	CODEC_ERROR_IMAGE_TYPE,					//!< Could not determine the characteristics of the input image
	CODEC_ERROR_BAD_IMAGE_FORMAT,			//!< Bad image format (VC-5 Part 3 only)
	CODEC_ERROR_PATTERN_DIMENSIONS,			//!< Bad pattern dimensions (VC-5 Part 3 only)
    CODEC_ERROR_COMPONENTS_PER_SAMPLE,      //!< Wrong number of components per sample (VC-5 Part 3 only)
	CODEC_ERROR_ENABLED_PARTS,				//!< Incorrect enabled parts of the VC-5 standard
    CODEC_ERROR_SYNTAX_ERROR,               //!< Unspecified error in the bitstream syntax
    CODEC_ERROR_UMID_LABEL,                 //!< Incorrect UMID label
    CODEC_ERROR_BAD_SECTION_TAG,            //!< The specified tag does not correspond to a section header
    CODEC_ERROR_COULD_NOT_PARSE_FILENAME,   //!< Unable to obtain the image parameters from the filename


    /***** Reserve a block of error codes for the metadata subsystem *****/

    CODEC_ERROR_METADATA = 100,


	/***** Reserve a block of error codes for the bitstream *****/

	CODEC_ERROR_BITSTREAM = (1 << CODEC_ERROR_SUBSYSTEM_SHIFT),

    
    /***** Reserve a block of error codes for the calling application *****/

	CODEC_ERROR_APPLICATION = (16 << CODEC_ERROR_SUBSYSTEM_SHIFT),
    
	CODEC_ERROR_MISSING_ARGUMENT,			//!< Program did not have enough arguments
	CODEC_ERROR_BAD_ARGUMENT,				//!< Invalid value for one of the arguments
	CODEC_ERROR_OPEN_FILE_FAILED,			//!< Could not open the file for reading
	CODEC_ERROR_CREATE_FILE_FAILED,			//!< Could not open the file for writing
	CODEC_ERROR_UNSUPPORTED_FILE_TYPE,		//!< The output file type is not supported
	CODEC_ERROR_FILE_SIZE_FAILED,			//!< Could not determine the size of the file
	CODEC_ERROR_READ_FILE_FAILED,			//!< Could not read a file
	CODEC_ERROR_FILE_WRITE_FAILED,			//!< Could not write to the file
	CODEC_ERROR_FILE_FLUSH_FAILED,			//!< Could not flush the file buffer
	CODEC_ERROR_PARSE_ARGUMENTS,			//!< Error while parsing the command-line arguments
	CODEC_ERROR_USAGE_INFO,					//!< User asked for help with program usage
	CODEC_ERROR_BANDFILE_FAILED,			//!< Could not write the bandfile
	CODEC_ERROR_BAD_PARAMETER,				//!< Missing or inconsistent parameters
    CODEC_ERROR_BAD_LAYER_IMAGE_LIST,       //!< All images must have the same dimensions and format
    CODEC_ERROR_FILELIST_MISSING_PATHNAME,  //!< Could not obtain another pathname from the file list

} CODEC_ERROR;

#endif
