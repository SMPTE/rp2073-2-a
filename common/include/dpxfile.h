/*! @file common/include/dpxfile.h

	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _DPXFILE_H
#define _DPXFILE_H

/*!
	@brief Data structure for information about a DPX file
*/
typedef struct DPX_file_info
{
	bool byte_swap_flag;		//!< Flags indicating whether values in the file are byte swapped
	uint32_t offset;			//!< Byte offset to the start of the image in the DPX file
	uint32_t width;				//!< Width of the DPX image
	uint32_t height;			//!< Height of the DPX image
	uint8_t descriptor;			//!< Pixel format of the DPX image (Cineon standard)
	uint8_t bit_size;			//!< Number of bits of precision

	PIXEL_FORMAT format;		//!< Pixel format of the DPX image (reference codec)

} DPX_FileInfo;

#ifdef __cplusplus
extern "C" {
#endif

uint32_t Pack10(uint32_t R, uint32_t G, uint32_t B);

void Unpack10(uint32_t word, uint16_t *R, uint16_t *G, uint16_t *B);
CODEC_ERROR PackBufferRowsToDPX0(PIXEL *input_buffer, size_t input_pitch,
								 PIXEL *output_buffer, size_t output_pitch,
								 DIMENSION width, DIMENSION height);

CODEC_ERROR ConvertBufferRowsToDPX0(PIXEL *input_buffer, size_t input_pitch,
									PIXEL *output_buffer, size_t output_pitch,
									DIMENSION width, DIMENSION height);

CODEC_ERROR PackBayerRowsToDPX0(PIXEL *input_buffer, size_t input_pitch,
								PIXEL *output_buffer, size_t output_pitch,
								DIMENSION width, DIMENSION height);

CODEC_ERROR DPX_ReadFile(IMAGE *image, const char *pathname);

CODEC_ERROR DPX_ParseHeader(IMAGE *image, DPX_FileInfo *info);

CODEC_ERROR UnpackImageRowDPX0(uint8_t *input, DIMENSION width, PIXEL *buffer[],
							   PRECISION bits_per_component[], int channel_count,
							   ENABLED_PARTS enabled_parts);

CODEC_ERROR PackLowpassBandsToDPX0(void **lowpass_buffer_array,
								   size_t *lowpass_pitch_array,
								   DIMENSION width,
								   DIMENSION height,
								   PIXEL_FORMAT format,
								   int shift,
								   IMAGE *output);

CODEC_ERROR DPX_SetByteSwapFlag();

CODEC_ERROR DPX_WriteImage(const IMAGE *image, const char *pathname);

#ifdef __cplusplus
}
#endif

#endif
