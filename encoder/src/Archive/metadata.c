/*!	@file encoder/src/metadata.c

	Implementation of routines for inserting metadata into the bitstream.
	
	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#include "config.h"

#if VC5_ENABLED_PART(VC5_PART_METADATA)

#include "headers.h"


/*!
	@brief Encode metadata contained in the metadata file passed in the parameters into the bitstream
*/
CODEC_ERROR EncodeMetadataChunk(ENCODER *encoder, BITSTREAM *bitstream, const PARAMETERS *parameters)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;

	if (encoder->metadata_flag)
	{
		FILE *metadata_file = fopen(parameters->metadata_pathname, "rb");
		assert (metadata_file != NULL);
		if (metadata_file == NULL) {
			return CODEC_ERROR_FILE_OPEN;
		}

		// Read the metadata for the file and inject it into the bitstream
		error = WriteMetadataFile(encoder, bitstream, metadata_file);

		fclose(metadata_file);
	}

	return error;
}


/*!
	@brief Parse the XML representation of a metadata test case and write the metadata into the bitstream

	The XML representation of the metadata is described in ST 2073-7 Annex A.
*/
CODEC_ERROR WriteMetadataFile(ENCODER *encoder, BITSTREAM *bitstream, FILE *metadata_file)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;





	return error;
}

#endif


#if 0

#include "headers.h"


#ifndef MAKETAG
//! Macro for creating the four character code for a metadata tag
#define MAKETAG(d,c,b,a) (((a) << 24) | ((b) << 16) | ((c) << 8) | (d))
#endif


//! Metadata tags
typedef enum _metadata_tag
{
	METADATA_TAG_CLIP_GUID = MAKETAG('G','U','I','D'),		// unique ID for each video clip, 16-bytes of type 'G'
	METADATA_TAG_UNIQUE_FRAMENUM = MAKETAG('U','F','R','M'),// unique frame number in a video clip, starts at 0.
	METADATA_TAG_FREESPACE = MAKETAG('F','R','E','E'),		// no type, Free space
	METADATA_TAG_DEMOSAIC = MAKETAG('D','E','M','O'),		// Demosaic number, type 'B'-Byte
	METADATA_TAG_BAYER_FORMAT = MAKETAG('B','F','M','T'),	// Bayer format/phase RGGB = 0, GRBG = 1, GBRG = 2, BGGR = 3, type 'L'-Byte 

} METADATA_TAG;


//! Define the size of the metadata header (in bytes)
const int metadata_header_size = 8;

//! Define the size of the metadata free space block (in bytes)
const int metadata_free_space_size = 512;


/*!
	@brief Write a block of metadata free space into the bitstream

	@todo Make the size of the metadata free space adjustable
*/
CODEC_ERROR PutMetadataFreeSpace(BITSTREAM *bitstream)
{
	if (metadata_free_space_size)
	{
		STREAM *stream = bitstream->stream;
		uint32_t payload_size_and_type;
		uint32_t payload_value;
		uint32_t metadata_size_remaining = metadata_free_space_size;
		uint8_t guid[16];
		uint8_t i;

		// Convert the size of the free space to the number of segments
		uint32_t chunk_size = (metadata_free_space_size + sizeof(SEGMENT) - 1) / sizeof(SEGMENT);

		// Write the tag value pair for the metadata free space chunk
		PutTagPairOptional(bitstream, CODEC_TAG_METADATA, chunk_size);

		// Must be able to write directly to the byte stream
		AlignBitsSegment(bitstream);
		FlushBitstream(bitstream);

		//NOTE: Metadata is written to the bitstream without byte swapping

		// Write the metadata tag
		for (i = 0; i < sizeof(guid); i++) {
			guid[i] = rand();
		}

		payload_size_and_type = 'G'<<24 | 16; // 16 bytes of type 'G' (for GUID)
		payload_value = 1;// Bayer phase 0 thru 3
		PutWord(stream, METADATA_TAG_CLIP_GUID); 
		PutWord(stream, payload_size_and_type);

		for (i = 0; i < sizeof(guid); i++) {
			PutByte(stream, guid[i]);
		}

		metadata_size_remaining -= 24;
		
		payload_size_and_type = 'L'<<24 | 4; // four bytes of type Long
		payload_value = 0;// start with frame zero and increment
		PutWord(stream, METADATA_TAG_UNIQUE_FRAMENUM); 
		PutWord(stream, payload_size_and_type);
		PutWord(stream, payload_value);
		metadata_size_remaining -= 12;

		payload_size_and_type = 'B'<<24 | 1; // one byte of type Byte
		payload_value = 1;// Bayer phase 0 thru 3
		PutWord(stream, METADATA_TAG_BAYER_FORMAT); 
		PutWord(stream, payload_size_and_type);
		PutWord(stream, payload_value);
		metadata_size_remaining -= 12;

		payload_size_and_type = 'L'<<24 | 4; // four bytes of type Long
		payload_value = 4;// Demosaic filter style 0 thru 9
		PutWord(stream, METADATA_TAG_DEMOSAIC); 
		PutWord(stream, payload_size_and_type);
		PutWord(stream, payload_value);
		metadata_size_remaining -= 12;


		PutWord(stream, METADATA_TAG_FREESPACE);

		// The metadata payload size does not include the metadata header
		payload_size_and_type = metadata_size_remaining - metadata_header_size;
		PutWord(stream, payload_size_and_type);

		// Write zero bytes into the metadata free space
		PadBytes(stream, payload_size_and_type);
	}

	return CODEC_ERROR_OKAY;
}

#endif
