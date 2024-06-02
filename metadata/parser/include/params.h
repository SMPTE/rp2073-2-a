/*! @file metadata/include/params.h

	Declaration of the data structures for command-line arguments.

	TODO: Replace PARAMETERS with ARGUMENTS to align with the XML dumper.

	(c) 2013-2019 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _PARAMS_H
#define _PARAMS_H


#ifdef _ARGPARSE

/*!
	@brief Used by the argument parser to pass information to the main program.
*/
typedef struct _arguments
{
    char *input_filename;       //!< Input pathname
    char *output_filename;      //!< Output pathname (default stdout)
    char *error_filename;		//!< File for error messages (default stderr)
    bool verbose_flag;          //!< Enable verbose output
    bool debug_flag;		    //!< Enable debugging output
    bool quiet_flag;            //!< Suppress all program output

} ARGUMENTS;


//! For compatibility with older code
typedef struct _arguments PARAMETERS;


/*!
	@brief Parse the command-line arguments
*/
//CODEC_ERROR parse_args(int argc, const char *argv[], ARGUMENTS *arguments);
CODEC_ERROR ParseParameters(int argc, const char *argv[], ARGUMENTS *arguments);

#else

/*!
	@brief Used by the argument parser to pass information to the main program.
*/
typedef struct _parameters
{
	char *input_filename;
	char *output_filename;
	char *error_filename;
	bool verbose_flag;

} PARAMETERS;


//typedef char *FILELIST[];


//CODEC_ERROR ParseParameters(int argc, char *argv[], PARAMETERS *parameters, FILELIST *input, FILELIST *output);
CODEC_ERROR ParseParameters(int argc, char *argv[], PARAMETERS *parameters);

#endif

#endif
