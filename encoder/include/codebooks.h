/*! @file encoder/include/codebooks.h

	Declaration of the routines for computing the encoding tables from a codebook.

	A codebooks contains the variable-length codes for coefficient magnitudes, runs
	of zeros, and special codewords that mark entropy codec locations in bitstream.

	The codebook is used to create tables and simplify entropy coding of signed values
	and runs of zeros.

	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _CODEBOOKS_H
#define _CODEBOOKS_H


#ifdef __cplusplus
extern "C" {
#endif

//CODEC_ERROR InitCodebooks(ALLOCATOR *allocator, CODESET *cs);
CODEC_ERROR PrepareCodebooks(ALLOCATOR *allocator, CODESET *cs);

//CODEC_ERROR FreeCodebooks(ALLOCATOR *allocator, CODESET *cs);
CODEC_ERROR ReleaseCodebooks(ALLOCATOR *allocator, CODESET *cs);


CODEC_ERROR ComputeRunLengthCodeTable(ALLOCATOR *allocator,
									  RLV *input_codes, int input_length,
									  RLC *output_codes, int output_length);

CODEC_ERROR SortDecreasingRunLength(RLC *codebook, int length);

CODEC_ERROR FillRunLengthEncodingTable(RLC *codebook, int codebook_length,
									   RLC *table, int table_length);

CODEC_ERROR FillMagnitudeEncodingTable(const CODEBOOK *codebook, VLE *table, int size, uint32_t flags);

#ifdef __cplusplus
}
#endif

#endif
