/*!	@file base64.h

	Definitions for the base64 encoder
*/

#ifndef _BASE64_H
#define _BASE64_H

CODEC_ERROR encode_base64(const uint8_t *buffer, size_t buffer_size, char *output, size_t output_size);

//! Decode a base64 string into binary data
CODEC_ERROR decode_base64(const char *input_buffer, size_t input_length, uint8_t *output_buffer, size_t output_length, size_t *actual_length_out);

#endif
