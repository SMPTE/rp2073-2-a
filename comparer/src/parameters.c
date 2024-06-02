/*!	@file parameters.c

	Implementation of the data structure used to pass decoding
	parameters to the decoder.

	The parameters data structure is currently a simple struct, but
	fields may be added, removed, or replaced.  A version number is
	included in the parameters data structure to allow decoders to
	adapt to changes.

	It is contemplated that future decoder inplementation may use a
	dictionary of key-value pairs which would allow the decoder to
	determine whether a parameter was present.

	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#include "headers.h"

//! Current version number of the parameters data structure
#define PARAMETERS_VERSION		0

/*!
	@brief Initialize the parameters data structure

	The version number of the parameters data structure must be
	incremented whenever a change is made to the definition of
	the parameters data structure.

	@todo Special initialization required by the metadata?
*/
CODEC_ERROR InitParameters(PARAMETERS *parameters)
{
	memset(parameters, 0, sizeof(PARAMETERS));
	return CODEC_ERROR_OKAY;
}
