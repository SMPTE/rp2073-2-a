/*! @file common/include/fileinfo.h

	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _FILEINFO_H
#define _FILEINFO_H

/*!
	@brief Enumerated values for the type of file

	Note that media containers such as AVI and MOV files are not
	currently supported by the reference codec.
*/
typedef enum
{
	FILE_TYPE_UNKNOWN = 0,
	FILE_TYPE_RAW,
	FILE_TYPE_DPX,
	FILE_TYPE_AVI,
	FILE_TYPE_MOV,

} FILE_TYPE;

/*!
	@brief Data structure for information about a file

	It may be possible to determine the pixel format and number of bits per pixel
	from the filename extension, otherwise the format is reported as unknown and
	the precision is reported as zero.
*/
typedef struct _file_info
{
	FILE_TYPE type;			//!< Type of the file (see the file type enumeration)
	PIXEL_FORMAT format;	//!< Pixel format of the frame in the file
	int precision;			//!< Number of bits per pixel of the frame

} FILE_INFO;


#ifdef __cplusplus
extern "C" {
#endif

FILE_TYPE GetFileType(const char *pathname);
    
CODEC_ERROR GetFileRoot(const char *pathname, char *rootpath, size_t pathsize);

CODEC_ERROR GetFileInfo(const char *pathname, FILE_INFO *info);

#ifdef __cplusplus
}
#endif

#endif
