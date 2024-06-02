/*!	@file dumper.h

	@brief Definitions for the XML dumper.
*/

#ifndef _DUMPER_H
#define _DUMPER_H


//! Global flag that controls debug output
extern bool debug_flag;


//! Log file for reporting errors detected in the XML test cases
extern FILE *logfile;


//! Dump a plain text representation of the binary metadata test case to the output file
CODEC_ERROR DumpBinaryFile(FILE *input_file, FILE *output_file, ARGUMENTS *args);

#if _DATABASE

//! Read metadata from a binary file and store the metadata in a database
CODEC_ERROR ReadBinaryMetadata(FILE *input_file, DATABASE *database, ARGUMENTS *args);

#endif

#endif
