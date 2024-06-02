/*!	@file bayer.h

	Declaration of routines for unpacking a row of pixels in a Bayer frame into separate rows
	for each channel.  Note that the unpacking routines for non-Bayer formats are defined in
	other files.

	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _BAYER_H
#define _BAYER_H

#ifdef __cplusplus
extern "C" {
#endif

CODEC_ERROR UnpackFrameRowBYR3(uint8_t *input, DIMENSION width, PIXEL *buffer[], int channel_count);
CODEC_ERROR UnpackFrameRowBYR4(uint8_t *input, DIMENSION width, PIXEL *buffer[], int channel_count);

#ifdef __cplusplus
}
#endif

#endif
