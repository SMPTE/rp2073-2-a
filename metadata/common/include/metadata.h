/*!	@file metadata.h

	Declaration of the data structures for metadata described in SMMPTE ST 2073-7 Metadata.

	(c) 2013-2021 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _METADATA_H
#define _METADATA_H

//! The VC-5 bitstream comprises an integer number of 32-bit segments
typedef uint32_t SEGMENT;

//! Binary representation of a four character code (little endian)
typedef uint32_t FOURCC;

// Define tuple tags and types
typedef FOURCC TUPLE_TAG;
typedef char TUPLE_TYPE;
typedef uint32_t TUPLE_SIZE;
typedef uint16_t TUPLE_COUNT;
typedef uint16_t TUPLE_PADDING;

//! Metadata tuple header (SMPTE ST 2073-7, section 7.1)
typedef struct _tuple_header
{
	TUPLE_TAG tag;
	TUPLE_TYPE type;
	TUPLE_SIZE size;
	TUPLE_COUNT count;
	TUPLE_PADDING padding;

} TUPLE_HEADER;

//! Metadata tuple including the header and payload (value plus padding)
typedef struct _tuple
{
	TUPLE_HEADER header;
	void *payload;
	size_t payload_size;

} TUPLE;

// Define bitstream tag-value pairs
typedef int16_t BITSTREAM_TAG;
typedef int16_t BITSTREAM_VALUE;

// Define the metadata chunk tags
#define METADATA_CHUNK_SMALL 0x4010
#define METADATA_CHUNK_LARGE 0x61

typedef struct _chunk
{
	uint16_t tag;
	uint32_t size;

} CHUNK;

// Define a universally unique identifier (UUID)
typedef uint8_t UUID[16];

typedef uint32_t ATTRIBUTE_MASK;

enum _attribute_mask_bits
{
	ATTRIBUTE_TAG =		(1 << 0),
	ATTRIBUTE_TYPE =	(1 << 1),
	ATTRIBUTE_SIZE =	(1 << 2),
	ATTRIBUTE_COUNT =	(1 << 3),
	ATTRIBUTE_VALUE =	(1 << 4),
	ATTRIBUTE_PADDING =	(1 << 5),

};

//! Macro to negate the tag for an optional bitstream element
#define NEG(tag)	(-(tag))

//! Macro for printing a FOURCC using formatted print statement
#define FOURCC_CHARS(fourcc) (fourcc & 0xFF), ((fourcc >> 8) & 0xFF), ((fourcc >> 16) & 0xFF), ((fourcc >> 24) & 0xFF)

//! Macro for defining a FOURCC at compile time
#define FOURCC_VALUE(string) (string[3] << 24 | string[2] << 16 | string[1] << 8 | string[0])

//! Return the byte-swapped value of the next element in the payload
#define SWAPPED_VALUE(_type, _swapper, _payload) (_type)_swapper(((_type *)_payload)[0])

//! Return the byte-swapped value for the indexed element in the array
#define SWAPPED_ELEMENT(_type, _swapper, _array, _index) (_type)_swapper(_array[_index])


//! Convert the string representation of a metadata tuple tag to a FOURCC
static inline FOURCC TupleTag(const char *string)
{
	assert(strlen(string) == 4);
	return Swap32(string[0] << 24 | string[1] << 16 | string[2] << 8 | string[3]);
}


//! Convert data type zero to a printable character
static inline char PrintableType(TUPLE_TYPE type)
{
	return (type == 0) ? '0' : type;
}


//! Return true if the metadata tuple is a class instance
static inline bool IsClassInstance(TUPLE_TAG tag, TUPLE_TYPE type)
{
	return (type == 'E');
}


//! Return true if the metadata tuple value comprises other metadata tuples
static inline bool IsNestedTuple(TUPLE_TYPE type)
{
	switch (type)
	{
		case 0:
		case 'P':
			return true;

		default:
			return false;
	}
}


static inline size_t TuplePadding(size_t size, size_t count)
{
	// Set count to one for computing the payload size
	count = (count > 0) ? count : 1;
	size_t payload_size = size * count;

	// Round up the payload size to a segment boundary
	size_t segment_count = (payload_size + 3)/4;
	size_t padding = 4 * segment_count - size;

	return padding;
}


/*!
	@brief Return the leader for indicating the level in the tuple tree
*/
static inline char *Indentation(int level)
{
	const int indent = 4;
	const char leader = ' ';
	static char string[1024];
	assert(indent * level + 1 < sizeof(string));

	for (int i = 0; i < level; i++) {
		for (int j = 0; j < indent; j++) {
			string[i * indent + j] = leader;
		}
	}

	// Terminate the string with a nul character
	string[level * indent] = '\0';
	assert(strlen(string) == indent * level * sizeof(leader));

	return string;
}


#if 0
/*!
	@brief Return the indentation for the specified level in the XML tree
*/
static inline const char *indent(int level)
{
	static char buffer[1024];
	int i;

	for (i = 0; i < level; i++) {
		strcpy(&buffer[4 * i], "    ");
	}
	buffer[4 * i] = '\0';

	return buffer;
}
#endif


#ifdef __cplusplus
extern "C" {
#endif

//! Return true if the segment is a metadata chunk header
//bool IsChunkElement(SEGMENT segment);

//! Parse the metadata chunk header into the tag and size
bool ParseChunkHeader(SEGMENT chunk_header, uint16_t *tag_out, uint32_t *size_out, bool swapped_flag);

//! Return true if the metadata tuple has a repeat count
bool HasRepeatCount(TUPLE_TYPE type);

//! Return true if the tuple data type is numerical (SMPTE ST 2073-7 Table 4)
bool IsNumericalType(TUPLE_TYPE type);

//! Return true if the value is a vector
bool IsVectorValued(TUPLE_TYPE type, TUPLE_SIZE size);

// Return the size of each element in a vector of the specified type
size_t ElementSize(TUPLE_TYPE type);

// Convert a string to a binary four character code
FOURCC ConvertTagToBinary(const char *string);

// Convert a string value to its binary representation
CODEC_ERROR ConvertValueToBinary(void *buffer, size_t length, const TUPLE_HEADER *tuple_header, const char *value);

// Convert the string representation of a pixel format to an RGBALayout (SMPTE ST 377-1, section G.2.40)
CODEC_ERROR ConvertValueToLayout(void *buffer, size_t length, const TUPLE_HEADER *tuple_header, const char *value);

// Convert the string representation of a Bayer pattern to its binary representation
//CODEC_ERROR ConvertValueToPattern(void *buffer, const size_t length, const TUPLE_HEADER *tuple_header, const char *value);

// Write a metadata chunk or tuple header to the binary output file
CODEC_ERROR WriteMetadataHeader(FILE *output_file,  TUPLE_HEADER *tuple_header);

// Write the metadata value to the binary output file
CODEC_ERROR WriteMetadataValue(FILE *output_file, const TUPLE_HEADER *tuple_header, const void *value_data, const size_t value_size);

// Write the metadata tuple padding
CODEC_ERROR WriteMetadataPadding(FILE *output_file, size_t value_size);

// Write the metadata chunk header
CODEC_ERROR WriteChunkHeader(FILE *output_file, uint32_t tag, uint32_t value);

// Print the tuple header to the terminal (for debugging)
CODEC_ERROR PrintTupleHeader(TUPLE_HEADER *tuple_header, char *label);

#ifdef __cplusplus
}
#endif

#endif
