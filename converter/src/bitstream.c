/*!	@file bitstream.c

	Implementation of a bitstream for reading bits from a stream.

	The bitstream is connected to a stream to read bytes from an input
	source.  The stream may be a binary file, memory buffer, or a track
	in a media container.  The bitstream data structure stores the last
	word read from the byte stream and the count of the number of bits
	that remains in the buffer.

	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#include "headers.h"

/*!
	@brief Return a mask with the specified number of bits set to one
*/
BITWORD BitMask(int n)
{
	if (n < bit_word_count)
	{
		BITWORD mask = 0;
		if (n > 0) {
			mask = ((1 << n) - 1);
		}
		return mask;
	}
	return BIT_WORD_MAX;
}

/*!
	@brief Initialize a bitstream data structure

	This routine is the constructor for the bitstream data type.

	The sample offset stack is used to mark the offset to a position
	in the bitstream for computing the size field of sample chunks.
*/
CODEC_ERROR InitBitstream(BITSTREAM *bitstream)
{
	if (bitstream != NULL)
	{
		bitstream->error = BITSTREAM_ERROR_OKAY;
		bitstream->stream = NULL;
		bitstream->buffer = 0;
		bitstream->count = 0;

		// Initialize the stack of sample offsets
		memset(bitstream->sample_offset_stack, 0, sizeof(bitstream->sample_offset_stack));
		bitstream->sample_offset_count = 0;

		// Clear the debugging parameters
		bitstream->logfile = NULL;
		bitstream->putbits_flag = false;

		return CODEC_ERROR_OKAY;
	}

	return CODEC_ERROR_NULLPTR;
}

/*!
	@brief Attach a bitstream to a byte stream.

	It is permitted for the byte stream to be NULL, in which case the
	bitstream will not be able to replenish its internal buffer, but
	the consequences are undefined.
*/
CODEC_ERROR AttachBitstream(struct _bitstream *bitstream, struct _stream *stream)
{
	assert(bitstream != NULL);
	bitstream->stream = stream;
	return CODEC_ERROR_OKAY;
}

/*!
	@brief Detach a bitstream from a byte stream.

	Any resources allocated by the bitstream are released without deallocating
	the bitstream data structure itself.  The byte stream associated with the
	bitstream is not closed by this routine.  The byte stream must be closed,
	if and when appropriate, by the caller.
*/
CODEC_ERROR ReleaseBitstream(BITSTREAM *bitstream)
{
	//TODO: What cleanup needs to be performed?
	(void) bitstream;
	return CODEC_ERROR_OKAY;
}

/*!
	@brief Write the specified number of bits to the bitstream
*/
CODEC_ERROR PutBits(BITSTREAM *stream, BITWORD bits, BITCOUNT count)
{
	BITCOUNT unused_bit_count;

	if (count == 0) {
		return CODEC_ERROR_OKAY;
	}

	if (stream->putbits_flag) {
		fprintf(stream->logfile, "%d %08X\n", count, bits);
	}

	// Check that the unused portion of the input bits is empty
	assert((bits & (BitMask(bit_word_count - count) << count)) == 0);

	// Check that the number of input bits is valid
	assert(0 <= count && count <= bit_word_count);

	// Check that the unused portion of the bit buffer is empty
	unused_bit_count = bit_word_count - stream->count;
	assert((stream->buffer & BitMask(unused_bit_count)) == 0);

	// Is there room in the bit buffer for the new bits?
	if (count <= unused_bit_count)
	{
		// Fill the remaining space in the bit buffer
		stream->buffer |= (bits << (unused_bit_count - count));

		// Reduce the number of unused bits in the bit buffer
		stream->count += count;
	}
	else
	{
		// Any room in the bit buffer?
		if (unused_bit_count > 0)
		{
			// Use the number of input bits that will fit in the bit buffer
			stream->buffer |= (bits >> (count - unused_bit_count));

			// Reduce the number of input bits by the amount used
			count -= unused_bit_count;
			assert(count >= 0);
		}

		// Reduce the number of unused bits in the bit buffer
		stream->count += unused_bit_count;

		// The bit buffer should be full
		assert(stream->count == bit_word_count);

		// Output the full bit buffer
		PutBuffer(stream);

		// The bit buffer should be empty
		assert(stream->count == 0);

		// Insert the remaining input bits into the bit buffer
		stream->buffer = (bits << (bit_word_count - count));

		// Increment the number of bits in the bit buffer
		stream->count += count;
	}

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Write the internal bitstream buffer to a byte stream

	@todo Need to modify this routine to return an error code if it cannot
	write to the byte stream associated with the bitstream.
*/
CODEC_ERROR PutBuffer(BITSTREAM *bitstream)
{
	//TODO: Need to signal an overflow error
	assert(bitstream != NULL && bitstream->stream != NULL);
	if (! (bitstream != NULL && bitstream->stream != NULL)) {
		return (bitstream->error = BITSTREAM_ERROR_OVERFLOW);
	}

	// The bit buffer should be full
	assert(bitstream->count == bit_word_count);

	// Write the bit buffer to the byte stream
	PutWord(bitstream->stream, Swap32(bitstream->buffer));

	// Empty the bit buffer
	bitstream->buffer = 0;
	bitstream->count = 0;

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Write a longword (32 bits) to the stream
*/
CODEC_ERROR PutLong(BITSTREAM *stream, BITWORD longword)
{
	return PutBits(stream, longword, bit_word_count);
}

/*!
	@brief Reset the bitstream

	This routine resets the bitstream to the beginning of the byte stream
	that has been attached to the bitstream.  The byte stream is also reset.

	If the byte stream could not be reset, then the internal bitstream state
	is not reset.
*/
//CODEC_ERROR ResetBitstream(BITSTREAM *bitstream)
CODEC_ERROR RewindBitstream(BITSTREAM *bitstream)
{
	assert(bitstream != NULL);
	if (! (bitstream != NULL)) {
		return CODEC_ERROR_NULLPTR;
	}

	// Write remaining bits to the byte stream
	FlushBitstream(bitstream);

	if (bitstream->stream != NULL) {
		//CODEC_ERROR error = ResetStream(bitstream->stream);
		CODEC_ERROR error = RewindStream(bitstream->stream);
		if (error != CODEC_ERROR_OKAY) {
			return error;
		}
	}

	// Reset the bitstream internal state
	bitstream->buffer = 0;
	bitstream->count = 0;
	bitstream->error = BITSTREAM_ERROR_OKAY;

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Convert a bitstream error code into a codec error code

	The bitstream and byte stream modules might be used in other
	applications and have their own errors codes.  This routine
	embeds a bitstream error code into a codec error code.  The
	bitstream error code is carried in the low bits of the codec
	error code.

	@todo Eliminate separate error codes for bitstreams?
*/
CODEC_ERROR BitstreamCodecError(BITSTREAM *bitstream)
{
	assert(bitstream != NULL);
	if (! (bitstream != NULL)) {
		return CODEC_ERROR_NULLPTR;
	}

	return CodecErrorBitstream(bitstream->error);
}

/*!
	@brief Get the current position in the byte stream

	The current position in the byte stream associated with the
	bitstream is returned.  The intent is to allow the bitstream
	(and the associated byte stream) to be restored to the saved
	position.

	@todo Need to record the state of the bitstream buffer and
	bit count so that the entire bitstream state can be restored.
*/
size_t GetBitstreamPosition(BITSTREAM *bitstream)
{
	if (bitstream->count == bit_word_count) {
		PutBuffer(bitstream);
	}

	// The bit buffer must be empty
	assert(bitstream->count == 0);

	return (bitstream->stream->byte_count);
}

/*!
	@brief Align the bitstream to a byte boundary

	Enough bits are written to the bitstream to align the
	bitstream to the next byte.
*/
CODEC_ERROR AlignBitsByte(BITSTREAM *bitstream)
{
	if (bitstream->count > 0 && (bitstream->count % 8) != 0)
	{
		// Compute the number of bits of padding
		BITCOUNT count = (8 - (bitstream->count % 8));
		PutBits(bitstream, 0, count);
	}
	assert((bitstream->count % 8) == 0);
	return CODEC_ERROR_OKAY;
}

/*!
	@brief Align the bitstream to the next word boundary

	Enough bits are written to the bitstream to align the
	bitstream to the next word.
*/
CODEC_ERROR AlignBitsWord(BITSTREAM *bitstream)
{
	if (bitstream->count > 0)
	{
		// Compute the number of bits of padding
		BITCOUNT count = (bit_word_count - bitstream->count);
		PutBits(bitstream, 0, count);

		// The buffer should be full
		assert(bitstream->count == bit_word_count);

		// Write out the buffer
		PutBuffer(bitstream);
	}

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Write any bits in the buffer to the byte stream
*/
CODEC_ERROR FlushBitstream(BITSTREAM *bitstream)
{
	// Any bits remaining in the bit buffer?
	if (bitstream->count > 0)
	{
		// Write the bit buffer to the output stream
		PutBuffer(bitstream);
	}

	// Indicate that the bitstream buffer is empty
	bitstream->count = 0;
	bitstream->buffer = 0;

	// Flush the output stream
	FlushStream(bitstream->stream);

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Pop the top value from the sample offset stack
*/
uint32_t PopSampleOffsetStack(BITSTREAM *bitstream)
{
	assert(bitstream->sample_offset_count > 0);
	return bitstream->sample_offset_stack[--bitstream->sample_offset_count];
}
