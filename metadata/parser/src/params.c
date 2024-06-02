/*!	@file metadata/src/params.c

	Program for converting the test cases in XML format to binary files.

	The argument parser originally used getopt but was modified to use argparse after the XML dumper
	was modified to use that library.

	(c) 2013-2021 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#include "config.h"

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
    "parser [options] testcase.xml",
    NULL,
};


//CODEC_ERROR parse_args(int argc, const char *argv[], ARGUMENTS *args)
CODEC_ERROR ParseParameters(int argc, const char *argv[], ARGUMENTS *args)
{
    // ARGUMENTS args;
    // memset(&args, 0, sizeof(args));

    struct argparse_option options[] = {
        OPT_HELP(),
 
        // Pathnames for the output and error logging files
        OPT_STRING('o', "output", &args->output_filename, "Output file for the metadata test case in binary format"),
        OPT_STRING('e', "error", &args->error_filename, "Log error messages to the specified file (default stderr)"),

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
        "\nXML parser for converting a VC-5 metadata test case in XML format to a binary file.",
        "\nThe input file is expected to be a metadata test case in XML format created by the test case generator.\n");

    // Parse the command-line arguments and return the number of arguments remaining on the command line
    argc = argparse_parse(&argparse, argc, argv);

    if (argc > 1) {
        fprintf(stderr, "Extra input files will be ignored (argc: %d)\n", argc);
    }

    args->input_filename = (char *)argv[0];

    return 0;
}

#else

#include <getopt.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <assert.h>

#include "error.h"
#include "params.h"


/*!
	@brief Parse the program command-line arguments to get the parser parameters

	The parser parameters must be initialized by the caller with default values.
*/
//CODEC_ERROR ParseParameters(int argc, char *argv[], PARAMETERS *parameters, FILELIST *input, FILELIST *output)
CODEC_ERROR ParseParameters(int argc, char *argv[], PARAMETERS *parameters)
{
	int c;

	bool help_flag = false;

	//int option_index = 0;

	if (argc < 2)
	{
        // Print the usage message and exit
        //PrintUsageMessage(argc, argv);
		//exit(0);
		return CODEC_ERROR_BAD_COMMAND;
	}

	// Process the command-line options
	while ((c = getopt(argc, argv, "o:e:vh")) != -1)
	{
		assert(c != 0);

		// Process the command-line option
		switch (c)
		{
			case 'o':
				//fprintf(stderr, "Output filename: %s\n", optarg);
				parameters->output_filename = optarg;
				break;

			case 'e':
				parameters->error_filename = optarg;
				break;

			case 'v':
				parameters->verbose_flag = true;
				break;

			case 'h':
				help_flag = true;
				break;

			default:
				printf("Unknown option '%s'\n", argv[optind]);
				help_flag = true;
				break;
		}
	}

    // The remaining command line arguments must be the single input pathname
	if (optind < argc)
	{
    	//fprintf(stderr, "Input filename: %s\n", argv[optind]);
		parameters->input_filename = argv[optind];
		optind++;
	}

	if (help_flag)
	{
		//PrintUsageMessage(argc, argv);
		fprintf(stderr, "Usage: parser [-v] [-h] [-o output.bin] input.xml\n");
		return CODEC_ERROR_USAGE_INFO;
	}

	return CODEC_ERROR_OKAY;
}

#endif
