/*!	@file comparer/include/parameters.h

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

/*!
	@brief Declaration of a data structure for passing decoding parameters to the decoder
*/
typedef struct _parameters
{
	uint32_t version;		//!< Version number for this definition of the parameters

	//! Dimensions and format of the first image to compare
	struct
	{
		DIMENSION width;
		DIMENSION height;
		PIXEL_FORMAT format;
	} image;

} PARAMETERS;

#ifdef __cplusplus
extern "C" {
#endif

CODEC_ERROR InitParameters(PARAMETERS *parameters);

#ifdef __cplusplus
}
#endif

#endif
