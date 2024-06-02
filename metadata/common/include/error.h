/*! @file metadata/include/error.h

	Declaration of error codes for the XML parser and dumper.

	(c) 2013-2019 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _ERROR_H
#define _ERROR_H

typedef enum _codec_error
{
	CODEC_ERROR_OKAY = 0,
	CODEC_ERROR_UNEXPECTED,

#if _ENCODER || _DECODER
	// The sample encoder and reference decoder reserve a block of error codes for the metadata subsystem
    CODEC_ERROR_METADATA = 100,
#endif

	CODEC_ERROR_USAGE_INFO,
	CODEC_ERROR_BAD_COMMAND,
	CODEC_ERROR_FILE_CREATE,
	CODEC_ERROR_FILE_OPEN,
	CODEC_ERROR_FILE_WRITE,
	CODEC_ERROR_FILE_READ,
	CODEC_ERROR_BAD_INPUT,
	CODEC_ERROR_STACK_ERROR,
	CODEC_ERROR_BAD_CHUNK,
	CODEC_ERROR_BAD_TUPLE,
	CODEC_ERROR_BAD_TYPE,
	CODEC_ERROR_BUFFER_SIZE,
	CODEC_ERROR_UNKNOWN_TYPE,
	CODEC_ERROR_XML_PARSER,
	CODEC_ERROR_XML_WRITE,
	CODEC_ERROR_ALLOC_FAILED,

} CODEC_ERROR;


// Output file for reporting errors
extern FILE *logfile;


static inline void LogError(const char *format, ...)
{
	if (logfile != NULL)
	{
	   	va_list args;
		va_start(args, format);
	    vfprintf(logfile, format, args);
	    va_end(args);
	}
}

#endif
