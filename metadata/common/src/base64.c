/*!	@file base64.c

	@brief Base64 encoder

	The base64 encoder was adapted from:
	John's Blog, Base64 Encode and Decode in C
	https://nachtimwald.com/2017/11/18/base64-encode-and-decode-in-c/
*/

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>


#if _ENCODER || _DECODER

// Use relative paths to avoid conflicts with the encoder and decoder include paths
#include "../include/error.h"
#include "../include/base64.h"

#else

#include "error.h"
#include "base64.h"

#endif


CODEC_ERROR encode_base64(const uint8_t *buffer, size_t buffer_size, char *output, size_t output_size)
{
	static const char b64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	for (size_t i = 0, j = 0; i < buffer_size; i += 3, j += 4)
	{
		uint8_t b1 = buffer[i];
		uint8_t b2 = (i + 1) < buffer_size ? buffer[i + 1] : 0;
		uint8_t b3 = (i + 2) < buffer_size ? buffer[i + 2] : 0;

		// Concatenate the three bytes it a 24-bit word
		uint32_t word = (b1 << 16 | b2 << 8 | b3);

		// Split the 24-bit word into four 6-bit characters
		char c1 = b64chars[(word >> 18) & 0x3F];
		char c2 = b64chars[(word >> 12) & 0x3F];
		char c3 = (i + 1) < buffer_size ? b64chars[(word >> 6) & 0x3F] : '=';
		char c4 = (i + 2) < buffer_size ? b64chars[(word >> 0) & 0x3F] : '=';

		// Write the characters to the output buffer
		output[j + 0] = c1;
		output[j + 1] = c2;
		output[j + 2] = c3;
		output[j + 3] = c4;
	}

	return CODEC_ERROR_OKAY;
}


/*!
	@brief base64 decoder adapted from code by Jouni Malinen

	Copyright (c) 2005-2011, by Jouni Malinen <j@w1.fi>

 	This software may be distributed under the terms of the BSD license.
 	See README for more details.

	https://web.mit.edu/freebsd/head/contrib/wpa/src/utils/base64.c

	@todo Need to check that the output buffer is large enough

	@todo Check that the output length is computed correctly

	@todo Move this function into the common directory

	@ Modify this function to avoid writing past the end of the output buffer
 */

static const uint8_t base64_table[65] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


CODEC_ERROR decode_base64(const char *input_buffer, size_t input_length, uint8_t *output_buffer, size_t output_length, size_t *actual_length_out)
{
	//fprintf(stderr, "decode_base64 input_length: %zd, output_length: %zd\n", input_length, output_length);

	uint8_t dtable[256], block[4], tmp;
	size_t i;
	size_t count;
	//size_t olen;
	int pad = 0;

	uint8_t *out = output_buffer;
	uint8_t *pos = output_buffer;

	memset(dtable, 0x80, 256);
	for (i = 0; i < sizeof(base64_table) - 1; i++)
		dtable[base64_table[i]] = (uint8_t) i;
	dtable['='] = 0;

	count = 0;
	for (i = 0; i < input_length; i++)
	{
		int index = input_buffer[i];
		if (dtable[index] != 0x80) count++;
	}

	if (count == 0 || count % 4) {
		return CODEC_ERROR_BAD_INPUT;
	}

	//olen = count / 4 * 3;

	count = 0;
	for (i = 0; i < input_length; i++)
	{
		int index = input_buffer[i];
		tmp = dtable[index];
		if (tmp == 0x80) continue;

		if (input_buffer[i] == '=') pad++;
		block[count] = tmp;
		count++;
		if (count == 4)
		{
			*pos++ = (block[0] << 2) | (block[1] >> 4);
			*pos++ = (block[1] << 4) | (block[2] >> 2);
			*pos++ = (block[2] << 6) | block[3];
			count = 0;
			if (pad)
			{
				if (pad == 1) pos--;
				else if (pad == 2) pos -= 2;
				else return CODEC_ERROR_BAD_INPUT;
			}
		}
	}

	size_t actual_length = pos - out;

	//fprintf(stderr, "decode_base64 input_length: %zd, output_length: %zd, actual_length: %zd\n", input_length, output_length, actual_length);

	assert(actual_length <= output_length);

	*actual_length_out = actual_length;

	return CODEC_ERROR_OKAY;
}
