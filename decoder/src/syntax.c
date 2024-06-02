/*!	@file decoder/src/syntax.c

	Implementation of functions for parsing the bitstream syntax of encoded samples.

	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#include "headers.h"

//! Size of a tag or value (in bits)
#define BITSTREAM_TAG_SIZE				16


/*!
	@brief Read the next tag-valie pair from the bitstream.

	The next tag is read from the bitstream and the next value that
	immediately follows the tag in the bitstreeam are read from the
	bitstream.

	The value may be the length of the payload in bytes or the value
	may be a single scalar.  This routine only reads the next tag and
	value and does not intepret the tag or value and does not read any
	data that may follow the segment in the bitstream.

	The tag and value are interpreted by @ref UpdateCodecState and that
	routine may read additional information from the bitstream.

	If the value is the length of the payload then it encodes the number
	of bytes in the segment payload, not counting the segment header.
*/
TAGVALUE GetSegment(BITSTREAM *stream)
{
	TAGVALUE segment;
	segment.tuple.tag = (TAGWORD)GetBits(stream, 16);
	segment.tuple.value = (TAGWORD)GetBits(stream, 16);
	return segment;
}

/*!
	@brief Read the specified tag from the bitstream and return the value
*/
TAGWORD GetValue(BITSTREAM *stream, int tag)
{
	TAGVALUE segment = GetTagValue(stream);

	assert(stream->error == BITSTREAM_ERROR_OKAY);
	if (stream->error == BITSTREAM_ERROR_OKAY) {
		assert(segment.tuple.tag == tag);
		if (segment.tuple.tag == tag) {
			return segment.tuple.value;
		}
		else {
			stream->error = BITSTREAM_ERROR_BADTAG;
		}
	}

	// An error has occurred so return zero (error code was set above)
	return 0;
}

/*!
	@brief Read the next tag value pair from the bitstream
*/
TAGVALUE GetTagValue(BITSTREAM *stream)
{
	TAGVALUE segment = GetSegment(stream);
	while (segment.tuple.tag < 0) {
		segment = GetSegment(stream);
	}

	return segment;
}

/*!
	@brief Convert the tag to a required tag
*/
TAGWORD RequiredTag(TAGWORD tag)
{
	if (tag < 0) {
		return -tag;
	}
	return tag;
}

/*!
	@brief Return true if the tag is optional
*/
bool IsTagOptional(TAGWORD tag)
{
	return (tag < 0);
}

/*!
	@brief Return true if the tag is required
*/
bool IsTagRequired(TAGWORD tag)
{
	return (tag >= 0);
}

/*!
	@brief Return true if a valid tag read from the bitstream
*/
bool IsValidSegment(BITSTREAM *stream, TAGVALUE segment, TAGWORD tag)
{
	return (stream->error == BITSTREAM_ERROR_OKAY &&
			segment.tuple.tag == tag);
}

/*!
	@brief Align the bitstream to the next tag value pair
*/
CODEC_ERROR AlignBitsSegment(BITSTREAM *bitstream)
{
	STREAM *stream = bitstream->stream;
	size_t byte_count;

	// Byte align the bitstream
	AlignBitsByte(bitstream);
	assert((bitstream->count % 8) == 0);

	// Compute the number of bytes in the bit buffer
	byte_count = bitstream->count / 8;

	// Add the number of bytes read from the stream
	byte_count += stream->byte_count;

	while ((byte_count % sizeof(TAGVALUE)) != 0)
	{
		GetBits(bitstream, 8);
		byte_count++;
	}

	// The bitstream should be aligned to the next segment
	assert((bitstream->count == 0) || (bitstream->count == bit_word_count));
	assert((byte_count % sizeof(TAGVALUE)) == 0);

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Return true if the tag value pair has the specified tag and value
*/
bool IsTagValue(TAGVALUE segment, int tag, TAGWORD value)
{
	return (segment.tuple.tag == tag && segment.tuple.value == value);
}

/*!
	Brief Check that the bitstream is aligned to a tag word boundary
*/
bool IsAlignedTag(BITSTREAM *stream)
{
	return ((stream->count % BITSTREAM_TAG_SIZE) == 0);
}
