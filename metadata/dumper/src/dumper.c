/*!	@file dumper.c

	@brief Top-level routines for the XML dumper.
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

#if _DECODER && !_TESTING

// Use relative paths to avoid conflicts with the decoder include paths
#include "../../common/include/config.h"
#include "../../common/include/error.h"
#include "../../common/include/swap.h"
#include "../../common/include/base64.h"
#include "../../common/include/buffer.h"
#include "../../common/include/metadata.h"

#include "mxml.h"

#include "../include/params.h"
#include "../include/nodetree.h"
#include "../include/database.h"
#include "../include/dumper.h"

#else

#include "config.h"
#include "swap.h"
#include "error.h"
#include "params.h"
#include "base64.h"
#include "buffer.h"

#include "mxml.h"

#include "metadata.h"

#if _DATABASE
#include "database.h"
#endif

#include "nodetree.h"
#include "dumper.h"

#endif


// #ifdef __clang__
// //! Maximum depth of the XML node stack
// static const int MAXIMUM_XML_NODE_STACK_DEPTH = 12;
// #else
// //! Maximum depth of the XML node stack (defined as a macro for compilers other than Clang)
// #define MAXIMUM_XML_NODE_STACK_DEPTH 12
// #endif


//! Macro for computing the size of a struct member
//#define FIELD_SIZE(type, field) sizeof(((type *)0)->field)


//! Macro for computing the length of a struct member array
//#define FIELD_LENGTH(type, field) (sizeof(((type *)0)->field)/sizeof(((type *)0)->field[0]))



/*!
	@brief Convert the string representing a floating-point number into canonical form

	Remove trailing zeros from the string.
*/
void NormalizeFloatString(char *string)
{
	char *p = NULL;
	char *q = NULL;
	char z = 0;

	// Find the decimal point
	for (p = string; *p != '\0'; p++)
	{
		if (*p == '.') break;
	}

	// Found the decimal point?
	if (*p != '.') return;

	// Skip a single zero after the decimal point
	++p;
	if (*p != '\0' && *p == '0') ++p;

	// Find the beginning of the trailing zeros
	for (; *p != '\0'; p++)
	{
		// Found the beginning of a run of zero digit characters?
		if (*p == '0' && z != '0')
		{
			// Mark the beginning of a run of zeros
			q = p;
			z = *p;
		}
		else if (*p != '0')
		{
			// Terminate the run of zeros
			z = *p;
		}
	}

	// Did the number end in a run of zeros?
	if (z == '0' && q != NULL)
	{
		// Remove the training zeros from the string
		*q = '\0';
	}
}


/*!
	@brief Return the maximum number of characters required to represent a number including the sign if applicable
*/
size_t MaxDigitsWithSign(TUPLE_TYPE type)
{
	// Table that maps a numerical tuple type to the maximum number and flag for signed versus unsigned
	static const struct _number_limit_entry
	{
		TUPLE_TYPE type;
		size_t maximum;
		bool signed_flag;
	}
	number_limit_table[] =
	{
		{ 'b',   INT8_MAX,  true },
		{ 'B',  UINT8_MAX, false },
		{ 's',  INT16_MAX,  true },
		{ 'S', UINT16_MAX, false },
		{ 'l',  INT32_MAX,  true },
		{ 'L', UINT32_MAX, false },
		{ 'j',  INT64_MAX,  true },
		{ 'J', UINT64_MAX, false },
	};

	static const int number_limit_table_length = sizeof(number_limit_table) / sizeof(number_limit_table[0]);

	size_t max_digits_with_sign = 0;
	size_t max_number_limit = 0;
	bool number_signed_flag = false;

	for (int i = 0; number_limit_table_length; i++)
	{
		if (number_limit_table[i].type == type) {
			max_number_limit = number_limit_table[i].maximum;
			number_signed_flag = number_limit_table[i].signed_flag;
			break;
		}

	}

	while (max_number_limit > 0)
	{
		max_number_limit /= 10;
		max_digits_with_sign++;
	}

	if (max_digits_with_sign == 0) {
		// Return zero if the maxumum size of the number could not be determined
		return 0;
	}

	if (number_signed_flag) {
		max_digits_with_sign++;
	}

	return max_digits_with_sign;
}


/*!
	@brief Return the maximum number of characters required to represent a floating-point number
*/
size_t MaxCharactersFloat(TUPLE_TYPE type)
{
	switch (type)
	{
		case 'f':
			return (3 + FLT_MANT_DIG - FLT_MIN_EXP);
			break;

		case 'd':
			return (3 + DBL_MANT_DIG - DBL_MIN_EXP);
			break;

		default:
			assert(0);
			break;
	}

	return 0;
}


/*!
	@brief Dump a buffer of characters
*/
CODEC_ERROR DumpTupleString(const void *payload_buffer, size_t payload_size, mxml_node_t *node)
{
	// Allocate a buffer large enough for the payload characters plus the terminating nul character
	ALLOC_STRING(string, payload_size + 1);
	memset(string, 0, sizeof(string));

	//snprintf(string, sizeof(string), "%*s", (int)actual_size, (const char *)payload);
	memcpy(string, payload_buffer, payload_size);
	string[payload_size] = '\0';

	mxmlElementSetAttr(node, "value", string);

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Dump a vector of signed 8-bit integers

	Scalers are handled as the special case of a vector with only one element.
*/
CODEC_ERROR DumpVectorInt8(const void *payload_buffer, size_t payload_size, const TUPLE_HEADER *tuple_header, mxml_node_t *node)
{
	assert(tuple_header->type == 'b');

	// Maximum number of digits in the string representation of a number
	const size_t max_number_length = 4;

	size_t actual_count = (tuple_header->count > 0) ? tuple_header->count : 1;
	size_t total_size = actual_count * tuple_header->size;
	size_t vector_length = total_size / ElementSize(tuple_header->type);

	ALLOC_STRING(string, max_number_length * vector_length + 1);
	memset(string, 0, sizeof(string));

	const int8_t *array = payload_buffer;

	char *string_ptr = string;
	size_t unused_size = sizeof(string);

	for (int i = 0; i < vector_length; i++)
	{
		if (unused_size == 0) break;

		const char *separator = ((i > 0) ? " " : "");
		snprintf(string_ptr, unused_size, "%s%d", separator, array[i]);
		unused_size -= strlen(string_ptr);
		string_ptr += strlen(string_ptr);
		assert(*string_ptr == '\0');
	}

	mxmlElementSetAttr(node, "value", string);

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Dump a vector of unsigned 8-bit integers

	Scalers are handled as the special case of a vector with only one element.
*/
CODEC_ERROR DumpVectorUint8(const void *payload_buffer, size_t payload_size, const TUPLE_HEADER *tuple_header, mxml_node_t *node)
{
	assert(tuple_header->type == 'B');

	// Maximum number of digits in the string representation of a number
	const size_t max_number_length = 4;

	size_t actual_count = (tuple_header->count > 0) ? tuple_header->count : 1;
	size_t total_size = actual_count * tuple_header->size;
	size_t vector_length = total_size / ElementSize(tuple_header->type);

	ALLOC_STRING(string, max_number_length * vector_length + 1);
	memset(string, 0, sizeof(string));

	const uint8_t *array = payload_buffer;

	char *string_ptr = string;
	size_t unused_size = sizeof(string);

	for (int i = 0; i < vector_length; i++)
	{
		if (unused_size == 0) break;

		const char *separator = ((i > 0) ? " " : "");
		snprintf(string_ptr, unused_size, "%s%u", separator, array[i]);
		unused_size -= strlen(string_ptr);
		string_ptr += strlen(string_ptr);
		assert(*string_ptr == '\0');
	}

	mxmlElementSetAttr(node, "value", string);

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Dump a vector of signed 16-bit integers

	Scalers are handled as the special case of a vector with only one element.
*/
CODEC_ERROR DumpVectorInt16(const void *payload_buffer, size_t payload_size, const TUPLE_HEADER *tuple_header, mxml_node_t *node)
{
	assert(tuple_header->type == 's');

	// Maximum number of digits in the string representation of a number including the sign
	const size_t max_number_length = MaxDigitsWithSign(tuple_header->type);

	size_t actual_count = (tuple_header->count > 0) ? tuple_header->count : 1;
	size_t total_size = actual_count * tuple_header->size;
	size_t vector_length = total_size / ElementSize(tuple_header->type);

	// Allocate a string for a sequence of numbers separated by whitesapce and terminated by a nul character
	ALLOC_STRING(string, max_number_length * vector_length + vector_length);
	memset(string, 0, sizeof(string));

	const int16_t *array = payload_buffer;

	char *string_ptr = string;
	size_t unused_size = sizeof(string);

	for (int i = 0; i < vector_length; i++)
	{
		if (unused_size == 0) break;

		const char *separator = ((i > 0) ? " " : "");
		snprintf(string_ptr, unused_size, "%s%d", separator, SWAPPED_ELEMENT(int16_t, Swap16, array, i));
		unused_size -= strlen(string_ptr);
		string_ptr += strlen(string_ptr);
		assert(*string_ptr == '\0');
	}

	mxmlElementSetAttr(node, "value", string);

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Dump a vector of unsigned 16-bit integers

	Scalers are handled as the special case of a vector with only one element.
*/
CODEC_ERROR DumpVectorUint16(const void *payload_buffer, size_t payload_size, const TUPLE_HEADER *tuple_header, mxml_node_t *node)
{
	assert(tuple_header->type == 'S');

	// Maximum number of digits in the string representation of a number including the sign
	const size_t max_number_length = MaxDigitsWithSign(tuple_header->type);

	size_t actual_count = (tuple_header->count > 0) ? tuple_header->count : 1;
	size_t total_size = actual_count * tuple_header->size;
	size_t vector_length = total_size / ElementSize(tuple_header->type);

	// Allocate a string for a sequence of numbers separated by whitesapce and terminated by a nul character
	ALLOC_STRING(string, max_number_length * vector_length + vector_length);
	memset(string, 0, sizeof(string));

	const uint16_t *array = payload_buffer;

	char *string_ptr = string;
	size_t unused_size = sizeof(string);

	for (int i = 0; i < vector_length; i++)
	{
		if (unused_size == 0) break;

		const char *separator = ((i > 0) ? " " : "");
		snprintf(string_ptr, unused_size, "%s%u", separator, SWAPPED_ELEMENT(uint16_t, Swap16, array, i));
		unused_size -= strlen(string_ptr);
		string_ptr += strlen(string_ptr);
		assert(*string_ptr == '\0');
	}

	mxmlElementSetAttr(node, "value", string);

	return CODEC_ERROR_OKAY;
}



/*!
	@brief Dump a vector of signed 32-bit integers

	Scalers are handled as the special case of a vector with only one element.
*/
CODEC_ERROR DumpVectorInt32(const void *payload_buffer, size_t payload_size, const TUPLE_HEADER *tuple_header, mxml_node_t *node)
{
	assert(tuple_header->type == 'l');

	// Maximum number of digits in the string representation of a number including the sign
	const size_t max_number_length = MaxDigitsWithSign(tuple_header->type);

	size_t actual_count = (tuple_header->count > 0) ? tuple_header->count : 1;
	size_t total_size = actual_count * tuple_header->size;
	size_t vector_length = total_size / ElementSize(tuple_header->type);

	// Allocate a string for a sequence of numbers separated by whitesapce and terminated by a nul character
	ALLOC_STRING(string, max_number_length * vector_length + vector_length);
	memset(string, 0, sizeof(string));

	const int32_t *array = payload_buffer;

	char *string_ptr = string;
	size_t unused_size = sizeof(string);

	for (int i = 0; i < vector_length; i++)
	{
		if (unused_size == 0) break;

		const char *separator = ((i > 0) ? " " : "");
		snprintf(string_ptr, unused_size, "%s%d", separator, SWAPPED_ELEMENT(int32_t, Swap32, array, i));
		unused_size -= strlen(string_ptr);
		string_ptr += strlen(string_ptr);
		assert(*string_ptr == '\0');
	}

	mxmlElementSetAttr(node, "value", string);

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Dump a vector of unsigned 32-bit integers

	Scalers are handled as the special case of a vector with only one element.
*/
CODEC_ERROR DumpVectorUint32(const void *payload_buffer, size_t payload_size, const TUPLE_HEADER *tuple_header, mxml_node_t *node)
{
	assert(tuple_header->type == 'L');

	// Maximum number of digits in the string representation of a number including the sign
	const size_t max_number_length = MaxDigitsWithSign(tuple_header->type);

	size_t actual_count = (tuple_header->count > 0) ? tuple_header->count : 1;
	size_t total_size = actual_count * tuple_header->size;
	size_t vector_length = total_size / ElementSize(tuple_header->type);

	// Allocate a string for a sequence of numbers separated by whitesapce and terminated by a nul character
	ALLOC_STRING(string, max_number_length * vector_length + vector_length);
	memset(string, 0, sizeof(string));

	const uint32_t *array = payload_buffer;

	char *string_ptr = string;
	size_t unused_size = sizeof(string);

	for (int i = 0; i < vector_length; i++)
	{
		if (unused_size == 0) break;

		const char *separator = ((i > 0) ? " " : "");
		snprintf(string_ptr, unused_size, "%s%u", separator, SWAPPED_ELEMENT(uint32_t, Swap32, array, i));
		unused_size -= strlen(string_ptr);
		string_ptr += strlen(string_ptr);
		assert(*string_ptr == '\0');
	}

	mxmlElementSetAttr(node, "value", string);

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Dump a vector of signed 64-bit integers

	Scalers are handled as the special case of a vector with only one element.
*/
CODEC_ERROR DumpVectorInt64(const void *payload_buffer, size_t payload_size, const TUPLE_HEADER *tuple_header, mxml_node_t *node)
{
	assert(tuple_header->type == 'j');

	// Maximum number of digits in the string representation of a number including the sign
	const size_t max_number_length = MaxDigitsWithSign(tuple_header->type);

	size_t actual_count = (tuple_header->count > 0) ? tuple_header->count : 1;
	size_t total_size = actual_count * tuple_header->size;
	size_t vector_length = total_size / ElementSize(tuple_header->type);

	// Allocate a string for a sequence of numbers separated by whitesapce and terminated by a nul character
	ALLOC_STRING(string, max_number_length * vector_length + vector_length);
	memset(string, 0, sizeof(string));

	const int64_t *array = payload_buffer;

	char *string_ptr = string;
	size_t unused_size = sizeof(string);

	for (int i = 0; i < vector_length; i++)
	{
		if (unused_size == 0) break;

		//NOTE: The long long integer data types are formatted as long on some platforms
		const char *format = (sizeof(int64_t) == 8) ? "%s%lld" : "%s%ld";
		const char *separator = ((i > 0) ? " " : "");
		snprintf(string_ptr, unused_size, format, separator, SWAPPED_ELEMENT(int64_t, Swap64, array, i));
		unused_size -= strlen(string_ptr);
		string_ptr += strlen(string_ptr);
		assert(*string_ptr == '\0');
	}

	mxmlElementSetAttr(node, "value", string);

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Dump a vector of unsigned 64-bit integers

	Scalers are handled as the special case of a vector with only one element.
*/
CODEC_ERROR DumpVectorUint64(const void *payload_buffer, size_t payload_size, const TUPLE_HEADER *tuple_header, mxml_node_t *node)
{
	assert(tuple_header->type == 'J');

	// Maximum number of digits in the string representation of a number including the sign
	const size_t max_number_length = MaxDigitsWithSign(tuple_header->type);

	size_t actual_count = (tuple_header->count > 0) ? tuple_header->count : 1;
	size_t total_size = actual_count * tuple_header->size;
	size_t vector_length = total_size / ElementSize(tuple_header->type);

	// Allocate a string for a sequence of numbers separated by whitesapce and terminated by a nul character
	ALLOC_STRING(string, max_number_length * vector_length + vector_length);
	memset(string, 0, sizeof(string));

	const uint64_t *array = payload_buffer;

	char *string_ptr = string;
	size_t unused_size = sizeof(string);

	for (int i = 0; i < vector_length; i++)
	{
		if (unused_size == 0) break;

		//NOTE: The long long integer data types are formatted as long on some platforms
		const char *format = (sizeof(uint64_t) == 8) ? "%s%llu" : "%s%ld";
		const char *separator = ((i > 0) ? " " : "");
		snprintf(string_ptr, unused_size, format, separator, SWAPPED_ELEMENT(uint64_t, Swap64, array, i));
		unused_size -= strlen(string_ptr);
		string_ptr += strlen(string_ptr);
		assert(*string_ptr == '\0');
	}

	mxmlElementSetAttr(node, "value", string);

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Dump a vector of single-precision floating-point numbers

	Scalers are handled as the special case of a vector with only one element.
*/
CODEC_ERROR DumpVectorFloat32(const void *payload_buffer, size_t payload_size, const TUPLE_HEADER *tuple_header, mxml_node_t *node)
{
	assert(tuple_header->type == 'f');

	// Maximum number of digits in the string representation of a number including the sign
	const size_t max_number_length = MaxCharactersFloat(tuple_header->type);

	size_t actual_count = (tuple_header->count > 0) ? tuple_header->count : 1;
	size_t total_size = actual_count * tuple_header->size;
	size_t vector_length = total_size / ElementSize(tuple_header->type);

	// Allocate a string for a sequence of numbers separated by whitesapce and terminated by a nul character
	ALLOC_STRING(string, max_number_length * vector_length + vector_length);
	memset(string, 0, sizeof(string));

	const float *array = payload_buffer;

	char *string_ptr = string;
	size_t unused_size = sizeof(string);

	for (int i = 0; i < vector_length; i++)
	{
		if (unused_size == 0) break;

		const char *separator = ((i > 0) ? " " : "");
		snprintf(string_ptr, unused_size, "%s%f", separator, SWAPPED_ELEMENT(float, SwapFloat32, array, i));

		// Normmalize the string representation of the floating-point number
		NormalizeFloatString(string_ptr);

		unused_size -= strlen(string_ptr);
		string_ptr += strlen(string_ptr);
		assert(*string_ptr == '\0');
	}

	// Add the string of floating-point numbers to the XML node
	mxmlElementSetAttr(node, "value", string);

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Dump a vector of double-precision floating-point numbers

	Scalers are handled as the special case of a vector with only one element.
*/
CODEC_ERROR DumpVectorFloat64(const void *payload_buffer, size_t payload_size, const TUPLE_HEADER *tuple_header, mxml_node_t *node)
{
	assert(tuple_header->type == 'd');

	// Maximum number of digits in the string representation of a number including the sign
	const size_t max_number_length = MaxCharactersFloat(tuple_header->type);

	size_t actual_count = (tuple_header->count > 0) ? tuple_header->count : 1;
	size_t total_size = actual_count * tuple_header->size;
	size_t vector_length = total_size / ElementSize(tuple_header->type);

	// Allocate a string for a sequence of numbers separated by whitesapce and terminated by a nul character
	ALLOC_STRING(string, max_number_length * vector_length + vector_length);
	memset(string, 0, sizeof(string));

	const double *array = payload_buffer;

	char *string_ptr = string;
	size_t unused_size = sizeof(string);

	for (int i = 0; i < vector_length; i++)
	{
		if (unused_size == 0) break;

		const char *separator = ((i > 0) ? " " : "");
		snprintf(string_ptr, unused_size, "%s%f", separator, SWAPPED_ELEMENT(double, SwapFloat64, array, i));

		// Normmalize the string representation of the floating-point number
		NormalizeFloatString(string_ptr);

		unused_size -= strlen(string_ptr);
		string_ptr += strlen(string_ptr);
		assert(*string_ptr == '\0');
	}

	// Add the string of floating-point numbers to the XML node
	mxmlElementSetAttr(node, "value", string);

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Dump a buffer of bytes as a sequence of four character codes (FOURCC)
*/
CODEC_ERROR DumpVectorFOURCC(const void *payload_buffer, size_t payload_size, const TUPLE_HEADER *tuple_header, mxml_node_t *node)
{
	// Allocate a string large enough for a sequence of FOURCC separated by whitespace and terminated by a nul character
	size_t payload_count = payload_size / sizeof(FOURCC);
	ALLOC_STRING(string, payload_size + payload_count);
	memset(string, 0, sizeof(string));

	const uint8_t *byte_array = payload_buffer;
	char *string_ptr = string;

	for (int i = 0; i < payload_count; i++)
	{
		if (i > 0) *(string_ptr++) = ' ';

		int j = i * tuple_header->size;
		sprintf(string_ptr, "%c%c%c%c", byte_array[j+0], byte_array[j+1], byte_array[j+2], byte_array[j+3]);
		string_ptr += tuple_header->size;
	}

	*string_ptr = '\0';

	mxmlElementSetAttr(node, "value", string);

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Dump a buffer of bytes as a sequence of universally unique identifiers (UUID)
*/
CODEC_ERROR DumpVectorUUID(const void *payload_buffer, size_t payload_size, const TUPLE_HEADER *tuple_header, mxml_node_t *node)
{
	// Allocate a string large enough for a sequence of UUID separated by whitespace and terminated by a nul character
	size_t payload_count = tuple_header->count;
	ALLOC_STRING(string, 2 * payload_size + payload_count);
	memset(string, 0, sizeof(string));

	const uint8_t *byte_array = payload_buffer;
	char *string_ptr = string;

	for (int i = 0; i < payload_count; i++)
	{
		if (i > 0) *(string_ptr++) = ' ';

		int j = i * tuple_header->size;
		for (int k = 0; k < tuple_header->size; k++)
		{
			sprintf(string_ptr, "%02x", byte_array[j + k]);
			string_ptr += 2;
		}
	}

	*string_ptr = '\0';

	//fprintf(stderr, "%s\n", string);

	mxmlElementSetAttr(node, "value", string);

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Dump a buffer of bytes as a SMPTE Universal Label (UL)

	@todo Consider adding the SMPTE label prefix to the output string.

	@todo Need to handle more than one Universal Label since data type 'U' has a repeat count.
*/
CODEC_ERROR DumpTupleLabel(const void *payload_buffer, size_t payload_size, mxml_node_t *node)
{
	// Allocate a string large enough for two characters per byte plus the terminating nul character
	ALLOC_STRING(string, 2 * payload_size + 1);
	memset(string, 0, sizeof(string));

	const uint8_t *byte_array = payload_buffer;
	char *string_ptr = string;

	for (int i = 0; i < payload_size; i++)
	{
		sprintf(string_ptr, "%02X", byte_array[i]);
		string_ptr += 2;
	}

	*string_ptr = '\0';

	mxmlElementSetAttr(node, "value", string);

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Write the tuple value to the output file

	@todo Replace switch statements with table-driven code.

	@todo Replace the constant @var max_number_length with a length that depends on the tuple type.
*/
CODEC_ERROR DumpTupleValue(const void *payload,
						   const size_t payload_size,
						   const TUPLE_HEADER *tuple_header,
						   const int level,
						   mxml_node_t *node,
						   const ARGUMENTS *args)
{
   const FOURCC tuple_tag = tuple_header->tag;
   const char type = tuple_header->type;
   const TUPLE_SIZE size = tuple_header->size;
   const TUPLE_COUNT count = tuple_header->count;

	size_t actual_count = (count > 0) ? count : 1;
	size_t actual_size = actual_count * size;

	// printf("%c%c%c%c %c %u %u %zu\n", FOURCC_CHARS(tuple_tag), type, size, count, payload_size);

	// fprintf(stderr, "DumpTupleValue tag: %c%c%c%c, size: %zd, count: %zd, actual_size: %zd, payload_size: %zd\n",
	// 	FOURCC_CHARS(tuple_tag), size, count, actual_size, payload_size);

	// Maximum number of digits in the string representation of a number (long long integer or floating-point without exponent)
	const size_t max_number_length = 48;

	// Allocate a buffer large enough for the payload characters plus the terminating nul character
	ALLOC_STRING(string, max_number_length * actual_size + 1);
	memset(string, 0, sizeof(string));

	//TODO: Modify this code to output the value of a extrinsic metadata as element text

#if __clang__

	// Is the value a buffer of bytes that are base64 encoded or a buffer of text?
	switch (tuple_tag)
	{

		case FOURCC_VALUE("PFMT"):
		case FOURCC_VALUE("ICCP"):
			encode_base64(payload, payload_size, string, sizeof(string));
			mxmlElementSetAttr(node, "value", string);
			return CODEC_ERROR_OKAY;

		case FOURCC_VALUE("DPXh"):
		case FOURCC_VALUE("MXFd"):
		case FOURCC_VALUE("ACEh"):
			encode_base64(payload, actual_size, string, sizeof(string));
			mxmlNewText(node, 0, string);
			return CODEC_ERROR_OKAY;

		case FOURCC_VALUE("CVTD"):
			encode_base64(payload, actual_size, string, sizeof(string));
			mxmlElementSetAttr(node, "value", string);
			return CODEC_ERROR_OKAY;

		case FOURCC_VALUE("XMPd"):
			memcpy(string, payload, actual_size);
			mxmlNewCDATA(node, string);
			return CODEC_ERROR_OKAY;

		case FOURCC_VALUE("ALEd"):
			mxmlNewText(node, 0, payload);
			return CODEC_ERROR_OKAY;

		case FOURCC_VALUE("VEND"):
			encode_base64(payload, actual_size, string, sizeof(string));
			mxmlElementSetAttr(node, "value", string);
			return CODEC_ERROR_OKAY;

		default:
			break;
	}

#else

	// Is the value a buffer of bytes that are base64 encoded or a buffer of text?
	if (tuple_tag == FOURCC_VALUE("PFMT") ||
		tuple_tag == FOURCC_VALUE("ICCP"))
	{
		encode_base64(payload, payload_size, string, sizeof(string));
		mxmlElementSetAttr(node, "value", string);
		return CODEC_ERROR_OKAY;
	}
	else
	if (tuple_tag == FOURCC_VALUE("DPXh") ||
		tuple_tag == FOURCC_VALUE("MXFd") ||
		tuple_tag == FOURCC_VALUE("ACEh"))
	{
		encode_base64(payload, actual_size, string, sizeof(string));
		mxmlNewText(node, 0, string);
		return CODEC_ERROR_OKAY;
	}
	else
	if (tuple_tag == FOURCC_VALUE("CVTD"))
	{
		encode_base64(payload, actual_size, string, sizeof(string));
		mxmlElementSetAttr(node, "value", string);
		return CODEC_ERROR_OKAY;
	}
	else
	if (tuple_tag == FOURCC_VALUE("XMPd"))
	{
		memcpy(string, payload, actual_size);
		mxmlNewCDATA(node, string);
		return CODEC_ERROR_OKAY;
	}
	else
	if (tuple_tag == FOURCC_VALUE("ALEd"))
	{
		mxmlNewText(node, 0, payload);
		return CODEC_ERROR_OKAY;
	}
	else
	if (tuple_tag == FOURCC_VALUE("VEND"))
	{
		encode_base64(payload, actual_size, string, sizeof(string));
		mxmlElementSetAttr(node, "value", string);
		return CODEC_ERROR_OKAY;
	}

#endif

	switch (type)
	{
		case 0:
		case 'P':
			// Nested tuples will be output as they are encountered in the file
			return CODEC_ERROR_OKAY;
			break;

		case 'c':
		case 'u':
		case 'x':
			return DumpTupleString(payload, actual_size, node);
			break;

		case 'b':
			return DumpVectorInt8(payload, payload_size, tuple_header, node);
			break;

		case 'B':
			return DumpVectorUint8(payload, payload_size, tuple_header, node);
			break;

		case 's':
			//printf("Tuple tag: %c%c%c%c\n", FOURCC_CHARS(tuple_header->tag));
			return DumpVectorInt16(payload, payload_size, tuple_header, node);
			break;

		case 'S':
			//printf("Tuple tag: %c%c%c%c\n", FOURCC_CHARS(tuple_header->tag));
			return DumpVectorUint16(payload, payload_size, tuple_header, node);
			break;

		case 'l':
			//printf("Tuple tag: %c%c%c%c\n", FOURCC_CHARS(tuple_header->tag));
			return DumpVectorInt32(payload, payload_size, tuple_header, node);
			break;

		case 'L':
			//printf("Tuple tag: %c%c%c%c\n", FOURCC_CHARS(tuple_header->tag));
			return DumpVectorUint32(payload, payload_size, tuple_header, node);
			break;

		case 'j':
			//printf("Tuple tag: %c%c%c%c\n", FOURCC_CHARS(tuple_header->tag));
			return DumpVectorInt64(payload, payload_size, tuple_header, node);
			break;

		case 'J':
			//printf("Tuple tag: %c%c%c%c\n", FOURCC_CHARS(tuple_header->tag));
			return DumpVectorUint64(payload, payload_size, tuple_header, node);
			break;

		case 'f':
			return DumpVectorFloat32(payload, payload_size, tuple_header, node);
			break;

		case 'd':
			return DumpVectorFloat64(payload, payload_size, tuple_header, node);
			break;

		case 'F':
			return DumpVectorFOURCC(payload, payload_size, tuple_header, node);
			break;

		case 'G':
			return DumpVectorUUID(payload, payload_size, tuple_header, node);
			break;

		case 'U':
			return DumpTupleLabel(payload, actual_size, node);
			break;

		default:
			LogError("Uknown tuple type; %c (%02X)\n", type, type);
			return CODEC_ERROR_BAD_TYPE;
			break;
	}

	return CODEC_ERROR_OKAY;
}


#if 0
/*!
	@brief Mini-XML whitespace callback

	Do not add whitespace before the XML header
*/
const char *
whitespace_cb(mxml_node_t *node, int where)
{
	static char string[1024];
	int level = -1;

	const char *element = mxmlGetElement(node);
	//fprintf(stderr, "Element: %s\n", element);

	// Add white space before and after elements
	if (where == MXML_WS_BEFORE_OPEN)
	{
		if (strcmp(element, "metadata") == 0)
		{
			level = 0;
		}
		if (strcmp(element, "chunk") == 0)
		{
			level = 1;
		}
		else if (strcmp(element, "tuple") == 0)
		{
			//const char *tag = mxmlElementGetAttr(node, "tag");
			const char *type = mxmlElementGetAttr(node, "type");

			if (type != NULL && strcmp(type, "E") == 0) {
				level = 2;
			}
			else {
				level = 3;
			}
		}

		if (level >= 0)
		{
			snprintf(string, sizeof(string), "\n%s", Indentation(level));
			return string;
		}
	}

	if (where == MXML_WS_AFTER_CLOSE)
	{
	      return "\n";
	}

	return NULL;
}
#endif


#if _DECODER

// Do not need the code for dumping an XML tree from a file

#else

//! Nesting level in the metadata tuple hierarchy
static int current_level = 0;


//! Dump node information if the debug flag is true
static bool dump_node_info_flag = false;


/*!
	@brief Dump a plain text representation of the binary metadata test case to the output file

	Unlike other routines in this program, this routine creates an XML tree directly from the
	metadata read from a binary file without creating an intermediate representation such as a
	database. This routine provides the most straightforward way to convert a binary representation
	of metadata created by the XML parser or sample encoder in to a textual representation in the
	same XML format as the test cases input to the XML parser or sample encoder.
*/
CODEC_ERROR DumpBinaryFile(FILE *input_file, FILE *output_file, ARGUMENTS *args)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;

	//int mxml_result = 0;
	int file_result = 1;

	// Initialize the Mini-XML writer
	mxml_node_t *xml = mxmlNewXML("1.0");

	// The XML version tuple is added automatically with UTF-8 encoding

	// Create the root metadata element
	mxml_node_t *metadata = mxmlNewElement(xml, "metadata");
	mxmlElementSetAttr(metadata, "xmlns", "https://www.vc5codec.org/xml/metadata");

	// Define nodes for the current chunk elemenet and metadata tuples
	mxml_node_t *chunk = NULL;
	mxml_node_t *class = NULL;

	// Initialize the current nesting level in the metadata tuple hierarchy
	current_level = 0;

	// Nesting level for metadata tuples that follow the current tuple
	int next_level = 0;

	// Read the chunk header
	SEGMENT chunk_header;
	file_result = fread(&chunk_header, sizeof(chunk_header), 1, input_file);
	if (file_result != 1) {
		return CODEC_ERROR_FILE_READ;
	}

	uint16_t chunk_tag = 0;
	uint32_t chunk_size = 0;

	if (! ParseChunkHeader(chunk_header, &chunk_tag, &chunk_size, false)) {
		return CODEC_ERROR_BAD_CHUNK;
	}

	if (args->debug_flag) {
		fprintf(stderr, "Chunk tag: 0x%0X, size: %d\n", chunk_tag, chunk_size);
	}
	else if (args->verbose_flag)
	{
		if (chunk_tag == METADATA_CHUNK_LARGE) {
			printf("%sChunk tag: 0x%02X, value: 0x%06X (%d)\n", Indentation(current_level), chunk_tag, chunk_size, chunk_size);
		} else {
			printf("%sChunk tag: 0x%04X, value: 0x%04X (%d)\n", Indentation(current_level), chunk_tag, chunk_size, chunk_size);
		}
	}

	chunk = mxmlNewElement(metadata, "chunk");
	mxmlElementSetAttrf(chunk, "tag", (chunk_tag == 0x61) ? "0x%2X" : "0x%04X", chunk_tag);
	mxmlElementSetAttrf(chunk, "size", "%d", chunk_size);

	// Push the node for the chunk element onto the node stack
	PushNode(chunk);

	// Process the chunk payload at the next level in the tuple hierarchy
	//current_level++;

	// Read metadata tuples from the input file until end of file
	while (file_result == 1)
	{
		FOURCC tuple_tag;
		file_result = fread(&tuple_tag, sizeof(tuple_tag), 1, input_file);
		if (file_result != 1) break;

		// Is this tuple another metadata chunk element?
		SEGMENT chunk_header = (SEGMENT)tuple_tag;

		if (ParseChunkHeader(chunk_header, &chunk_tag, &chunk_size, false))
		{
			// Process the chunk payload at the top level in the tuple hierarchy
			current_level = 0;

			if (args->debug_flag) {
				fprintf(stderr, "Chunk tag: 0x%0X, size: %d\n", chunk_tag, chunk_size);
			}
			else if (args->verbose_flag)
			{
				if (chunk_tag == METADATA_CHUNK_LARGE) {
					printf("%sChunk tag: 0x%02X, value: 0x%06X (%d)\n", Indentation(current_level), chunk_tag, chunk_size, chunk_size);
				} else {
					printf("%sChunk tag: 0x%04X, value: 0x%04X (%d)\n", Indentation(current_level), chunk_tag, chunk_size, chunk_size);
				}
			}

			chunk = mxmlNewElement(metadata, "chunk");
			mxmlElementSetAttrf(chunk, "tag", (chunk_tag == 0x61) ? "0x%2X" : "0x%04X", chunk_tag);
			mxmlElementSetAttrf(chunk, "size", "%d", chunk_size);

			// The chunk node should be the first node on the XML node stack
			ResetStack();

			// Push the node for the chunk element onto the node stack
			PushNode(chunk);

			// Process the chunk payload at the next level in the tuple hierarchy
			current_level++;

			continue;
		}

		// Read the next segment containing the data type and the tuple size and repeat count
		SEGMENT type_size_count;
		file_result = fread(&type_size_count, sizeof(type_size_count), 1, input_file);
		if (file_result != 1) break;

		// Swap the segment into network (big endian) order
		type_size_count = Swap32(type_size_count);

		// The first byte is the tuple data type
		char type = (type_size_count >> 24);
		size_t count;
		size_t size;

		if (HasRepeatCount(type))
		{
			// One byte size and two byte count and two byte repeast count
			size = (type_size_count >> 16) & 0xFF;
			count = (type_size_count & 0xFFFF);
		}
		else
		{
			// Three byte size with no repeat count
			size = type_size_count & 0xFFFFFF;
			count = 0;
		}

		// Update the nesting level based on the new tuple tag and the current nested tag and level
		UpdateNestingLevel(tuple_tag, type, &current_level, &next_level);

		if (args->verbose_flag)
		{
			printf("%sTuple tag: %c%c%c%c, ", Indentation(current_level), FOURCC_CHARS(tuple_tag));

			// if (type == 0) {
			// 	printf("type: (none), size: %zu, count: %zu\n", size, count);
			// }
			// else {
				printf("type: %c, size: %zu, count: %zu\n", type, size, count);
			// }
		}

		size_t total_size = ((count > 0) ? count : 1) * size;
		//fprintf(stderr, "%zd\n", total_size);

		//if (tuple_tag == TupleTag("CFHD"))
		if (IsClassInstance(tuple_tag, type))
		{
			class = mxmlNewElement(chunk, "tuple");
			mxmlElementSetAttrf(class, "tag", "%c%c%c%c", FOURCC_CHARS(tuple_tag));
			mxmlElementSetAttrf(class, "type", "%c", type);
			mxmlElementSetAttrf(class, "size", "%zu", size);

			// Round up the size to a segment boundary
			size_t segment_count = (size + 3)/4;
			size_t padding = 4 * segment_count - size;

			// Always output the padding even if it is zero
			mxmlElementSetAttrf(class, "padding", "%zu", padding);

			//TODO: Check that the current node at the top of the stack is a metadata chunk element

			// Push the node for the metadata class instance onto the node stack
			PushNode(class);
		}
		else
		{
			// if (debug_flag && (tuple_tag == FOURCC_VALUE("GYRO") || tuple_tag == FOURCC_VALUE("LAYD")))
			// {
			// 	fprintf(stderr, "Tuple: %c%c%c%c, size: %zd, count: %zd, total_size: %zd, padding: %zd, payload_size: %zd\n",
			// 		FOURCC_CHARS(tuple_tag), size, count, total_size, padding, payload_size);
			// }

			// Create the XML element for the metadata tuple
			//mxml_node_t *tuple = mxmlNewElement(class, "tuple");
			mxml_node_t *tuple = NewTupleNode(tuple_tag);
			assert(tuple != NULL);
			if (! (tuple != NULL)) {
				return CODEC_ERROR_BAD_TUPLE;
			}

			// The tuple tag attributes should have been set when the XML node was created
			assert(mxmlElementGetAttr(tuple, "tag") != NULL);

			// Add the tag attribute if it is not already set
			if (mxmlElementGetAttr(tuple, "tag") == NULL) {
				mxmlElementSetAttrf(tuple, "tag", "%c%c%c%c", FOURCC_CHARS(tuple_tag));
			}

			// Add the additional tuple attributes unless this is a nested tuple
			//if (type != 0)
			{
				// Always output the tuple type and size
				mxmlElementSetAttrf(tuple, "type", "%c", (type == 0) ? '0' : type);
				mxmlElementSetAttrf(tuple, "size", "%zu", size);

				// Output the count if the tuple has a repeat count
				if (HasRepeatCount(type)) {
					mxmlElementSetAttrf(tuple, "count", "%zu", count);
				}

				// Round up the size to a segment boundary
				// size_t segment_count = (size + 3)/4;
				// size_t padding = 4 * segment_count - size;
			}

			//printf("Tuple: %c%c%c%c, type: %c (%X)\n", FOURCC_CHARS(tuple_tag), ((type == 0) ? '0' : type), type);

			// Tuple must have a payload unless it is a nested tuple or an encoding curve
			assert(type == 0 || type == 'P' || total_size > 0);

			size_t segment_count = (total_size + 3) / 4;
			size_t payload_size = 4 * segment_count;
			size_t padding = payload_size - total_size;

			// Output the tuple value unless this is a nested tuple
			//if (total_size > 0)
			if (! IsNestedTuple(type))
			{
				// Read the payload (metadata value and padding)
				//size_t padding = sizeof(SEGMENT) - (total_size % sizeof(SEGMENT));
				//if (padding == sizeof(SEGMENT)) padding = 0;
				//size_t payload_size = total_size + padding;
				ALLOC_BUFFER(payload, payload_size);
				int file_result = fread(payload, sizeof(payload), 1, input_file);
				if (file_result != 1) {
					return CODEC_ERROR_FILE_READ;
				}

				//DumpTupleValue(payload, sizeof(payload), tuple_tag, type, size, count, current_level, tuple, args);
				TUPLE_HEADER tuple_header = {tuple_tag, type, size, count};
				DumpTupleValue(payload, sizeof(payload), &tuple_header, current_level, tuple, args);
			}

			// Always output the padding even if it is zero
			mxmlElementSetAttrf(tuple, "padding", "%zu", padding);

			if (args->debug_flag) {
				fprintf(stderr, "Tuple: %c%c%c%c, size: %zd, count: %zd, total_size: %zd, padding: %zd, payload_size: %zd\n",
					FOURCC_CHARS(tuple_tag), size, count, total_size, padding, payload_size);
			}
			//DumpNodeInfo(tuple, "New node");

		}

		// Update the nesting level for the next tuple in the bitstream
		current_level = next_level;
	}

	if (args->verbose_flag) fprintf(stderr, "\n");

	if (file_result != 1 && !feof(input_file)) {
		if (args->debug_flag) fprintf(stderr, "File read error: %d\n", file_result);
		return CODEC_ERROR_FILE_READ;
	}

	if (args->duplicates_flag) {
		// Prune duplicate entries from the database
		error = PruneDuplicateTuples(xml, args);
		assert(error == CODEC_ERROR_OKAY);
		if (! (error == CODEC_ERROR_OKAY)) {
			return error;
		}
	}

	// Write the XML tree to the output file without text wrapping
	mxmlSetWrapMargin(0);
	int mxml_result = mxmlSaveFile(xml, output_file, whitespace_cb);

	if (args->debug_flag) {
		fprintf(stderr, "Saved XML file result: %d\n", mxml_result);
	}

	// Delete the XML tree
	mxmlDelete(xml);

	return CODEC_ERROR_OKAY;
}

#endif
