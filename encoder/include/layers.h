/*! @file encoder/include/layers.h
 
	Declaration of routines for handling layers in the encoder.
 
	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
 */

#ifndef _LAYERS_H
#define _LAYERS_H

#include "fileinfo.h"
#include "parseargs.h"

#ifdef __cplusplus
extern "C" {
#endif

#if VC5_ENABLED_PART(VC5_PART_LAYERS)

//CODEC_ERROR ReadImageList(IMAGE_LIST *image_list,
//                          PARAMETERS *parameters,
//                          const char *format_specification_string);

CODEC_ERROR ReadListImage(IMAGE_LIST *image_list,
                          int image_index,
                          const char *pathname);

#endif

#ifdef __cplusplus
}
#endif

#endif
