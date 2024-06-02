/*! @file encoder/include/identifier.h
 
	Declaration of routines for writing the unique image identifier.
 
	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
 */

#ifndef _IDENTIFIER_H
#define _IDENTIFIER_H

#ifdef __cplusplus
extern "C" {
#endif

CODEC_ERROR ParseUniqueImageIdentifier(DECODER *decoder, BITSTREAM *stream, size_t identifier_length);

#ifdef __cplusplus
}
#endif

#endif
