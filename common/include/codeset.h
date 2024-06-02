/*! @file common/include/codeset.h

	The codeset data structure includes the codebook and flags that indicate
	how to use the codebook.  The codeset may also include tables that are
	derived from the codebook to facilite encoding and decoding.

	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _CODESET_H
#define _CODESET_H

/*!
	@brief Codeset flags that determine how the codebook is used for encoding

	The codeset flags determine how the codebook in the codeset is used to
	compute the tables for encoding coefficient magnitudes and runs of zeros.

	The companding curve is applied to the quantized coefficients before the
	values are entropy coded to fit the coefficient magnitudes into the range
	of magnitudes provided by the codebook.  The companding curve is applied
	when the encoding table for coefficient magnitudes is computed.
*/
typedef enum _codeset_flags
{
	CODESET_FLAGS_COMPANDING_NONE = 0x0002,		//!< Do not apply a companding curve
	CODESET_FLAGS_COMPANDING_CUBIC = 0x0004,	//!< Apply a cubic companding curve

} CODESET_FLAGS;


/*!
	The collection of codebooks that are used by the codec are called a codeset.

	The codebook in seach codeset is derived from the master codebook that is
	included in the codec by including the table for the codebook.  The encoder
	uses specialized codebooks for coefficient magnitudes and runs of zeros that
	are derived from the master codebook.
*/
typedef struct codeset {
	const char *title;					//!< Identifying string for the codeset
	const CODEBOOK *codebook;			//!< Codebook for runs and magnitudes
#if _ENCODER
	const MAGS_TABLE *mags_table;		//!< Table for encoding coefficient magnitudes
	const RUNS_TABLE *runs_table;		//!< Table for encoding runs of zeros
#endif
	uint32_t flags;						//!< Encoding flags (see the codeset flags)

} CODESET;

//TODO: Need to support other codesets in the reference decoder?
extern CODESET cs17;

#endif
