/*! @file encoder/include/sections.h
 
	Declaration of routines for handling sections in the encoder.
 
	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
 */

#ifndef _SECTIONS_H
#define _SECTIONS_H

// Forward reference
typedef struct _pathname_list PATHNAME_LIST;

#ifdef __cplusplus
extern "C" {
#endif

bool IsEncoderSectionEnabled(struct _encoder *encoder, SECTION_NUMBER section_number);

CODEC_ERROR BeginSection(BITSTREAM *bitstream, TAGWORD tag);

CODEC_ERROR EndSection(BITSTREAM *bitstream);

CODEC_ERROR BeginImageSection(struct _encoder *encoder, BITSTREAM *stream);

CODEC_ERROR BeginHeaderSection(struct _encoder *encoder, BITSTREAM *stream);

CODEC_ERROR BeginLayerSection(struct _encoder *encoder, BITSTREAM *stream);

CODEC_ERROR BeginChannelSection(struct _encoder *encoder, BITSTREAM *stream);

CODEC_ERROR BeginWaveletSection(struct _encoder *encoder, BITSTREAM *stream);

CODEC_ERROR BeginSubbandSection(struct _encoder *encoder, BITSTREAM *stream);

bool GetEnabledSections(const char *string, uint32_t *enabled_sections_out);
    
CODEC_ERROR PutCodecState(struct _encoder *encoder, BITSTREAM *stream, SECTION_NUMBER section_number);

//CODEC_ERROR ReadInputPathnameList(IMAGE_LIST *image_list, PATHNAME_LIST *input_pathname_list);
CODEC_ERROR ReadInputPathnameList(IMAGE_LIST *image_list, const struct _pathname_list *input_pathname_list);

#ifdef __cplusplus
}
#endif

#endif
