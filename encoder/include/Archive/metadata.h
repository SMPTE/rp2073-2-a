/*!	@file common/include/metadata.h

	Declaration of a data structure for holding a table of metadata entries
	that can override the default decoding behavior and delarations of the
	routines used to insert metadata into the bitstream.

	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _METADATA_H
#define _METADATA_H

#if VC5_ENABLED_PART(VC5_PART_METADATA)

/*!
	@brief Declaration of a data structure for metadata

	The reference decoder does not currently include any functionality for
	metadata so this data structures is not used by the reference decoder.
*/
typedef struct _metadata
{
	uint_least16_t entry_count;		//!< Number of metadata entries

} METADATA;

#ifdef __cplusplus
extern "C" {
#endif

CODEC_ERROR PutMetadataFreeSpace(BITSTREAM *stream);

CODEC_ERROR EncodeMetadataChunk(ENCODER *encoder, BITSTREAM *bitstream, const PARAMETERS *parameters);

CODEC_ERROR WriteMetadataFile(ENCODER *encoder, BITSTREAM *bitstream, FILE *metadata_file);

#ifdef __cplusplus
}
#endif

#endif

#endif
