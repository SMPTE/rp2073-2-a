/*!	@file common/include/arguments.h

	Definitions of the routines for parsing the values of command-line arguments.

	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _ARGUMENTS_H
#define _ARGUMENTS_H

bool GetDimension(const char *string, DIMENSION *dimension_out);

bool GetPixelFormat(const char *string, PIXEL_FORMAT *format_out);

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
bool GetImageFormat(const char *string, IMAGE_FORMAT *format_out);
#endif

bool GetPrecision(const char *string, PRECISION *precision_out);

bool GetQuantization(const char *string, QUANT *quant);

bool GetChannelOrder(const char *string,
                     CHANNEL *channel_order_table,
                     int *channel_order_count,
                     int channel_order_table_length);

bool GetEnabledParts(const char *string, uint32_t *enabled_parts_out);

bool GetBandfileInfo(const char *string, BANDFILE_INFO *bandfile);

#if VC5_ENABLED_PART(VC5_PART_LAYERS)
bool GetLayerCount(const char *string, COUNT *layer_count_out);
#endif

#if VC5_ENABLED_PART(VC5_PART_SECTIONS)
bool GetEnabledSections(const char *string, uint32_t *enabled_sections_out);
#endif

//bool GetStringParameter(const char *string, char *parameter, size_t length);

#endif
