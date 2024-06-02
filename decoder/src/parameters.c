/*!	@file decoder/src/parameters.c

	Implementation of the data structure used to pass decoding
	parameters to the decoder.

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
*/
CODEC_ERROR InitParameters(PARAMETERS *parameters)
{
	memset(parameters, 0, sizeof(PARAMETERS));
	parameters->version = 1;
	parameters->verbose_flag = false;
	parameters->bandfile.channel_mask = 0;
	parameters->bandfile.subband_mask = 0;
	return CODEC_ERROR_OKAY;
}

/*!
    @brief Initialize the pathname for the sections information logfile
 
    The sections logfile records information about all sections encountered in the bitstream.
    It is required for verifying the conformance of a VC-5 Part 6 decoder (SMPTE RP 2073-2).
*/
CODEC_ERROR SetSectionsLogfilePathname(PARAMETERS *parameters, const char *output_pathname)
{
    char *logfile_pathname = parameters->sections.logfile_pathname;
    const char *logfile_extension = ".log";
    
    // Initialize the logfile pathname to the output file pathname without the extension
    CODEC_ERROR error = GetFileRoot(output_pathname, logfile_pathname, sizeof(parameters->sections.logfile_pathname));
    if (error != CODEC_ERROR_OKAY) {
        return error;
    }
    
    // Check for enough space remaining to add the logfile extension
    if (strlen(logfile_pathname) + strlen(logfile_extension) < PATH_MAX)
    {
        // Append the logfile extension
        strcat(logfile_pathname, ".log");
        
        return CODEC_ERROR_OKAY;
    }
    
    return CODEC_ERROR_OUTOFMEMORY;
}
