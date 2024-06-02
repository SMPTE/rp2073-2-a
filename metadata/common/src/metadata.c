/*!	@file metadata/src/metadata.c

	Module for writing metadata to a binary file.

	(c) 2013-2019 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <assert.h>

#if _ENCODER || (_DECODER && !_TESTING)

//#include <expat.h>

// Use relative paths to avoid conflicts with the encoder and decoderinclude paths
#include "../include/config.h"
#include "../include/error.h"
#include "../include/swap.h"
#include "../include/base64.h"
#include "../include/buffer.h"
#include "../include/metadata.h"

#else

//#include "mxml.h"

#include "config.h"
#include "error.h"
#include "swap.h"
#include "base64.h"
#include "buffer.h"
#include "metadata.h"

#endif


/*!
	@brief Parse the metadata chunk header into the tag and size

	Return true if the segment is a valid chunk header
*/
bool ParseChunkHeader(SEGMENT segment,			//!< Four byte metadata chunk header
					  uint16_t *tag_out,		//!< Return the metadata chunk tag
					  uint32_t *size_out,		//!< Return the metadata chunk size
					  bool swapped_flag			//!< True if the segment has been swapped and the tag has been negated
					  )
{
	uint16_t chunk_tag;
	uint32_t chunk_size;

	// Need to byte swap and segment and negate the tuple tag?
	if (! swapped_flag)
	{
		segment = Swap32(segment);

		// Is this segment a small metadata chunk?
		chunk_tag = segment >> 16;
		chunk_tag = NEG(chunk_tag);
	}
	else
	{
		// The segment has already been swapoed and the tuple tag negated
		chunk_tag = segment >> 16;
	}

	// Is this segment a small chunk element?
	if (chunk_tag == METADATA_CHUNK_SMALL)
	{
		chunk_size = segment & 0xFFFF;

		*tag_out = chunk_tag;
		*size_out = chunk_size;

		return true;
	}

	// Is this segment a large chunk element?
	if ((chunk_tag >> 8) == METADATA_CHUNK_LARGE)
	{
		chunk_size = ((chunk_tag & 0xFF) << 16) | (segment & 0xFFFF);
		chunk_tag >>= 8;

		*tag_out = chunk_tag;
		*size_out = chunk_size;

		return true;
	}

	return false;
}


/*!
	@brief Return true if the metadata tuple has a repeat count
*/
bool HasRepeatCount(TUPLE_TYPE type)
{
	// Tuple data types that have a repeat count (SMPTE ST 2073-7 Table 4)
	static char *repeat_count_types = "bBfdFGlLjJqQrRsSU";

	return type != 0 && strchr(repeat_count_types, type) != NULL;
}


/*!
	@brief Return the size of each scalar element in a vector of the specified type

	@todo Refactor the code so that all routines used this routine to compute the size of each scalar element
*/
size_t ElementSize(TUPLE_TYPE type)
{
	size_t type_size = 0;

	static const struct _type_size
	{
		TUPLE_TYPE type;
		TUPLE_SIZE size;
	}
	type_size_table[] =
	{
		{'b', sizeof(int8_t)},
		{'B', sizeof(uint8_t)},
		{'s', sizeof(int16_t)},
		{'S', sizeof(uint16_t)},
		{'l', sizeof(int32_t)},
		{'L', sizeof(uint32_t)},
		{'j', sizeof(int64_t)},
		{'J', sizeof(uint64_t)},
		{'f', sizeof(float)},
		{'d', sizeof(double)}
	};

	static const int type_size_table_length = sizeof(type_size_table)/sizeof(type_size_table[0]);

	for (int i = 0; i < type_size_table_length; i++)
	{
		if (type == type_size_table[i].type) {
			type_size = type_size_table[i].size;
		}
	}

	assert(type_size > 0);

	return type_size;
}



FOURCC ConvertTagToBinary(const char *string)
{
	assert(strlen(string) == 4);
	return Swap32(string[0] << 24 | string[1] << 16 | string[2] << 8 | string[3]);
}


/*!
	@brief Convert hexadecimal character to its integer equivalent
*/
int hexint(char hex)
{
	if (isdigit(hex)) return hex - '0';
	if ('a' <= hex && hex <= 'f') return hex - 'a' + 10;
	if ('A' <= hex && hex <= 'F') return hex - 'A' + 10;

	// Not a hexadecimal character
	return 0;
}


/*!
	@brief Convert a string representation of a universal label to binary
*/
CODEC_ERROR ConvertLabelValue(const TUPLE_HEADER *tuple_header, const char *string, uint8_t *buffer, const size_t buffer_size)
{
	uint8_t *bufptr = buffer;
	size_t index = 0;

	assert(tuple_header->type == 'U');
	size_t string_length = 2 * tuple_header->size;

	//TODO: Handle count greater than one

	//fprintf(stderr, "Convert label value: %s\n", string);

	// Skip the hexadecimal flag at the beginning of the string
	//if (strncmp("0x", string, 2) == 0) index += 2;

	//TODO: Add code to skip the prefix in SMPTE standard label strings

	// The hexadecimal string should be twice as long as the output buffer
	assert(strlen(string) == string_length);

	assert(string_length <= 2 * buffer_size);

	while (index < string_length && string[index] != '\0')
	{
		// Convert the first hexadecimal digit
		uint8_t h1 = hexint(string[index++]);

		// Check for premature end of the string
		if (! (index < string_length && string[index] != '\0')) break;

		// Convert the second hexadecimal digit
		uint8_t h2 = hexint(string[index++]);

		// Combine the two nibbles into one byte
		*(bufptr++) = (h1 << 4) | h2;
	}

	// fprintf(stderr, "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n",
	// 	buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7],
	// 	buffer[8], buffer[9], buffer[10], buffer[11], buffer[12], buffer[13], buffer[14], buffer[15]);

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Return true if the tuple type is a numerical type
*/
bool IsNumericalType(TUPLE_TYPE type)
{
	switch (type)
	{
		case 'b':
		case 'B':		// Unsigned byte could be a number or a buffer of bytes encoded in base64
		case 's':
		case 'S':
		case 'l':
		case 'L':
		case 'j':
		case 'J':
		case 'f':
		case 'd':
			return true;
			break;

		default:
			return false;
	}
}


/*!
	@brief Return true if the value is a vector
*/
bool IsVectorValued(TUPLE_TYPE type, TUPLE_SIZE size)
{
	static const struct _scalar_size
	{
		TUPLE_TYPE type;
		TUPLE_SIZE size;
	}
	scalar_size_table[] =
	{
		{'b', sizeof(int8_t)},
		{'B', sizeof(uint8_t)},
		{'s', sizeof(int16_t)},
		{'S', sizeof(uint16_t)},
		{'l', sizeof(int32_t)},
		{'L', sizeof(uint32_t)},
		{'j', sizeof(int64_t)},
		{'J', sizeof(uint64_t)},
		{'f', sizeof(float)}
	};

	static const int scalar_size_table_length = sizeof(scalar_size_table)/sizeof(scalar_size_table[0]);

	for (int i = 0; i < scalar_size_table_length; i++)
	{
		if (type == scalar_size_table[i].type)
		{
			size_t scalar_size = scalar_size_table[i].size;
			return (size > scalar_size && size % scalar_size == 0);
		}
	}

	return false;
}


/*!
	@brief Convert a string of signed 8-bit integers to binary
*/
CODEC_ERROR ConvertVectorInt8(const TUPLE_HEADER *tuple_header, const char *string, int8_t *output_buffer, size_t buffer_size)
{
	// Compute the number of elements in the array
	size_t actual_count = (tuple_header->count > 0) ? tuple_header->count : 1;
	size_t length = tuple_header->size * actual_count / ElementSize(tuple_header->type);
	char * const string_ptr = (char * const)string;

	assert(length * sizeof(int8_t) <= buffer_size);
	if (! (length * sizeof(int8_t) <= buffer_size)) {
		return CODEC_ERROR_BUFFER_SIZE;
	}

	for (size_t i = 0; i < length; i++) {
		output_buffer[i] = (int8_t)strtol(string_ptr, (char **)&string_ptr, 0);
	}

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Convert a string of unsigned 8-bit integers to binary
*/
CODEC_ERROR ConvertVectorUint8(const TUPLE_HEADER *tuple_header, const char *string, uint8_t *output_buffer, size_t buffer_size)
{
	// Compute the number of elements in the array
	size_t actual_count = (tuple_header->count > 0) ? tuple_header->count : 1;
	size_t length = tuple_header->size * actual_count / ElementSize(tuple_header->type);
	char * const string_ptr = (char * const)string;

	assert(length * sizeof(output_buffer[0]) <= buffer_size);
	if (! (length * sizeof(output_buffer[0]) <= buffer_size)) {
		return CODEC_ERROR_BUFFER_SIZE;
	}

	for (size_t i = 0; i < length; i++) {
		output_buffer[i] = (uint8_t)strtoul(string_ptr, (char **)&string_ptr, 0);
	}

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Convert a string of signed 16-bit integers to binary
*/
CODEC_ERROR ConvertVectorInt16(const TUPLE_HEADER *tuple_header, const char *string, int16_t *output_buffer, size_t buffer_size)
{
	// Compute the number of elements in the array
	size_t actual_count = (tuple_header->count > 0) ? tuple_header->count : 1;
	size_t length = tuple_header->size * actual_count / ElementSize(tuple_header->type);
	char * const string_ptr = (char * const)string;

	assert(length * sizeof(output_buffer[0]) <= buffer_size);
	if (! (length * sizeof(output_buffer[0]) <= buffer_size)) {
		return CODEC_ERROR_BUFFER_SIZE;
	}

	for (size_t i = 0; i < length; i++) {
		output_buffer[i] = (int16_t)Swap16((uint16_t)strtol(string_ptr, (char **)&string_ptr, 0));
	}

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Convert a string of signed 16-bit integers to binary
*/
CODEC_ERROR ConvertVectorUint16(const TUPLE_HEADER *tuple_header, const char *string, uint16_t *output_buffer, size_t buffer_size)
{
	// Compute the number of elements in the array
	size_t actual_count = (tuple_header->count > 0) ? tuple_header->count : 1;
	size_t length = tuple_header->size * actual_count / ElementSize(tuple_header->type);
	char * const string_ptr = (char * const)string;

	assert(length * sizeof(output_buffer[0]) <= buffer_size);
	if (! (length * sizeof(output_buffer[0]) <= buffer_size)) {
		return CODEC_ERROR_BUFFER_SIZE;
	}

	for (size_t i = 0; i < length; i++) {
		output_buffer[i] = (uint16_t)Swap16((uint16_t)strtoul(string_ptr, (char **)&string_ptr, 0));
	}

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Convert a string of signed 32-bit integers to binary
*/
CODEC_ERROR ConvertVectorInt32(const TUPLE_HEADER *tuple_header, const char *string, int32_t *output_buffer, size_t buffer_size)
{
	// Compute the number of elements in the array
	size_t actual_count = (tuple_header->count > 0) ? tuple_header->count : 1;
	size_t length = tuple_header->size * actual_count / ElementSize(tuple_header->type);
	char * const string_ptr = (char * const)string;

	assert(length * sizeof(output_buffer[0]) <= buffer_size);
	if (! (length * sizeof(output_buffer[0]) <= buffer_size)) {
		return CODEC_ERROR_BUFFER_SIZE;
	}

	for (size_t i = 0; i < length; i++) {
		output_buffer[i] = (int32_t)Swap32(strtol(string_ptr, (char **)&string_ptr, 0));
	}

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Convert a string of signed 32-bit integers to binary
*/
CODEC_ERROR ConvertVectorUint32(const TUPLE_HEADER *tuple_header, const char *string, uint32_t *output_buffer, size_t buffer_size)
{
	// Compute the number of elements in the array
	size_t actual_count = (tuple_header->count > 0) ? tuple_header->count : 1;
	size_t length = tuple_header->size * actual_count / ElementSize(tuple_header->type);
	char * const string_ptr = (char * const)string;

	assert(length * sizeof(output_buffer[0]) <= buffer_size);
	if (! (length * sizeof(output_buffer[0]) <= buffer_size)) {
		return CODEC_ERROR_BUFFER_SIZE;
	}

	for (size_t i = 0; i < length; i++) {
		output_buffer[i] = (uint32_t)Swap32(strtoul(string_ptr, (char **)&string_ptr, 0));
	}

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Convert a string of signed 64-bit integers to binary
*/
CODEC_ERROR ConvertVectorInt64(const TUPLE_HEADER *tuple_header, const char *string, int64_t *output_buffer, size_t buffer_size)
{
	// Compute the number of elements in the array
	size_t actual_count = (tuple_header->count > 0) ? tuple_header->count : 1;
	size_t length = tuple_header->size * actual_count / ElementSize(tuple_header->type);
	char * const string_ptr = (char * const)string;

	assert(length * sizeof(output_buffer[0]) <= buffer_size);
	if (! (length * sizeof(output_buffer[0]) <= buffer_size)) {
		return CODEC_ERROR_BUFFER_SIZE;
	}

	for (size_t i = 0; i < length; i++) {
		output_buffer[i] = (int64_t)Swap64(strtol(string_ptr, (char **)&string_ptr, 0));
	}

	return CODEC_ERROR_OKAY;
}


/*!
	@ Convert a string of signed 64-bit integers to binary
*/
CODEC_ERROR ConvertVectorUint64(const TUPLE_HEADER *tuple_header, const char *string, uint64_t *output_buffer, size_t buffer_size)
{
	// Compute the number of elements in the array
	size_t actual_count = (tuple_header->count > 0) ? tuple_header->count : 1;
	size_t length = tuple_header->size * actual_count / ElementSize(tuple_header->type);
	char * const string_ptr = (char * const)string;

	assert(length * sizeof(output_buffer[0]) <= buffer_size);
	if (! (length * sizeof(output_buffer[0]) <= buffer_size)) {
		return CODEC_ERROR_BUFFER_SIZE;
	}

	for (size_t i = 0; i < length; i++) {
		output_buffer[i] = (uint64_t)Swap64(strtoul(string_ptr, (char **)&string_ptr, 0));
	}

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Convert a string of single-precision floating-point numbers to binary
*/
CODEC_ERROR ConvertVectorFloat32(const TUPLE_HEADER *tuple_header, const char *string, float *output_buffer, size_t buffer_size)
{
	// Compute the number of elements in the array
	size_t actual_count = (tuple_header->count > 0) ? tuple_header->count : 1;
	size_t length = tuple_header->size * actual_count / ElementSize(tuple_header->type);
	char * const string_ptr = (char * const)string;

	assert(length * sizeof(output_buffer[0]) <= buffer_size);
	if (! (length * sizeof(output_buffer[0]) <= buffer_size)) {
		return CODEC_ERROR_BUFFER_SIZE;
	}

	for (size_t i = 0; i < length; i++) {
		output_buffer[i] = (float)SwapFloat32(strtof(string_ptr, (char **)&string_ptr));
	}

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Convert a string of signed 64-bit integers to binary
*/
CODEC_ERROR ConvertVectorFloat64(const TUPLE_HEADER *tuple_header, const char *string, double *output_buffer, size_t buffer_size)
{
	// Compute the number of elements in the array
	size_t actual_count = (tuple_header->count > 0) ? tuple_header->count : 1;
	size_t length = tuple_header->size * actual_count / ElementSize(tuple_header->type);
	char * const string_ptr = (char * const)string;

	assert(length * sizeof(output_buffer[0]) <= buffer_size);
	if (! (length * sizeof(output_buffer[0]) <= buffer_size)) {
		return CODEC_ERROR_BUFFER_SIZE;
	}

	for (size_t i = 0; i < length; i++) {
		output_buffer[i] = (double)SwapFloat64(strtod(string_ptr, (char **)&string_ptr));
	}

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Convert a string of four character codes (FOURCC) to binary
*/
CODEC_ERROR ConvertVectorFOURCC(const TUPLE_HEADER *tuple_header, const char *string, uint8_t *output_buffer, size_t buffer_size)
{
	const char *string_ptr = (char * const)string;
	uint8_t *output_ptr = output_buffer;
	size_t ununsed_size = buffer_size;

	TUPLE_COUNT output_count = 0;

	for (int i = 0; i < tuple_header->count; i++)
	{
		// Skip leading spaces
		while (*string_ptr != '\0' && isspace(*string_ptr)) {
			string_ptr++;
		}

		// Copy the four character codes
		while (*string_ptr != '\0' && !isspace(*string_ptr) && ununsed_size > 0) {
			*(output_ptr++) = *(string_ptr++);
		}

		// Count the number of FOURCC copied to the output buffer
		output_count++;
	}

	//fprintf(stderr, "actual: %d, count: %d\n", output_count, tuple_header->count);
	assert(output_count == tuple_header->count);

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Convert a hexadecimal character to its integer value
*/
uint8_t ordinal(char c)
{
	// Check that the character is in the hexadecimal character set
	assert(strchr("0123456789abcdefABCDEF", c) != NULL);
	if (! (strchr("0123456789abcdefABCDEF", c) != NULL)) {
		return 0;
	}

	if (isdigit(c)) return (c - '0');
	if (islower(c)) return (c - 'a' + 10);
	if (isupper(c)) return (c - 'A' + 10);

	return 0;
}


/*!
	@brief Convert a string of universally unique identifiers (UUID) to binary
*/
CODEC_ERROR ConvertVectorUUID(const TUPLE_HEADER *tuple_header, const char *string, uint8_t *output_buffer, size_t buffer_size)
{
	const char *string_ptr = (char * const)string;
	uint8_t *output_ptr = output_buffer;
	size_t ununsed_size = buffer_size;

	TUPLE_COUNT output_count = 0;

	for (int i = 0; i < tuple_header->count; i++)
	{
		// Skip leading spaces
		while (*string_ptr != '\0' && isspace(*string_ptr)) {
			string_ptr++;
		}

		// Convert the UUID string to binary
		while (*string_ptr != '\0' && !isspace(*string_ptr) && ununsed_size > 0) {
			uint8_t byte =  ordinal(*(string_ptr++));
			byte = (byte << 4) | ordinal(*(string_ptr++));
			//fprintf(stderr, "Byte: %02X\n", byte);
			*(output_ptr++) = byte;
		}

		// Count the number of UUID copied to the output buffer
		output_count++;
	}

	//fprintf(stderr, "actual: %d, count: %d\n", output_count, tuple_header->count);
	assert(output_count == tuple_header->count);

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Convert a string of numbers separated by white space to a vector of numbers in binary

	Binary blobs encoded as a base64 string are handled as a special case in another routine.
*/
CODEC_ERROR ConvertValueToBinary(void *buffer, size_t buffer_size, const TUPLE_HEADER *tuple_header, const char *value)
{
	//printf("ConvertVectorToBinary size: %d, count: %d\n", tuple_header->size, tuple_header->count);

	switch (tuple_header->type)
	{
		case 'c':
		case 'u':
		case 'x':
			memcpy(buffer, value, buffer_size);
			break;

		case 'b':
			return ConvertVectorInt8(tuple_header, value, buffer, buffer_size);
			break;

		case 'B':
			return ConvertVectorUint8(tuple_header, value, buffer, buffer_size);
			break;

		case 's':
			return ConvertVectorInt16(tuple_header, value, buffer, buffer_size);
			break;

		case 'S':
			return ConvertVectorUint16(tuple_header, value, buffer, buffer_size);
			break;

		case 'l':
			return ConvertVectorInt32(tuple_header, value, buffer, buffer_size);
			break;

		case 'L':
			return ConvertVectorUint32(tuple_header, value, buffer, buffer_size);
			break;

		case 'j':
			return ConvertVectorInt64(tuple_header, value, buffer, buffer_size);
			break;

		case 'J':
			return ConvertVectorUint64(tuple_header, value, buffer, buffer_size);
			break;

		case 'f':
			return ConvertVectorFloat32(tuple_header, value, buffer, buffer_size);
			break;

		case 'd':
			return ConvertVectorFloat64(tuple_header, value, buffer, buffer_size);
			break;

		case 'F':
			return ConvertVectorFOURCC(tuple_header, value, buffer, buffer_size);
			break;

		case 'G':
			return ConvertVectorUUID(tuple_header, value, buffer, buffer_size);
			break;

		case 'U':
			return ConvertLabelValue(tuple_header, value, buffer, buffer_size);
			break;

		case 'E':
			// The value comprises the metadata tuples nested in the class instance
			//assert(0);
			break;

		default:
			fprintf(stderr, "Unknown tuple type: %c (0x%02X)\n", tuple_header->type, tuple_header->type);
			assert(0);
			break;
	}

	return CODEC_ERROR_UNKNOWN_TYPE;
}


/*!
	@brief Convert the base64 representation of a pixel format to an RGBALayout (SMPTE ST 377-1, section G.2.40)
*/
CODEC_ERROR ConvertValueToLayout(void *buffer, size_t length, const TUPLE_HEADER *tuple_header, const char *value)
{
	// Caller should have provided a buffer sized for an RGBALayout in binary
	assert(length == tuple_header->size && tuple_header->size == 16);

#if _RGBA_LAYOUT_STRING

	int index = 0;
	char *value_ptr = (char *)value;

	// Convert the string representation of an RGBALayout to binary
	while (index < length && *value_ptr != '\0')
	{
		// Read the code for the color component
		char code = *(value_ptr++);
		buffer[index++] = code;

		// Handle error when the buffer size is not an even number
		if (index >= length) break;

		// Read the component size (in bits)
		int depth = 0;
		char digit = *(value_ptr++);
		while (isdigit(digit))
		{
			depth = 10 * depth + digit - '0';
			digit = *(value_ptr++);
		}
		buffer[index++] = depth;
	}

	// Write zeros into the rest of the buffer
	while (index < length) {
		buffer[index++] = 0;
	}

#else

	// Convert the base64 representation of an RGBALayout to binary
	size_t actual = 0;
	decode_base64(value, strlen(value), buffer, length, &actual);

	//NOTE: The number of bytes written to the output buffer by the base64-decode routine is discarded

#endif

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Convert the base64 representation of an ICC Profile to binary
*/
CODEC_ERROR ConvertValueToICCProfile(void *buffer, size_t length, const TUPLE_HEADER *tuple_header, const char *value)
{
	// Convert the base64 representation of an RGBALayout to binary
	size_t actual = 0;
	decode_base64(value, strlen(value), buffer, length, &actual);

	//NOTE: The number of bytes written to the output buffer by the base64-decode routine is discarded

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Write a metadata chunk or tuple header to the binary output file
*/
CODEC_ERROR WriteMetadataHeader(FILE *output_file, TUPLE_HEADER *tuple_header)
{
	// fprintf(stderr, "WriteMetadataHeader tag: %c%c%c%c, type: %c, size: %d, count: %d\n",
	// 	FOURCC_CHARS(tuple_header->tag), PrintableType(tuple_header->type), tuple_header->size, tuple_header->count);

	// Size of the metadata tuple value excluding padding
	size_t total_size = ((tuple_header->count > 0) ? tuple_header->count : 1) * tuple_header->size;

	if (HasRepeatCount(tuple_header->type) && total_size > UINT8_MAX)
	{
		// Refactor the size and count to represent the large metadata value

		if (tuple_header->count == 0) tuple_header->count = 1;

		//assert(tuple_header->size % 2 == 0);

		while (tuple_header->size > UINT8_MAX)
		{
			// Force the size to be a multiple of two
			if ((tuple_header->size % 2) != 0) tuple_header->size++;

			tuple_header->size /= 2;
			tuple_header->count *= 2;
		}
	}

#if 0
	if (tuple_header->tag == FOURCC_VALUE("GAMA") ||
		tuple_header->tag == FOURCC_VALUE("GAMp") ||
		tuple_header->tag == FOURCC_VALUE("LAYR") ||
		tuple_header->tag == FOURCC_VALUE("LAYN") ||
		tuple_header->tag == FOURCC_VALUE("LAYD"))
	{
		printf("WriteMetadataHeader tag: %c%c%c%c, type: %c, size: %d, count: %d\n",
			FOURCC_CHARS(tuple_header->tag), tuple_header->type, tuple_header->size, tuple_header->count);
	}
#endif

	// Output the four byte metadata tuple tag (four character code)
	fwrite(&tuple_header->tag, sizeof(tuple_header->tag), 1, output_file);

	// Output the single byte metadata tuple type
	fwrite(&tuple_header->type, sizeof(tuple_header->type), 1, output_file);

	if (HasRepeatCount(tuple_header->type))
	{
		int result;

		 assert(tuple_header->size <= UINT8_MAX);

		// fprintf(stderr, "Has repeat count tag: %c%c%c%c, type: %c, size: %d, count: %d\n",
		// 	FOURCC_CHARS(tuple_header->tag), tuple_header->type, tuple_header->size, tuple_header->count);

		// Output the one byte size and two byte repeat count
		result = fwrite(&tuple_header->size, 1, 1, output_file);
		assert(result == 1);

		uint16_t count = Swap16(tuple_header->count);
		result = fwrite(&count, 2, 1, output_file);
		assert(result == 1);

		// fprintf(stderr, "Wrote size: %d, repeat count: %d\n", tuple_header->size, tuple_header->count);
	}
	else
	{
		int result;

		// The metadata tuple cannot have a repeat count greater than one
		assert(! (tuple_header->count > 1));

		// fprintf(stderr, "No repeat count tag: %c%c%c%c, type: %c, size: %d, count: %d\n",
		// 	FOURCC_CHARS(tuple_header->tag), tuple_header->type, tuple_header->size, tuple_header->count);

		// Output the three byte size (no repeat count)
		uint32_t size = Swap32(tuple_header->size);
		result = fwrite(&((uint8_t *)&size)[1], 3, 1, output_file);
		assert(result == 1);

		// fprintf(stderr, "Wrote size: %d (no repeat count)\n", tuple_header->size);
	}

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Write the metadata value to the binary output file

	For the purposes of converting the value to binary, the count is assumed to be one if zero is passed as the argument.
*/
CODEC_ERROR WriteMetadataValue(FILE *output_file, const TUPLE_HEADER *tuple_header, const void *value_data, const size_t value_size)
{
	//fprintf(stderr, "value_size: %zu\n", value_size);

	if (value_size > 0)
	{
		//uint8_t buffer[value_size];
		ALLOC_BUFFER(buffer, value_size);

		memset(buffer, 0, sizeof(buffer));

		// Some metadata items must be handled as special cases
		if (tuple_header->tag == FOURCC_VALUE("PFMT"))
		{
			// Convert the base64 representation of a pixel format to an RGBALayout (SMPTE ST 377-1, section G.2.40)
			ConvertValueToLayout(buffer, sizeof(buffer), tuple_header, value_data);
		}
		else if (tuple_header->tag == FOURCC_VALUE("ICCP"))
		{
			// Convert the base64 representation of an ICC Profile to binary
			ConvertValueToICCProfile(buffer, sizeof(buffer), tuple_header, value_data);
		}
		else if (tuple_header->tag == FOURCC_VALUE("CVTD"))
		{
			// Decode the base64 encoded string to a block of bytes
			size_t actual_length;
			size_t string_length = strlen(value_data);
			//fprintf(stderr, "value_size: %zd, string_length: %zd\n", value_size, string_length);
			decode_base64(value_data, string_length, buffer, sizeof(buffer), &actual_length);
		}
		else if (tuple_header->tag == FOURCC_VALUE("VEND"))
		{
			// Decode the base64 encoded string to a block of bytes
			size_t actual_length;
			size_t string_length = strlen(value_data);
			//fprintf(stderr, "value_size: %zd, string_length: %zd\n", value_size, string_length);
			decode_base64(value_data, string_length, buffer, sizeof(buffer), &actual_length);
		}
		else
		{
			// Copy strings and convert numbers from strings to binary
			ConvertValueToBinary(buffer, sizeof(buffer), tuple_header, value_data);
		}

		int result = fwrite(buffer, sizeof(buffer), 1, output_file);
		assert(result == 1);
		if (! (result == 1)) {
			return CODEC_ERROR_FILE_WRITE;
		}
	}

	// Okay if the value is empty

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Write the metadata tuple padding
*/
CODEC_ERROR WriteMetadataPadding(FILE *output_file, size_t value_size)
{
	//Compute the size of the metadata tuple padding
	size_t padding_size = sizeof(SEGMENT) - (value_size % sizeof(SEGMENT));

	//printf("WriteMetadataPadding size: %d, padding: %d\n", value_size, padding_size);

	if (padding_size > 0 && padding_size < sizeof(SEGMENT))
	{
		// fprintf(stderr, "Value size: %d, padding: %d\n", value_size, padding_size);
		//printf("Value size: %d, padding: %d\n", value_size, padding_size);

		// Write the padding bytes
		//uint8_t padding[padding_size];
		ALLOC_BUFFER(padding, padding_size);
		memset(padding, 0, sizeof(padding));
		int result = fwrite(padding, padding_size, 1, output_file);
		if (result != 1) {
			return CODEC_ERROR_FILE_WRITE;
		}
	}

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Write the metadata chunk header

	The bitstream value is the chunk payload size in segments. Large chunk elements have a
	three-byte size which is why the size (tuple value) is passed as a 32-bit unsigned integer.
*/
CODEC_ERROR WriteChunkHeader(FILE *output_file, uint32_t tag, uint32_t value)
{
	int result;

	uint16_t upper_half_segment;
	uint16_t lower_half_segment;

	// The chunk element must be a large or small metadata chunk
	assert(tag == 0x61 || tag == 0x4010);

	// Is this a large or small metadata chunk?
	if (tag == 0x61)
	{
		// The tag occupies the first byte in the segment
		assert(tag <= UINT8_MAX);

		// The last three bytes in the sgement comprise the size of the metadata chunk payload
		assert((value & 0xFFFFFF) == value);

		upper_half_segment = (tag << 8) | (value >> 16);
		lower_half_segment = value & 0xFFFF;
	}
	else
	{
		// The tag an dvalue are both 16-bit unsigned integers
		assert(tag <= UINT16_MAX);
		assert(value <= UINT16_MAX);

		upper_half_segment = tag;
		lower_half_segment = value;
	}

	// The metadata chunk is an optional bitstream element
	upper_half_segment = NEG(upper_half_segment);

	// The tag-value pair is a single segment in network order
	SEGMENT segment = (upper_half_segment << 16) | lower_half_segment;
	segment = Swap32(segment);

	result = fwrite(&segment, sizeof(segment), 1, output_file);
	if (result != 1) {
		return CODEC_ERROR_FILE_WRITE;
	}

	//fprintf(stderr, "Wrote chunk header\n");

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Print the tuple header to the terminal (for debugging)
*/
CODEC_ERROR PrintTupleHeader(TUPLE_HEADER *tuple_header, char *label)
{
	if (label != NULL) {
		label = "Tuple";
	}

	printf("%s tag: %c%c%c%c, type: %c, size: %d, count: %d\n",
		label, FOURCC_CHARS(tuple_header->tag), tuple_header->type, tuple_header->size, tuple_header->count);

	return CODEC_ERROR_OKAY;
}
