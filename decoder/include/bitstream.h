/*! @file decoder/include/bitstream.h

	Declaration of the bitstream data structure.

	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _BITSTREAM_H
#define _BITSTREAM_H

//! Reserve the upper bits to designate the subsystem associated with the error
#define BITSTREAM_ERROR_SUBSYSTEM_SHIFT 5

/*!
	@brief Bitstream error codes

	The bitstream contains its own enumeration of error codes since this
	module may be used in other applications.
*/
typedef enum _bitstream_error
{
	BITSTREAM_ERROR_OKAY = 0,		//!< No error
	BITSTREAM_ERROR_UNDERFLOW,		//!< No unread bits remaining in the bitstream
	BITSTREAM_ERROR_OVERFLOW,		//!< No more bits can be written to the bitstream
	BITSTREAM_ERROR_BADTAG,			//!< Unexpected tag found in the bitstream
    BITSTREAM_ERROR_UNEXPECTED,     //!< Unexpected error

    
    /***** Reserve a block of error codes for the byte stream *****/
    
    BITSTREAM_ERROR_STREAM = (1 << BITSTREAM_ERROR_SUBSYSTEM_SHIFT),

} BITSTREAM_ERROR;


typedef uint32_t BITWORD;			//!< Data type of the internal bitstream buffer
typedef uint_fast8_t BITCOUNT;		//!< Number of bits in the bitsteam buffer

//! Maximum number of bits in a bit word
static const BITCOUNT bit_word_count = 32;

//! Maximum value of a bit word
#define BIT_WORD_MAX 0xFFFFFFFF

//! Sample offset stack depth
#define MAX_SAMPLE_OFFSET_COUNT		8

/*!
	@brief Declaration of the bitstream data structure

	The bitstream uses a byte stream to read bytes from a file or a buffer
	in memory.  This isolates that bitstream module from the type of byte
	stream.
*/
typedef struct _bitstream
{
	BITSTREAM_ERROR error;		//!< Error while processing the bitstream
	struct _stream *stream;		//!< Stream for reading bytes into the buffer
	BITWORD buffer;				//!< Internal buffer holds remaining bits
	BITCOUNT count;				//!< Number of bits remaining in the buffer

#if VC5_ENABLED_PART(VC5_PART_SECTIONS)
	/*!
		The sample offset stack is used to record offsets to the start of nested
		syntax structures.  For example, a sample size segment is written into the
		bitstream with a size of zero, since the size of the syntax element is not
		known in advance.  The offset to the sample size segment is pushed onto the
		sample offset stack so that the location of the sample size segment can be
		updated with the actual size of a syntax element after the complete element
		is written into the bitstream.

		This data structure is called the ChunkSizeOffset in the current codec
		implementation.
	*/
	uint32_t sample_offset_stack[MAX_SAMPLE_OFFSET_COUNT];

	//! Number of entries in the sample offset stack
	uint_fast8_t sample_offset_count;
#endif
#if _DEBUG
	//! File for bitstream debugging output
	FILE *logfile;
#endif
#if _DEBUG
	//! The putbits flags controls debug output during entropy coding
	bool putbits_flag;
#endif

} BITSTREAM;

// Initialize a bitstream data structure
CODEC_ERROR InitBitstream(BITSTREAM *bitstream);

// Bind the bitstream to a byte stream
CODEC_ERROR AttachBitstream(struct _bitstream *bitstream, struct _stream *stream);

CODEC_ERROR ReleaseBitstream(BITSTREAM *stream);

BITWORD GetBits(BITSTREAM *stream, BITCOUNT count);

BITWORD GetBuffer(BITSTREAM *stream);

// Rewind the bitstream and the associated byte stream
CODEC_ERROR RewindBitstream(BITSTREAM *bitstream);

CODEC_ERROR BitstreamCodecError(BITSTREAM *bitstream);

CODEC_ERROR SkipPayload(BITSTREAM *bitstream, int chunk_size);

// Return the current position of the bitstream pointer in the sample
size_t GetBitstreamPosition(BITSTREAM *stream);

CODEC_ERROR AlignBitsByte(BITSTREAM *bitstream);
CODEC_ERROR AlignBitsWord(BITSTREAM *bitstream);

BITWORD AddBits(BITSTREAM *stream, BITWORD bits, BITCOUNT count);

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
CODEC_ERROR GetByteArray(BITSTREAM *bitstream, uint8_t *array, size_t size);
#endif

#if VC5_ENABLED_PART(VC5_PART_SECTIONS)
uint32_t PopSampleOffsetStack(BITSTREAM *bitstream);
#endif

BITWORD BitMask(int n);

BITSTREAM_ERROR BitstreamErrorStream(STREAM_ERROR error);

bool EndOfBitsteam(BITSTREAM *bitstream);

#endif
