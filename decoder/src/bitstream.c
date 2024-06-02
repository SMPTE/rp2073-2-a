/*!	@file decoder/src/bitstream.c

	Implementation of a bitstream for reading bits from a byte stream.

	The bitstream is connected to a byte stream to read bytes from an input
	source.  The stream may be a binary file, memory buffer, or a track in
	a media container.  The bitstream data structure stores the last word
	read from the byte stream and the count of the number of bits that remain
	in the bit buffer.

	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#include "headers.h"

/*!
	@brief Return a mask with the specified number of right-justified bits set to one
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
*/
CODEC_ERROR InitBitstream(BITSTREAM *bitstream)
{
	if (bitstream != NULL)
	{
		bitstream->error = BITSTREAM_ERROR_OKAY;
		bitstream->stream = NULL;
		bitstream->buffer = 0;
		bitstream->count = 0;
		return CODEC_ERROR_OKAY;
	}

	return CODEC_ERROR_NULLPTR;
}

/*!
	@brief Attach a bitstream to a byte stream.

	It is permitted for the byte stream to be NULL, in which case the
	bitstream will not be able to replenish its internal buffer and the
	consequences are undefined.
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
	(void) bitstream;
	return CODEC_ERROR_OKAY;
}

/*!
	@brief Return the specified number of bits from the bitstream
*/
BITWORD GetBits(BITSTREAM *stream, BITCOUNT count)
{
    CODEC_ERROR error = CODEC_ERROR_OKAY;

    // Return zero if the request cannot be satisfied
	BITWORD bits = 0;

	// Check that the number of requested bits is valid
	//assert(0 <= count && count <= bit_word_count);
	assert(count <= bit_word_count);

	// Check that the unused portion of the bit buffer is empty
	assert((stream->buffer & BitMask(bit_word_count - stream->count)) == 0);

	if (count == 0) goto finish;

	// Are there enough bits in the buffer to satisfy the request?
	if (count <= stream->count)
	{
		// Right align the requested number of bits in the bit buffer
		bits = (stream->buffer >> (bit_word_count - count));

		// Reduce the number of bits in the bit buffer
		stream->buffer <<= count;
		stream->count = (stream->count - count);
	}
	else
	{
		BITCOUNT low_bit_count;

		// Use the remaining bits in the bit buffer
		assert(stream->count > 0 || stream->buffer == 0);
		bits = (stream->buffer >> (bit_word_count - count));

		// Compute the number of bits to be used from the next word
		low_bit_count = count - stream->count;
		stream->count = 0;
		assert(low_bit_count > 0);

		// Fill the bit buffer from the byte stream
		assert(stream->count == 0);
		error = GetBuffer(stream);
        if (error != CODEC_ERROR_OKAY) {
            return 0;
        }
		assert(stream->count >= low_bit_count);

		// Use the new bits in the bit buffer
		bits |= (stream->buffer >> (bit_word_count - low_bit_count));

		// Reduce the number of bits in the bit buffer
		if (low_bit_count < bit_word_count) {
			stream->buffer <<= low_bit_count;
		}
		else {
			stream->buffer = 0;
		}
		assert(low_bit_count <= stream->count);
		stream->count = (stream->count - low_bit_count);
	}

finish:
	// The bit count should never be negative or larger than the size of the bit buffer
	//assert(0 <= stream->count && stream->count <= bit_word_count);
	assert(stream->count <= bit_word_count);

	// The unused bits in the bit buffer should all be zero
	assert((stream->buffer & BitMask(bit_word_count - stream->count)) == 0);

	// The unused bits in the result should all be zero
	assert((bits & ~BitMask(count)) == 0);

	return bits;
}

/*!
	@brief Fill the internal bitstream buffer by reading a byte stream

	@todo Need to modify this routine to return an error code if it cannot
	read from the byte stream associated with the bitstream.
*/
CODEC_ERROR GetBuffer(BITSTREAM *bitstream)
{
    STREAM_ERROR error = STREAM_ERROR_OKAY;
    
	// Need to signal an underflow error?
	assert(bitstream != NULL && bitstream->stream != NULL);
	if (! (bitstream != NULL && bitstream->stream != NULL)) {
		return (bitstream->error = BITSTREAM_ERROR_UNDERFLOW);
	}

	// The bit buffer should be empty
	assert(bitstream->count == 0);

	// Fill the bit buffer with a word from the byte stream
	bitstream->buffer = Swap32(GetWord(bitstream->stream));
    
    error = bitstream->stream->error;
    if (error != STREAM_ERROR_OKAY)
    {
        // Record the error in the bitstream
        bitstream->error = BitstreamErrorStream(error);
        
        // Return a codec error code corresponding to the bitstream error
        return CodecErrorBitstream(bitstream->error);
    }
    
	bitstream->count = bit_word_count;
	return CODEC_ERROR_OKAY;
}

/*!
	@brief Rewind the bitstream

	This routine rewinds the bitstream to the beginning of the byte stream
	that has been attached to the bitstream.  The byte stream is also reset.

	If the byte stream could not be reset, then the internal bitstream state
	is not reset.
*/
CODEC_ERROR RewindBitstream(BITSTREAM *bitstream)
{
	assert(bitstream != NULL);
	if (! (bitstream != NULL)) {
		return CODEC_ERROR_NULLPTR;
	}

	if (bitstream->stream != NULL) {
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
	@brief Skip the payload in a chunk

	A chunk is a tag value pair where the value specifies the length
	of a payload.  If the tag is a negative number, then the payload
	can be skipped without affecting the decoding process.
*/
CODEC_ERROR SkipPayload(BITSTREAM *bitstream, int chunk_size)
{
	// The chunk size is in units of 32-bit words
	size_t size = 4 * chunk_size;

	// This routine assumes that the bit buffer is empty
	assert(bitstream->count == 0);

	// Skip the specified number of bytes in the stream
	return SkipBytes(bitstream->stream, size);
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
	// The bit buffer must be empty
	assert(bitstream->count == 0);
	return (bitstream->stream->byte_count);
}

/*!
	@brief Align the bitstream to a byte boundary

	Enough bits are removed from the bitstream buffer to
	align the bitstream to the next byte.
*/
CODEC_ERROR AlignBitsByte(BITSTREAM *bitstream)
{
	// Compute the number of bits to skip
	BITCOUNT count = bitstream->count % 8;
	GetBits(bitstream, count);
	assert((bitstream->count % 8) == 0);
	return CODEC_ERROR_OKAY;
}

/*!
	@brief Align the bitstream to the next word boundary

	All of the bits in the bitstream buffer are flushed unless
	the bitstream buffer is completely empty or completely full.
*/
CODEC_ERROR AlignBitsWord(BITSTREAM *bitstream)
{
	BITCOUNT count = bitstream->count;

	if (0 < count && count < bit_word_count) {
		GetBits(bitstream, count);
	}

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Read more bits and append to an existing word of bits

	Read the specified number of bits from the bitstream and append
	the new bits to the right end of the word supplied as an argument.
	This is a convenience routine that is used to accumulate bits that
	may match a codeword.
*/
BITWORD AddBits(BITSTREAM *bitstream, BITWORD bits, BITCOUNT count)
{
	BITWORD new_bits = GetBits(bitstream, count);
	assert((new_bits & ~BitMask(count)) == 0);

	bits = (bits << count) | new_bits;

	return bits;
}

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)

/*!
    @brief Read a block of bytes from the bitstream
*/
CODEC_ERROR GetByteArray(BITSTREAM *bitstream, uint8_t *array, size_t size)
{
    size_t i;
    
    for (i = 0; i < size; i++)
    {
        array[i] = GetBits(bitstream, 8);
    }
    
    return CODEC_ERROR_OKAY;
}

#endif


/*!
	@brief Check that the bitstream is aligned to a segment boundary

    This function definition duplicates the same function in common/src/syntax.c,
    but that file includes definitions intended only for the encoder.

    @todo Remove duplicate function definitions.
 */
bool IsAlignedSegment(BITSTREAM *stream)
{
    return (stream->count == 0 || stream->count == bit_word_count);
}

/*!
    @brief Convert a byte stream error code into a bitstream error
*/
BITSTREAM_ERROR BitstreamErrorStream(STREAM_ERROR error)
{
    return (BITSTREAM_ERROR_STREAM | error);
}


/*!
	@Return true of the bitstream has reached the end of the stream
*/
bool EndOfBitsteam(BITSTREAM *bitstream)
{
	if (bitstream->count > 0) {
		// The bitstream buffer is not empty
		return false;
	}

	STREAM *stream = bitstream->stream;

	return EndOfStream(stream);
}
