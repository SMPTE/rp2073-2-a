/*! @file common/include/dump.h
 
	Declaration of the inverse component transform and permutation.
 
	(c) 2015 Society of Motion Picture & Television Engineers LLC and GoPro, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
 */

#ifndef _DUMP_H
#define _DUMP_H

CODEC_ERROR DumpTransformWavelets(TRANSFORM transform_array[MAX_CHANNEL_COUNT],
                                  int channel_count,
                                  int wavelet_level,
                                  const char *pathname);

CODEC_ERROR DumpUnpackedImage(UNPACKED_IMAGE *image,
                              const char *pathname);

#endif
