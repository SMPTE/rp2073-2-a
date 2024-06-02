/*! @file common/include/syntax.h

	Declaration of bitstream elements and functions that define the syntax
	of an encoded sample.

	The information that would normally be encoded in a header is represented
	as a set of tag value pairs with a 16 bit tag and a 16 bit value.  If the
	tag is negative, then the tag is optional and may be skipped by the decoder.
	The value is usually a simple parameter such as the encoded width or height
	or the number of channels (color components).

	Some tags designate chunks of data in which case the value is the length of
	the chunk in units of segments (32-bit words).

	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _SYNTAX_H
#define _SYNTAX_H

#define CODEC_TAG_SIZE		16			//!< Size of a codec tag (in bits)
#define CODEC_TAG_MASK		0xFFFF		//!< Mask for usable part of tag or value

typedef uint32_t SEGMENT;		//!< The bitstream is a sequence of segments

typedef int16_t TAGWORD;		//!< Bitstream tag or value

//! Number of bits in a tag or value
static const BITCOUNT tagword_count = 16;

//! Number of bits in a segment (tag value pair)
static const BITCOUNT segment_count = 32;

typedef union tagvalue			//!< Bitstream tag and value pair
{
	struct {					// Fields are in the order for byte swapping
		TAGWORD value;
		TAGWORD tag;
	} tuple;					//!< Tag value pair as separate members

	uint32_t  longword;			//!< Tag value pair as a int32_t word

} TAGVALUE;

/*!
	@brief Values corresponding to the special codewords

	Special codewords are inserted into an entropy coded band to
	mark certain locations in the bitstream.  For example, the end
	of an encoded band is marked by the band end codeword.  Special
	codewords are recorded in the codebook as entries that have a
	run length of zero.  The value indicates the syntax element that
	is represented by the codeword.
*/
typedef enum _special_marker
{
	SPECIAL_MARKER_BAND_END = 1,

} SPECIAL_MARKER;


// The encoded quality is inserted into the bitstream using two tag value pairs
#define ENCODED_QUALITY_LOW_SHIFT	0			//!< Shift for the low part of the quality
#define ENCODED_QUALITY_LOW_MASK	0xFFFF		//!< Mask for the low part of the quality
#define ENCODED_QUALITY_HIGH_SHIFT	16			//!< Shift for the high part of the quality
#define ENCODED_QUALITY_HIGH_MASK	0xFFFF		//!< Mask for the high part of the quality


#ifdef __cplusplus
extern "C" {
#endif

TAGVALUE GetSegment(BITSTREAM *stream);

TAGWORD GetValue(BITSTREAM *stream, int tag);

TAGVALUE GetTagValue(BITSTREAM *stream);

TAGWORD RequiredTag(TAGWORD tag);

bool IsTagOptional(TAGWORD tag);

bool IsTagRequired(TAGWORD tag);

bool IsValidSegment(BITSTREAM *stream, TAGVALUE segment, TAGWORD tag);
//TAGVALUE GetSegment(BITSTREAM *stream);

//TAGWORD GetValue(BITSTREAM *stream, int tag);

CODEC_ERROR PutTagValue(BITSTREAM *stream, TAGVALUE segment);

// Output a tagged value with double word alignment
CODEC_ERROR PutTagPair(BITSTREAM *stream, int tag, int value);

// Output an optional tagged value
CODEC_ERROR PutTagPairOptional(BITSTREAM *stream, int tag, int value);

// Output a tag that marks a place in the bitstream for debugging
CODEC_ERROR PutTagMarker(BITSTREAM *stream, uint32_t  marker, int size);

TAGWORD OptionalTag(TAGWORD tag);

//bool IsTagOptional(TAGWORD tag);

//bool IsTagRequired(TAGWORD tag);

//bool IsValidSegment(BITSTREAM *stream, TAGVALUE segment, TAGWORD tag);

//CODEC_ERROR AlignBitsTag(BITSTREAM *stream);
CODEC_ERROR AlignBitsSegment(BITSTREAM *stream);

bool IsLowPassHeaderMarker(int marker);
bool IsLowPassBandMarker(int marker);
bool IsHighPassBandMarker(int marker);

bool IsTagValue(TAGVALUE segment, int tag, TAGWORD value);

bool IsAlignedTag(BITSTREAM *stream);

bool IsAlignedSegment(BITSTREAM *stream);

// Write an index block for the sample bands
CODEC_ERROR PutGroupIndex(BITSTREAM *stream,
						  void *index_table[],
						  int index_table_length,
						  size_t *channel_size_table_offset);

TAGWORD PackTransformPrescale(TRANSFORM *transform);

CODEC_ERROR PushSampleSize(BITSTREAM *bitstream, TAGWORD tag);

CODEC_ERROR PopSampleSize(BITSTREAM *bitstream);

CODEC_ERROR GetSampleOffsetSegment(BITSTREAM *bitstream, uint32_t offset, TAGVALUE *segment_out);

CODEC_ERROR PutSampleOffsetSegment(BITSTREAM *bitstream, uint32_t offset, TAGVALUE segment);


//TODO: Move other declarations for routines that write syntax elements here
struct _encoder ;

CODEC_ERROR PutBitstreamStartMarker(BITSTREAM *stream);

//CODEC_ERROR PutVideoGroupExtension(BITSTREAM *stream, CODEC_STATE *codec);
CODEC_ERROR PutVideoGroupExtension(struct _encoder *encoder, BITSTREAM *stream);

//CODEC_ERROR PutVideoSampleFlags(BITSTREAM *stream, CODEC_STATE *codec);
CODEC_ERROR PutVideoSampleFlags(struct _encoder *encoder, BITSTREAM *stream);

CODEC_ERROR PutFrameStructureFlags(struct _encoder *encoder, BITSTREAM *stream);

// Output marker between channel information within a group or frame
CODEC_ERROR PutVideoChannelHeader(BITSTREAM *stream, int channel);

CODEC_ERROR PutVideoChannelTrailer(BITSTREAM *stream, int channel);

CODEC_ERROR PutVideoLowpassTrailer(BITSTREAM *stream);

// Output a tag and marker before the lowpass coefficients for debugging
CODEC_ERROR PutVideoLowpassMarker(BITSTREAM *stream);

CODEC_ERROR PutVideoHighpassHeader(BITSTREAM *stream,
								   int wavelet_type,
								   int wavelet_index,
								   int wavelet_level,
								   int band_width,
								   int band_height,
								   int band_count,
								   //int lowpass_border,
								   //int highpass_border,
								   int lowpass_scale,
								   int lowpass_divisor);

#if 1
CODEC_ERROR PutVideoHighpassTrailer(BITSTREAM *stream);
#else
CODEC_ERROR PutVideoHighpassTrailer(BITSTREAM *stream,
									uint32_t cntPositive,
									uint32_t cntNegative,
									uint32_t cntZeroValues,
									uint32_t cntZeroTrees,
									uint32_t cntZeroNodes);
#endif

CODEC_ERROR PutVideoBandHeader(BITSTREAM *stream, int band, int width, int height,
							   int subband, int encoding, int quantization,
							   int scale, int divisor, uint32_t *counters, int codingflags, int do_peaks);

#if 0
CODEC_ERROR PutVideoCoefficientHeader(BITSTREAM *stream,int band, int coefficient_count,
									  int bits_per_coefficient, int quantization_divisor);
#endif

// Append the band end codeword to the encoded coefficients
//CODEC_ERROR FinishEncodeBand(BITSTREAM *stream, unsigned int code, int size);

CODEC_ERROR PutVideoBandTrailer(BITSTREAM *stream);

#ifdef __cplusplus
}
#endif

#endif
