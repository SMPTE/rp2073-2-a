/*!	@file encoder/src/vlc.c

	Implementation of routines to insert variable length codes into the bitstream

	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#include "headers.h"


#ifndef DEBUG
//! Control debugging statements in this module
#define DEBUG	(1 && _DEBUG)
#endif


/*!
	@brief Insert a signed value into the bitstream
*/
CODEC_ERROR PutValue(BITSTREAM *stream, const MAGS_TABLE *mags_table, int32_t value)
{
	int mags_table_length = mags_table->length;
	VLE *mags_table_entry = (VLE *)((uint8_t *)mags_table + sizeof(MAGS_TABLE));

	int last_mags_table_entry = mags_table_length - 1;

	// Compute the sign bit that follows the codeword for the coefficient magnitude
	BITWORD sign = (value > 0) ? VLC_POSITIVE_CODE : VLC_NEGATIVE_CODE;

#if (1 && DEBUG)
	BITWORD bits;
	BITCOUNT size;
#endif

	//int32_t best_value;
	//int best_index;
	int mags_table_index;

	// The value zero is run length coded and handled by another routine
	assert(value != 0);

	// Convert the value to a magnitude
	mags_table_index = abs(value);
	if (mags_table_index > last_mags_table_entry) {
		mags_table_index = last_mags_table_entry;
	}

	// Apply the companding curve
	//value = CompandedValue(value);

#if (1 && DEBUG)
	// Get the codebook entry for the coefficient magnitude
	bits = mags_table_entry[mags_table_index].bits;
	size = mags_table_entry[mags_table_index].size;

	// Append the code for the sign
	bits = (bits << VLC_SIGNCODE_SIZE) | sign;
	size += VLC_SIGNCODE_SIZE;

	PutBits(stream, bits, size);
#else
	// Write the code for the magnitude
	PutBits(stream, mags_table_entry[mags_table_index].bits, mags_table_entry[mags_table_index].size);

	// Write the code for the sign
	PutBits(stream, sign, VLC_SIGNCODE_SIZE);
#endif

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Write the codewords for a run of zeros into the bitstream

	The codebook contains codewords for a few runs of zeros.  This routine writes
	multiple codewords into the bitstream until the specified number of zeros has
	been written into the bitstream.
*/
CODEC_ERROR PutZeros(BITSTREAM *stream, const RUNS_TABLE *runs_table, uint32_t count)
{
	// Get the length of the codebook and a pointer to the entries
	uint32_t length = runs_table->length;
	RLC *rlc = (RLC *)((uint8_t *)runs_table + sizeof(RUNS_TABLE));
	int index;

	// Output one or more run lengths until the run is finished
	while (count > 0)
	{
		// Index into the codebook to get a run length code that covers most of the run
		index = (count < length) ? count : length - 1;

		// Output the run length code
		PutBits(stream, rlc[index].bits, rlc[index].size);

		// Reduce the length of the run by the amount output
		count -= rlc[index].count;
	}

	// Should have output enough runs to cover the run of zeros
	assert(count == 0);

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Insert a special codeword into the bitstream

	The codebook contains special codewords in addition to the codebook
	entries for coefficient magnitudes and runs of zeros.  Special codewords
	are used to mark important locations in the bitstream.  Currently,
	the only special codeword is the one that marks the end of an encoded
	band.
*/
CODEC_ERROR PutSpecial(BITSTREAM *stream, const CODEBOOK *codebook, SPECIAL_MARKER marker)
{
	int codebook_length = codebook->length;
	RLV *codebook_entry = (RLV *)((uint8_t *)codebook + sizeof(CODEBOOK));

	int index;

	// Find the special code that corresponds to the marker
	for (index = 0; index < codebook_length; index++)
	{
		// Is this entry a special code?
		if (codebook_entry[index].count != 0) {
			continue;
		}
		
		// Is this entry the special code for the marker?
		if (codebook_entry[index].value == marker) {
			break;
		}
	}
	assert(index < codebook_length);
	if (! (index < codebook_length)) {
		// Did not find the special code for the marker in the codebook
		return CODEC_ERROR_INVALID_MARKER;
	}

	PutBits(stream, codebook_entry[index].bits, codebook_entry[index].size);

	return CODEC_ERROR_OKAY;
}
