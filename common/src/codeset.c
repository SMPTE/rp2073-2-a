/*!	@file common/src/codeset.c

	Instantiation of the codesets used by the reference codec.

	A codeset is a codebook and other information not directly represented
	in the codebook data structure that is used for entropy coding.

	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#include "headers.h"

#pragma pack(push, 1)

// Include codebook #17
#include "table17.inc"

/*!
	@brief Define the codeset used by the reference codec

	The baseline codec only supports codebook #17.

	Codebook #17 is intended to be used with cubic companding
	(see @ref FillMagnitudeEncodingTable and @ref ComputeCubicTable).
*/
CODESET cs17 = {
	"Codebook set 17 from data by David Newman with tables automatically generated for the FSM decoder",
	(const CODEBOOK *)&table17,
#if _ENCODER
	NULL,
	NULL,
#endif
	CODESET_FLAGS_COMPANDING_CUBIC,
};

#pragma pack(pop)
