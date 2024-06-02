/*!	@file params.h

	Definitions corresponding the the command-line options
*/

#ifndef _PARAMS_H
#define _PARAMS_H


/*!
	@brief Used by the argument parser to pass information to the main program
*/
typedef struct _arguments
{
#if _DECODER && !_TESTING

    bool duplicates_flag;       //!< Remove duplicate tuples
    bool verbose_flag;          //!< Enable verbose output
    bool debug_flag;            //!< Enable debugging output

#elif _ARGPARSE

    char *input_filename;       //!< Input pathname
    char *output_filename;      //!< Output pathname (default stdout)
    char *error_filename;		//!< File for error messages (default stderr)
    bool duplicates_flag;       //!< Remove duplicate tuples
    bool verbose_flag;          //!< Enable verbose output
    bool debug_flag;		    //!< Enable debugging output
    bool quiet_flag;            //!< Suppress all program output

#else

    char *arg1;                 //!< arg1
    char **strings;             //!< list of strings passed on the command line
    char *input_filename;       //!< Input pathname
    char *output_filename;      //!< Output pathname (default stdout)
    char *error_filename;		//!< File for error messages (default stderr)
    bool verbose_flag;          //!< Enable verbose output
    bool quiet_flag;            //!< Suppress all program output
    bool debug_flag;			//!< Enable debugging output

#endif

} ARGUMENTS;


/*!
	@brief Parse the command-line arguments
*/
CODEC_ERROR parse_args(int argc, const char *argv[], ARGUMENTS *arguments);

#endif
