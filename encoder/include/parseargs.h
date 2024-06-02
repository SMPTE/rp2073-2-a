/*!	@file encoder/include/parseargs.h

	Declarations for the command line parser.
	
	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _PARSEARGS_H
#define _PARSEARGS_H

// Print help message for command line arguments
CODEC_ERROR PrintHelpMessage(int argc, const char *argv[]);
CODEC_ERROR ParseParameters(int argc, const char *argv[], PARAMETERS *parameters);

#if VC5_ENABLED_PART(VC5_PART_SECTIONS) && VC5_ENABLED_PART(VC5_PART_LAYERS)
bool GetImageSectionLayers(const char *string, PARAMETERS *parameters);
#endif

#if VC5_ENABLED_PART(VC5_PART_SECTIONS)
bool GetPathname(const char *string, char *pathname);
#endif

#endif
