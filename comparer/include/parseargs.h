/*!	@file comparer/include/parseargs.h

	Declarations for the command line parser.

	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _PARSEARGS_H
#define _PARSEARGS_H

// Print help message for command line arguments
CODEC_ERROR PrintHelpMessage(int argc, char *argv[]);

/*!
	@brief Parse the command line arguments.

	The default quantization values are set according to the default
	quality level in this routine.
*/
CODEC_ERROR ParseParameters(int *argc, char *argv[], PARAMETERS *parameters);

#endif
