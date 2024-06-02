/*!	@file main.c

	@brief Main program for the XML dumper.
*/

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <assert.h>

#include "config.h"
#include "swap.h"
#include "error.h"
#include "params.h"
#include "base64.h"

//#include "mxml.h"

#include "metadata.h"

#if _DATABASE
#include "database.h"
#endif

#include "dumper.h"


//! Global flag that controls debug output
bool debug_flag = false;


//! Log file for reporting errors detected in the XML test cases
FILE *logfile = NULL;


// if 0
// #ifdef __clang__
// //! Maximum depth of the XML node stack
// const int MAXIMUM_XML_NODE_STACK_DEPTH = 12;
// #else
// //! Maximum depth of the XML node stack (defined as a macro for compilers other than Clang)
// #define MAXIMUM_XML_NODE_STACK_DEPTH 12
// #endif
// #endif

//! Macro for printing Boolean variables
#define BOOLEAN_STRING(expression) ((expression) ? "true" : "false")


//! Macro for computing the size of a struct member
//#define FIELD_SIZE(type, field) sizeof(((type *)0)->field)


//! Macro for computing the length of a struct member array
//#define FIELD_LENGTH(type, field) (sizeof(((type *)0)->field)/sizeof(((type *)0)->field[0]))


/*!
	@brief Main program for the metadata XML dumper
*/
int
main(int argc, const char *argv[])
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;

	ARGUMENTS args;
	memset(&args, 0, sizeof(args));

	parse_args(argc, argv, &args);

	if (args.input_filename == NULL) {
		fprintf(stderr, "Must provide exactly one input filename on the command line\n");
		//exit(1);
	}

	if (args.verbose_flag)
	{
		fprintf(stderr, "Input filename: %s\n", args.input_filename);
		fprintf(stderr, "Output filename: %s\n", args.output_filename);
		fprintf(stderr, "Error filename: %s\n", args.error_filename);
		fprintf(stderr, "Remove duplicates: %s\n", BOOLEAN_STRING(args.duplicates_flag));
		fprintf(stderr, "Verbose flag: %s\n", BOOLEAN_STRING(args.verbose_flag));
		fprintf(stderr, "Quiet flag: %s\n", BOOLEAN_STRING(args.quiet_flag));
		fprintf(stderr, "Debug flag: %s\n", BOOLEAN_STRING(args.debug_flag));
		fprintf(stderr, "\n");
	}

	// Set the global debug flag for print statements used only for debugging
	debug_flag = args.debug_flag;

	FILE *input_file = fopen(args.input_filename, "rb");
	if (input_file == NULL) {
		fprintf(stderr, "Could not open input file: %s\n", args.input_filename);
	}

	FILE *output_file = stdout;
	if (args.output_filename != NULL)
	{
		output_file = fopen(args.output_filename, "w");
		if (output_file == NULL) {
			fprintf(stderr, "Could not open output file: %s\n", args.output_filename);
		}
	}

	if (args.error_filename != NULL)
	{
		// Append error reports to the log file
		//logfile = fopen(parameters.error_filename, "a");

		// Overwrite existing log files with the log entries for this run
		logfile = fopen(args.error_filename, "w");
	}
	else
	{
		//LogError("Setting error file to stderr\n");
		logfile = stderr;
	}

#if _TESTING

	// Test the code used by the reference decoder
	DATABASE *database = NULL;
	error = CreateMetadataDatabase(&database, args.verbose_flag, args.debug_flag, args.duplicates_flag);
	if (error != CODEC_ERROR_OKAY) {
		fprintf(stderr, "Could not create metadata database: %d\n", error);
		return error;
	}

	error = DecodeBinaryMetadata(input_file, output_file, database, &args);
	if (error == CODEC_ERROR_FILE_WRITE) {
		fprintf(stderr, "Could not write XML metadata file: %s\n", args.output_filename);
		return error;
	}
	else if (error != CODEC_ERROR_OKAY) {
		fprintf(stderr, "Could not read binary metadata file: %s\n", args.input_filename);
		return error;
	}

	error = DestroyMetadataDatabase(database);
	database = NULL;

#elif _DATABASE

	// Raad the metadata from a binary file and store in a database
	DATABASE *database = NULL;
	error = CreateMetadataDatabase(&database, &args);
	if (error != CODEC_ERROR_OKAY) {
		fprintf(stderr, "Could not create metadata database: %d\n", error);
		return error;
	}

	error = ReadBinaryMetadata(input_file, database, &args);
	if (error != CODEC_ERROR_OKAY) {
		fprintf(stderr, "Could not read binary metadata: %s\n", args.input_filename);
		return error;
	}

	error = DestroyMetadataDatabase(database);
	database = NULL;

#else

	// Read the metadata from a binary fle and output the metadata in XML format
	error = DumpBinaryFile(input_file, output_file, &args);
	if (error != CODEC_ERROR_OKAY) {
		fprintf(stderr, "Could not dump metadata to a file: %d\n", error);
		return error;
	}

#endif

	fclose(input_file);
	if (output_file != NULL && output_file != stdout) {
		fclose(output_file);
	}

	return error;
}
