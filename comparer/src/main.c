/*!	@file main.c

	This module is not part of the reference codec but is included
	to allow the codec to be tested.

	This program is used to compute the PSNR difference between two images.

	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#include <string.h>
#include <sys/stat.h>
#include "headers.h"
#include "fileinfo.h"
#include "dpxfile.h"
#include "psnr.h"
#include "parseargs.h"


#ifdef WIN32
#define strcasecmp stricmp
#define stat _stat
#define fileno _fileno
#define fstat _fstat
#endif


// Forward reference
CODEC_ERROR RAW_ReadImage(IMAGE *image, const char *pathname);
CODEC_ERROR DPX_ReadImage(IMAGE *image, const char *pathname);


/*!
	@brief Routine for reading any file based on the file type
*/
CODEC_ERROR ReadImage(IMAGE *image, const char *pathname)
{
	FILE_TYPE file_type = GetFileType(pathname);

	switch (file_type)
	{
	case FILE_TYPE_RAW:
		return RAW_ReadImage(image, pathname);
		break;

	case FILE_TYPE_DPX:
		return DPX_ReadImage(image, pathname);
		break;

	default:
		assert(0);
		break;
	}

	return CODEC_ERROR_UNSUPPORTED_FILE_TYPE;
}

/*!
	@brief Read a single input image from a file

	The file does not contain a header so the dimensions and pixel
	format must be determined by other means.  The image buffer must
	be allocated before this routine is called and the image dimensions
	and format must be set before the image is sent to the encoder.
*/
CODEC_ERROR RAW_ReadImage(IMAGE *image, const char *pathname)
{
	FILE *file = NULL;
	struct stat info;
	int fd;
	size_t result;

	// Open the file that contains the image
	file = fopen(pathname, "rb");
	if (file == NULL) {
		return CODEC_ERROR_OPEN_FILE_FAILED;
	}

	// Get the size of the file
	fd = fileno(file);
	result = fstat(fd, &info);
	if (result != 0) {
		return CODEC_ERROR_FILE_SIZE_FAILED;
	}

	result = fread(image->buffer, info.st_size, 1, file);
	if (result != 1) {
		return CODEC_ERROR_READ_FILE_FAILED;
	}

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Read a single from a DPX file

	The image buffer will be allocated by this routine and the image
	dimensions and format will be set based on the information obtained
	from the DPX file header.

	It may cause a memory leak if the image buffer is allocated before
	calling this routine because the buffer will be reallocated without
	freeing a buffer that was already allocated.
*/
CODEC_ERROR DPX_ReadImage(IMAGE *image, const char *pathname)
{
	DPX_FileInfo info;
	DIMENSION pitch;

	// Read the entire file including the DPX file header
	DPX_ReadFile(image, pathname);

	// Parse the header in the DPX file
	DPX_ParseHeader(image, &info);

	// Compute the pitch
	pitch = ImagePitch(info.width, info.format);

	// Set the image dimensions and format
	SetImageFormat(image, info.width, info.height, pitch, info.format, info.offset);

	return CODEC_ERROR_OKAY;
}

#if 0
/*!
	@brief Translate the string representation of a pixel format into the enumerated value
*/
PIXEL_FORMAT PixelFormat(const char *string)
{
	struct _pixel_format_table_entry
	{
		char *string;
		PIXEL_FORMAT format;
	};

	static const struct _pixel_format_table_entry pixel_format_table[] =
	{
		{"byr3", PIXEL_FORMAT_BYR3},
		{"byr4", PIXEL_FORMAT_BYR4},
	};

	static const int pixel_format_table_length = sizeof(pixel_format_table) / sizeof(pixel_format_table[0]);

	int index;

	for (index = 0; index < pixel_format_table_length; index++)
	{
		if (strcasecmp(string, pixel_format_table[index].string) == 0) {
			return pixel_format_table[index].format;
		}
	}

	// Did not find the format string in the pixel format table
	return PIXEL_FORMAT_UNKNOWN;
}
#endif

/*!

*/
CODEC_ERROR ReadInputImage(const char *pathname,
						   IMAGE *image,
						   DIMENSION width,
						   DIMENSION height,
						   PIXEL_FORMAT format)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;
	FILE_INFO info;

	// Get the pixel format and precision of the input image
	GetFileInfo(pathname, &info);

	if (info.type == FILE_TYPE_DPX)
	{
		// This program only supports 10-bit DPX images
		//assert(format == PIXEL_FORMAT_DPX0);

		// The DPX file reader will allocate a buffer for the input image
		InitImage(image);
	}
	else if (info.type == FILE_TYPE_RAW)
	{
		// The image dimensions and pixel format must be provided by the caller
		assert(width > 0 && height > 0 && format != PIXEL_FORMAT_UNKNOWN);

		// Must allocate a buffer for the input image before calling the file reader
		AllocImage(NULL, image, width, height, format);
	}

	// Read the input image (may reallocate the image)
	error = ReadImage(image, pathname);

	return error;
}

/*!
	@brief Main entry point for the program that computes PSNR

	The first two arguments are the pathnames to the files that contain the images
	used for the PSNR calculations.

	Test cases:
		D:\Test\Bayer\Jarrah1_RAW_1080p\temp2\Jarrah1_RAW_1080p-0000-ref.dpx D:\Temp\Jarrah1_RAW_1080p-0000-ref-2.dpx

		Working directory: C:\Users\bschunck\Lost\Cedoc\Standard\Media\Jarrah1
		.\byr3\Jarrah1_RAW_1080p-0000.byr3 .\byr3\Jarrah1_RAW_1080p-0009.byr3
		.\byr4\Jarrah1_RAW_1080p-0000.byr4 .\byr4\Jarrah1_RAW_1080p-0009.byr4
		.\rg48\Jarrah1_RAW_1080p-0000.rg48 .\rg48\Jarrah1_RAW_1080p-0009.rg48
*/
int main(int argc, char *argv[])
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;
	IMAGE image1;
	IMAGE image2;

#if 1
	PARAMETERS parameters;

	DIMENSION image_width = 1920;
	DIMENSION image_height = 1080;
	PIXEL_FORMAT image_format = PIXEL_FORMAT_UNKNOWN;

	int stats = 0;

	InitParameters(&parameters);

	error = ParseParameters(&argc, argv, &parameters);
	if (error != CODEC_ERROR_OKAY) {
		printf("Could not parse the command-line arguments\n");
	}

	if (argc < 3) {
		fprintf(stderr, "Must provide input and output pathname arguments\n");
		return CODEC_ERROR_MISSING_ARGUMENT;
	}
#else
	//TODO: Set the image dimensions from the input arguments
	DIMENSION image_width = 1920;
	DIMENSION image_height = 1080;
	PIXEL_FORMAT image_format = PIXEL_FORMAT_BYR4;

	int stats = 0;

	if (argc < 3) {
		fprintf(stderr, "Must provide input and output pathname arguments\n");
		return CODEC_ERROR_MISSING_ARGUMENT;
	}

	// The dimensions of raw images may be provided as optional arguments
	if (argc > 4)
	{
		int width = 0;
		int height = 0;

		if (sscanf(argv[3], "%d", &width) &&
			sscanf(argv[4], "%d", &height) &&
			width > 0 && height > 0)
		{
			image_width = width;
			image_height = height;
		}
	}

	// The pixel format of raw images may be provided as an optional argument
	if (argc > 5)
	{
		PIXEL_FORMAT format = PixelFormat(argv[5]);
		if (format != PIXEL_FORMAT_UNKNOWN) {
			image_format = format;
		}
	}
#endif

	//printf("Input image one: %s\n", argv[1]);
	//printf("Input image two: %s\n", argv[2]);
	//printf("\n");

	// Command-line arguments override the default image parameters
	if (parameters.image.width > 0) {
		image_width = parameters.image.width;
	}
	if (parameters.image.height > 0) {
		image_height = parameters.image.height;
	}
	if (parameters.image.format != PIXEL_FORMAT_UNKNOWN) {
		image_format = parameters.image.format;
	}

	if (image_width == 0 || image_height == 0 || image_format == PIXEL_FORMAT_UNKNOWN) {
		printf("Could not determine the image dimensions and pixel format\n");
	}

	//TODO: Modify the code to allow different pixel formats for each image

	// Allocate and read the input images
	error = ReadInputImage(argv[1], &image1, image_width, image_height, image_format);
	if (error != CODEC_ERROR_OKAY) {
		fprintf(stderr, "Could not read input image one: %s\n", argv[1]);
		return error;
	}

	ReadInputImage(argv[2], &image2, image_width, image_height, image_format);
	if (error != CODEC_ERROR_OKAY) {
		fprintf(stderr, "Could not read input image two: %s\n", argv[2]);
		return error;
	}

	// The PSNR calculation depends on the pixel format
	switch (image1.format)
	{
	case PIXEL_FORMAT_DPX0:
		ComputePSNR_DPX0(image1.width, image1.height, ImageData(&image1), ImageData(&image2), stats);
		break;

	case PIXEL_FORMAT_BYR4:
		ComputePSNR_BYR4(image1.width, image1.height, ImageData(&image1), ImageData(&image2), stats);
		break;

	case PIXEL_FORMAT_BYR3:
		ComputePSNR_BYR3(image1.width, image1.height, ImageData(&image1), ImageData(&image2), stats);
		break;

	case PIXEL_FORMAT_RG48:
		ComputePSNR_RG48(image1.width, image1.height, ImageData(&image1), ImageData(&image2), stats);
		break;

	default:
		//TODO: Add code to compute the PSNR for other pixel formats
		assert(0);
		break;
	}

	return CODEC_ERROR_OKAY;
}
