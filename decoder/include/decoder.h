/*! @file decoder/include/decoder.h

	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _DECODER_H
#define _DECODER_H


/*!
	Data structure for the buffers and information used by
	the decoder.

	The decoder data structure contains information that will be
	used by the decoder for decoding every sample in the sequence.
	Information that varies during decoding, such as the current
	subband index or the dimensions of the bands in the wavelet that
	is being decoded, is stored in the codec state.

	@todo Consider changing the transform data structure to use a
	vector of wavelets rather than a vector of wavelet pointers.
	
	@todo Remove unused substructures

	@todo Dynamically allocate the vector of wavelet trees based on the
	actual number of channels rather than the maximum channel count.
	
	@todo Need to handle the cases where header parameters are provided
	by the application instead of being in the bitstream.
*/
typedef struct _decoder
{
	CODEC_ERROR error;			//!< Error code from the most recent codec operation
	ALLOCATOR *allocator;		//!< Memory allocator used to allocate all dyynamic data
	CODEC_STATE codec;			//!< Information gathered while decoding the current sample

	bool verbose_flag;			//!< Control the output of status messages
	bool debug_flag;			//!< Control extra output for debugging
	bool quiet_flag;			//!< Suppress all output to the terminal

	//! Parts of the VC-5 standard that are supported at runtime by the codec implementation
	ENABLED_PARTS enabled_parts;

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
	uint64_t frame_number;		//!< Every sample in a clip has a unique frame number
#endif

#if _DEBUG
	FILE *logfile;				//!< Output file for debugging information
#endif

	uint16_t header_mask;		//!< Track which header parameters have been decoded
	bool header_finished;		//!< Finished decoding the bitstream header?
	bool memory_allocated;		//!< True if memory for decoding has been allocated
	
	//! Dimensions of each channel found in the bitstream
	struct _channel
	{
		DIMENSION width;				//!< Width of this channel
		DIMENSION height;				//!< Height of this channnel

		//! Bits per component for the component array corresponding to this channel
		uint_least8_t bits_per_component;

		bool initialized;				//!< Have the channel dimensions been initialized?
        
        bool found_first_codeblock;     //!< Has the first codeblock in the channel been found?

	} channel[MAX_CHANNEL_COUNT];	//!< Information about each channel in the bitstream

#if _DEBUG
	//! Dimensions and format of the image that was input to the encoder
	struct _input
	{
		DIMENSION width;		//!< Width of the frame input to the encoder
		DIMENSION height;		//!< Height of the frame input to the encoder
		PIXEL_FORMAT format;	//!< Pixel format of the frame input to the encoder

	} input;			//!< Information about the frame input to the encoder
#endif

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
	//! Dimensions and format of the encoded image
	struct _encoded
	{
		DIMENSION width;		//!< Encoded width
		DIMENSION height;		//!< Encoded height
		IMAGE_FORMAT format;	//!< Encoded format

	} encoded;			//!< Information about the image as represented in the bitstream
#endif

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
	//! Dimensions and format of the decoded image
	struct _decoded
	{
		DIMENSION width;		//!< Decoded width
		DIMENSION height;		//!< Decoded height
		//RESOLUTION resolution;

	} decoded;			//!< Information about the decoded component arrays
#endif

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
	//! Dimensions and format of the frame after post-processing (see @ref ImageRepackingProcess)
	struct _output
	{
		DIMENSION width;		//!< Output frame width
		DIMENSION height;		//!< Output frame height
		PIXEL_FORMAT format;	//!< Output frame pixel format

	} output;			//!< Information about the packed image output by the image repacking process
#endif

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
	//! Dimensions and format of the image output by the display process
	struct _display
	{
		DIMENSION width;		//!< Output frame width
		DIMENSION height;		//!< Output frame height
		PIXEL_FORMAT format;	//!< Output frame pixel format

	} display;			//!< Information about the displayable image output by the display process
#endif

#if VC5_ENABLED_PART(VC5_PART_LAYERS)
    bool decode_all_layers_flag;    //!< True if decoding all layers in the bitstream
	COUNT decoded_layer_count;      //!< Number of layers that have been decoded
#endif

	int wavelet_count;			//!< Number of wavelets in each channel

	//! Wavelet tree for each channel
	TRANSFORM transform[MAX_CHANNEL_COUNT];

	//! Pointer to the active codebook for variable-length codes
	CODEBOOK *codebook;

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
    uint8_t image_sequence_identifier[16];      //!< UUID for the unique image sequence identifier
    uint32_t image_sequence_number;             //!< Number of the image in the image sequence
#endif
    
#if 0   //VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
    COMPONENT_TRANSFORM *component_transform;
    COMPONENT_PERMUTATION *component_permutation;
#endif
    
#if VC5_ENABLED_PART(VC5_PART_LAYERS)
	//bool progressive;			//!< True if the encoded frame is progressive
	//bool top_field_first;		//!< True if the top field is encoded first
#endif
    
#if VC5_ENABLED_PART(VC5_PART_SECTIONS)
    //TODO: Move sections fields into a sections struct (like metadata below)
    // struct _sections
    // {
    ENABLED_SECTIONS enabled_sections;      //!< Control whether section processing is enabled
    bool image_section_flag;                //!< True if an image section was encountered in the bitstream
    FILE *section_logfile;                  //!< Log file for writing section information

	// } sections;
#endif


#if VC5_ENABLED_PART(VC5_PART_METADATA)
    struct _metadata
    {
	    DATABASE *database;						//!< Metadata database used by the decoder
	    char output_pathname[PATH_MAX];			//!< Output filename for the metadata

	} metadata;
#endif

} DECODER;

/*!
	@brief Information that can be obtained from an bitstream header

	The bitstream header consists of tag-value pairs that must occur in the bitstream
	before the first codeblock if the parameters are present in the bitstream.  The
	routine @ref ParseBitstreamHeader decodes the bitstream up to but not including the
	first codeblock in the bitstream.

	Consider organizing the values obtained from the bitstream into input parameters
	and encoded parameters, as is done elsewhere in the decoder, even though the
	bitstream does not have such a rigid syntax.
*/
typedef struct _bitstream_header
{
	uint16_t channel_count;			//!< Number of channels in the bitstream

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
	uint64_t frame_number;			//!< Every sample in a clip has a unique frame number
	PIXEL_FORMAT input_format;		//!< Pixel format of the frame input to the encoder

	// Encoded dimensions and format of the encoded frame (including padding)
	DIMENSION encoded_width;			//!< Width of the encoded frame
	DIMENSION encoded_height;			//!< Height of the encoded frame

	IMAGE_FORMAT encoded_format;		//!< Encoded format

	// The display aperture within the encoded frame
	DIMENSION row_offset;
	DIMENSION column_offset;
	DIMENSION display_width;			//!< Width of the displayable frame (if specified in the sample)
	DIMENSION display_height;			//!< Height of the displayable frame (if specified in the sample)
#endif
#if VC5_ENABLED_PART(VC5_PART_LAYERS)
	DIMENSION video_channel_count;		// Number of layers?
	DIMENSION current_video_channel;	//TODO: Find better way to handle this
	int layer_count;					//!< Number of layers in the sample
	bool progressive;					//!< Progressive versus interlaced frames
	bool top_field_first;				//!< Interlaced frame with top field first
#endif

} BITSTREAM_HEADER;

//! Flags that indicate which header parameters have been assigned values
typedef enum _bitstream_header_flags
{
	BITSTREAM_HEADER_FLAGS_IMAGE_WIDTH = (1 << 0),
	BITSTREAM_HEADER_FLAGS_IMAGE_HEIGHT = (1 << 1),
	BITSTREAM_HEADER_FLAGS_CHANNEL_COUNT = (1 << 2),
	BITSTREAM_HEADER_FLAGS_SUBBAND_COUNT = (1 << 3),

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
	BITSTREAM_HEADER_FLAGS_IMAGE_FORMAT = (1 << 4),
	BITSTREAM_HEADER_FLAGS_PATTERN_WIDTH = (1 << 5),
	BITSTREAM_HEADER_FLAGS_PATTERN_HEIGHT = (1 << 6),
	BITSTREAM_HEADER_FLAGS_COMPONENTS_PER_SAMPLE = (1 << 7),
	BITSTREAM_HEADER_FLAGS_MAX_BITS_PER_COMPONENT = (1 << 8),

	//! Required header parameters
	BITSTREAM_HEADER_FLAGS_REQUIRED = (BITSTREAM_HEADER_FLAGS_IMAGE_WIDTH |
									   BITSTREAM_HEADER_FLAGS_IMAGE_HEIGHT |
									   BITSTREAM_HEADER_FLAGS_IMAGE_FORMAT |
									   BITSTREAM_HEADER_FLAGS_PATTERN_WIDTH |
									   BITSTREAM_HEADER_FLAGS_PATTERN_HEIGHT |
									   BITSTREAM_HEADER_FLAGS_COMPONENTS_PER_SAMPLE),
#else

	//! Required header parameters
	BITSTREAM_HEADER_FLAGS_REQUIRED = (BITSTREAM_HEADER_FLAGS_IMAGE_WIDTH |
									   BITSTREAM_HEADER_FLAGS_IMAGE_HEIGHT),
#endif

} BITSTREAM_HEADER_FLAGS;

#if 0
/*!
	@brief Data structure for storing information in the channel header

	The channel header separates channels (color components) in the encoded
	sample.
*/
typedef struct _channel_header
{
	int channel;			//!< Number of the next channel to decode

} CHANNEL_HEADER;
#endif

//! Define an error code for bitstream end of file
#define BITSTREAM_ERROR_EOF (BITSTREAM_ERROR_STREAM | STREAM_ERROR_EOF)

#ifdef __cplusplus
extern "C" {
#endif

CODEC_ERROR InitDecoder(DECODER *decoder, ALLOCATOR *allocator);

CODEC_ERROR InitMetadataDatabase(DATABASE **database_out, const PARAMETERS *parameters, bool duplicates_flag);

CODEC_ERROR SetDecoderLogfile(DECODER *decoder, FILE *logfile);

CODEC_ERROR ReleaseDecoder(DECODER *decoder);

CODEC_ERROR CodecErrorBitstream(BITSTREAM_ERROR error);
    
BITSTREAM_ERROR CodecBitstreamError(CODEC_ERROR error);

CODEC_ERROR PrepareDecoderState(DECODER *decoder, const PARAMETERS *parameters);

CODEC_ERROR PrepareDecoderTransforms(DECODER *decoder);

CODEC_ERROR SetOutputImageFormat(DECODER *decoder,
								 const PARAMETERS *parameters,
								 DIMENSION *width_out,
								 DIMENSION *height_out,
								 PIXEL_FORMAT *format_out);

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
CODEC_ERROR SetDisplayImageFormat(DECODER *decoder,
								  const PARAMETERS *parameters,
								  DIMENSION *width_out,
								  DIMENSION *height_out,
								  PIXEL_FORMAT *format_out);
#endif

bool ChannelLowpassBandsAllValid(const DECODER *decoder, int wavelet_index);

PIXEL_FORMAT EncodedPixelFormat(const DECODER *decoder, const PARAMETERS *parameters);

CODEC_ERROR ImageRepackingProcess(const UNPACKED_IMAGE *unpacked_image,
								  PACKED_IMAGE *packed_image,
								  DATABASE *database,
								  const PARAMETERS *parameters);

CODEC_ERROR UpdateCodecState(DECODER *decoder, BITSTREAM *stream, TAGVALUE segment);

bool IsHeaderParameter(TAGWORD tag);

CODEC_ERROR UpdateHeaderParameter(DECODER *decoder, TAGWORD tag);

CODEC_ERROR ParseBitstreamHeader(BITSTREAM *stream, BITSTREAM_HEADER *header);

CODEC_ERROR PrepareDecoder(DECODER *decoder, ALLOCATOR *allocator, DATABASE *database, const PARAMETERS *parameters);

CODEC_ERROR AllocDecoderTransforms(DECODER *decoder);

CODEC_ERROR ReleaseDecoderTransforms(DECODER *decoder);

CODEC_ERROR AllocDecoderBuffers(DECODER *decoder);

CODEC_ERROR ReleaseDecoderBuffers(DECODER *decoder);

CODEC_ERROR AllocateChannelWavelets(DECODER *decoder, int channel);

CODEC_ERROR DecodeStream(STREAM *stream, UNPACKED_IMAGE *image, DATABASE *database, const PARAMETERS *parameters);

CODEC_ERROR DecodeImage(STREAM *stream, IMAGE *image, DATABASE *database, const PARAMETERS *parameters);

CODEC_ERROR DecodingProcess(DECODER *decoder, BITSTREAM *stream, UNPACKED_IMAGE *image, DATABASE *database, const PARAMETERS *parameters);

CODEC_ERROR DecodeSingleImage(DECODER *decoder, BITSTREAM *input, UNPACKED_IMAGE *image);

CODEC_ERROR DecodeSampleLayer(DECODER *decoder, BITSTREAM *input, IMAGE *image);

CODEC_ERROR DecodeChannelSubband(DECODER *decoder, BITSTREAM *input, size_t chunk_size);

CODEC_ERROR ReconstructWaveletBand(DECODER *decoder, int channel, WAVELET *wavelet, int index);

CODEC_ERROR ParseChannelIndex(BITSTREAM *stream, uint32_t *channel_size, int channel_count);

CODEC_ERROR SetTransformParameters(DECODER *decoder);

DIMENSION ChannelWidth(DECODER *decoder, int channel_index, DIMENSION width);

DIMENSION LayerWidth(DECODER *decoder, DIMENSION width);
DIMENSION LayerHeight(DECODER *decoder, DIMENSION height);

CODEC_ERROR ProcessSampleMarker(DECODER *decoder, BITSTREAM *stream, TAGWORD marker);

CODEC_ERROR SetDecodedBandMask(CODEC_STATE *codec, int subband);

CODEC_ERROR DecodeLowpassBand(DECODER *decoder, BITSTREAM *stream, WAVELET *wavelet);

CODEC_ERROR DecodeHighpassBand(DECODER *decoder, BITSTREAM *stream, WAVELET *wavelet, int band);

CODEC_ERROR DecodeBandRuns(BITSTREAM *stream, CODEBOOK *codebook, PIXEL *data,
						   DIMENSION width, DIMENSION height, DIMENSION pitch);

CODEC_ERROR DecodeBandTrailer(BITSTREAM *stream);

CODEC_ERROR DecodeSampleChannelHeader(DECODER *decoder, BITSTREAM *stream);

bool IsHeaderComplete(DECODER *decoder);

bool EndOfSample(DECODER *decoder);

#if VC5_ENABLED_PART(VC5_PART_LAYERS)
    CODEC_ERROR DecodeLayer(DECODER *decoder, BITSTREAM *input, PACKED_IMAGE *output, DATABASE *database, const PARAMETERS *parameters);
    CODEC_ERROR UpdateLayerParameters(DECODER *decoder);
    CODEC_ERROR ResetWaveletDecodingFlags(DECODER *decoder);
    bool AllLayersDecoded(DECODER *decoder);
#endif

    
#if VC5_ENABLED_PART(VC5_PART_SECTIONS)
    CODEC_ERROR DecodeImageSection(DECODER *decoder, BITSTREAM *input, PACKED_IMAGE *output, DATABASE *database, const PARAMETERS *parameters);
    CODEC_ERROR ResetDecoderImageSection(DECODER *decoder, ALLOCATOR *allocator, PARAMETERS *parameters);
    bool AllImageSectionsDecoded(DECODER *decoder);
#endif

bool IsDecodingComplete(DECODER *decoder);

CODEC_ERROR ReconstructUnpackedImage(DECODER *decoder, UNPACKED_IMAGE *image);

#if VC5_ENABLED_PART(VC5_PART_LAYERS)
    CODEC_ERROR ReconstructLayerImage(DECODER *decoder, IMAGE *image);
#endif

CODEC_ERROR TransformInverseSpatialQuantBuffer(DECODER *decoder, void *output_buffer, DIMENSION output_width, DIMENSION output_pitch);

CODEC_ERROR PrintDecoderQuantization(const DECODER *decoder);

CODEC_ERROR DumpDecodedSubbands(DECODER *decoder, int channel_index,
								uint32_t subband_mask, const char *pathname);

CODEC_ERROR DumpWaveletBands(DECODER *decoder, int channel_index, int wavelet_index,
							 uint32_t wavelet_band_mask, const char *pathname);

CODEC_ERROR DumpTransformBands(DECODER *decoder, uint32_t channel_mask, uint32_t wavelet_mask,
							   uint32_t wavelet_band_mask, const char *pathname);

CODEC_ERROR DumpTransformSubbands(DECODER *decoder, uint32_t channel_mask, uint32_t subband_mask,
								  const char *pathname);

CODEC_ERROR WriteLowpassBands(const DECODER *decoder, int wavelet_level, const char *basename);

#if VC5_ENABLED_PART(VC5_PART_METADATA)
CODEC_ERROR DecodeMetadataChunk(DECODER *decoder, BITSTREAM *stream, TAGWORD chunk_tag, int32_t chunk_size);
#endif

#ifdef __cplusplus
}
#endif

#endif
