/*! @file common/src/fileinfo.c

	@brief Module to determine the type of media file.
	
	This module determines the type of media container that contains the image.
	The image may be stored in an unformatted file (also called a RAW file) that
	contains only the image data without any header or the container may provide
	the image dimensions and pixel format.
	
	The file extension of an unformatted image file can indicate the pixel format
	of the image data in the file, but the image dimensions must be determined by
	other means.

	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#include <string.h>
#include "headers.h"
#include "fileinfo.h"

#if defined __GNUC__
#define stricmp strcasecmp
#else
#endif // if defined __GNUC__

FILE_TYPE GetFileType(const char *pathname)
{
	const char *extension = NULL;

	if (pathname == NULL) {
		return FILE_TYPE_UNKNOWN;
	}

	// Get the pathname extension
	extension = strrchr(pathname, '.');
	if (extension == NULL) {
		return FILE_TYPE_UNKNOWN;
	}

	if (stricmp(extension, ".hd") == 0) {
		return FILE_TYPE_RAW;
	}

	if (stricmp(extension, ".vc5") == 0) {
		return FILE_TYPE_RAW;
	}
	
	if (stricmp(extension, ".raw") == 0) {
		return FILE_TYPE_RAW;
	}

	if (stricmp(extension, ".byr3") == 0) {
		return FILE_TYPE_RAW;
	}

	if (stricmp(extension, ".byr4") == 0) {
		return FILE_TYPE_RAW;
	}

	if (stricmp(extension, ".rg48") == 0) {
		return FILE_TYPE_RAW;
	}
	
	if (stricmp(extension, ".b64a") == 0) {
		return FILE_TYPE_RAW;
	}

	if (stricmp(extension, ".yuy2") == 0) {
		return FILE_TYPE_RAW;
	}

	if (stricmp(extension, ".dpx") == 0) {
		return FILE_TYPE_DPX;
	}

	if (stricmp(extension, ".mov") == 0) {
		return FILE_TYPE_MOV;
	}

	if (stricmp(extension, ".avi") == 0) {
		return FILE_TYPE_AVI;
	}

	if (stricmp(extension, ".nv12") == 0) {
		return FILE_TYPE_RAW;
	}

	return FILE_TYPE_UNKNOWN;
}

CODEC_ERROR GetFileRoot(const char *pathname, char *rootpath, size_t pathsize)
{
    const char *extension = NULL;
    size_t length;
    
    extension = strrchr(pathname, '.');
    if (extension == NULL) {
        return CODEC_ERROR_BAD_ARGUMENT;
    }
    
    // Compute the number of characters in the root pathname
    length = extension - pathname;

    // Copy the root pathname to the output string
    strncpy(rootpath, pathname, length);
    rootpath[length] = '\0';
    
    return CODEC_ERROR_OKAY;
}

/*! @brief Return information about the file
	This routine returns information about the file by parsing the
	pathname and using the file extension to determine the type of
	file.

	@todo Make this routine table-driven for extensibility.
*/
CODEC_ERROR GetFileInfo(const char *pathname, FILE_INFO *info)
{
	const char *extension = NULL;

	if (pathname == NULL || info == NULL) {
		return CODEC_ERROR_NULLPTR;
	}

	memset(info, 0, sizeof(FILE_INFO));

	// Get the pathname extension
	extension = strrchr(pathname, '.');
	if (extension == NULL) {
		return CODEC_ERROR_UNSUPPORTED_FILE_TYPE;
	}

	if (stricmp(extension, ".hd") == 0)
	{
		info->type = FILE_TYPE_RAW;
		info->format = PIXEL_FORMAT_UNKNOWN;
		info->precision = 0;
		return CODEC_ERROR_OKAY;
	}

	if (stricmp(extension, ".ca32") == 0)
	{
		info->type = FILE_TYPE_RAW;
		info->format = PIXEL_FORMAT_CA32;
		info->precision = 0;
		return CODEC_ERROR_OKAY;
	}

	if (stricmp(extension, ".raw") == 0)
	{
		info->type = FILE_TYPE_RAW;
		info->format = PIXEL_FORMAT_UNKNOWN;
		info->precision = 0;
		return CODEC_ERROR_OKAY;
	}

#if 0
	if (stricmp(extension, ".byr3") == 0)
	{
		info->type = FILE_TYPE_RAW;
		info->format = PIXEL_FORMAT_BYR3;
		info->precision = 10;
		return CODEC_ERROR_OKAY;
	}
#endif

	if (stricmp(extension, ".byr4") == 0)
	{
		info->type = FILE_TYPE_RAW;
		info->format = PIXEL_FORMAT_BYR4;
		info->precision = 16;
		return CODEC_ERROR_OKAY;
	}

	if (stricmp(extension, ".rg48") == 0)
	{
		info->type = FILE_TYPE_RAW;
		info->format = PIXEL_FORMAT_RG48;
		info->precision = 16;
		return CODEC_ERROR_OKAY;
	}

	if (stricmp(extension, ".b64a") == 0)
	{
		info->type = FILE_TYPE_RAW;
		info->format = PIXEL_FORMAT_B64A;
		info->precision = 16;
		return CODEC_ERROR_OKAY;
	}

	if (stricmp(extension, ".yuy2") == 0)
	{
		info->type = FILE_TYPE_RAW;
		info->format = PIXEL_FORMAT_YUY2;
		info->precision = 8;
		return CODEC_ERROR_OKAY;
	}
    
    if (stricmp(extension, ".nv12") == 0)
    {
        info->type = FILE_TYPE_RAW;
        info->format = PIXEL_FORMAT_NV12;
        info->precision = 8;
        return CODEC_ERROR_OKAY;
    }
    
	if (stricmp(extension, ".dpx") == 0)
	{
		info->type = FILE_TYPE_DPX;
		info->format = PIXEL_FORMAT_DPX_50;
		info->precision = 10;
		return CODEC_ERROR_OKAY;
	}

	if (stricmp(extension, ".mov") == 0)
	{
		info->type = FILE_TYPE_MOV;
		info->format = PIXEL_FORMAT_UNKNOWN;
		info->precision = 0;
		return CODEC_ERROR_OKAY;
	}

	if (stricmp(extension, ".avi") == 0)
	{
		info->type = FILE_TYPE_AVI;
		info->format = PIXEL_FORMAT_UNKNOWN;
		info->precision = 0;
		return CODEC_ERROR_OKAY;
	}

	if (stricmp(extension, ".nv12") == 0)
	{
		info->type = FILE_TYPE_RAW;
		info->format = PIXEL_FORMAT_NV12;
		info->precision = 8;
		return CODEC_ERROR_OKAY;
	}

	return CODEC_ERROR_UNSUPPORTED_FILE_TYPE;
}





#if 0
/*!
	@brief Enumerated values for the type of media file

	The reference codec does not support media containers like AVI or MOV files,
	but may do so in the future.

	To make the code for reading files as simple as possible, the preferred format
	is a sample or image stored in a file without a header.  Encoded samples use
	the ".hd" file extension.

	The reference codec also supports DPX files that use the most common pixel.
*/
typedef enum
{
	FILE_TYPE_UNKNOWN = 0,
	FILE_TYPE_RAW,
	FILE_TYPE_DPX,
	FILE_TYPE_AVI,
	FILE_TYPE_MOV,

} FILE_TYPE;
#endif
#if 0
/*!
	@brief Return the type of file based on the file extension
*/
int GetFileType(const char *pathname)
{
	const char *extension = NULL;

	if (pathname == NULL) {
		return FILE_TYPE_UNKNOWN;
	}

	extension = strrchr(pathname, '.');
	if (extension != NULL) extension++;

	if (stricmp(extension, "hd") == 0) {
		return FILE_TYPE_RAW;
	}

	if (stricmp(extension, "raw") == 0) {
		return FILE_TYPE_RAW;
	}

	if (stricmp(extension, "dpx") == 0) {
		return FILE_TYPE_DPX;
	}

	if (stricmp(extension, "mov") == 0) {
		return FILE_TYPE_MOV;
	}

	if (stricmp(extension, "avi") == 0) {
		return FILE_TYPE_AVI;
	}

	return FILE_TYPE_UNKNOWN;
}
#endif
