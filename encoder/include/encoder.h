/*! @file encoder/include/encoder.h

	Declaration of the data structures and constants used for encoding.

	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _ENCODER_H
#define _ENCODER_H

/*!
	@brief Data structure for the pixel or picture aspect ratio

	@todo Should the members of the aspect ratio data structure be unsigned?
*/
typedef struct _aspect_ratio
{
	int16_t x;		//!< Numerator of the aspect ratio
	int16_t y;		//!< Denominator of the aspect ratio

} ASPECT_RATIO;

/*!
	@brief Data structure for the buffers and information used by the encoder

	The encoder data structure contains information that will be
	used by the encoder for decoding every sample in the sequence.
	Information that varies during decoding, such as the current
	subband index or the dimensions of the bands in the wavelet that
	is being decoded, is stored in the codec state.

	The encoded dimensions are the width and height of the array of pixels
	for each encoded channel (image plane), including padding added to
	satisfy the requirements of the wavelet transforms.  In the case
	of 4:2:2 sampling, the encoded width and height are for the luma channel.

	The display dimensions are the width and height of the display aperture,
	the displayable portion of the decoded image with padding removed.
	
	The display dimensions can include a row and column offset to trim
	top rows and left columns from the decoded image prior to display.

	The decoded dimensions equal the encoded dimensions at full resolution
	and are reduced by a power of two if decoded to a lower resolution.
	The decoded dimensions are derived from the encoded dimensions and the
	decoded resolution.

	The decoded dimensions are used to allocate the wavelet tree for the
	lowpass and highpass coefficients decoded from the bitstream.  It is
	not necessary to allocate wavelets for larger resolutions than the
	decoded resolution.

	For Bayer encoded images, the encoded dimensions are half the width
	and height of the input dimensions (after windowing).  Typically,
	media containers report the display dimensions as twice the encoded
	dimensions since a demosaic algorithm must be applied to produce a
	displayable image that looks right to most people.

	@todo Consider changing the transform data structure to use a
	vector of wavelets rather than a vector of wavelet pointers.
*/
typedef struct _encoder
{
	//	CODEC codec;			//!< Common fields for both the encoder and decoder

	FILE *logfile;				//!< File for writing debugging information
	CODEC_ERROR error;			//!< Error code from the most recent codec operation
	ALLOCATOR *allocator;		//!< Memory allocator used to allocate all dyynamic data
	CODEC_STATE codec;			//!< Information gathered while decoding the current sample
	VERSION version;			//!< Codec version (major, minor, revision, build)

	//! Parts of the VC-5 standard that are supported at runtime by the codec implementation
	ENABLED_PARTS enabled_parts;

	uint64_t frame_number;		//!< Every sample in a clip has a unique frame number

	//! Number of color channels in the input and encoded images
	uint_fast8_t channel_count;

	//! Number of wavelet transforms in each channel
	uint_fast8_t wavelet_count;

	//! Internal precision used by this encoder
	PRECISION internal_precision;

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
	IMAGE_FORMAT image_format;			//!< Type of the image represented by the bitstream
	DIMENSION image_width;				//!< Number of samples per row in the image represented by the bitstream
	DIMENSION image_height;				//!< Number of rows of samples in the image represented by the bitstream
	DIMENSION pattern_width;			//!< Number of samples per row in each pattern element
	DIMENSION pattern_height;			//!< Number of rows of samples in each pattern element
	DIMENSION components_per_sample;	//!< Number of components per sample in the image
	DIMENSION max_bits_per_component;	//!< Upper bound on the number of significant bits per component value
#else
	DIMENSION image_width;				//!< Upper bound on the width of each channel
	DIMENSION image_height;				//!< Upper bound on the height of each channel
#endif

#if 0   //VC5_ENABLED_PART(VC5_PART_LAYERS)
	//! Progressive frame flag
	BOOLEAN progressive;

	// Interlaced frame with the top field encoded first
	BOOLEAN top_field_first;

	// The encoded frame is upside down (not used)
	BOOLEAN frame_inverted;
#endif

    PIXEL_FORMAT pixel_format;          //!< Record the input image pixel format (for debugging)
    
	struct _channel
	{
		DIMENSION width;		//!< Width of the next channel in the bitstream
		DIMENSION height;		//!< Height of the next channel in the bitstream

		//! Precision of the component array for the next channel in the bitstream
		PRECISION bits_per_component;

		//! Number of bits per lowpass coefficient
		PRECISION lowpass_precision;

	} channel[MAX_CHANNEL_COUNT];	//!< Information about each channel

#if 0
	//! Dimensions and format of the image that was input to the encoder
	struct _input
	{
		DIMENSION width;			//!< Width of the image input to the encoder
		DIMENSION height;			//!< Height of the image input to the encoder
		PIXEL_FORMAT format;		//!< Pixel format of the image input to the encode
		//uint_fast8_t precision;	//!< Precision of the input pixels

		//! Precision of the input component arrays
		//uint_least8_t bits_per_component;

#if VC5_ENABLED_PART(VC5_PART_LAYERS)
		COLOR_SPACE color_space;	//!< Color space of YUV input images
#endif

	} input;			//!< Information about the image input to the encoder
#endif
    
#if 0
	//! Dimensions and format of the encoded image
	struct _encoded
	{
		DIMENSION width;			//!< Encoded width
		DIMENSION height;			//!< Encoded height
#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
		IMAGE_FORMAT format;		//!< Encoded format
#endif
		//! Precision of the input pixels after unpacking (encoded precision)
		uint_fast8_t precision;

	} encoded;			//!< Information about the image as represented in the bitstream
#endif

#if 0
	//! Dimensions and format of the displayable image with padding removed
	struct _display
	{
		DIMENSION width;		//!< Displayable image width
		DIMENSION height;		//!< Displayable image height
		//PIXEL_FORMAT format;	//!< Displayable image pixel format

	} display;			//!< Information about the displayable portion of the bitstream
#endif

	//! Wavelet tree for each channel
	TRANSFORM transform[MAX_CHANNEL_COUNT];

	//! Codebook to use for encoding
	CODESET *codeset;

	//! Scratch buffer for unpacking the input image
	PIXEL *unpacked_buffer[MAX_CHANNEL_COUNT];

	//! Six rows of horizontal lowpass results for each channel
	PIXEL *lowpass_buffer[MAX_CHANNEL_COUNT][6];

	//! Six rows of horizontal highpass results for each channel
	PIXEL *highpass_buffer[MAX_CHANNEL_COUNT][6];

	//! Parameter that controls the amount of rounding before quantization
	int midpoint_prequant;

#if (1 && _DEBUG)
	// Band data file and bitstream used to debug entropy coding of highpass bands
	BANDFILE encoded_band_file;
	BITSTREAM *encoded_band_bitstream;
#endif

	//! Table for the order in which channels are encoded into the bitstream
	CHANNEL channel_order_table[MAX_CHANNEL_COUNT];

	//! Number of entries in the channel order table (may be less than the channel count)
	int channel_order_count;

	struct _timing
	{
		TIMER transform;	//!< Time for wavelet transforms
		TIMER encoding;		//!< Time for entropy coding

	} timing;		//!< Timers for measuring encoder performance

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
    uint8_t image_sequence_identifier[16];      //!< UUID used for the unique image identifier
    uint32_t image_sequence_number;             //!< Number of the image in the encoded sequence
#endif

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
    COMPONENT_TRANSFORM *component_transform;
    COMPONENT_PERMUTATION *component_permutation;
#endif

#if VC5_ENABLED_PART(VC5_PART_LAYERS)
    COUNT layer_count;                          //!< Number of layers in all image sections
    bool layer_flag;
#endif
    
#if VC5_ENABLED_PART(VC5_PART_SECTIONS)
    ENABLED_SECTIONS enabled_sections;
    PATHNAME_LIST input_pathname_list;
#endif

// #if VC5_ENABLED_PART(VC5_PART_METADATA)
//     // True if metadata should be encoded into the bitsteam
//     bool metadata_flag;
// #endif

} ENCODER;

#if 0
/*!
	@brief Data structure for storing information in the channel header

	The channel header separates channels (color components) in the encoded
	sample.
*/
typedef struct _channel_header {
	int channel;			//!< Number of the next channel to decode

} CHANNEL_HEADER;
#endif


#ifdef __cplusplus
extern "C" {
#endif

CODEC_ERROR InitEncoder(ENCODER *encoder, const ALLOCATOR *allocator, const VERSION *version);

//TAGWORD PackedEncoderVersion(ENCODER *encoder);

CODEC_ERROR CodecErrorBitstream(BITSTREAM_ERROR error);

CODEC_ERROR PrepareEncoder(ENCODER *encoder,
						   const UNPACKED_IMAGE *image,
						   ALLOCATOR *allocator,
						   const PARAMETERS *parameters,
                           int input_image_index);

CODEC_ERROR PrepareEncoderState(ENCODER *encoder,
								const UNPACKED_IMAGE *image,
								const PARAMETERS *parameters,
                                int input_image_index);

CODEC_ERROR SetInputChannelFormats(ENCODER *encoder, PARAMETERS *parameters);

CODEC_ERROR ReleaseEncoder(ENCODER *encoder);

CODEC_ERROR ReleaseEncoderTransforms(ENCODER *encoder, ALLOCATOR *allocator);

CODEC_ERROR AllocEncoderTransforms(ENCODER *encoder);

CODEC_ERROR AllocEncoderBuffers(ENCODER *encoder);

CODEC_ERROR EncodeImage(IMAGE *image, STREAM *stream, const PARAMETERS *parameters);

CODEC_ERROR EncodingProcess(ENCODER *encoder,
							const UNPACKED_IMAGE *image,
							BITSTREAM *stream,
							const PARAMETERS *parameters);

CODEC_ERROR EncodeSingleImage(ENCODER *encoder, const UNPACKED_IMAGE *image, BITSTREAM *stream);

CODEC_ERROR EncodeSingleChannel(ENCODER *encoder, void *buffer, size_t pitch, BITSTREAM *stream);

CODEC_ERROR PrepareEncoderTransforms(ENCODER *encoder);

CODEC_ERROR ImageUnpackingProcess(const PACKED_IMAGE *packed_image,
								  UNPACKED_IMAGE *unpacked_image,
								  const PARAMETERS *parameters,
								  ALLOCATOR *allocator);

CODEC_ERROR UnpackImage(const PACKED_IMAGE *input, UNPACKED_IMAGE *output, ENABLED_PARTS enabled_parts);

CODEC_ERROR UnpackImageRow(uint8_t *input_row_ptr,
						   DIMENSION image_width,
						   PIXEL_FORMAT pixel_format,
						   PIXEL *output_row_ptr[],
						   PRECISION bits_per_component[],
						   int channel_count,
						   ENABLED_PARTS enabled_parts);

CODEC_ERROR EncodeBitstreamHeader(ENCODER *encoder, BITSTREAM *bitstream);

CODEC_ERROR EncodeBitstreamTrailer(ENCODER *encoder, BITSTREAM *bitstream);

CODEC_ERROR EncodeExtensionHeader(ENCODER *encoder, BITSTREAM *bitstream);

CODEC_ERROR EncodeExtensionTrailer(ENCODER *encoder, BITSTREAM *bitstream);

CODEC_ERROR EncodeMultipleChannels(ENCODER *encoder, const UNPACKED_IMAGE *image, BITSTREAM *stream);

    
#if VC5_ENABLED_PART(VC5_PART_LAYERS) || VC5_ENABLED_PART(VC5_PART_SECTIONS)
CODEC_ERROR EncodeImageList(IMAGE_LIST *image_list, STREAM *stream, const PARAMETERS *parameters);

CODEC_ERROR ImageListUnpackingProcess(PACKED_IMAGE_LIST *packed_image_list,
                                      UNPACKED_IMAGE_LIST *unpacked_image_list,
                                      const PARAMETERS *parameters,
                                      ALLOCATOR *allocator);

CODEC_ERROR ImageListEncodingProcess(ENCODER *encoder,
                                     UNPACKED_IMAGE_LIST *unpacked_image_list,
                                     BITSTREAM *bitstream,
                                     const PARAMETERS *parameters);
#endif

#if VC5_ENABLED_PART(VC5_PART_LAYERS)
CODEC_ERROR EncodeImageLayers(ENCODER *encoder, const UNPACKED_IMAGE_LIST *image_list, BITSTREAM *stream);

CODEC_ERROR EncodeLayerHeader(ENCODER *encoder, BITSTREAM *bitstream, COUNT layer_number);

CODEC_ERROR EncodeLayerTrailer(ENCODER *encoder, BITSTREAM *bitstream);
#endif
    
#if VC5_ENABLED_PART(VC5_PART_SECTIONS)
CODEC_ERROR EncodeImageSections(ENCODER *encoder,
                                const UNPACKED_IMAGE_LIST *image_list,
                                BITSTREAM *stream,
                                const PARAMETERS *parameters);
#endif

#if VC5_ENABLED_PART(VC5_PART_LAYERS) && VC5_ENABLED_PART(VC5_PART_SECTIONS)
CODEC_ERROR EncodeImageSectionLayers(IMAGE_LIST *image_list, STREAM *stream, const PARAMETERS *parameters);

CODEC_ERROR ImageSectionLayersEncodingProcess(ENCODER *encoder,
                                              UNPACKED_IMAGE_LIST *unpacked_image_list,
                                              BITSTREAM *bitstream,
                                              const PARAMETERS *parameters);
#endif

//CODEC_ERROR SetTransformParameters(ENCODER *encoder);

CODEC_ERROR SetEncoderQuantization(ENCODER *encoder,
								   const PARAMETERS *parameters);

CODEC_ERROR SetTransformQuantTable(ENCODER *encoder, int channel, const QUANT table[], int length);

CODEC_ERROR GetChannelDimensions(ENCODER *encoder,
								 int channel_number,
								 DIMENSION *channel_width_out,
								 DIMENSION *channel_height_out);

DIMENSION ChannelWidth(ENCODER *encoder, int channel_index, DIMENSION width);

CODEC_ERROR GetMaximumChannelDimensions(const UNPACKED_IMAGE *image, DIMENSION *width_out, DIMENSION *height_out);

DIMENSION EncodedLayerHeight(ENCODER *encoder, DIMENSION height);

CODEC_ERROR SetEncodedBandMask(CODEC_STATE *codec, int subband);

CODEC_ERROR EncodeChannelSubbands(ENCODER *encoder, int channel, BITSTREAM *stream);

CODEC_ERROR EncodeChannelHeader(ENCODER *encoder,
								int channel_number,
								BITSTREAM *stream);

CODEC_ERROR EncodeChannelTrailer(ENCODER *encoder, int channel, BITSTREAM *stream);

//CODEC_ERROR EncodeLayerChannels(ENCODER *encoder, BITSTREAM *stream);
CODEC_ERROR EncodeChannelWavelets(ENCODER *encoder, BITSTREAM *stream);

CODEC_ERROR PutVideoLowpassHeader(ENCODER *encoder, int channel_number, BITSTREAM *stream);

CODEC_ERROR PutVideoSubbandHeader(ENCODER *encoder, int subband, QUANT quantization, BITSTREAM *stream);
CODEC_ERROR PutVideoSubbandTrailer(ENCODER *encoder, BITSTREAM *stream);

CODEC_ERROR TransformForwardSpatialQuantFrame(ENCODER *encoder, void *buffer, size_t pitch);

CODEC_ERROR AllocateEncoderHorizontalBuffers(ENCODER *encoder, int buffer_width);

CODEC_ERROR DeallocateEncoderHorizontalBuffers(ENCODER *encoder);

CODEC_ERROR AllocateEncoderUnpackingBuffers(ENCODER *encoder, int frame_width);

CODEC_ERROR DeallocateEncoderUnpackingBuffers(ENCODER *encoder);

CODEC_ERROR AllocateHorizontalBuffers(ALLOCATOR *allocator,
									  PIXEL *lowpass_buffer[],
									  PIXEL *highpass_buffer[],
									  int buffer_width);

CODEC_ERROR DeallocateHorizontalBuffers(ALLOCATOR *allocator,
										PIXEL *lowpass_buffer[],
										PIXEL *highpass_buffer[]);

//CODEC_ERROR UnpackImageRow(ENCODER *encoder, uint8_t *input_row_ptr);

CODEC_ERROR ShiftHorizontalResultBuffers(ENCODER *encoder);

CODEC_ERROR ShiftHorizontalBuffers(PIXEL *lowpass[], PIXEL *highpass[]);

CODEC_ERROR TransformForwardSpatialChannel(ENCODER *encoder, const UNPACKED_IMAGE *image, int channel_number);

CODEC_ERROR TransformForwardSpatialLowpass(ENCODER *encoder, WAVELET *input, WAVELET *output, int prescale);

CODEC_ERROR PadWaveletBands(ENCODER *encoder, WAVELET *wavelet);

CODEC_ERROR EncodeLowpassBand(ENCODER *encoder, WAVELET *wavelet, int channel_number, BITSTREAM *stream);

CODEC_ERROR EncodeHighpassBand(ENCODER *encoder, WAVELET *wavelet, int band, int subband, BITSTREAM *stream);

CODEC_ERROR EncodeHighpassBandLongRuns(BITSTREAM *stream, CODESET *codeset, PIXEL *data,
									   DIMENSION width, DIMENSION height, DIMENSION pitch);

CODEC_ERROR EncodeHighpassBandRowRuns(BITSTREAM *stream, CODESET *codeset, PIXEL *data,
									  DIMENSION width, DIMENSION height, DIMENSION pitch);

CODEC_ERROR DumpEncodedSubbands(ENCODER *encoder, int channel_index,
								uint32_t subband_mask, const char *pathname);

CODEC_ERROR DumpWaveletBands(ENCODER *encoder, int channel_index, int wavelet_index,
							 uint32_t wavelet_band_mask, const char *pathname);

CODEC_ERROR DumpTransformBands(ENCODER *encoder, uint32_t channel_mask, uint32_t wavelet_mask,
							   uint32_t wavelet_band_mask, const char *pathname);

CODEC_ERROR CreateEncodedBandFile(ENCODER *encoder, const char *pathname);

CODEC_ERROR CloseEncodedBandFile(ENCODER *encoder);

CODEC_ERROR EncodeMetadataChunk(ENCODER *encoder, BITSTREAM *bitstream, const PARAMETERS *parameters);

//CODEC_ERROR WriteMetadataFile(ENCODER *encoder, BITSTREAM *bitstream, FILE *metadata_file);

#ifdef __cplusplus
}
#endif

#endif
