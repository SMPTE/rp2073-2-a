/*!	@file common/src/syntax.c

	Implementation of functions for parsing the bitstream syntax of encoded samples.

	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#include "headers.h"


//TODO: Simplify the definition of the marker bits
#define CODEC_LOWPASS_START_CODE		0x1A4A
#define CODEC_LOWPASS_START_SIZE		16
#define CODEC_LOWPASS_END_CODE			0x1B4B
#define CODEC_LOWPASS_END_SIZE			16

#define CODEC_HIGHPASS_START_CODE		0x0D0D
#define CODEC_HIGHPASS_START_SIZE		16
#define CODEC_HIGHPASS_END_CODE			0x0C0C
#define CODEC_HIGHPASS_END_SIZE			16

#define CODEC_BAND_START_CODE			0x0E0E
#define CODEC_BAND_START_SIZE			16
//#define CODEC_BAND_END_CODE 			0x038F0B3E	//Codeset dependent cs9
//#define CODEC_BAND_END_SIZE 			26			//Codeset dependent cs9
#define CODEC_BAND_END_CODE				0x0000E33F	//Codeset dependent cs15
#define CODEC_BAND_END_SIZE				16			//Codeset dependent cs15

#define CODEC_SAMPLE_STOP_CODE			0x1E1E
#define CODEC_SAMPLE_STOP_SIZE			16

#define CODEC_COEFFICIENT_START_CODE	0x0F0F
#define CODEC_COEFFICIENT_START_SIZE	16

//! Size of a tag or value (in bits)
#define BITSTREAM_TAG_SIZE				16


// Bits in the interlace structure flags

#define CODEC_FLAGS_INTERLACED			0x01	//!< Interlaced flags
#define CODEC_FLAGS_FIELD1_FIRST		0x02	//!< NTSC has this bit cleared
#define CODEC_FLAGS_FIELD1_ONLY			0x04	//!< Indicates missing fields
#define CODEC_FLAGS_FIELD2_ONLY			0x08
#define CODEC_FLAGS_DOMINANCE			0x10

#define CODEC_FLAGS_INTERLACED_MASK		0x1F	//!< Unused bits must be zero

// Useful macros for testing the interlaced flags

#define INTERLACED(flags)			(((flags) & CODEC_FLAGS_INTERLACED) != 0)
#define PROGRESSIVE(flags)			(((flags) & CODEC_FLAGS_INTERLACED) == 0)
#define FIELD_ORDER_NTSC(flags)		(((flags) & CODEC_FLAGS_FIELD1_FIRST) == 0)
#define FIELD_ORDER_PAL(flags)		(((flags) & CODEC_FLAGS_FIELD1_FIRST) != 0)
#define FIELD_ONE_ONLY(flags)		(((flags) & CODEC_FLAGS_FIELD1_ONLY) != 0)
#define FIELD_TWO_ONLY(flags)		(((flags) & CODEC_FLAGS_FIELD2_ONLY) != 0)
#define FIELD_ONE_PRESENT(flags)	(((flags) & CODEC_FLAGS_FIELD2_ONLY) == 0)
#define FIELD_TWO_PRESENT(flags)	(((flags) & CODEC_FLAGS_FIELD1_ONLY) == 0)
#define FIELD_BOTH_PRESENT(flags)	(((flags) & (CODEC_FLAGS_FIELD1_ONLY | CODEC_FLAGS_FIELD1_ONLY)) == 0)

// Bits in the copy protection flags

#define CODEC_FLAGS_PROTECTED			0x01	//!< Copy protection flags
#define CODEC_FLAGS_PROTECTION_MASK		0x01	//!< Unused bits must be zero


/*!
	@brief Write the next tag value pair to the bitstream

	@todo Change the code to use the @ref PutLong function?
*/
CODEC_ERROR PutTagValue(BITSTREAM *stream, TAGVALUE segment)
{
	CODEC_ERROR error;

	// Write the tag to the bitstream
	error = PutBits(stream, segment.tuple.tag, tagword_count);
	if (error != CODEC_ERROR_OKAY) {
		return error;
	}

	// Write the value to the bitstream
	error = PutBits(stream, segment.tuple.value, tagword_count);

	return error;
}

/*!
	@brief Write a required tag value pair

	@todo Should the tag value pair be output on a segment boundary?
*/
CODEC_ERROR PutTagPair(BITSTREAM *stream, int tag, int value)
{
	// The bitstream should be aligned on a tag word boundary
	assert(IsAlignedTag(stream));

	// The value must fit within a tag word
	assert(((uint32_t)value & ~(uint32_t)CODEC_TAG_MASK) == 0);

	PutLong(stream, ((uint32_t )tag << 16) | (value & CODEC_TAG_MASK));

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Write an optional tag value pair

	@todo Should the tag value pair be output on a segment boundary?
*/
CODEC_ERROR PutTagPairOptional(BITSTREAM *stream, int tag, int value)
{
	// The bitstream should be aligned on a tag word boundary
	assert(IsAlignedTag(stream));

	// The value must fit within a tag word
	assert(((uint32_t)value & ~(uint32_t)CODEC_TAG_MASK) == 0);

	// Set the optional tag bit
	//tag |= CODEC_TAG_OPTIONAL;
	//tag = NEG(tag);
	tag = neg(tag);

	PutLong(stream, ((uint32_t )tag << 16) | (value & CODEC_TAG_MASK));

	return CODEC_ERROR_OKAY;
}

#if 0
/*!
	@brief Write a tag that marks a place in the bitstream for debugging

	@todo Make sure that the debugging markers are not actually used by
	the decoder.
*/
CODEC_ERROR PutTagMarker(BITSTREAM *stream, uint32_t  marker, int size)
{
	// Not using bitstream markers anymore
	assert(0);

	// The marker must fit within the tag value
	assert(0 < size && size <= 16);

	// Output a tag and marker value for debugging
	PutTagPair(stream, CODEC_TAG_MARKER, marker);

	return CODEC_ERROR_OKAY;
}
#endif

/*!
	@brief Convert the tag to an optional tag

	An optional tag has a negative value.
*/
TAGWORD OptionalTag(TAGWORD tag)
{
	return ((tag < 0) ? tag : neg(tag));
}

/*!
	@brief Convert the tag to a required tag

	An optional tag has a negative value.
*/
TAGWORD RequiredTag(TAGWORD tag)
{
	return ((tag >= 0) ? tag : neg(tag));
}

/*
    @brief Return true if the tag is an optional tag
 */
bool IsTagOptional(TAGWORD tag)
{
    return (tag < 0);
}

/*
    @brief Return true if the tag is a required tag
 */
bool IsTagRequired(TAGWORD tag)
{
    return (tag > 0);
}

/*!
	@brief Align the bitstream to the next segment

	The corresponding function in the existing codec flushes the bitstream.

	@todo Is it necessary to flush the bitstream (and the associated byte stream)
	after aligning the bitstream to a segment boundary?
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

	// Add the number of bytes written to the stream
	byte_count += stream->byte_count;

	while ((byte_count % sizeof(TAGVALUE)) != 0)
	{
		PutBits(bitstream, 0, 8);
		byte_count++;
	}

	// The bitstream should be aligned to the next segment
	assert((bitstream->count == 0) || (bitstream->count == bit_word_count));
	assert((byte_count % sizeof(TAGVALUE)) == 0);

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Check that the bitstream is aligned to a tag word boundary

	@todo Check the places in the code where this function is used to
	determine whether the bitstream should actually be aligned to a
	segment boundary.
*/
bool IsAlignedTag(BITSTREAM *stream)
{
	return ((stream->count % BITSTREAM_TAG_SIZE) == 0);
}

/*!
	@brief Check that the bitstream is aligned to a segment boundary
*/
bool IsAlignedSegment(BITSTREAM *stream)
{
	return (stream->count == 0 || stream->count == bit_word_count);
}

#if 0
/*!
	@brief Write an index block for the sample bands

	@todo Change this syntax element to write offsets or channel sizes
	rather than pointers into the channel.  Need to modify the bitstream
	functions to be able to seek to a byte offset in the the bitstream.

	@todo Change the name of this routine to EncodeChannelOffsets?
*/
CODEC_ERROR PutGroupIndex(BITSTREAM *stream,
						  void *index_table[],
						  int index_table_length,
						  size_t *channel_size_table_offset)
{
	int index;

	// Save the location of the channel size table in the encoded sample
	if (channel_size_table_offset != NULL) {
		*channel_size_table_offset = GetBitstreamPosition(stream);
	}

	// Output the tag and the length of the index
	PutTagPair(stream, CODEC_TAG_INDEX, index_table_length);

	// Was a vector of index entries provided by the caller?
	if (index_table == NULL)
	{
		// Output an empty vector of longwords for the index entries
		// that can be filled later when the addresses are available
		for (index = 0; index < index_table_length; index++)
		{
			PutTagPair(stream, CODEC_TAG_ENTRY, (uint32_t)index);
		}
	}
	else
	{
		// Output the vector of longwords for the index entries
		for (index = 0; index < index_table_length; index++)
		{
			uintptr_t longword = (uintptr_t)index_table[index];
			assert(longword <= UINT32_MAX);
			PutLong(stream, (uint32_t)longword);
		}
	}

	return CODEC_ERROR_OKAY;
}
#endif

/*!
	@brief Pack the vector of prescale values into a single word

	The wavelet transform uses a vector of prescale values indexed by the
	wavelet level with the input image at level zero to specify the amount
	of prescaling that should be performed on the input the wavelet transform.

	This routine packs the prescale values into a segment value that can be
	written into the bitstream.
*/
TAGWORD PackTransformPrescale(TRANSFORM *transform)
{
	TAGWORD packed_prescale = 0;
	int i;

	// Encode the prescale values that are actually used
	for (i = 0; i < MAX_WAVELET_COUNT; i++)
	{
		assert((transform->prescale[i] & ~0x03) == 0);
		packed_prescale += transform->prescale[i] << (14 - i * 2);
	}

	// The remaining prescale values are filled with zeros

	return packed_prescale;
}

/*!
	@brief Write a tag value pair that specifies the size of a syntax element

	The routine pushes the current position in the bitstream onto the sample offset
	stack and writes a tag value pair for the size of the current syntax element.
	The routine @ref PopSampleSize overwrites the segment with a tag value pair
	that contains the actual size of the syntax element.

	This routine corresponds to the routine SizeTagPush in the current codec implementation.
*/
CODEC_ERROR PushSampleSize(BITSTREAM *bitstream, TAGWORD tag)
{
	size_t position = GetBitstreamPosition(bitstream);

	// Check for stack overflow
	assert(bitstream->sample_offset_count < MAX_SAMPLE_OFFSET_COUNT);

	// Check that the bitstream position can be pushed onto the stack
	assert(position <= UINT32_MAX);

	// Push the current sample offset onto the stack
	bitstream->sample_offset_stack[bitstream->sample_offset_count++] = (uint32_t)position;

	// Write a tag value pair for the size of this chunk
	//PutTagPair(bitstream, tag, 0);
    PutTagPairOptional(bitstream, tag, 0);

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Update a sample size segment with the actual size of the syntax element

	This routine pops the offset in the bitstream to the most recent tag value pair
	that was written into the bitstream from the sample offset stack and overwrites
	the segment with the tag value pair that contains the actual size of the syntax
	element.

	This routine corresponds to the routine SizeTagPop in the current codec implementation.
*/
CODEC_ERROR PopSampleSize(BITSTREAM *bitstream)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;

	if (bitstream->sample_offset_count > 0)
	{
		TAGVALUE segment;
		TAGWORD tag;

		uint32_t current_offset;
		uint32_t previous_offset;

		uint32_t chunk_size;

		size_t position = GetBitstreamPosition(bitstream);

		// Get the offset to the current position in the bitstream
		assert(position <= UINT32_MAX);
		current_offset = (uint32_t)position;

		// Pop the offset for this chunk from the sample offset stack
		previous_offset = PopSampleOffsetStack(bitstream);

		assert(previous_offset < current_offset);

		// Get the segment for the chunk written at the most recent offset
		error = GetSampleOffsetSegment(bitstream, previous_offset, &segment);
		if (error != CODEC_ERROR_OKAY) {
			return error;
		}
 
		// Get the tag for the chunk segment
		tag = segment.tuple.tag;
        
        // Should be an optional tag-value pair
        assert(IsTagOptional(tag));
        if (! (IsTagOptional(tag))) {
            return CODEC_ERROR_UNEXPECTED;
        }
        
        // Convert the tag to required
        tag = RequiredTag(tag);

		// Compute the size of the current chunk
		chunk_size = current_offset - previous_offset;

		if (chunk_size >= 4)
		{
			// The chunk payload should contain an integer number of segments
			assert((chunk_size % sizeof(TAGVALUE)) == 0);

			// Compute the number of segments in the chunk payload
			chunk_size = (chunk_size / sizeof(TAGVALUE)) - 1;
		}
		else
		{
			chunk_size = 0;
		}

		// Does this chunk have a 24-bit size field?
		if (tag & CODEC_TAG_LARGE_CHUNK)
		{
			// Add the most significant eight bits of the size to the tag			
			tag |= ((chunk_size >> 16) & 0xFF);
		}

		// The segment value is the least significant 16 bits of the payload size
		chunk_size &= 0xFFFF;

		// Update the segment with the optional tag and chunk size
		segment.tuple.tag = OptionalTag(tag);
		segment.tuple.value = chunk_size;

		return PutSampleOffsetSegment(bitstream, previous_offset, segment);
	}

	return CODEC_ERROR_UNEXPECTED;
}

/*!
	@brief Read the segment at the specified offset in the bitstream

	This routine is used to read a segment that was previously written at a previous
	location in the encoded sample.  This allows the encoder to update, rather than
	overwrite, a segment that has already been written.  Typically, this is done to
	insert the size or offset to a portion of the sample (syntax element) into a
	segment that acts as an index to the syntax element.
*/
CODEC_ERROR GetSampleOffsetSegment(BITSTREAM *bitstream, uint32_t offset, TAGVALUE *segment)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;
	uint32_t buffer;

	error = GetBlock(bitstream->stream, &buffer, sizeof(buffer), offset);
	if (error != CODEC_ERROR_OKAY) {
		return error;
	}

	// Translate the segment to native byte order
	segment->longword = Swap32(buffer);

	// Cannot return a segment if the offset stack is empty
	return CODEC_ERROR_OKAY;
}

/*!
	@brief Write a segment at the specified offset in the bitstream

	The segment at the specified offset in the bitstream is overwritten by the new
	segment provided as an argument.  Typically this is done to update a segment that
	is intended to provide the size or offset to a syntax element in the encoded sample.
*/
CODEC_ERROR PutSampleOffsetSegment(BITSTREAM *bitstream, uint32_t offset, TAGVALUE segment)
{
	// Translate the segment to network byte order
	uint32_t buffer = Swap32(segment.longword);

	// Must write the segment on a segment boundary
	assert((offset % sizeof(TAGVALUE)) == 0);

	// Write the segment to the byte stream at the specified offset
	return PutBlock(bitstream->stream, &buffer, sizeof(buffer), offset);
}

/*!
	@brief Write the bitstream start marker
*/
CODEC_ERROR PutBitstreamStartMarker(BITSTREAM *stream)
{
	assert(stream != NULL);
	if (! (stream != NULL)) {
		return CODEC_ERROR_UNEXPECTED;
	}

	return PutLong(stream, StartMarkerSegment);
}

/*!
	@brief Write the characteristics of the encoded image into the bitstream
*/
CODEC_ERROR PutVideoGroupExtension(ENCODER *encoder, BITSTREAM *stream)
{
	//TODO: Any information besides what is present in the bitstream header?
	return CODEC_ERROR_OKAY;
}

#if 0
/*!
	@brief Write the sample flags into the bitstream

	The sample flags describe characteristics of the encoded frame that can
	be represented by a single bit in the value of the sample flags segment.
*/
CODEC_ERROR PutVideoSampleFlags(ENCODER *encoder, BITSTREAM *stream)
{
	return CODEC_ERROR_OKAY;
}
#endif

#if 0   //VC5_ENABLED_PART(VC5_PART_COLOR_SAMPLING)
/*!
	@brief Write the frame structure flags into the bitstream

	The frame structure flags describe the characteristics of the input frame,
	including progressive versus interlaced fields, top or bottom field first,
	and whether the frame is upside down (bottom row first).

	@todo Change the frame structure tag-value pair to required.
*/
CODEC_ERROR PutFrameStructureFlags(ENCODER *encoder, BITSTREAM *stream)
{
	// The flags are all zero by default
	int flags = 0;

	if (encoder->progressive == 0)
	{
		// Set the interlaced bit
		flags |= IMAGE_STRUCTURE_INTERLACED;

		if (encoder->top_field_first == 0) {
			// Set the bottom field first bit
			flags |= IMAGE_STRUCTURE_BOTTOM_FIELD_FIRST;
		}
	}

	if (encoder->frame_inverted) {
		// Set the bottom row first bit (the frame is upside down)
		flags |= IMAGE_STRUCTURE_BOTTOM_ROW_FIRST;
	}

	return CODEC_ERROR_OKAY;
}
#endif

/*!
	@brief Write the channel header to the bitstream

	It is not necessary to write the channel header for the first channel
	because the decoder is initialized to begin decoding channel zero.

	The channel header is used to mark a new channel in the bitstream.

	A sample tag is written with the value indicating that the sample is a
	channel.  The original codec handled channels and layers as sub-samples
	within the sample, but recommended practice is the structure the encoded
	sample as a hierarchy of sample, layer, and channel.
*/
CODEC_ERROR PutVideoChannelHeader(BITSTREAM *stream, int channel)
{
	AlignBitsSegment(stream);
	PutTagPair(stream, CODEC_TAG_ChannelNumber, channel);
	return CODEC_ERROR_OKAY;
}

/*!
	@brief Write the channel trailer

	This routine is not implemented since the reference encoder does
	not write a trailer after encoding the channel.  The channel header
	separates encoded channels in the bitstream so a channel trailer
	is not needed.
*/
CODEC_ERROR PutVideoChannelTrailer(BITSTREAM *stream, int channel)
{
	assert(0);
	return CODEC_ERROR_UNIMPLEMENTED;
}

/*!
	@brief Write the lowpass band header into the bitstream

	Each channel is encoded separately, so the lowpass band (subband zero)
	is the lowpass band in the wavelet at the highest level for each channel.

	The last element in the lowpass band header is a segment that contains the
	size of this subband.  The actual size is updated when the lowpass trailer
	is written (see @ref PutVideoLowpassTrailer).

	The lowpass start code is used to uniquely identify the start of the lowpass
	band header and is used by the decode to navigate to the next channel in the
	bitstream.

	@todo Consider writing a composite lowpass band for all channels with
	interleaved rows to facilitate access to the thumbnail image in the
	encoded sample.
*/
CODEC_ERROR PutVideoLowpassHeader(ENCODER *encoder, int channel_number, BITSTREAM *stream)
{
	CODEC_STATE *codec = &encoder->codec;
	PRECISION lowpass_precision = encoder->channel[channel_number].lowpass_precision;

	// Output the subband number
	if (codec->subband_number != 0)
	{
		PutTagPair(stream, CODEC_TAG_SubbandNumber, 0);
		codec->subband_number = 0;
	}

	// Output the lowpass precision
	//if (encoder->lowpass.precision != codec->lowpass.precision)
	if (lowpass_precision != codec->lowpass_precision)
	{
		PutTagPair(stream, CODEC_TAG_LowpassPrecision, lowpass_precision);
		codec->lowpass_precision = lowpass_precision;
	}

	// Write the chunk header for the codeblock
	PushSampleSize(stream, CODEC_TAG_LargeCodeblock);

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Write the trailer for the lowpass band into the bitstream

	This routine writes a marker into the bitstream that can aid in debugging,
	but the most important function is to update the segment that contains the
	size of this subband with the actual size of the lowpass band.
*/
CODEC_ERROR PutVideoLowpassTrailer(BITSTREAM *stream)
{
	// Check that the bitstream is tag aligned before writing the pixels
	assert(IsAlignedSegment(stream));

	// Set the size of the large chunk for the lowpass band codeblock
	PopSampleSize(stream);

	return CODEC_ERROR_OKAY;
}

#if 0
/*!
	@brief Write a tag and marker before the lowpass coefficients (for debugging)

	A marker containing the coefficient start code is written into the bitstream to
	provide a unique a segment for debugging.
*/
CODEC_ERROR PutVideoLowpassMarker(BITSTREAM *stream)
{
	assert(CODEC_COEFFICIENT_START_SIZE == 16);
	PutTagMarker(stream, CODEC_COEFFICIENT_START_CODE, CODEC_COEFFICIENT_START_SIZE);
	return CODEC_ERROR_OKAY;
}
#endif

/*!
	Write the header for the highpass bands into the bitstream

	A highpass band header is written into the bitstream before the highpass bands
	for each wavelet to inform the decoder about the structure of the next wavelet.
*/
CODEC_ERROR PutVideoHighpassHeader(BITSTREAM *stream,
								   int wavelet_type,
								   int wavelet_index,
								   int wavelet_level,
								   int band_width,
								   int band_height,
								   int band_count,
								   int lowpass_scale,
								   int lowpass_divisor)
{
	return CODEC_ERROR_OKAY;
}

/*!
	@brief Write the trailer for the highpass bands into the bitstream

	This routine inserts a marker into the bitstream to identify the end of the
	highpass bands in a wavelet (for debugging).

	The most important function of this routine is to update the sample size segment
	that contains the size of the encoded highpass bands for this wavelet.
*/
CODEC_ERROR PutVideoHighpassTrailer(BITSTREAM *stream)
{
	return CODEC_ERROR_OKAY;
}

#if 0
/*!
	@brief Write the header for a highpass band (one subband) into the bitstream

	This routine writes the header for a single highpass band in a single wavelet
	into the bitstream.
*/
CODEC_ERROR PutVideoBandHeader(BITSTREAM *stream,
							   int band,
							   int width,
							   int height,
							   int subband,
							   int encoding,
							   int quantization,
							   int scale,
							   int divisor,
							   uint32_t *counters,
							   int codingflags,
							   int do_peaks)
{
#ifdef CODEC_BAND_START_CODE
	assert(CODEC_BAND_START_SIZE == 16);
	PutTagMarker(stream, CODEC_BAND_START_CODE, CODEC_BAND_START_SIZE);
#endif

	// Output the band parameters
	//PutTagPair(stream, CODEC_TAG_BAND_NUMBER, band);

#if 0
	if (codingflags) {
		PutTagPair(stream, CODEC_TAG_BAND_CODING_FLAGS, codingflags);
	}
#endif

	//PutTagPair(stream, CODEC_TAG_BAND_WIDTH, width);
	//PutTagPair(stream, CODEC_TAG_BAND_HEIGHT, height);
	PutTagPair(stream, CODEC_TAG_SubbandNumber, subband);
	//PutTagPair(stream, CODEC_TAG_BAND_ENCODING, encoding);
	PutTagPair(stream, CODEC_TAG_BAND_Quantization, quantization);
	//PutTagPair(stream, CODEC_TAG_BAND_SCALE, scale);

#if 0	//(CODEC_PROFILE > CODEC_PROFILE_BASELINE)
	if (do_peaks)
	{
		PutTagPair(stream, OPTIONALTAG(CODEC_TAG_PEAK_TABLE_OFFSET_L), 0);	// later filled in if needed
		PutTagPair(stream, OPTIONALTAG(CODEC_TAG_PEAK_TABLE_OFFSET_H), 0);	// later filled in if needed
		PutTagPair(stream, OPTIONALTAG(CODEC_TAG_PEAK_LEVEL), 0);			// later filled in if needed
	}
#endif

	// Create a sample size syntax element for this subband
	//SizeTagPush(stream, CODEC_TAG_SUBBAND_SIZE);
	PushSampleSize(stream, CODEC_TAG_SUBBAND_SIZE);

	//TODO: Explain the purpose for this tag value pair
	// was PutTagPair(output, CODEC_TAG_BAND_DIVISOR, divisor);
	//PutTagPair(stream, CODEC_TAG_BAND_HEADER, 0);

	// Must encode the counters if the encoding method is zerotree
	//assert(encoding != BAND_ENCODING_ZEROTREE || counters != NULL);

	return CODEC_ERROR_OKAY;
}
#endif
#if 0
CODEC_ERROR PutVideoCoefficientHeader(BITSTREAM *stream,
									  int band,
									  int coefficient_count,
									  int bits_per_coefficient,
									  int quantization)
{
#ifdef CODEC_COEFFICIENT_START_CODE
	// Must have some bits per coefficient unless there are no coefficients
	assert(bits_per_coefficient > 0 || coefficient_count == 0);

	PutBits(stream, CODEC_COEFFICIENT_START_CODE, CODEC_COEFFICIENT_START_SIZE);

	// Align output to next byte boundary
	//PadBits(stream);

	// Output the band number (redundant with data provided in header)
	PutBits(stream, band, CODEC_BAND_SIZE);

	// Output the number of coefficients
	PutBits(stream, coefficient_count, CODEC_COUNTER_SIZE);

	// Output the number of bits per transmitted coefficient
	PutBits(stream, bits_per_coefficient, CODEC_NUMBITS_SIZE);

	// Output the quantization divisor
	PutBits(stream, quantization, CODEC_QUANT_SIZE);
#endif

	return CODEC_ERROR_OKAY;
}
#endif
#if 0
// Append the band end codeword to the encoded coefficients
CODEC_ERROR FinishEncodeBand(BITSTREAM *stream, unsigned int code, int size)
{
#ifdef CODEC_BAND_END_CODE
	// Output the codeword that marks the end of the band coefficients
	//PutBits(stream, CODEC_BAND_END_CODE, CODEC_BAND_END_SIZE);
	PutBits(stream, code, size);
#endif

	return CODEC_ERROR_OKAY;
}
#endif
#if 0
/*!
	@brief Write the trailer for a highpass band (one subband) into the bitstream

	This routine writes a marker into the bitstream indicating the end of the subband
	(for debugging).

	This most important function of this routine is to update the sample size segment
	that contains the size of the encoded highpass subband.
*/
CODEC_ERROR PutVideoBandTrailer(BITSTREAM *stream)
{
	// Output the tag value pairs for the band trailer
	//PutTagPair(stream, CODEC_TAG_BAND_TRAILER, 0);

	// Set the size of the syntax element for the lowpass band
	//SizeTagPop(stream);
	PopSampleSize(stream);

	return CODEC_ERROR_OKAY;
}
#endif

CODEC_ERROR PutVideoSubbandHeader(ENCODER *encoder, int subband_number, QUANT quantization, BITSTREAM *stream)
{
	CODEC_STATE *codec = &encoder->codec;

	if (subband_number != codec->subband_number) {
		PutTagPair(stream, CODEC_TAG_SubbandNumber, subband_number);
		codec->subband_number = subband_number;
	}

	if (quantization != codec->band.quantization) {
		PutTagPair(stream, CODEC_TAG_Quantization, quantization);
		codec->band.quantization = quantization;
	}

	// Write the chunk header for the codeblock	
	PushSampleSize(stream, CODEC_TAG_LargeCodeblock);

	return CODEC_ERROR_OKAY;
}

CODEC_ERROR PutVideoSubbandTrailer(ENCODER *encoder, BITSTREAM *stream)
{
	// Set the size of the large chunk for the highpass band codeblock
	PopSampleSize(stream);

	return CODEC_ERROR_OKAY;
}
