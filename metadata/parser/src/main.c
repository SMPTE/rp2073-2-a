/*!	@file metadata/parser/src/main.c

	Routines for parsing the XML representation of metadata defined in ST 2073-7 Annex A.

	This program is based on sample code from the Expat repository:
	https://github.com/libexpat/libexpat/blob/master/expat/examples/elements.c

	(c) 2013-2021 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <wchar.h>
#include <locale.h>
#include <expat.h>

#include "config.h"
#include "error.h"
#include "swap.h"
#include "base64.h"
#include "params.h"
#include "metadata.h"
#include "parser.h"

#include "expat.h"


/*!
	@brief Main routine

	Parse the command-line arguments, initialize the XML parser, open the input file, and parse the XML.
	The optional output commmand-line argument enables binary output and specifies the output file.
*/
int main(int argc, const char *argv[])
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;

	// Data structure for the command-line arguments
	PARAMETERS parameters;
	memset(&parameters, 0, sizeof(parameters));

	// Parse the command-line arguments
	error = ParseParameters(argc, argv, &parameters);
	if (error != CODEC_ERROR_OKAY) {
		return error;
	}

	if (parameters.verbose_flag)
	{
		fprintf(stderr, "Input filename: %s\n", parameters.input_filename);
		fprintf(stderr, "Output filename: %s\n", parameters.output_filename);
		fprintf(stderr, "Error filename: %s\n", parameters.error_filename);
		fprintf(stderr, "Verbose flag: %d\n", parameters.verbose_flag);
	}

	// Initialize the data passed to the XML element handlers
	PARSER_DATA parser_data;
	memset(&parser_data, 0, sizeof(parser_data));
	parser_data.verbose = parameters.verbose_flag;

	if (parameters.output_filename)
	{
		// Open the outout file for the binary representation of the XML metadata
		parser_data.output = fopen(parameters.output_filename, "wb");
		if (parser_data.output == NULL) {
			fprintf(stderr, "Could not open output filename: %s\n", parameters.output_filename);
			return 1;
		}
	}

	if (parameters.error_filename)
	{
		// Append error reports to the log file
		//logfile = fopen(parameters.error_filename, "a");

		// Overwrite existing log files with the log entries for this run
		logfile = fopen(parameters.error_filename, "w");
	}
	else
	{
		//LogError("Setting error file to stderr\n");
		logfile = stderr;
	}

	//fprintf(stderr, "XML_Char size: %zd\n", sizeof(XML_Char));

	error = ParseMetadataFile(&parser_data, parameters.input_filename);
	if (error != CODEC_ERROR_OKAY) {
		fprintf(stderr, "Metadata parser error: %d\n", error);
		return error;
	}

	// Close the output file
	if (parser_data.output != NULL) {
		fclose(parser_data.output);
	}

	// Close the log file
	if (logfile != NULL && logfile != stderr) {
		fclose(logfile);
	}

	return 0;
}
