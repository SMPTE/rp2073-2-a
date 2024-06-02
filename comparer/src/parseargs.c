/*!	@file parseargs.c

	Implementation of the routines for parsing command-line arguments

	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#include "headers.h"
#include "getopt.h"
#include "parseargs.h"


/*!
	@brief Help message describing the program usage

	The help message is customized according to which parts of the VC-5
	standard are supported in this implementation.
*/
static const char help_message[] =
{
	"NAME\n"
	"\t%s - VC-5 Reference Codec.\n"
	"\n"
	"USAGE\n"
#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
	"\t%s [-p PixelFormat] encoded_image packed_image\n"
#else
	"\t%s [-p PixelFormat] encoded_image packed_image\n"
#endif
	"\n"
	"DESCRITION\n"
	"\t-f PixelFormat\n"
	"\t\tformat of packed image output by the image repacking process.\n"
	"\n"
};


/*!
	@brief Print the help message describing how to use the program
*/
CODEC_ERROR PrintHelpMessage(int argc, char *argv[])
{
	assert(argc >= 1);
	fprintf(stderr, help_message, argv[0], argv[0]);
	return CODEC_ERROR_OKAY;
}

/*!
	@brief Convert a command-line argument to an image dimension
*/
bool GetDimension(const char *string, DIMENSION *dimension_out)
{
	int value;
	if (string != NULL && dimension_out != NULL && sscanf(string, "%d", &value) == 1) {
		*dimension_out = (DIMENSION)value;
		return true;
	}
	return false;
}

/*!
	@brief Convert a command-line argument to a pixel format
*/
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
	@brief Parse the program command-line arguments to get the encoding parameters

	The encoding parameters must be initialized by the caller with default values.

	@todo Should replace the routines that parse integer strings with routines that
	check whether the string is a valid integer.
*/
CODEC_ERROR ParseParameters(int *argc_ref, char *argv[], PARAMETERS *parameters)
{
	FILE_INFO info;

	int image1_file_index = 0;
	int image2_file_index = 0;

	int argc = *argc_ref;
	//int digit_optind = 0;
	int c;

	bool verbose_flag = false;
	bool help_flag = false;

	static struct option long_options[] = {
		{"width", 1, 0, 0},
		{"height", 1, 0, 0},
		{"pixel", 1, 0, 0},
		{"verbose", 0, 0, 0},
		{"help", 0, 0, 0},
		{NULL, 0, NULL, 0}
	};
	const int long_options_length = sizeof(long_options)/sizeof(long_options[0]);

	// Map long options to short options
	static char short_options[] = {
		'w', 'h', 'p', 'v', '?', 0,
	};
	const int short_options_length = sizeof(short_options)/sizeof(short_options[0]);

	int option_index = 0;

	assert(short_options_length == long_options_length);

	// Process the command-line options
	//while ((c = getopt_long(argc, argv, "w:h:p:v", long_options, &option_index)) != -1)
	while ((c = getopt_long(argc, argv, "w:h:p:v", long_options, &option_index)) != -1)
	{
		//int this_option_optind = optind ? optind : 1;

		// Convert a long option to a short option
		if (c == 0)
		{
			assert(0 <= option_index && option_index < short_options_length);
			c = short_options[option_index];
		}

		// Process the command-line option
		switch (c)
		{
		case 'w':
			if (!GetDimension(optarg, &parameters->image.width)) {
				printf("Bad image width\n");
				help_flag = true;
			}
			break;

		case 'h':
			if (!GetDimension(optarg, &parameters->image.height)) {
				printf("Bad image height\n");
				help_flag = true;
			}
			break;

		case 'p':
			if (!GetPixelFormat(optarg, &parameters->image.format)) {
				printf("Bad input pixel format\n");
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

	// The remaining command-line arguments are pathnames
	image1_file_index = (optind < argc) ? optind : 0;
	image2_file_index = ((optind + 1) < argc) ? (optind + 1) : 0;

	if (image1_file_index == 0 || image2_file_index == 0)
	{
		fprintf(stderr, "Must provide input and output pathname arguments\n");
		PrintHelpMessage(argc, argv);
		return CODEC_ERROR_MISSING_ARGUMENT;
	}

	// Determine the type and pixel format of the first input file
	GetFileInfo(argv[image1_file_index], &info);

	// The command-line options override the file information
	if (parameters->image.format == PIXEL_FORMAT_UNKNOWN) {
		parameters->image.format = info.format;
	}

	// Replace the command-line arguments with the input and output pathnames
	argv[1] = argv[image1_file_index];
	argv[2] = argv[image2_file_index];

	// Update the number of command-line arguments
	*argc_ref = 3;

	return CODEC_ERROR_OKAY;
}
