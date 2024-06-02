/*!	@file metadata/parser/src/parser.c

	Program for parsing the XML representation of metadata defined in ST 2073-7 Annex A.

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

#if _ENCODER

// Use relative paths to avoid conflicts with the encoder include paths
#include "../../common/include/config.h"
#include "../../common/include/error.h"
#include "../../common/include/swap.h"
#include "../../common/include/base64.h"
#include "../../common/include/buffer.h"
//#include "params.h"
#include "../../common/include/metadata.h"
#include "../include/parser.h"

#else

#include "config.h"
#include "error.h"
#include "swap.h"
#include "base64.h"
#include "buffer.h"
//#include "params.h"
#include "metadata.h"
#include "parser.h"

#endif

// Expat library for parsing XML
#include "expat.h"


// Log file for reporting errors detected in the XML test cases
FILE *logfile = NULL;


/*!
	@brief Replace missing or incorrect information in the tuple header
*/
CODEC_ERROR AdjustTupleHeader(TUPLE_HEADER *tuple_header, const char *value_data, const size_t value_size)
{
	// Replace the character zero with the numerical value
	if (tuple_header->type == '0') {
		tuple_header->type = 0;
	}

	// Compute the correct tuple header size based on the size of the metadata value
	//tuple_header->size = TupleHeaderSize(tuple_header, value_data, value_size);

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Return the size of the binary representation of the metadata tuple value
*/
size_t ValueSize(size_t size, size_t count)
{
	//fprintf(stderr, "size: %ld, count: %ld\n", size, count);
	return (count == 0) ? size : count * size;
}


//bool inside_text_element = false;


// Size of the text buffer when it is first allocated
const size_t initial_text_buffer_size = 4096;


/*!
	@brief Handler for processing the element text for a base64 encoded binary blob

	@todo Refactor this routine to call @ref WriteMetadataValue
*/
void process_binary_payload(PARSER_DATA *parser_data, const void *text_buffer, size_t buffer_size)
{
	//fprintf(stderr, "Element text length: %zu\n%s\n", buffer_size, (char *)text_buffer);

	// Allocate an output buffer large enough for the size of the metadata value
	size_t output_size = parser_data->tuple_header.size * parser_data->tuple_header.count;
	assert(output_size > 0);
	//uint8_t output_buffer[output_size];
	ALLOC_BUFFER(output_buffer, output_size);

	// Decode the element text into binary
	size_t actual_size = 0;
	decode_base64(text_buffer, buffer_size, output_buffer, sizeof(output_buffer), &actual_size);

	// Pad the decoded binary block with zeros to fill the size of the metadata value specified in the tuple header
	assert(output_size >= actual_size);
	int32_t padding = output_size - actual_size;
	assert(padding >= 0 && (actual_size + padding) == output_size);
	if (padding >= 0) {
		memset(&output_buffer[actual_size], 0, padding);
		//fprintf(stderr, "Added padding: %d, output_size: %zu, actual_size: %zu\n", padding, output_size, actual_size);
	}

	// Update the tuple header with the size of the metadata value
	assert(output_size == actual_size + padding);

	// Write the binary blob
	int result = fwrite(output_buffer, output_size, 1, parser_data->output);
	assert(result == 1);
	(void)result;

	// Write the metadata tuple padding to fill the tuple to a multiple of four bytes
	WriteMetadataPadding(parser_data->output, output_size);

	//NOTE: The metadata padding is in addition to any padding in the metadata value
}


/*!
	@brief Handler for processing the element text for character string payload

	@todo Refactor this routine to call @ref WriteMetadataValue
*/
void process_text_payload(PARSER_DATA *parser_data, const void *text_buffer, size_t buffer_size)
{
	//fprintf(stderr, "Element text length: %zu\n%s\n", buffer_size, (char *)text_buffer);

	// The text buffer should contain the entire element text with size as specified in the tuple header
	size_t actual_count = (parser_data->tuple_header.count > 0) ? parser_data->tuple_header.count : 1;
	size_t total_size = parser_data->tuple_header.size * actual_count;
	(void)total_size;

	//fprintf(stderr, "actual_count: %zu, total_size: %zu, buffer_size: %zu\n", actual_count, total_size, buffer_size);

	assert(total_size == buffer_size);

	// Write the text payload
	int result = fwrite(text_buffer, buffer_size, 1, parser_data->output);
	assert(result == 1);
	(void)result;

	// Write the metadata tuple padding to fill the tuple to a multiple of four bytes
	WriteMetadataPadding(parser_data->output, buffer_size);

	//NOTE: The metadata padding is in addition to any padding in the metadata value
}


/*!
	@brief Handler for processing the CDATA section containing a character string payload

	@todo Refactor this routine to call @ref WriteMetadataValue
*/
void process_CDATA_payload(PARSER_DATA *parser_data, const void *text_buffer, size_t buffer_size)
{
	//fprintf(stderr, "Element text length: %zu\n%s\n", buffer_size, (char *)text_buffer);

	// The text buffer should contain the entire element text with size as specified in the tuple header
	size_t actual_count = (parser_data->tuple_header.count > 0) ? parser_data->tuple_header.count : 1;
	size_t total_size = parser_data->tuple_header.size * actual_count;

	// fprintf(stderr, "CDATA payload actual_count: %zu, total_size: %zu, buffer_size: %zu\n", actual_count, total_size, buffer_size);
	// fprintf(stderr, "%s\n", (const char *)text_buffer);

	assert(total_size == buffer_size);
	(void)total_size;

	// Write the text payload
	int result = fwrite(text_buffer, buffer_size, 1, parser_data->output);
	assert(result == 1);
	(void)result;

	// Write the metadata tuple padding to fill the tuple to a multiple of four bytes
	WriteMetadataPadding(parser_data->output, buffer_size);
}


/*!
	@brief Table of element text processors indexed by tuple tag

	@todo Should use CDATA to hold text and base64 encoded payloads.
*/
struct _element_text_processor_entry
{
	FOURCC tag;
	ElementTextProcessor processor;

} element_text_processor_table[] = {

#if _MSC_VER
	{ 'DPXh', process_binary_payload },
	{ 'MXFd', process_binary_payload },
	{ 'ACEh', process_binary_payload },
	{ 'ALEd', process_text_payload   },
#else
	{ FOURCC_VALUE("DPXh"), process_binary_payload },
	{ FOURCC_VALUE("MXFd"), process_binary_payload },
	{ FOURCC_VALUE("ACEh"), process_binary_payload },
	{ FOURCC_VALUE("ALEd"), process_text_payload   },
	//{ FOURCC_VALUE("MXPd"), process_CDATA_payload  },

#endif
};

const size_t element_text_processor_table_length = sizeof(element_text_processor_table) / sizeof(element_text_processor_table[0]);


/*!
	@brief Lookup an elememnt text processor by tuple tag
*/
ElementTextProcessor LookupElementTextProcessor(FOURCC tag)
{
	ElementTextProcessor processor = NULL;

	for (size_t i = 0; i < element_text_processor_table_length; i++)
	{
		if (element_text_processor_table[i].tag == tag) {
			processor = element_text_processor_table[i].processor;
			break;
		}
	}

	return processor;
}


/*!
	@brief Handler for the start of an XML element
*/
HANDLER ElementStartHandler(void *user_data, const XML_Char *name, const XML_Char **attribute_list)
{
	int i;
	//int *depthPtr = (int *)userData;
	PARSER_DATA *parser_data = (PARSER_DATA *)user_data;

	const bool verbose = parser_data->verbose;
	const bool debug = parser_data->debug;

	(void)verbose;
	(void)debug;

	// Output the metadata to a binary file?
	if (parser_data->output)
	{
		//printf("Output XML to binary file\n");

		if (strcmp(name, "metadata") == 0)
		{
			if (debug) printf("metadata");

			for (i = 0; attribute_list[i]; i += 2)
			{
				// The attribute and its value are the next two items in the list
				const XML_Char *attribute = attribute_list[i];
				const XML_Char *value = attribute_list[i + 1];

				if (strcmp(attribute, "xmlns") == 0)
				{
					if (debug) printf(" %s=\"%s\"", attribute, value);
				}
			}

			if (debug) printf("\n");

			//NOTE: The metadata tuple is not written into the bitstream
		}

		else if (strcmp(name, "chunk") == 0)
		{
			uint32_t bitstream_tag;
			uint32_t bitstream_value;

			bool found_chunk_tag = false;
			bool found_chunk_size = false;

			if (debug) printf("chunk");

			for (i = 0; attribute_list[i]; i += 2)
			{
				// The attribute and its value are the next two items in the list
				const XML_Char *attribute = attribute_list[i];
				const XML_Char *value = attribute_list[i + 1];

				if (strcmp(attribute, "tag") == 0)
				{
					if (debug) printf(" %s=\"%s\"", attribute, value);
					bitstream_tag = strtoul(value, NULL, 0);
					found_chunk_tag = true;
				}

				if (strcmp(attribute, "size") == 0)
				{
					if (debug) printf(" %s=\"%s\"", attribute, value);
					bitstream_value = strtoul(value, NULL, 0);
					found_chunk_size = true;
				}
			}

			if (debug) printf("\n");

			assert(found_chunk_tag && found_chunk_size);
			if (! (found_chunk_tag && found_chunk_size)) {
				printf("Missing chunk tag or size\n");
			}

			//TODO: Update the chunk size after the chunk payload is written
			WriteChunkHeader(parser_data->output, bitstream_tag, bitstream_value);
 		}

		else if (strcmp(name, "tuple") == 0)
		{
			TUPLE_HEADER tuple_header;
			memset(&tuple_header, 0, sizeof(tuple_header));

			XML_Char *value_data = NULL;

			// Is the metadata value in an attribute or the text between start and end elements?
			bool has_value_attribute = false;

			// Bit mask indicating which attributes where found in the tuple
			ATTRIBUTE_MASK attribute_mask = 0;

			if (debug) printf("tuple");

			for (i = 0; attribute_list[i]; i += 2)
			{
				// The attribute and its value are the next two items in the list
				const XML_Char *attribute = attribute_list[i];
				const XML_Char *value = attribute_list[i + 1];

				if (debug) printf(" %s=\"%s\"", attribute, value);

				if (strcmp(attribute, "tag") == 0)
				{
					tuple_header.tag = ConvertTagToBinary(value);
					attribute_mask |= ATTRIBUTE_TAG;
				}
				else if (strcmp(attribute, "type") == 0)
				{
					tuple_header.type = (value[0] == '0') ? 0 : value[0];
					attribute_mask |= ATTRIBUTE_TYPE;
				}
				else if (strcmp(attribute, "size") == 0)
				{
					tuple_header.size = strtoul(value, NULL, 0);
					attribute_mask |= ATTRIBUTE_SIZE;
				}
				else if (strcmp(attribute, "count") == 0)
				{
					tuple_header.count = (TUPLE_COUNT)strtoul(value, NULL, 0);
					attribute_mask |= ATTRIBUTE_COUNT;
				}
				else if (strcmp(attribute, "value") == 0)
				{
					value_data = (XML_Char *)value;
					attribute_mask |= ATTRIBUTE_VALUE;

					// The element value is in a value attribute
					has_value_attribute = true;
				}
				else if (strcmp(attribute, "padding") == 0)
				{
					//tuple_header.padding = strtoul(value, NULL, 0);
					attribute_mask |= ATTRIBUTE_PADDING;
				}
				else
				{
					LogError("Unknown tuple attribute: %s\n", attribute);
				}
			}

			if (debug) printf("\n");

			if ((attribute_mask & ATTRIBUTE_TAG) == 0) {
				LogError("Missing tag attribute");
			}

			if ((attribute_mask & ATTRIBUTE_TYPE) == 0) {
				LogError("Missing type attribute tag: %c%c%c%c\n", FOURCC_CHARS(tuple_header.tag));
			}

			if ((attribute_mask & ATTRIBUTE_SIZE) == 0) {
				LogError("Missing size attribute tag: %c%c%c%c\n", FOURCC_CHARS(tuple_header.tag));
			}

			if (HasRepeatCount(tuple_header.type) && (attribute_mask & ATTRIBUTE_COUNT) == 0) {
				LogError("Missing count attribute tag: %c%c%c%c, type: %c\n", FOURCC_CHARS(tuple_header.tag), tuple_header.type);				
			}

			// Copy the current tupel header into the parser data for use by the element text and CDATA section handlers
			parser_data->tuple_header.tag = tuple_header.tag;
			parser_data->tuple_header.type = tuple_header.type;
			parser_data->tuple_header.size = tuple_header.size;
			parser_data->tuple_header.count = tuple_header.count;
			//parser_data->tuple_header.padding = tuple_header.padding;

			// Lookup the processor for the text in this element
			ElementTextProcessor element_text_processor = LookupElementTextProcessor(tuple_header.tag);
			if (element_text_processor != NULL && (attribute_mask & ATTRIBUTE_SIZE) != 0)
			{
				//fprintf(stderr, "Initializing element text processing: %c%c%c%c\n", FOURCC_CHARS(tuple_header.tag));

				if (parser_data->text_buffer == NULL)
				{
					// Initialize the buffer for holding the element text
					parser_data->text_buffer = malloc(initial_text_buffer_size);
					parser_data->text_buffer_size = initial_text_buffer_size;
					parser_data->text_buffer_used = 0;

					//parser_data->element_text_processor = process_dpx_payload;
					parser_data->element_text_processor = element_text_processor;

					//inside_text_element = true;
				}
			}

			// Write the metadata tuple header to the binary output file
			WriteMetadataHeader(parser_data->output, &tuple_header);

			//NOTE: The metadata header may have to be rewritten after element text has been processed

			if (has_value_attribute && (attribute_mask & ATTRIBUTE_SIZE) != 0)
			{
				size_t value_size = ValueSize(tuple_header.size, tuple_header.count);

				// Write the metadata value to the binary output file
				WriteMetadataValue(parser_data->output, &tuple_header, value_data, value_size);

				// Write the metadata tuple padding to the binary output file
				WriteMetadataPadding(parser_data->output, value_size);
			}
		}
	}
	else
	{
		//printf("%" XML_FMT_STR, name);
		printf("%s", name);

		for (i = 0; attribute_list[i]; i += 2)
		{
			// The attribute and its value are the next two items in the list
			const XML_Char *attribute = attribute_list[i];
			const XML_Char *value = attribute_list[i + 1];

			printf(" %s=\"%s\"", attribute, value);
		}

		printf("\n");
	}

	// Descend into the contents of the new element
	parser_data->depth += 1;
}


/*!
	@brief Handler for the text between start and end elements
*/
HANDLER CharacterDataHandler(void *userData, const XML_Char *data, int length)
{
	PARSER_DATA *parser_data = (PARSER_DATA *)userData;

	//fprintf(stderr, "data: %s, length: %d\n", (const char *)data, length);

	// Collecting text from the current element?
	if (parser_data->text_buffer != NULL)
	{
		//fprintf(stderr, "Character data length: %d\nvalue: \"%s\"\n", length, data);

		//Enough room for the new portion of this element text?
		if (parser_data->text_buffer_used + length > parser_data->text_buffer_size)
		{
			void *old_text_buffer = parser_data->text_buffer;

			// Allocate a larger buffer
			size_t new_text_buffer_size = 2 * parser_data->text_buffer_size;
			void *new_text_buffer = malloc(new_text_buffer_size);
			assert(new_text_buffer != NULL);
			if (! (new_text_buffer != NULL)) {
				// Cannot allocate a larger buffer for the new portion of the element text
				return;
			}

			// Copy the contents of the current buffer into the new buffer
			memcpy(new_text_buffer, old_text_buffer, parser_data->text_buffer_used);

			// Free the old buffer
			free(old_text_buffer);

			// Update the parser data with the new buffer and its size
			parser_data->text_buffer = new_text_buffer;
			parser_data->text_buffer_size = new_text_buffer_size;
		}

		uint8_t *text_buffer = (uint8_t *)parser_data->text_buffer;
		memcpy(&text_buffer[parser_data->text_buffer_used], data, length);
		parser_data->text_buffer_used += length;
	}
}


/*!
	@brief Handler for the end of an XML element
*/
HANDLER ElementEndHandler(void *user_data, const XML_Char *name)
{
	//int *depthPtr = (int *)userData;
	PARSER_DATA *parser_data = (PARSER_DATA *)user_data;
	(void)name;

	if (parser_data->text_buffer != NULL)
	{
		//fprintf(stderr, "Element text length: %zu\n%s\n", parser_data->text_buffer_used, (char *)parser_data->text_buffer);

		if (parser_data->element_text_processor != NULL)
		{
			//Process the element text
			(parser_data->element_text_processor)(parser_data, parser_data->text_buffer, parser_data->text_buffer_used);
		}

		// Reset the element text buffer
		free(parser_data->text_buffer);
		parser_data->text_buffer = NULL;
		parser_data->text_buffer_size = 0;
		parser_data->text_buffer_used = 0;

		//TODO: Could reduce the number of memory allocations by reusing the buffer
	}

	// Stop collecting text from the body of this element
	//inside_text_element = false;

	//*depthPtr -= 1;
	parser_data->depth -= 1;
}


/*!
	@brief Handler for the start of a CDATA section
*/
HANDLER CDATA_SectionStart(void *user_data)
{
	//int *depthPtr = (int *)userData;
	PARSER_DATA *parser_data = (PARSER_DATA *)user_data;

	//fprintf(stderr, "CDATA Section Start\n");

	// Initialize the buffer for holding the element text
	parser_data->text_buffer = malloc(initial_text_buffer_size);
	parser_data->text_buffer_size = initial_text_buffer_size;
	parser_data->text_buffer_used = 0;

	parser_data->element_text_processor = process_CDATA_payload;
}


/*!
	@brief Handler for the end of a CDATA section
*/
HANDLER CDATA_SectionEnd(void *user_data)
{
	//int *depthPtr = (int *)userData;
	PARSER_DATA *parser_data = (PARSER_DATA *)user_data;

	//fprintf(stderr, "CDATA Section End\n");

	if (parser_data->text_buffer != NULL)
	{
		//fprintf(stderr, "CDATA section length: %zu\n%s\n", parser_data->text_buffer_used, (char *)parser_data->text_buffer);

		if (parser_data->element_text_processor != NULL)
		{
			//Process the element text
			(parser_data->element_text_processor)(parser_data, parser_data->text_buffer, parser_data->text_buffer_used);
		}

		// Reset the element text buffer
		free(parser_data->text_buffer);
		parser_data->text_buffer = NULL;
		parser_data->text_buffer_size = 0;
		parser_data->text_buffer_used = 0;
	}
}


/*!
	@brief Parse the test case in XML format in the file passed as a argument.

*/
CODEC_ERROR ParseMetadataFile(PARSER_DATA *parser_data, const char *pathname)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;
	char buffer[BUFSIZ];
	bool done;

	if (parser_data->verbose) {
		DumpParserData(parser_data);
	}

	//XML_Parser parser = XML_ParserCreate("UTF-8");
	XML_Parser parser = XML_ParserCreate(NULL);
	assert(parser != NULL);

	// Initialize the XML parser with user data and element handlers
	XML_SetUserData(parser, parser_data);
	XML_SetElementHandler(parser, ElementStartHandler, ElementEndHandler);
	XML_SetCharacterDataHandler(parser, CharacterDataHandler);
	XML_SetCdataSectionHandler(parser, CDATA_SectionStart, CDATA_SectionEnd);

	// Open the input file that contains the XML representation of metadata
	FILE *input = fopen(pathname, "r");
	if (input == NULL) {
		fprintf(stderr, "Could not open metadata filename: %s\n", pathname);
		error = CODEC_ERROR_FILE_OPEN;
	}
	else
	{
		// Process the XML file one buffer of data at a time
		do {
			size_t length = fread(buffer, 1, sizeof(buffer), input);
			done = length < sizeof(buffer);
			if (XML_Parse(parser, buffer, (int)length, done) == XML_STATUS_ERROR) {
				fprintf(stderr,
					"%" XML_FMT_STR " at line %" XML_FMT_INT_MOD "u\n",
					XML_ErrorString(XML_GetErrorCode(parser)),
					XML_GetCurrentLineNumber(parser));
				error = CODEC_ERROR_XML_PARSER;
				break;
			}
		} while (!done);

		// Close the input file and the XML parser
		fclose(input);
	}

	XML_ParserFree(parser);

	return error;
}


/*!
	@brief Dump the parser data (for debugging)
*/
CODEC_ERROR DumpParserData(PARSER_DATA *parser_data)
{
	//printf("Verbose: %d\n", parser_data->verbose);
	//int depth;
	//printf("Output: %p\n", parser_data->output);

	//TUPLE_HEADER tuple_header;
	//void *text_buffer;
	//size_t text_buffer_size;
	//size_t text_buffer_used;

	return CODEC_ERROR_OKAY;
}

