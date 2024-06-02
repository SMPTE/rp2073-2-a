/*!	@file params.c

	@brief Module for parsing the command-line arguments.

	This module was originally used Argp and was adapted from:
	https://www.gnu.org/software/libc/manual/html_node/Argp-Example-4.html#Argp-Example-4

    But this module now uses the argparse library which has an MIT license:
    https://github.com/cofyc/argparse

    (c) 2013-2021 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
    All rights reserved--use subject to compliance with end user license agreement.
*/

#include "config.h"

#if !defined(_DECODER) || _TESTING

#if _ARGPARSE

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdarg.h>
#include <assert.h>

// Use the argument parsing library in $(ROOT)/external
#include "argparse.h"

#include "error.h"
#include "params.h"


static const char * const usage[] = {
    "dumper [options] testcase.bin",
    NULL,
};


CODEC_ERROR parse_args(int argc, const char *argv[], ARGUMENTS *args)
{
    // ARGUMENTS args;
    // memset(&args, 0, sizeof(args));

    struct argparse_option options[] = {
        OPT_HELP(),

        // Pathnames for the output and error logging files
        OPT_STRING('o', "output", &args->output_filename, "Print to the specified file (default stdout)"),
        OPT_STRING('e', "error", &args->error_filename, "Log error messages to the specified file (default stderr)"),

        // This switch enables use of a database to detect duplicate tuples that should be ignored
        OPT_BOOLEAN('d', "duplicates", &args->duplicates_flag, "Remove duplicate tuples from the output"),

        // Switches that control debugging output
        OPT_BOOLEAN('v', "verbose", &args->verbose_flag, "Enable verbose output"),
        OPT_BOOLEAN('z', "debug", &args->debug_flag, "Enable debugging output"),
        OPT_BOOLEAN('q', "quiet", &args->quiet_flag, "Suppress all output to the terminal"),

        OPT_END(),
    };

    // Initialize the argument parser state
    struct argparse argparse;
    argparse_init(&argparse, options, usage, 0);

    // Add descriptive text before and after the listing of the command-line options
    argparse_describe(&argparse,
        "\nXML dumper for converting a VC-5 metadata test case in binary format to XML.",
        "\nThe bitstream is expected to be a binary file created by the XML parser.\n");

    // Parse the command-line arguments and return the number of arguments remaining on the command line
    argc = argparse_parse(&argparse, argc, argv);

    if (argc > 1) {
        fprintf(stderr, "Extra input files will be ignored (argc: %d)\n", argc);
    }

    // for (int i = 0; i < argc; i++) {
    //     fprintf(stderr, "argv[%d] = %s\n", i, argv[i]);
    // }

    //strcpy(args->input_filename, argv[0]);
    args->input_filename = (char *)argv[0];

    return CODEC_ERROR_OKAY;
}

#else

#include <stdlib.h>
#include <stdbool.h>
#include <argp.h>

#include "params.h"

const char *argp_program_version = "dumper 1.0";
const char *argp_program_bug_address = "<bugs@example.com>";


// Keys for options without short-options
#define OPT_ABORT  1

/*!
	@brief Parse a single command-line option.
*/
static error_t
parse_opt(int key,                      //!< Character corresponding the the command-line option
		  char *arg,                    //!< Pointer to the value of the command-line argument
		  struct argp_state *state		//!< Argument parser state
		  )
{
	/* Get the input argument from argp_parse, which we
	 know is a pointer to our arguments structure. */
	ARGUMENTS *arguments = state->input;

	switch (key)
    {
        case 'o':
            arguments->output_filename = arg;
            break;

        case 'e':
            arguments->error_filename = arg;
            break;

        case 'v':
            arguments->verbose_flag = true;
            break;

        case 'q':
            arguments->quiet_flag = true;
            break;

        case 'z':
            arguments->debug_flag = true;
            break;

        //case 'h':
        case ARGP_KEY_NO_ARGS:
            argp_usage(state);
            break;

        case ARGP_KEY_ARG:
            // Here we know that state->arg_num == 0 because argument parsing ends before any more arguments can get here
            arguments->arg1 = arg;

            /* Now we consume all the rest of the arguments.
                state->next is the index in state->argv of the next argument to be parsed,
                which is the first string we are interested in, so we can just use &state->argv[state->next]
                as the value for arguments->strings. In addition, by setting state->next to the end of the arguments,
                we can force argp to stop parsing here and return.
            */
            arguments->strings = &state->argv[state->next];
            state->next = state->argc;
            break;

        default:
            return ARGP_ERR_UNKNOWN;
            break;
    }

    // The first string on the command-line that is not an option is the input filename
    arguments->input_filename = arg;

  return 0;
}


/*!
	@brief Parse the command-ine options and return the options in the arguments data structure.
*/
int
parse_args(int argc,					//!< Number of command-line arguments (including the program filename)
		   const char *argv[],			//!< Array of strings with one string per command-line argument
		   ARGUMENTS *arguments	      	//!< Argument data structure for returning command-line arguments to the caller
		   )
{
	// Program documentation
	static char doc[] = "dumper for printing a VC-5 metadata bitstream in a readable format";

	// Argument documentation
	static char args_doc[] = "ARG1 [STRING...]";

	// Program command line arguments
	static struct argp_option options[] =
	{
        {"output",   'o', "FILE", 0, "Print to the specified file (default stdout)"},
        {"error",    'o', "FILE", 0, "Log error messages to the specified file (default stderr)"},
		{"verbose",  'v',      0, 0, "Enable verbose output"},
		{"quiet",    'q',      0, 0, "Suppress all output to the terminal"},
        {"debug",    'z',      0, 0, "Enable debugging output"},
        //{"help",     'h',      0, 0, "Output usage information"},
    	{0}
	};

	// Initialize the argument parser state
	static struct argp argp = {options, parse_opt, args_doc, doc};

	// Parse the command-line arguments
	argp_parse(&argp, argc, argv, 0, 0, arguments);

	return 0;
}

#endif

#endif
