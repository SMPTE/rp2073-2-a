/*!	@file decoder/src/parseargs.c

	Implementation of the routines for parsing command-line arguments

	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#include "headers.h"
#include "arguments.h"
#include "parseargs.h"

#if _MSC_VER
#include "ya_getopt.h"
#else
#include <getopt.h>
#endif

/*!
	@brief Help message describing how to use this program

	The usage message is customized according to which parts of the VC-5
	standard are supported in this implementation.
*/
static char usage_message[] =
{
	"NAME\n"
	"\t%s - VC-5 Reference Decoder\n"
	"\n"
	"USAGE\n"
	"\t%s [options] <bitstream file> <image file 1> <image file 2> … <image file n>\n"
	"\n"
	"OPTIONS\n\n"
	"\t-w <image width>\n\t\tWidth of the encoded image provided as an external parameter.\n"
	"\n"
	"\t-h <image height>\n\t\tHeight of the encoded image provided as an external parameter.\n"
	"\n"
    "\t-p <file format>\n\t\tPixel format of the image input to the encoder.\n"
    "\n"
    "\t-o <file format>\n\t\tPixel format of the output image.\n"
    "\n"
    "\t-P <parts list>\n"
    "\t\tComma-separated list of VC-5 part numbers enabled at runtime.\n"
    "\n"
#if VC5_ENABLED_PART(VC5_PART_SECTIONS)
    "\t-S <sections list>\n"
    "\t\tEnable decoding of the specified section elements in the bitstream.\n"
    "\n"
#endif
#if VC5_ENABLED_PART(VC5_PART_METADATA)
    "\t-M <metadata>\n"
    "\t\tFile of metadata extracted by the decoder in XML format.\n"
    "\n"
#endif
	"\t-B <bandfile pathname>[,<channel mask>][,<subband mask>]\n"
	"\t\tPathname of the bandfile with optional channel and subband masks\n"
	"\t\tthat specify which subbands to write to the bandfile.\n"
	"\n"
    "\t-v\n\t\tEnable verbose output.\n"
	"\n"
    "\t-z\n"
    "\t\tEnable extra output for debugging.\n"
	"\n"
    "\t-q\n"
    "\t\tSuppress all output to the terminal (overrides verbose and debug).\n"
	"\n"
};


/*!
	@brief Print the uage message describing how to use this program
*/
CODEC_ERROR PrintUsageMessage(int argc, char *argv[])
{
	(void)argv;
	assert(argc >= 1);
	fprintf(stderr, "\n");
	fprintf(stderr, usage_message, "decoder", "decoder");
	return CODEC_ERROR_OKAY;
}

#if 0
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
#endif

#if 0
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
#endif

#if 0
/*!
	@brief Parse the bandfile information

	The bandfile information is a string of fields delimited by commas.  The first field is the
	pathname of the bandfile, the second and third fields are the channel mask and subband mask
	as hexadecimal numbers.  The masks specify which channels and subbands to write to the bandfile.
	
	@todo Add a lowpass mask as an optional fourth argument in the band file information string
	to specify which lowpass bands in the intermediate wavelets are included in the bandfile or
	use extra bits in the subband mask.
*/
bool GetBandfileInfo(const char *string, BANDFILE_INFO *bandfile)
{
	char *p, *q;
	size_t n;

	// Assume that all subbands will be written
	bandfile->channel_mask = UINT32_MAX;
	bandfile->subband_mask = UINT32_MAX;
	
	// Clear the bandfile pathname
	memset(bandfile->pathname, 0, sizeof(bandfile->pathname));

	// Find the end of the bandfile pathname
	p = (char *)string;
	q = strchr(p, ',');

	if (q == NULL)
	{
		// The entire argument is the bandfile pathname
		strcpy(bandfile->pathname, string);
		return true;
	}

	// Copy the bandfile pathname up to but not including the comma
	n = q - p;
	strncpy(bandfile->pathname, string, n);

	// Advance to the channel mask
	p = q + 1;

	// Convert the next substring to the channel mask (hexadecimal)
	bandfile->channel_mask = (uint32_t)strtol(p, &q, 16);
	if (*q != ',') {
		return true;
	}

	// Advance to the subband mask
	p = q + 1;

	// Convert the next substring to the subband mask (hexadecimal)
	bandfile->subband_mask = (uint32_t)strtol(p, &q, 16);

	return true;
}
#endif



#if VC5_ENABLED_PART(VC5_PART_METADATA)

/*!
	@brief Set a pathname in the parameters data structure
 
	The routine assme that the pathname is a buffer in the parameters data structure
	allocated large enough to hold the maximum pathname on the current system.
 */
bool GetPathname(const char *string, char *pathname)
{
	strncpy(pathname, string, PATH_MAX);
	pathname[PATH_MAX - 1] = '\n';

	return true;
}

#endif


/*!
	@brief Parse the program command-line arguments to get the encoding parameters

	The encoding parameters must be initialized by the caller with default values.

	@todo Should replace the routines that parse integer strings with routines that
	check whether the string is a valid integer.

	@todo Finish work on the names option for printing information about files listed on the command line.
*/
CODEC_ERROR ParseParameters(int argc, char *argv[], PARAMETERS *parameters, FILELIST *input, FILELIST *output)
{
	int c;

	bool help_flag = false;

	static struct option long_options[] =
    {
        //{"input",  required_argument, NULL, 'i'},    //!< Input bitstream file
        {"width",    required_argument, NULL, 'w'},    //!< Image width
        {"height",   required_argument, NULL, 'h'},    //!< Image height
        {"pixel",    required_argument, NULL, 'p'},    //!< Pixel format
        {"output",   required_argument, NULL, 'o'},    //!< Output format
        {"parts",    required_argument, NULL, 'P'},    //!< Parts of the VC-5 standard supported by this program
        //{"layers", no_argument,       NULL, 'L'},    //!< Decode and output all layers with one layer per file
        {"sections", required_argument, NULL, 'S'},    //!< Enable decoding section elements in the bitstream
        //{"names",  no_argument,       NULL, 'N'},    //!< Parse and output information for image filenames
        {"metadata", required_argument, NULL, 'M'},    //!< Metadata output file in XML format
        {"bandfile", required_argument, NULL, 'B'},    //!< Write intermediate results to a band file (for debugging)
        {"verbose",  no_argument,       NULL, 'v'},    //!< Enable verbose output (for debugging)
		{"debug",    no_argument,       NULL, 'z'},    //!< Enable extra output for debugging
        {"quiet",    no_argument,       NULL, 'q'},    //!< Suppress all output to the terminal
        {"help",     no_argument,       NULL, '?'},    // Print program usage information
        {NULL, 0, NULL, 0}
	};
	//const int long_options_length = sizeof(long_options)/sizeof(long_options[0]);
    
	int option_index = 0;
    
    if (argc < 2)
    {
        // Print the usage message and exit
        PrintUsageMessage(argc, argv);
        exit(0);
    }

	// Process the command-line options
	//while ((c = getopt_long(argc, argv, "i:w:h:p:o:P:LS:B:v", long_options, &option_index)) != -1)
	while ((c = getopt_long(argc, argv, "w:h:p:o:P:S:M:B:vzq", long_options, &option_index)) != -1)
	{
        assert(c != 0);

		// Process the command-line option
		switch (c)
		{
#if 0
        case 'i':
            AddFileListPathname(input, optarg);
            break;
#endif
		case 'w':
			if (!GetDimension(optarg, &parameters->input.width)) {
				printf("Bad image width\n");
				help_flag = true;
			}
			break;

		case 'h':
			if (!GetDimension(optarg, &parameters->input.height)) {
				printf("Bad image height\n");
				help_flag = true;
			}
			break;

		case 'p':
			if (!GetPixelFormat(optarg, &parameters->input.format)) {
				printf("Bad input pixel format\n");
				help_flag = true;
			}
			break;

		case 'o':
			if (!GetPixelFormat(optarg, &parameters->output.format)) {
				printf("Bad output pixel format\n");
				help_flag = true;
			}
			break;

		case 'P':
			if (!GetEnabledParts(optarg, &parameters->enabled_parts)) {
				printf("Invalid VC-5 parts: %s\n", optarg);
				help_flag = true;
			}
			break;

		case 'B':
			if (!GetBandfileInfo(optarg, &parameters->bandfile)) {
				printf("Bad bandfile information\n");
				help_flag = true;
			}
			break;
                
#if VC5_ENABLED_PART(VC5_PART_LAYERS)
        case 'L':
            parameters->layer_flag = true;
            //output_image_index = optind - 2;
            break;
#endif
#if VC5_ENABLED_PART(VC5_PART_SECTIONS)
        case 'S':
            if (!GetEnabledSections(optarg, &parameters->enabled_sections)) {
                printf("Invalid VC-5 sections: %s\n", optarg);
                help_flag = true;
            }
            break;
#if 0
        case 'N':
            // The names switch implies that section processing is enabled
            parameters->section_flag = true;
            parameters->filenames_flag = true;
            break;
#endif
#endif
#if VC5_ENABLED_PART(VC5_PART_METADATA)
        case 'M':
        	if (!GetPathname(optarg, parameters->metadata.output_pathname)) {
                printf("Could not get metadata pathname: %s\n", optarg);
        	}
        	parameters->metadata.output_flag = true;
        	break;
#endif
        case 'v':
			parameters->verbose_flag = true;
			break;

		case 'z':
			parameters->debug_flag = true;
			break;

		case 'q':
			parameters->quiet_flag = true;
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
    
    // The remaining command line arguments must be input pathname followed by the output pathnames
    
    if (optind < argc)
    {
        AddFileListPathname(input, argv[optind]);
        optind++;
    }
    
    for (; optind < argc; optind++)
    {
        if (IsPathnameTemplate(argv[optind]))
        {
            AddFileListTemplate(output, argv[optind]);
        }
        else
        {
            AddFileListPathname(output, argv[optind]);
        }
    }

    if (help_flag)
	{
		PrintUsageMessage(argc, argv);
		return CODEC_ERROR_USAGE_INFO;
	}

	return CODEC_ERROR_OKAY;
}
