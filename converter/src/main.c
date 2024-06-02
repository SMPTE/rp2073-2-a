/*!	@file converter/src/main.c

	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#define __STDC_LIMIT_MACROS

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include "headers.h"
#include "fileinfo.h"
#include "getopt.h"

#ifndef __GNUC__
#define stat _stat
#define fstat _fstat
#define fileno _fileno
#endif

/*!
	@brief Read a single input image from a file

	The file does not contain a header so the dimensions and pixel
	format must be determined by other means.  The image buffer must
	be allocated before this routine is called and the image dimensions
	and format must be set before the image is sent to the encoder.
*/
CODEC_ERROR RAW_ReadImage(IMAGE *image, const char *pathname, DIMENSION width, DIMENSION height, PIXEL_FORMAT format)
{
	FILE *file = NULL;
	struct stat info;
	int fd;
	size_t result;
	DIMENSION pitch;

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

	// Allocate the image buffer
	AllocImageSize(NULL, image, info.st_size);

	result = fread(image->buffer, info.st_size, 1, file);
	if (result != 1) {
		return CODEC_ERROR_READ_FILE_FAILED;
	}

	//TODO: Attempt to determine the image dimensions from the image size

	// Compute the pitch and set the image dimensions
	pitch = ImagePitch(width, format);
	SetImageFormat(image, width, height, pitch, format, 0);

	return CODEC_ERROR_OKAY;
}

CODEC_ERROR RAW_WriteImage(IMAGE *image, const char *pathname)
{
	FILE *file = fopen(pathname, "wb");
	if (file != NULL)
	{
		size_t image_size = image->height * image->pitch;
		fwrite(image->buffer, image_size, 1, file);
		fclose(file);
		return CODEC_ERROR_OKAY;
	}
	return CODEC_ERROR_CREATE_FILE_FAILED;
}

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

CODEC_ERROR ConvertDPXToBYR3(IMAGE *input, IMAGE *output)
{
	// Reduce the precision to the 10-bit precision of the BYR3 format
	int descale_shift = 6;
	int row;

	for (row = 0; row < input->height; row++)
	{
		uint32_t *input_row_ptr = (uint32_t *)((uint8_t *)ImageData(input) + row * input->pitch);
		uint16_t *output_row1_ptr = (uint16_t *)((uint8_t *)ImageData(output) + row * output->pitch);

		// Compute pointers to the other rows of Bayer components
		uint16_t *output_row2_ptr = output_row1_ptr + 1 * output->width;
		uint16_t *output_row3_ptr = output_row1_ptr + 2 * output->width;
		uint16_t *output_row4_ptr = output_row1_ptr + 3 * output->width;

		int column;

		for (column = 0; column < input->width; column++)
		{
			uint16_t R, G, B;

			Unpack10(input_row_ptr[column], &R, &G, &B);

			output_row1_ptr[column] = (R >> descale_shift);
			output_row2_ptr[column] = (G >> descale_shift);
			output_row3_ptr[column] = (G >> descale_shift);
			output_row4_ptr[column] = (B >> descale_shift);
		}
	}

	return CODEC_ERROR_OKAY;
}

CODEC_ERROR ConvertDPXToBYR4(IMAGE *input, IMAGE *output)
{
	DIMENSION output_width = output->width;
	DIMENSION output_height = output->height;
	size_t output_pitch = output->pitch;

	int row;

	// Convert the dimensions to units of Bayer pattern elements
	output_width /= 2;
	output_height /= 2;
	output_pitch *= 2;

	for (row = 0; row < input->height; row++)
	{
		uint32_t *input_row_ptr = (uint32_t *)((uint8_t *)ImageData(input) + row * input->pitch);
		uint16_t *output_row1_ptr = (uint16_t *)((uint8_t *)ImageData(output) + row * output_pitch);

		// Compute a pointer to the second row of Bayer components
		uint16_t *output_row2_ptr = output_row1_ptr + 2 * output_width;

		int column;

		for (column = 0; column < input->width; column++)
		{
			uint16_t R, G, B;

			Unpack10(input_row_ptr[column], &R, &G, &B);

			output_row1_ptr[2 * column + 0] = R;
			output_row1_ptr[2 * column + 1] = G;
			output_row2_ptr[2 * column + 0] = G;
			output_row2_ptr[2 * column + 1] = B;
		}
	}

	return CODEC_ERROR_OKAY;
}

CODEC_ERROR ConvertDPXToB64A(IMAGE *input, IMAGE *output)
{
	const uint16_t A = UINT16_MAX;
	int row;

	for (row = 0; row < input->height; row++)
	{
		uint32_t *input_row_ptr = (uint32_t *)((uint8_t *)ImageData(input) + row * input->pitch);
		uint16_t *output_row_ptr = (uint16_t *)((uint8_t *)ImageData(output) + row * output->pitch);
		int column;

		for (column = 0; column < input->width; column++)
		{
			uint16_t R, G, B;

			Unpack10(input_row_ptr[column], &R, &G, &B);

			output_row_ptr[4 * column + 0] = A;
			output_row_ptr[4 * column + 1] = R;
			output_row_ptr[4 * column + 2] = G;
			output_row_ptr[4 * column + 3] = B;
		}
	}

	return CODEC_ERROR_OKAY;
}

CODEC_ERROR ConvertB64AToDPX(IMAGE *input, IMAGE *output)
{
	int row;

	for (row = 0; row < input->height; row++)
	{
		uint16_t *input_row_ptr = (uint16_t *)RowAddress(input, row);
		uint32_t *output_row_ptr = (uint32_t *)RowAddress(output, row);
		int column;

		for (column = 0; column < input->width; column++)
		{
			uint16_t A, R, G, B;

			A = input_row_ptr[4 * column + 0];
			R = input_row_ptr[4 * column + 1];
			G = input_row_ptr[4 * column + 2];
			B = input_row_ptr[4 * column + 3];

			output_row_ptr[column] = Pack10(R, G, B);
		}
	}

	return CODEC_ERROR_OKAY;
}

CODEC_ERROR ConvertB64AToRG48(IMAGE *input, IMAGE *output)
{
	int row;

	for (row = 0; row < input->height; row++)
	{
		uint16_t *input_row_ptr = (uint16_t *)RowAddress(input, row);
		uint16_t *output_row_ptr = (uint16_t *)RowAddress(output, row);
		int column;

		for (column = 0; column < input->width; column++)
		{
			uint16_t A, R, G, B;

			A = input_row_ptr[4 * column + 0];
			R = input_row_ptr[4 * column + 1];
			G = input_row_ptr[4 * column + 2];
			B = input_row_ptr[4 * column + 3];

			output_row_ptr[3 * column + 0] = R;
			output_row_ptr[3 * column + 1] = G;
			output_row_ptr[3 * column + 2] = B;
		}
	}

	return CODEC_ERROR_OKAY;
}

CODEC_ERROR ConvertRG48ToDPX(IMAGE *input, IMAGE *output)
{
	int row;

	for (row = 0; row < input->height; row++)
	{
		uint16_t *input_row_ptr = (uint16_t *)RowAddress(input, row);
		uint32_t *output_row_ptr = (uint32_t *)RowAddress(output, row);
		int column;

		for (column = 0; column < input->width; column++)
		{
			uint16_t R, G, B;

			R = input_row_ptr[3 * column + 0];
			G = input_row_ptr[3 * column + 1];
			B = input_row_ptr[3 * column + 2];

			output_row_ptr[column] = Pack10(R, G, B);
		}
	}

	return CODEC_ERROR_OKAY;
}

CODEC_ERROR ConvertBYR3ToBYR4(IMAGE *input, IMAGE *output)
{
	// Scale the 10-bit precision of BYR3 to the 16-bit precision of BYR4
	int scale_shift = 6;
	int row;

	for (row = 0; row < input->height; row++)
	{
		uint16_t *input_row1_ptr = (uint16_t *)((uint8_t *)ImageData(input) + row * input->pitch);
		uint16_t *output_row1_ptr = (uint16_t *)((uint8_t *)ImageData(output) + row * output->pitch);

		// Compute pointers to the other input rows of Bayer components
		uint16_t *input_row2_ptr = input_row1_ptr + 1 * input->width;
		uint16_t *input_row3_ptr = input_row1_ptr + 2 * input->width;
		uint16_t *input_row4_ptr = input_row1_ptr + 3 * input->width;

		// Compute a pointer to the second output row of Bayer components
		uint16_t *output_row2_ptr = output_row1_ptr + 2 * output->width;

		int column;

		for (column = 0; column < input->width; column++)
		{
			uint16_t R1, G1, G2, B1;

			R1 = input_row1_ptr[column] << scale_shift;
			G1 = input_row2_ptr[column] << scale_shift;
			G2 = input_row3_ptr[column] << scale_shift;
			B1 = input_row4_ptr[column] << scale_shift;

			output_row1_ptr[2 * column + 0] = R1;
			output_row1_ptr[2 * column + 1] = G1;
			output_row2_ptr[2 * column + 0] = G2;
			output_row2_ptr[2 * column + 1] = B1;
		}
	}

	return CODEC_ERROR_OKAY;
}

CODEC_ERROR ConvertBYR3ToDPX(IMAGE *input, IMAGE *output)
{
	// Scale the 10-bit precision of BYR3 to the 16-bit precision of DPX
	int scale_shift = 6;
	int row;

	for (row = 0; row < input->height; row++)
	{
		uint16_t *input_row1_ptr = (uint16_t *)((uint8_t *)ImageData(input) + row * input->pitch);
		uint32_t *output_row_ptr = (uint32_t *)((uint8_t *)ImageData(output) + row * output->pitch);

		// Compute pointers to the other input rows of Bayer components
		uint16_t *input_row2_ptr = input_row1_ptr + 1 * input->width;
		uint16_t *input_row3_ptr = input_row1_ptr + 2 * input->width;
		uint16_t *input_row4_ptr = input_row1_ptr + 3 * input->width;

		int column;

		for (column = 0; column < input->width; column++)
		{
			uint16_t R1, B1;
			uint32_t G1, G2;
			uint16_t G_average;

			R1 = input_row1_ptr[column] << scale_shift;
			G1 = input_row2_ptr[column] << scale_shift;
			G2 = input_row3_ptr[column] << scale_shift;
			B1 = input_row4_ptr[column] << scale_shift;

			G_average = (G1 + G2) >> 1;

			output_row_ptr[column] = Pack10(R1, G_average, B1);
		}
	}

	return CODEC_ERROR_OKAY;
}

CODEC_ERROR ConvertBYR3ToRG48(IMAGE *input, IMAGE *output)
{
	// Scale the 10-bit precision of BYR3 to the 16-bit precision of RG48
	int scale_shift = 6;
	int row;

	for (row = 0; row < input->height; row++)
	{
		uint16_t *input_row1_ptr = (uint16_t *)((uint8_t *)ImageData(input) + row * input->pitch);
		uint16_t *output_row_ptr = (uint16_t *)((uint8_t *)ImageData(output) + row * output->pitch);

		// Compute pointers to the other input rows of Bayer components
		uint16_t *input_row2_ptr = input_row1_ptr + 1 * input->width;
		uint16_t *input_row3_ptr = input_row1_ptr + 2 * input->width;
		uint16_t *input_row4_ptr = input_row1_ptr + 3 * input->width;

		int column;

		for (column = 0; column < input->width; column++)
		{
			uint16_t R1, G1, G2, B1;
			uint16_t G_average;

			R1 = input_row1_ptr[column] << scale_shift;
			G1 = input_row2_ptr[column] << scale_shift;
			G2 = input_row3_ptr[column] << scale_shift;
			B1 = input_row4_ptr[column] << scale_shift;

			G_average = (G1 + G2) >> 1;

			output_row_ptr[3 * column + 0] = R1;
			output_row_ptr[3 * column + 1] = G_average;
			output_row_ptr[3 * column + 2] = B1;
		}
	}

	return CODEC_ERROR_OKAY;
}

CODEC_ERROR ConvertBYR4ToBYR3(IMAGE *input, IMAGE *output)
{
	// Reduce the precision to the 10-bit precision of the BYR3 format
	int descale_shift = 6;
	int row;

	for (row = 0; row < input->height; row++)
	{
		uint16_t *input_row1_ptr = (uint16_t *)((uint8_t *)ImageData(input) + row * input->pitch);
		uint16_t *output_row1_ptr = (uint16_t *)((uint8_t *)ImageData(output) + row * output->pitch);

		// Compute pointers to the other input rows of Bayer components
		uint16_t *input_row2_ptr = input_row1_ptr + 2 * input->width;

		// Compute pointers to the other output rows of Bayer components
		uint16_t *output_row2_ptr = output_row1_ptr + 1 * output->width;
		uint16_t *output_row3_ptr = output_row1_ptr + 2 * output->width;
		uint16_t *output_row4_ptr = output_row1_ptr + 3 * output->width;

		int column;

		for (column = 0; column < input->width; column++)
		{
			uint16_t R1, G1, G2, B1;

			R1 = input_row1_ptr[2 * column + 0];
			G1 = input_row1_ptr[2 * column + 1];
			G2 = input_row2_ptr[2 * column + 0];
			B1 = input_row2_ptr[2 * column + 1];

			output_row1_ptr[column] = (R1 >> descale_shift);
			output_row2_ptr[column] = (G1 >> descale_shift);
			output_row3_ptr[column] = (G2 >> descale_shift);
			output_row4_ptr[column] = (B1 >> descale_shift);
		}
	}

	return CODEC_ERROR_OKAY;
}

CODEC_ERROR ConvertBYR4ToDPX(IMAGE *input, IMAGE *output)
{
	// Scale the 10-bit precision of BYR3 to the 16-bit precision of DPX
	//int scale_shift = 6;

	DIMENSION input_width = input->width;
	DIMENSION input_height = input->height;
	size_t input_pitch = input->pitch;

	int row;

	// Adjust the dimensions to the size of the grid of pattern elements
	input_width /= 2;
	input_height /= 2;
	input_pitch *= 2;

	for (row = 0; row < input_height; row++)
	{
		uint16_t *input_row1_ptr = (uint16_t *)((uint8_t *)ImageData(input) + row * input_pitch);
		uint32_t *output_row_ptr = (uint32_t *)((uint8_t *)ImageData(output) + row * output->pitch);

		// Compute pointer to the odd row of Bayer components
		uint16_t *input_row2_ptr = input_row1_ptr + 2 * input_width;

		int column;

		for (column = 0; column < input_width; column++)
		{
			uint16_t R1, G1, G2, B1;
			uint16_t G_average;

			R1 = input_row1_ptr[2 * column + 0];
			G1 = input_row1_ptr[2 * column + 1];
			G2 = input_row2_ptr[2 * column + 0];
			B1 = input_row2_ptr[2 * column + 1];

			G_average = ((uint32_t)G1 + (uint32_t)G2) >> 1;

			output_row_ptr[column] = Pack10(R1, G_average, B1);
		}
	}

	return CODEC_ERROR_OKAY;
}

CODEC_ERROR ConvertImage(IMAGE *input, IMAGE *output)
{
	//assert(input->format == PIXEL_FORMAT_DPX_50);

	switch (input->format)
	{
	case PIXEL_FORMAT_DPX_50:
		switch (output->format)
		{
		case PIXEL_FORMAT_BYR3:
			return ConvertDPXToBYR3(input, output);
			break;

		case PIXEL_FORMAT_BYR4:
			return ConvertDPXToBYR4(input, output);
			break;

		case PIXEL_FORMAT_B64A:
			return ConvertDPXToB64A(input, output);
			break;

		default:
			assert(0);
			break;
		}

	case PIXEL_FORMAT_BYR3:
		switch (output->format)
		{
		case PIXEL_FORMAT_BYR4:
			return ConvertBYR3ToBYR4(input, output);
			break;

		case PIXEL_FORMAT_DPX_50:
			return ConvertBYR3ToDPX(input, output);
			break;

		case PIXEL_FORMAT_RG48:
			return ConvertBYR3ToRG48(input, output);
			break;

		default:
			assert(0);
			break;
		}

	case PIXEL_FORMAT_BYR4:
		switch (output->format)
		{
		case PIXEL_FORMAT_BYR3:
			return ConvertBYR4ToBYR3(input, output);
			break;

		case PIXEL_FORMAT_DPX_50:
			return ConvertBYR4ToDPX(input, output);
			break;

		default:
			assert(0);
			break;
		}

	case PIXEL_FORMAT_B64A:
		switch (output->format)
		{
		case PIXEL_FORMAT_DPX_50:
			return ConvertB64AToDPX(input, output);
			break;

		case PIXEL_FORMAT_RG48:
			return ConvertB64AToRG48(input, output);
			break;

		default:
			assert(0);
			break;
		}

	case PIXEL_FORMAT_RG48:
		switch (output->format)
		{
		case PIXEL_FORMAT_DPX_50:
			return ConvertRG48ToDPX(input, output);
			break;

		default:
			assert(0);
			break;
		}

	default:
		assert(0);
		break;
	}

	return CODEC_ERROR_PIXEL_FORMAT;
}

bool GetDimension(const char *string, DIMENSION *dimension_out)
{
	int value;
	if (string != NULL && dimension_out != NULL && sscanf(string, "%d", &value) == 1) {
		*dimension_out = (DIMENSION)value;
		return true;
	}
	return false;
}

bool GetPixelFormat(const char *string, PIXEL_FORMAT *format_out)
{
	if (string != NULL && format_out != NULL)
	{
		PIXEL_FORMAT format = PixelFormat(string);
		if (format != PIXEL_FORMAT_UNKNOWN) {
			*format_out = format;
		}
		return true;
	}
	return false;
}

/*!
	@brief Main entry point for the image conversion tool

	To see the program arguments, run the program with --help.
*/
int main(int argc, char *argv[])
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;

	FILE_INFO info;

	IMAGE input;
	IMAGE output;

	// Set the default image dimensions
	DIMENSION image_width = 1920;
	DIMENSION image_height = 1080;

	// Set the default input and output pixel formats
	PIXEL_FORMAT input_format = PIXEL_FORMAT_UNKNOWN;
	PIXEL_FORMAT output_format = PIXEL_FORMAT_UNKNOWN;

	int c;
	//int digit_optind = 0;
	int input_file_index = 0;
	int output_file_index = 0;

	bool verbose_flag = false;
	bool help_flag = false;

	static struct option long_options[] = {
		{"width", 1, 0, 0},
		{"height", 1, 0, 0},
		{"pixel", 1, 0, 0},
		{"output", 1, 0, 0},
		{"verbose", 0, 0, 0},
		{"help", 0, 0, 0},
		{NULL, 0, NULL, 0}
	};
    const int long_options_length = sizeof(long_options)/sizeof(long_options[0]);

	// Map long options to short options
	static char short_options[] = {
		0, 'w', 'h', 'p', 'o', 'v', '?'
	};
	const int short_options_length = sizeof(short_options)/sizeof(short_options[0]);

	int option_index = 0;
    
    assert(short_options_length == long_options_length);

	InitImage(&input);
	InitImage(&output);

	// Process the command-line options
	while ((c = getopt_long(argc, argv, "w:h:p:v", long_options, &option_index)) != -1)
	{
		//int this_option_optind = optind ? optind : 1;

		// Convert a long option to a short option
		if (c == 0) {
			c = short_options[option_index];
		}

		// Process the command-line option
		switch (c)
		{
		case 'w':
			if (!GetDimension(optarg, &image_width)) {
				printf("Bad image width\n");
				help_flag = true;
			}
			break;

		case 'h':
			if (!GetDimension(optarg, &image_height)) {
				printf("Bad image height\n");
				help_flag = true;
			}
			break;

		case 'p':
			if (!GetPixelFormat(optarg, &input_format)) {
				printf("Bad input pixel format\n");
				help_flag = true;
			}
			break;

		case 'o':
			if (!GetPixelFormat(optarg, &output_format)) {
				printf("Bad output pixel format\n");
				help_flag = true;
			}
			break;

		case 'v':
			verbose_flag = true;
			break;

		case '?':
			help_flag = true;
			break;

		default:
			printf("Unknown option '%s'\n", argv[optind]);
			help_flag = true;
			break;
		}
	}

	if (help_flag)
	{
		printf("Usage: convert [-w width] [-h height] [-p input_pixel_format] [-o output_pixel_format] infile outfile\n");
		return 0;
	}

	if (! ((optind + 1) < argc))
	{
		fprintf(stderr, "Must provide input and output pathname arguments\n");
		return CODEC_ERROR_MISSING_ARGUMENT;
	}

	// The last two command-line arguments are the input and output pathnames
	input_file_index = optind;
	output_file_index = input_file_index + 1;

	// Determine the type of input file and the pixel format
	GetFileInfo(argv[input_file_index], &info);

	// The command-line options override the file information
	if (input_format == PIXEL_FORMAT_UNKNOWN) {
		input_format = info.format;
	}

	if (input_format == PIXEL_FORMAT_UNKNOWN) {
		printf("Could not determine the input pixel format: %s\n", argv[input_file_index]);
		return 1;

	}

	// Read the type of file based on the file information
	switch (info.type)
	{
	case FILE_TYPE_DPX:
		// Read the input DPX file
		error = DPX_ReadImage(&input, argv[input_file_index]);
		break;

	case FILE_TYPE_RAW:
		// Read the raw input file
		error = RAW_ReadImage(&input, argv[input_file_index], image_width, image_height, input_format);
		break;

	default:
		printf("Could not determine the input file type: %s\n", argv[input_file_index]);
		error = CODEC_ERROR_UNSUPPORTED_FILE_TYPE;
		break;
	}

	if (error != CODEC_ERROR_OKAY) {
		printf("Could not read the input file: %s\n", argv[input_file_index]);
		return 1;
	}

	// Determine the type of output file and the pixel format
	GetFileInfo(argv[output_file_index], &info);

	// The command-line options override the file information
	if (output_format == PIXEL_FORMAT_UNKNOWN) {
		output_format = info.format;
	}

	if (output_format == PIXEL_FORMAT_UNKNOWN) {
		printf("Could not determine the output pixel format: %s\n", argv[output_file_index]);
		return 1;
	}

	// Allocate the output image
	AllocImageCopy(NULL, &output, &input, output_format);

	// Convert the DPX image to the output image in BYR4 format
	ConvertImage(&input, &output);

	switch (info.type)
	{
	case FILE_TYPE_DPX:
		DPX_WriteImage(&output, argv[output_file_index]);
		break;

	case FILE_TYPE_RAW:
		RAW_WriteImage(&output, argv[output_file_index]);
		break;

	default:
		assert(0);
		break;
	}

	// Free the image buffers
	ReleaseImage(NULL, &input);
	ReleaseImage(NULL, &output);

	return 0;
}
