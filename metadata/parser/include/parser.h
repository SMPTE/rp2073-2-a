/*!	@file metadata.h

	Declaration of the data structures for metadata described in SMMPTE ST 2073-7 Metadata.

	(c) 2013-2021 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _PARSER_H
#define _PARSER_H


#ifdef XML_LARGE_SIZE
#  if defined(XML_USE_MSC_EXTENSIONS) && _MSC_VER < 1400
#    define XML_FMT_INT_MOD "I64"
#  else
#    define XML_FMT_INT_MOD "ll"
#  endif
#
#else
#  define XML_FMT_INT_MOD "l"
#endif

#ifdef XML_UNICODE_WCHAR_T
#  include <wchar.h>
#  define XML_FMT_STR "ls"
#else
#  define XML_FMT_STR "s"
#endif


struct _parser_data;

typedef void (* ElementTextProcessor)(struct _parser_data *parser_data, const void *text_buffer, size_t buffer_size);


/*!
	@brief Data structure for passing information to XML handlers
*/
typedef struct _parser_data
{
	bool verbose;					//!< Enable verbose output if true
	bool debug;						//!<Enable extra output for debugging
	int depth;						//!< Enable debugging output if true
	FILE *output;					//!< Output file for the binary metadata

	TUPLE_HEADER tuple_header;

	void *text_buffer;				//!< Buffer for accumulating the text for the current element
	size_t text_buffer_size;		//!< Allocated size of the text buffer (in bytes)
	size_t text_buffer_used;		//!< Amount of the text buffer in use (in bytes)

	//! Function for processing the element text
	//void (* element_text_processor)(void *buffer, size_t buffer_size);
	ElementTextProcessor element_text_processor;

} PARSER_DATA;


#define HANDLER static void XMLCALL


// Log file for reporting errors detected in the XML test cases
extern FILE *logfile;


CODEC_ERROR ParseMetadataFile(PARSER_DATA *parser_data, const char *pathname);

CODEC_ERROR DumpParserData(PARSER_DATA *parser_data);


#endif
