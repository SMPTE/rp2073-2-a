/*! @file decoder/include/component.h
 
	Declaration of routines for the inverse component transform and permutation.
 
	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
 */

#ifndef _COMPONENT_H
#define _COMPONENT_H

#ifdef __cplusplus
extern "C" {
#endif

CODEC_ERROR ParseInverseComponentTransform(DECODER *decoder, BITSTREAM *stream, size_t chunk_size);

CODEC_ERROR ParseInverseComponentPermutation(DECODER *decoder, BITSTREAM *stream, size_t chunk_size);

#ifdef __cplusplus
}
#endif

#endif
