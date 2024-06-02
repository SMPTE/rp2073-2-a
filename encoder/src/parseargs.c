/*!	@file encoder/src/parseargs.c

	Implementation of the routines for parsing command-line arguments

	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#include <ctype.h>

#if _MSC_VER
// Use a third-party implementation of getopt and getopt_long
#include "ya_getopt.h"
#else
// The source code and header files for getopt and getopt_long are available on macOS and Linux
#include <getopt.h>
#endif

#include "headers.h"
#include "utilities.h"
#include "arguments.h"
#include "parseargs.h"


/*!
	@brief Help message describing how to execute this program

	The usage message is customized according to which parts of the VC-5
	standard are supported by this implementation.
*/
static const char usage_message[] =
{
	"NAME\n"
	"\t%s - VC-5 Sample Encoder\n"
	"\n"
	"USAGE\n"
	"\t%s [options] <image file 1> <image file 2> … <image file n> <bitstream file>\n"
	"\n"
	"OPTIONS\n\n"
	"\t-w <image width>\n"
	"\t\tWidth of the input image (samples per row).\n"
    "\n"
    "\t-h <image height>\n"
    "\t\tHeight of the packed input image (rows of samples).\n"
    "\n"
	"\t-p <file format>\n"
	"\t\tPixel format of the packed input image.\n"
    "\n"
#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
	"\t-f <image format>\n"
	"\t\tRepresentation of the input image in the bitstream (part 3 only).\n"
    "\n"
#endif
    "\t-P <parts list>\n"
    "\t\tComma-separated list of VC-5 part numbers enabled at runtime.\n"
    "\n"
#if VC5_ENABLED_PART(VC5_PART_SECTIONS)
    "\t-S <sections list>\n"
    "\t\tEnable encoding of the comma-separated list of sections into the bitstream.\n"
    "\n"
#endif
#if VC5_ENABLED_PART(VC5_PART_SECTIONS) && VC5_ENABLED_PART(VC5_PART_LAYERS)
    "\t-L <image section layers>\n"
    "\t\tComma-separated list of the number of nested layers per image section.\n"
    "\n"
#endif
#if VC5_ENABLED_PART(VC5_PART_METADATA)
    "\t-M <metadata>\n"
    "\t\tFile of metadata in XML format as described in ST 2073-7 Annex A.\n"
    "\n"
#endif
	"\t-Q q1,q2,q3,q4,q5,q6,q7,q8,q9\n"
	"\t\tQuantization table entries (lowpass quantization q0 is always 1).\n"
    "\n"
	"\t-B <bandfile pathname>[,<channel mask>][,<subband mask>]\n"
	"\t\tPathname of the bandfile with optional channel and subband masks\n"
	"\t\tthat specify which subbands to write to the bandfile.\n"
    "\n"
    "\t-v\n"
    "\t\tEnable verbose output.\n"
	"\n"
    "\t-z\n"
    "\t\tEnable extra output for debugging.\n"
	"\n"
    "\t-q\n"
    "\t\tSuppress all output to the terminal (overrides verbose and debug).\n"
	"\n"
};

/*!
	@brief Print the usage message describing how to use the program
*/
CODEC_ERROR PrintUsageMessage(int argc, const char *argv[])
{
	assert(argc >= 1);
	fprintf(stderr, "\n");
	fprintf(stderr, usage_message, "encoder", "encoder");
	return CODEC_ERROR_OKAY;
}

#if 0
/*!
	@brief Set the command-line parameters from the input file format
*/
CODEC_ERROR SetInputFileParameters(FILE_INFO *info, PARAMETERS *parameters)
{
	// The command-line options override the file information
	if (parameters->input.format == PIXEL_FORMAT_UNKNOWN) {
		parameters->input.format = info->format;
	}

	if (parameters->input.precision == 0) {
		parameters->input.precision = info->precision;
	}

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
	if (parameters->encoded.format == IMAGE_FORMAT_UNKNOWN) {
		// Set the image format using the pixel format of the input file
		parameters->encoded.format = DefaultImageFormat(info->format);
	}

	if (parameters->pattern_width == 0 ||
		parameters->pattern_height == 0 ||
		parameters->components_per_sample == 0)
	{
		switch (info->format)
		{
		case PIXEL_FORMAT_B64A:
			parameters->pattern_width = 1;
			parameters->pattern_height = 1;
			parameters->components_per_sample = 4;
			break;

		case PIXEL_FORMAT_RG48:
			parameters->pattern_width = 1;
			parameters->pattern_height = 1;
			parameters->components_per_sample = 3;
			break;

		case PIXEL_FORMAT_BYR4:
			parameters->pattern_width = 2;
			parameters->pattern_height = 2;
			parameters->components_per_sample = 1;
			break;
                
#if VC5_ENABLED_PART(VC5_PART_COLOR_SAMPLING)
        case PIXEL_FORMAT_NV12:
            parameters->pattern_width = 2;
            parameters->pattern_height = 2;
            parameters->components_per_sample = 0;      // Not applicable to images with subsampled color differences
            break;
#endif
		default:
			// Not able to set the command-line parameters from the input file format
			assert(0);
			return CODEC_ERROR_BAD_IMAGE_FORMAT;
			break;
		}

	}
#endif

	return CODEC_ERROR_OKAY;
}
#endif


#if VC5_ENABLED_PART(VC5_PART_SECTIONS) && VC5_ENABLED_PART(VC5_PART_LAYERS)

/*!
	@brief Set the flags that indicate which sections in VC-5 Part 6 are enabled
 
	The argument is a list of comma-separated integers for the section numbers
 in the VC-5 Part 2 conformance specification that are enabled for this invocation
 of the encoder.
 
 Note: Enabling sections at runtime has no effect unless support for sections
 is compiled into the program by enabling the corresponding compile-time switch
 for VC-5 part 6 (sections).
 */
bool GetImageSectionLayers(const char *string, PARAMETERS *parameters)
{
    if (string != NULL && parameters != NULL)
    {
        const char *p = string;
        int section_index;
        
        assert(p != NULL);
        for (section_index = 0; *p != '\0'; section_index++)
        {
            char *q = NULL;
            if (!isdigit(*p)) break;
            parameters->section_layer_count[section_index] = (COUNT)strtol(p, &q, 10);
            
            // Advance to the next layer count in the command-line argument
            p = (*q != '\0') ? q + 1 : q;
        }
        
        // Set the number of image sections
        parameters->image_section_count = section_index;
        
        // Should have parsed all of the numbers in the argument string
        assert(*p == '\0');
        return true;
    }
    
    // Invalid input arguments
    return false;
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
 
    This routine updates the parameters with values obtained from the command line.
    The result is an accurate representation of parameters present on the command
    line and the default values set before this routine was called.  Values obtained
    from the command line override default values.

	@todo Should replace the routines that parse integer strings with routines that
	check whether the string is a valid integer.
 
    @todo Add command-line argument for the UUID used for the unique image identifier.
*/
CODEC_ERROR ParseParameters(int argc, const char *argv[], PARAMETERS *parameters)
{
	//CODEC_ERROR error = CODEC_ERROR_OKAY;

	//FILE_INFO info;

	// Table specifying the long form of the command-line options
	static struct option long_options[] =
	{
		{"width", 1, 0, 0},				// Width of the input image (samples per row)
		{"height", 1, 0, 0},			// Height of the input image (rows per image)
		{"pixel", 1, 0, 0},				// Pixel format (four character code)
		{"format", 1, 0, 0},			// Image format (VC-5 Part 3 only)
		{"precision", 1, 0, 0},			// Number of bits per input pixel component
		{"quant", 1, 0, 0},				// Vector of quantization values (one per subband)
		{"channel", 1, 0, 0},			// Order of channels to encode into the bitstream
		{"lowpass", 1, 0, 0},			// Number of bits per lowpass coefficient
		{"parts", 1, 0, 0},				// Parts of the VC-5 standard supported by this program
        //{"layers", 1, 0, 0},            // Encode files matching the string format as layers
        //{"count", 1, 0, 0},             // Number of layers to encode into the bitstream
        {"sections", 1, 0, 0},          // Enable encoding of section elements in the bitstream
        {"layers", 1, 0, 0},            // Number of nested layers per image section
        {"metadata", 1, 0, 0},			// Pathname of an XML file containing metadata
        {"bandfile", 1, 0, 0},			// Write wavelet bands to a bandfile
        {"verbose", 0, 0, 0},			// Enable verbose output to the terminal
        {"debug", 0, 0, 0},				// Enable extra output for debugging
        {"quiet", 0, 0, 0},				// Suppress all output to the terminal
		{"help", 0, 0, 0},				// Print the program usage message
        {NULL, 0, NULL, 0}
	};

	// Map long options to short options
	static char short_options[] = {
		//'w', 'h', 'p', 'f', 'b', 'q', 'c', 'l', 'P', 'L', 'N', 'S', 'B', 'v', '?', 0
        //'w', 'h', 'p', 'f', 'b', 'q', 'c', 'l', 'P', 'S', 'L', 'B', 'v', '?', 0
        'w', 'h', 'p', 'f', 'b', 'Q', 'c', 'l', 'P', 'S', 'L', 'M', 'B', 'v', 'z', 'q', '?', 0
	};
	//const int short_options_length = sizeof(short_options)/sizeof(short_options[0]);

	//bool verbose_flag = false;
    parameters->verbose_flag = false;
	bool help_flag = false;

	const int channel_order_table_length = sizeof(parameters->channel_order_table)/sizeof(parameters->channel_order_table[0]);

	int option_index = 0;
    int input_image_index;
    int input_pathname_index;
	int output_stream_index;
    //int output_pathname_index;
	int c;
    
    if (argc < 2)
    {
        // Print the usage message if no command-line arguments
        PrintUsageMessage(argc, argv);
        exit(0);
    }

	// Process the command-line options
	//while ((c = getopt_long(argc, (char **)argv, "w:h:p:f:b:q:c:l:P:L:N:S:B:v", long_options, &option_index)) != -1)
	while ((c = getopt_long(argc, (char **)argv, "w:h:p:f:b:Q:c:l:P:S:L:M:B:vzq", long_options, &option_index)) != -1)
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
			if (!GetDimension(optarg, &parameters->width)) {
				printf("Bad image width: %s\n", optarg);
				help_flag = true;
			}
			break;

		case 'h':
			if (!GetDimension(optarg, &parameters->height)) {
				printf("Bad image height: %s\n", optarg);
				help_flag = true;
			}
			break;

		case 'p':
			if (!GetPixelFormat(optarg, &parameters->pixel_format)) {
				printf("Bad input pixel format: %s\n", optarg);
				help_flag = true;
			}
			break;

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
		case 'f':
			if (!GetImageFormat(optarg, &parameters->image_format)) {
				printf("Bad image format: %s\n", optarg);
				help_flag = true;
			}
			break;
#endif
		case 'b':
			if (!GetPrecision(optarg, &parameters->bits_per_component)) {
				printf("Bad bits per component: %s\n", optarg);
				help_flag = true;
			}
			break;

		case 'Q':
			if (!GetQuantization(optarg, parameters->quant_table)) {
				printf("Could not parse quantization values\n");
				help_flag = true;
			}
			break;

		case 'c':
			if (!GetChannelOrder(optarg, parameters->channel_order_table, &parameters->channel_order_count, channel_order_table_length)) {
				printf("Could not parse channel ordering\n");
				help_flag = true;
			}
			break;

		case 'l':
			if (!GetPrecision(optarg, &parameters->lowpass_precision)) {
				printf("Bad lowpass precision: %s\n", optarg);
				help_flag = true;
			}
			break;

		case 'P':
			if (!GetEnabledParts(optarg, &parameters->enabled_parts)) {
				printf("Invalid VC-5 parts: %s\n", optarg);
				help_flag = true;
			}
			break;
#if 1			
		case 'B':
			if (!GetBandfileInfo(optarg, &parameters->bandfile)) {
				printf("Bad bandfile information\n");
				help_flag = true;
			}
			break;
#endif
#if 0   //VC5_ENABLED_PART(VC5_PART_LAYERS)
        case 'L':
            parameters->layer_flag = true;
            input_image_index = optind - 1;
            break;

        case 'N':
            if (!GetLayerCount(optarg, &parameters->layer_count)) {
                printf("Bad layer count: %s\n", optarg);
                help_flag = true;
            }
            break;
#endif
#if VC5_ENABLED_PART(VC5_PART_SECTIONS)
        case 'S':
            if (!GetEnabledSections(optarg, &parameters->enabled_sections)) {
                printf("Invalid VC-5 sections: %s\n", optarg);
                help_flag = true;
            }
            break;
#endif
#if VC5_ENABLED_PART(VC5_PART_SECTIONS) && VC5_ENABLED_PART(VC5_PART_LAYERS)
        case 'L':
            if (!GetImageSectionLayers(optarg, parameters)) {
                printf("Invalid list of layers per image section: %s\n", optarg);
            }
            break;
#endif
#if VC5_ENABLED_PART(VC5_PART_METADATA)
        case 'M':
        	if (!GetPathname(optarg, parameters->metadata_pathname)) {
                printf("Could not get metadata pathname: %s\n", optarg);
        	}
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

	if (help_flag)
	{
		// Print the usage message and exit
		PrintUsageMessage(argc, argv);
		exit(0);
	}

    // The first of the remaining command-line arguments is the first input image
    input_image_index = optind;
    
    // The last command-line argument is the output pathname
    output_stream_index = argc - 1;
    //printf("Output file: %s\n", argv[output_stream_index]);
    
    // The remaining command-line arguments before the output pathname are the input pathnames
    memset(parameters->input_pathname_list.pathname_data, 0, sizeof(parameters->input_pathname_list.pathname_data));
    parameters->input_pathname_list.pathname_count = 0;
    
    // Copy each of the input pathnames to the parameters
    for (input_pathname_index = input_image_index;
         input_pathname_index < output_stream_index;
         input_pathname_index++)
    {
        int pathname_list_index = (input_pathname_index - input_image_index);

        // Initialize the pathname data structure
        InitPathnameData(&parameters->input_pathname_list.pathname_data[pathname_list_index]);
        
        // Copy the pathname from the command-line arguments to the pathname data structure
        CopyPathname(parameters->input_pathname_list.pathname_data[pathname_list_index].pathname,
                     argv[input_pathname_index],
                     sizeof(parameters->input_pathname_list.pathname_data[pathname_list_index].pathname));
    }
    parameters->input_pathname_list.pathname_count = (output_stream_index - input_image_index);
    
    // Copy the output pathname to the parameters
    CopyPathname(parameters->output_pathname, argv[output_stream_index], sizeof(parameters->output_pathname));

	return CODEC_ERROR_OKAY;
}
