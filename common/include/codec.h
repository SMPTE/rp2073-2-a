/*!	@file common/include/codec.h

	State of the decoder while decoding a sample.

	The codec state contains information about the current state of the
	decoding process.  The codec state is updated as the bitstream is decoded.

	The encoder process maintains the codec state during encoding to mimic the
	codec state in the decoder.  It is not necessary to encode parameters into
	the bitstream if the decoding process will automatically determine the same
	information.  For example, after decoding a subband, the decoder will
	increment the subband number, so the encoder does not have to write the
	next subband number in the bitstream if the subbands are transmitted in
	numerical order.

	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _CODEC_H
#define _CODEC_H

static const SEGMENT StartMarkerSegment = ((0x56 << 24) | (0x43 << 16) | (0x2D << 8) | 0x35);

/*!
	@brief Tags that define elements in the bitstream

	All syntax elements in the encoded bitstream begin with a 16-bit tag that
	specifies the type of element.  The 16-bit tag is followed by a 16-bit value,
	forming a tag-value pair.

	If the tag is a negative number, then the actual tag is the negation of the
	tag value and the negative sign indicates that the tag and its value are an
	optional tag value pair.  If the tag is a positive value, then the segment
	is required.  A decoder must be able to decode all required tag-value pairs,
	but can skip tag-value pairs that are optional.

	In a VC-5 Part 1 bitstream, the image width and height are an upper bound on the
	dimensions of each channel represented in the bitstream.  In a VC-5 Part 3 bitstream,
	the image width and height are the actual dimensions of the image represented in the
	bitstream.  The width and height of the image and each pattern element is sufficient
	to determine the width and height of each component array.

	A range of tags is reserved for chunks and the value is the size of the chunk.
*/
typedef enum _codec_tag
{
	CODEC_TAG_ImageWidth = 20,			//!< Upper bound on the width of the image
	CODEC_TAG_ImageHeight = 21,			//!< Upper bound on the height of the image
	CODEC_TAG_BitsPerComponent = 101,	//!< Number of bits in the source image
	CODEC_TAG_ChannelCount = 12,		//!< Number of channels in the transform
	CODEC_TAG_SubbandCount = 14,		//!< Number of encoded subbands
	CODEC_TAG_ChannelNumber = 62,		//!< Channel number
	CODEC_TAG_SubbandNumber = 48,		//!< Subband number of this wavelet band
	CODEC_TAG_LowpassPrecision = 35,	//!< Number of bits per lowpass coefficient
	CODEC_TAG_Quantization = 53,		//!< Quantization applied to band
	CODEC_TAG_PrescaleShift = 109,		//!< Packed prescale shift for each wavelet level
	CODEC_TAG_ChannelWidth = 104,		//!< Width of the next channel in the bitstream
	CODEC_TAG_ChannelHeight = 105,		//!< Height of the next channel in the bitstream

#if _DEBUG
	// Optional tag-value pairs used for debugging
	CODEC_TAG_PixelFormat = 1001,		//!< Pixel fomrat of the packed input image
#endif

	CODEC_TAG_LargeCodeblock = 0x6000,	//!< Large chunk that contains a codeblock

	CODEC_TAG_SMALL_CHUNK = 0x4000,		//!< Small chunk with a 16-bit payload size (in segments)
	CODEC_TAG_LARGE_CHUNK = 0x2000,		//!< Large chunk with a 24-bit payload size (in segments)

	//! Mask for detecting the tag for a small or large chunk (including codeblocks)
	CODEC_TAG_CHUNK_MASK = (CODEC_TAG_SMALL_CHUNK | CODEC_TAG_LARGE_CHUNK),

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
	// Codec tags used by VC-5 Part 3 bitstreams
	CODEC_TAG_PatternWidth = 106,				//!< Number of samples per row in each pattern element
	CODEC_TAG_PatternHeight = 107,				//!< Number of rows of samples in each pattern element
	CODEC_TAG_ComponentsPerSample = 108,		//!< Number of components in each sample in the pattern element
	CODEC_TAG_ImageFormat = 84,					//!< Format of the image represented by the bitstream
	CODEC_TAG_MaxBitsPerComponent = 102,		//!< Upper bound on the number of bits per component

	// Small chunk elements defined by VC-5 Part 3
	CODEC_TAG_VendorSpecificData = 0x4000,		//!< Small chunk containing vendor-specific data
	CODEC_TAG_InversePermutation = 0x4001,		//!< Small chunk containing the inverse component permutation
	CODEC_TAG_InverseTransform = 0x4002,		//!< Small chunk containing the inverse component transform (8 bit representation)
	CODEC_TAG_InverseTransform16 = 0x4003,		//!< Small chunk containing the inverse component transform (16 bit representation)
	CODEC_TAG_UniqueImageIdentifier = 0x4004,	//!< Small chunk containing the identifier and sequence number for the image
#endif

#if VC5_ENABLED_PART(VC5_PART_LAYERS)
	CODEC_TAG_LayerCount = 120,					//!< Number of layers in the bitstream
	CODEC_TAG_LayerNumber = 121,				//!< Number of the next layer in the bitstream
	CODEC_TAG_LayerPattern = 122,				//!< Mask indicating the use cases in the bitstream
#endif

#if VC5_ENABLED_PART(VC5_PART_SECTIONS)
	CODEC_TAG_ImageCount = 130,					//!< Number of image bitstream sections in the bitstream
	CODEC_TAG_ImageNumber = 131,				//!< Unique number assigned to an image bitstream section
    
    // Predefined codec tags for structures in the VC-5 bitstream
    CODEC_TAG_ImageSectionTag = 0x2700,         //!< Section that contains a single image
    CODEC_TAG_HeaderSectionTag = 0x2500,        //!< Section that contains the bitstream header
    CODEC_TAG_LayerSectionTag = 0x2600,         //!< Section that contains a single layer
    CODEC_TAG_ChannelSectionTag = 0x2400,       //!< Section that contains a single channel
    CODEC_TAG_WaveletSectionTag = 0x2100,       //!< Section that contains all subbands for one wavelet
    CODEC_TAG_SubbandSectionTag = 0x2000,       //!< Section that contains a single subband
#endif

#if VC5_ENABLED_PART(VC5_PART_METADATA)
	// Small and large chunks of metadata
	CODEC_TAG_SmallMetadata = 0x4010,			//!< Small chunk containing metadata tuples (VC-5 Part 7)
	CODEC_TAG_LargeMetadata = 0x6100,			//!< Large chunk containing metadata tuples (VC-5 Part 7)
#endif

} CODEC_TAG;

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
/*!
	@brief Format of the encoded sample

	The VC-5 Part 3 can support four bitstream representations of the encoded image.

	The image format must be specified for any VC-5 Part 3 bitstream.
*/
typedef enum _image_format
{
	IMAGE_FORMAT_UNKNOWN = 0,		//!< The image format has not been specified
	IMAGE_FORMAT_RGBA,				//!< RGB image with optional alpha channel
	IMAGE_FORMAT_YCbCrA,			//!< YCbCr image with optional alpha channel
	IMAGE_FORMAT_BAYER,				//!< Bayer image format (special case of CFA)
	IMAGE_FORMAT_CFA,				//!< Generic color filter array (CFA) image


	/***** Add new encoded formats above this line *****/

	IMAGE_FORMAT_COUNT,				//!< Number of image formats that have been defined

} IMAGE_FORMAT;

#endif

#if VC5_ENABLED_PART(VC5_PART_SECTIONS)

/*
 @brief Enumeration of the predefined section numbers
 
 The predefined section numbers are defined in ST 2073-2.
 */
typedef enum _section_number
{
    SECTION_NUMBER_IMAGE = 1,       //!< Image section
    SECTION_NUMBER_HEADER = 2,      //!< Bitstream header section
    SECTION_NUMBER_LAYER = 3,       //!< Layer section
    SECTION_NUMBER_CHANNEL = 4,     //!< Channel section
    SECTION_NUMBER_WAVELET = 5,     //!< Wavelet section
    SECTION_NUMBER_SUBBAND = 6,     //!< Subband section
    
    //TODO: Add more section number definitions as required
    
    //! Modify the smallest and largest section numbers as more sections are added
    SECTION_NUMBER_MINIMUM = SECTION_NUMBER_IMAGE,
    SECTION_NUMBER_MAXIMUM = SECTION_NUMBER_SUBBAND,
    
} SECTION_NUMBER;

/*
 @Macro for creating a section number bit mask from a section number
 
 The macro does not check that the section number argument is valid.
 */
#define SECTION_NUMBER_MASK(section_number)  (1 << (section_number - 1))

/*
 @brief Data type for the bit mask that represents enabled sections
 
 The bit mask indicates which section numbers defined in ST 2073-2 are enabled
 at runtime.
 */
typedef uint32_t ENABLED_SECTIONS;

#endif

/*!
	@brief Band encoding method

	Several different schemes have been tried for entropy coding the highpass bands.
	The baseline profile only supports the run lengths encoding method.

	The run lengths encoding method using a Huffman code to encode runs of zeros and
	highpass coefficient magnitudes (unsigned).  Runs of zeros can extend across row
	boundaries, so large sections of a highpass band that are mostly zeros can be
	encoded very efficiently.

	@todo Need to cull this list as many band encoding methods are no longer supported.
*/
enum band_encoding {
	BAND_ENCODING_ZEROTREE = 1,
	BAND_ENCODING_CODEBOOK,
	BAND_ENCODING_RUNLENGTHS,
	BAND_ENCODING_16BIT,
	BAND_ENCODING_LOSSLESS
};

/*!
	The codec state contains information about the decoding process obtained
	as a sample is decoded.  The information is transient and is only used
	while decoding a sample.  The decoder data structure contains information
	that should persist from onen sample to the next.
	
	The codec state is initialized using information in the decoder data structure
	at the start of decoding a sample.

	The intent is that the encoder can operate the same state machine during encoding
	and any information available in the state machine does not have to be encoded into
	the sample as it is assumed that the decoder can and will derive the same information.
	For example, the dimensions of the first subband can be computed from the encoded
	dimensions and the number of wavelet levels, so it is not necessary to encode this
	information.  Likewise, after the last band in a wavelet is decoded the dimensions
	of the bands in the wavelet at the next level can be deduced and it is not necessary
	to encode this information into the sample.
*/
typedef struct _codec_state
{
	uint16_t channel_number;		//!< Index of current channel being decoded
	DIMENSION channel_width;		//!< Width of the next channel in the bitstream
	DIMENSION channel_height;		//!< Height of the next channel in the bitstream
	PRECISION bits_per_component;	//!< Precision of the component array (in bits)

	uint16_t subband_number;		//!< Index of current subband being decoded

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
	IMAGE_FORMAT image_format;			//!< Format of the image represented by the bitstream
	DIMENSION pattern_width;			//!< Width of the pattern elements (in samples)
	DIMENSION pattern_height;			//!< Height of the pattern elements (in rows)
	DIMENSION components_per_sample;	//!< Number of components in each sample in the pattern element
	PRECISION max_bits_per_component;	//!< Maximum number of bits for each value in the component arrays
#endif

#if VC5_ENABLED_PART(VC5_PART_LAYERS)
	COUNT layer_count;                  //!< Number of layers in the bitstream
    COUNT layer_number;                 //!< Number of the next layer in the bitstream
    uint16_t layer_pattern;             //!< Indicates the use case for layers in the bitstream
    uint32_t decoded_layer_mask;        //!< Indicates which layers have been decoded
#endif

	uint_least8_t channel_count;	//!< Number of channels in the current layer
	uint_least8_t wavelet_count;	//!< Number of wavelets in the current layer
	uint_least8_t subband_count;	//!< Number of suibbands in the current layer

	//! The channel position is used for skipping subbands and jumping to particular channels
	size_t channel_position;

	uint32_t encoded_format;			//!< Internal encoded representation
	uint32_t encoded_quality;			//!< Quality setting of the encoded video

	uint32_t decoded_subband_mask;		//!< Indicates which subbands have been decoded

	bool progressive;					//!< True if the encoded frame is progressive

	bool top_field_first;				//!< True if the top field is encoded first

	bool frame_inverted;				//!< True if the frame is encoded upside down

	uint_least8_t group_length;			//!< Number of frames in a group of pictures (GOP)

#if 0
	uint8_t active_codebook;			//!< Non-zero value indicates which codebook is used
	bool difference_coding;				//!< Enables or disables differential coding
#endif

	//! Indicates that enough of the sample has been read to allow decoding the sample
	bool end_of_sample;

	//! Indicates that the layer has been decoded
	bool end_of_layer;

	//! Most recent tag-value pair was a header parameter
	bool header;

	//! Most recent syntax element was a codeblock (large chunk element)
	bool codeblock;

	//! Parameters of the most recently decoded subband
	struct
	{
		//DIMENSION width;				//!< Width of the decoded band
		//DIMENSION height;				//!< Height of the decoded band
		uint_least8_t subband;			//!< Subband index
		//uint_least8_t encoding;		//!< Band encoding method
		uint16_t quantization;			//!< Quantization parameter

	} band;			//!< Information about the current highpass band

	DIMENSION image_width;			//!< Upper bound on the channel width
	DIMENSION image_height;			//!< Upper bound on the channel height

	PRECISION lowpass_precision;	//!< Number of bits per lowpass coefficient

#if _DEBUG
	//! Information about the packed image input to the encoder (for debugging)
	struct
	{
		PIXEL_FORMAT format;			//!< Pixel format of the input image

	} input;
#endif

	/*!
		@brief Table of prescale shifts applied before computing each wavelet transform

		The prescale shift was applied by the encoder to each input to the forward
		wavelet transform.  The table of prescale values is indexed by the same
		index used for the wavelets in the transform.
	*/
	//uint_fast8_t prescale_table[MAX_WAVELET_COUNT];
	PRESCALE prescale_table[MAX_WAVELET_COUNT];

	//! Picture aspect ratio read from the encoded sample
	struct _picture_aspect_ratio
	{
		uint_least16_t x;		//!< Relative width of the picture
		uint_least16_t y;		//!< Relative height of the picture

	} picture_aspect_ratio;		//!< Picture aspect ratio read from the sample

#if VC5_ENABLED_PART(VC5_PART_LAYERS)
	//uint32_t interlaced_flags;
	//uint32_t protection_flags;

	//!< Parameters of the current layer
	struct
	{
		int width;			//!< Width of the current layer
		int height;			//!< Height of the current layer

	} layer;		//!< Information about the encoded layer from the sample

#endif
    
#if VC5_ENABLED_PART(VC5_PART_SECTIONS)
    int section_number;     //!< Number of the most recent section encountered in the bitstream
    int section_length;     //!< Length of the most recent section element payload (in segments)
#endif

} CODEC_STATE;

#if 0
/*!
	@brief Fields that are common to the encoder and decoder
*/
typedef struct _codec
{
	FILE *logfile;				//!< File for writing debugging information
	CODEC_ERROR error;			//!< Error code from the most recent codec operation
	ALLOCATOR *allocator;		//!< Memory allocator used to allocate all dyynamic data
	CODEC_STATE codec;			//!< Information gathered while decoding the current sample
	VERSION version;			//!< Codec version (major, minor, revision, build)

	//! Parts of the VC-5 standard that are supported at runtime by the codec implementation
	ENABLED_PARTS enabled_parts;

} CODEC;
#endif

// Initialize the codec state
CODEC_ERROR PrepareCodecState(CODEC_STATE *codec);

uint32_t EncoderVersion(uint32_t value);

uint32_t RepackedEncoderVersion(uint32_t value);

void SetCodecVersion(uint8_t version[3], uint16_t value);

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
IMAGE_FORMAT DefaultImageFormat(PIXEL_FORMAT pixel_format);
#endif

int EncodedPrecision(PIXEL_FORMAT format);

int InputPrecision(PIXEL_FORMAT format);

CODEC_ERROR UpdatePrescaleTable(CODEC_STATE *codec, TAGWORD value);

CODEC_ERROR UpdateSampleFlags(CODEC_STATE *codec, TAGWORD value);

CODEC_ERROR UpdateCodecFlags(CODEC_STATE *codec, TAGWORD value);

CODEC_ERROR UpdateFrameStructureFlags(CODEC_STATE *codec, TAGWORD value);

int LowpassChannelOffset(CODEC_STATE *codec, PIXEL_FORMAT output_format);

CODEC_ERROR SetBandCoding(CODEC_STATE *codec, TAGWORD value);

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
const char *ImageFormatString(IMAGE_FORMAT image_format);
#endif

bool IsPartEnabled(ENABLED_PARTS enabled_parts, int part_number);

#if VC5_ENABLED_PART(VC5_PART_SECTIONS)
bool IsSectionEnabled(ENABLED_SECTIONS enabled_sections, SECTION_NUMBER section_number);
bool IsImageSectionEnabled(ENABLED_PARTS enabled_parts, ENABLED_SECTIONS enabled_sections);
#endif

#endif
