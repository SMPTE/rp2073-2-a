/*! @file common/src/dpxfile.c

	Basic routines for reading and writing images to a DPX file.

	This module is not part of the codec, but is provided for use by
	programs that test the codec and read or write test image to a
	file in DPX format.

	The common 10-bit RGB DPX format is the only pixel format that is supported.

	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#include <string.h>
#include "headers.h"
#include <sys/stat.h>
#include "dpxfile.h"


// Data types used in the Cineon DPX file format specification
typedef uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef int32_t S32;
typedef float R32;
typedef char ASCII;

static const U32 SPDX = 0x53445058;
static const U32 XPDS = 0x58504453;

// Default offset to the start of the image data
//static const size_t default_header_size = 2048;


// Global flag that controls the byte-swapping routines (see below)
static bool byte_swap_flag = false;


// Conditional byte-swapping routines

// Conditional byte-swapping routines
static U16 DPX_Swap16(U16 word)
{
	if (byte_swap_flag) return Swap16(word);
	else return word;
}

static U32 DPX_Swap32(U32 word)
{
	if (byte_swap_flag) return Swap32(word);
	else return word;
}

static float DPX_Swap32f(float x)
{
	if (byte_swap_flag) return (float)Swap32((uint32_t)x);
	else return x;
}


//! DPX pixel formats
enum
{
	DPX_RGB_10BIT_444 = 50,
	DPX_YUV_10BIT_422 = 100,
	DPX_YUVA_16BIT_4444 = 103,
	DPX_YUVA_10BIT_4444 = 103,
};

/*!
	@brief File information header
	
	This header was copied from the Cineon DPX file format specification.
*/
typedef struct file_information
{
	U32   magic_num;        // Magic number 0x53445058 (SDPX) or 0x58504453 (XPDS) 
	U32   offset;           // Offset to image data in bytes 
	ASCII vers[8];          // Which header format version is being used (v1.0)
	U32   file_size;        // File size in bytes 
	U32   ditto_key;        // Read time short cut - 0 = same, 1 = new 
	U32   gen_hdr_size;     // Generic header length in bytes 
	U32   ind_hdr_size;     // Industry header length in bytes 
	U32   user_data_size;   // User-defined data length in bytes 
	ASCII file_name[100];   // Iamge file name 
	ASCII create_time[24];  // File creation date "yyyy:mm:dd:hh:mm:ss:LTZ" 
	ASCII creator[100];     // File creator's name 
	ASCII project[200];     // Project name 
	ASCII copyright[200];   // Right to use or copyright info 
	U32   key;              // Encryption ( FFFFFFFF = unencrypted ) 
	ASCII Reserved[104];    // Reserved field TBD (need to pad) 

} File_Information;


/*!
	@brief Image information header
	
	This header was copied from the Cineon DPX file format specification.
*/
typedef struct _image_information
{
	U16    orientation;          /* image orientation */
	U16    element_number;       /* number of image elements */
	U32    pixels_per_line;      /* or x value */
	U32    lines_per_image_ele;  /* or y value, per element */
	struct _image_element
	{
		U32    data_sign;        /* data sign (0 = unsigned, 1 = signed ) */
								 /* "Core set images are unsigned" */
		U32    ref_low_data;     /* reference low data code value */
		R32    ref_low_quantity; /* reference low quantity represented */
		U32    ref_high_data;    /* reference high data code value */
		R32    ref_high_quantity;/* reference high quantity represented */
		U8     descriptor;       /* descriptor for image element */
		U8     transfer;         /* transfer characteristics for element */
		U8     colorimetric;     /* colormetric specification for element */
		U8     bit_size;         /* bit size for element */
		U16    packing;          /* packing for element */
		U16    encoding;         /* encoding for element */
		U32    data_offset;      /* offset to data of element */
		U32    eol_padding;      /* end of line padding used in element */
		U32    eo_image_padding; /* end of image padding used in element */
		ASCII  description[32];  /* description of element */
	} image_element[8];          /* NOTE THERE ARE EIGHT OF THESE */

	U8 reserved[52];             /* reserved for future use (padding) */
} Image_Information;

/*!
	@brief Image orientation header
	
	This header was copied from the Cineon DPX file format specification.
*/
typedef struct _image_orientation
{
	U32   x_offset;               /* X offset */
	U32   y_offset;               /* Y offset */
	R32   x_center;               /* X center */
	R32   y_center;               /* Y center */
	U32   x_orig_size;            /* X original size */
	U32   y_orig_size;            /* Y original size */
	ASCII file_name[100];         /* source image file name */
	ASCII creation_time[24];      /* source image creation date and time */
	ASCII input_dev[32];          /* input device name */
	ASCII input_serial[32];       /* input device serial number */
	U16   border[4];              /* border validity (XL, XR, YT, YB) */
	U32   pixel_aspect[2];        /* pixel aspect ratio (H:V) */
	U8    reserved[28];           /* reserved for future use (padding) */
} Image_Orientation;

/*!
	@brief Motion picture film header
	
	This header was copied from the Cineon DPX file format specification
	and is an industry specific header.
*/
typedef struct _motion_picture_film_header
{
	ASCII film_mfg_id[2];    /* film manufacturer ID code (2 digits from film edge code) */
	ASCII film_type[2];      /* file type (2 digits from film edge code) */
	ASCII offset[2];         /* offset in perfs (2 digits from film edge code)*/
	ASCII prefix[6];         /* prefix (6 digits from film edge code) */
	ASCII count[4];          /* count (4 digits from film edge code)*/
	ASCII format[32];        /* format (i.e. academy) */
	U32   frame_position;    /* frame position in sequence */
	U32   sequence_len;      /* sequence length in frames */
	U32   held_count;        /* held count (1 = default) */
	R32   frame_rate;        /* frame rate of original in frames/sec */
	R32   shutter_angle;     /* shutter angle of camera in degrees */
	ASCII frame_id[32];      /* frame identification (i.e. keyframe) */
	ASCII slate_info[100];   /* slate information */
	U8    reserved[56];      /* reserved for future use (padding) */
} Motion_Picture_Film;

/*!
	@brief Television header
	
	This header was copied from the Cineon DPX file format specification
	and is an industry specific header.
*/
typedef struct _television_header
{
	U32 tim_code;            /* SMPTE time code */
	U32 userBits;            /* SMPTE user bits */
	U8  interlace;           /* interlace ( 0 = noninterlaced, 1 = 2:1 interlace*/
	U8  field_num;           /* field number */
	U8  video_signal;        /* video signal standard (table 4)*/
	U8  unused;              /* used for byte alignment only */
	R32 hor_sample_rate;     /* horizontal sampling rate in Hz */
	R32 ver_sample_rate;     /* vertical sampling rate in Hz */
	R32 frame_rate;          /* temporal sampling rate or frame rate in Hz */
	R32 time_offset;         /* time offset from sync to first pixel */
	R32 gamma;               /* gamma value */
	R32 black_level;         /* black level code value */
	R32 black_gain;          /* black gain */
	R32 break_point;         /* breakpoint */
	R32 white_level;         /* reference white level code value */
	R32 integration_times;   /* integration time(s) */
	U8  reserved[76];        /* reserved for future use (padding) */
} Television_Header;

/*!
	@brief Data structure for the pixel aspect ratio
	
	This data structure is not defined in the DPX file specification
*/
typedef struct _pixel_aspect_ratio
{
	U32 horizontal;
	U32 vertical;
} Pixel_Aspect_Ratio;


// Basic DPX file header
struct BasicHeader
{
	File_Information file_information;
	Image_Information image_information;
	Image_Orientation image_orientation;
	Motion_Picture_Film motion_picture_film;
	Television_Header television_header;
};

// Timecode record
struct TimecodeRecord
{
	uint16_t hours;
	uint16_t minutes;
	uint16_t seconds;
	uint16_t frames;
};


// Return the size of the file (in bytes)
size_t FileSize(FILE *file)
{
#if WIN32
	struct _stat info;
	int fd = _fileno(file);
	int result = _fstat(fd, &info);
	if (result != 0) {
		return 0;
	}
#else
	struct stat info;
	int fd = fileno(file);
	int result = fstat(fd, &info);
	if (result != 0) {
		return 0;
	}
#endif

	return info.st_size;
}

/*!
	@brief Translate the DPX pixel format and precision into a codec pixel format

	Note that only the most common 10-bit RGB format is supported.
*/
PIXEL_FORMAT DPX_PixelFormat(uint8_t descriptor, uint8_t bit_size)
{
	switch (descriptor)
	{
	case DPX_RGB_10BIT_444:
		return (bit_size == 10 ? PIXEL_FORMAT_DPX0 : PIXEL_FORMAT_UNKNOWN);
		break;
#if 0
	case DPX_YUV_10BIT_422:
		return (bit_size == 10 ? PIXEL_FORMAT_DPX1 : PIXEL_FORMAT_UNKNOWN);
		break;
#endif
	}

	return PIXEL_FORMAT_UNKNOWN;
}

CODEC_ERROR DPX_ReadFile(IMAGE *image, const char *pathname)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;
	size_t file_size;
	size_t result;

	// Open the file that contains the DPX image
	FILE *file = fopen(pathname, "rb");
	if (file == NULL) {
		//fprintf(stderr, "File open error: %s\n", pathname); 
		return CODEC_ERROR_OPEN_FILE_FAILED;
	}

	// Determine the size of the image
	file_size = FileSize(file);
	assert(file_size > 0);

	// Allocate a buffer for the entire file
	AllocImageSize(NULL, image, file_size);
	assert(image->buffer != NULL);

	// Read the file into the image buffer
	result = fread(image->buffer, file_size, 1, file);
	if (result != 1) {
		error = CODEC_ERROR_READ_FILE_FAILED;
	}

	fclose(file);

	return error;
}

CODEC_ERROR DPX_ParseHeader(IMAGE *image, DPX_FileInfo *info)
{
	const uint8_t *buffer = image->buffer;
	size_t size = image->size;

	// Clear the structure for the DPX file header
	memset(info, 0, sizeof(DPX_FileInfo));

	if (size >= sizeof(File_Information))
	{
		// Parse the file information header
		const File_Information *header = (const File_Information *)buffer;

		// The byte swap flag is used by all of the file reader methods
		info->byte_swap_flag = (header->magic_num == XPDS ? true : false);

		// Set the global byte swapping flag
		byte_swap_flag = info->byte_swap_flag;

		// Get the offset to the image data
		info->offset = DPX_Swap32(header->offset);

		//TODO: Get other information from the file information header
		buffer += sizeof(File_Information);
		size -= sizeof(File_Information);
	}

	if (size >= sizeof(Image_Information))
	{
		// Parse the image information header
		const Image_Information *header = (const Image_Information *)buffer;

		// Get the image dimensions and format
		info->width = DPX_Swap32(header->pixels_per_line);
		info->height = DPX_Swap32(header->lines_per_image_ele);
		info->descriptor = header->image_element[0].descriptor;
		info->bit_size = header->image_element[0].bit_size;

		info->format = DPX_PixelFormat(info->descriptor, info->bit_size);

		//TODO: Get other information from the image information header
		buffer += sizeof(Image_Information);
		size -= sizeof(Image_Information);
	}

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Pack the color components into a 32-bit word

	The common pixel format used for DPX files packs the three
	color primaries as 10 bits per component in a 32-bit word
	with two unused zero bits.

	The input values have 16 bits of precision since that is the
	bit depth used by decoder as its internal pixel format.  The
	packed word is byte swapped.
*/
uint32_t Pack10(uint32_t R, uint32_t G, uint32_t B)
{
	// Reduce each 16 bit color value to 10 bits
	const int descale_shift = 6;

	const int R_shift = 22;
	const int G_shift = 12;
	const int B_shift = 2;

	const uint32_t pixel_mask = 0x3FF;

	uint32_t word;

	R >>= descale_shift;
	G >>= descale_shift;
	B >>= descale_shift;

	// Pack the color values into a 32 bit word
	word = ((R & pixel_mask) << R_shift) | ((G & pixel_mask) << G_shift) | ((B & pixel_mask) << B_shift);

	// Return the packed word after byte swapping (if necessary)
	return DPX_Swap32(word);
}

// Unpack the 10-bit color components in a DPX pixel
void Unpack10(uint32_t word, uint16_t *R, uint16_t *G, uint16_t *B)
{
	static const int R_shift = 22;
	static const int G_shift = 12;
	static const int B_shift = 2;

	static const uint32_t pixel_mask = 0x3FF;

	// Scale each component value to 16 bits
	const int scale_shift = 6;

	// Swap the input pixel (if necessary)
	word = DPX_Swap32(word);

	// Shift and mask the DPX pixel to extract the components
	*R = (uint16_t)(((word >> R_shift) & pixel_mask) << scale_shift);
	*G = (uint16_t)(((word >> G_shift) & pixel_mask) << scale_shift);
	*B = (uint16_t)(((word >> B_shift) & pixel_mask) << scale_shift);
}

/*!
	@brief Unpack a row of DPX pixels into separate component array rows
*/
CODEC_ERROR UnpackImageRowDPX0(uint8_t *input_buffer, DIMENSION width, PIXEL *output_buffer[],
							   PRECISION bits_per_component[], int channel_count,
							   ENABLED_PARTS enabled_parts)
{
	uint32_t *input = (uint32_t *)input_buffer;
	uint16_t *R_output = (uint16_t *)output_buffer[0];
	uint16_t *G_output = (uint16_t *)output_buffer[1];
	uint16_t *B_output = (uint16_t *)output_buffer[2];

	// Scale each pixel to the internal precision for intermediate results
	//const int scale_shift = (16 - internal_precision);

	// Scale each pixel to the precision of the corresponding component array
	const int R_scale_shift = (16 - bits_per_component[0]);
	const int G_scale_shift = (16 - bits_per_component[1]);
	const int B_scale_shift = (16 - bits_per_component[2]);

	int column;

	(void)channel_count;
	(void)enabled_parts;

	// Separate each packed 10-bit DPX pixel into a buffer of 16-bit pixels for each plane
	for (column = 0; column < width; column++)
	{
		uint16_t R;
		uint16_t G;
		uint16_t B;

		Unpack10(input[column], &R, &G, &B);
#if 0
		*R = UINT16_MAX;
		*G = UINT16_MAX;
		*B = UINT16_MAX;
#endif
		R_output[column] = (uint16_t)(R >> R_scale_shift);
		G_output[column] = (uint16_t)(G >> G_scale_shift);
		B_output[column] = (uint16_t)(B >> B_scale_shift);
	}

	return CODEC_ERROR_OKAY;
}

#if 0
/*!
	@brief Write an image to the specified DPX file
*/
CODEC_ERROR DPX_WriteImage(IMAGE *image, const char *pathname)
{
	File_Information file_header;
	Image_Information image_header;
	Image_Orientation orientation_header;
	Motion_Picture_Film film_header;
	Television_Header television_header;

	size_t generic_header_size = sizeof(file_header) + sizeof(image_header) + sizeof(orientation_header);
	size_t industry_header_size = sizeof(film_header) + sizeof(television_header);
	size_t total_header_size = generic_header_size + industry_header_size;
	size_t image_size = image->width * image->height * sizeof(U32);
	size_t file_size = image_size + total_header_size;

	const U32 ditto_key = 1;
	const U32 ref_low_data = 0;
	const R32 ref_low_quantity = 0.0f;
	const U32 ref_high_data = 1023;
	const R32 ref_high_quantity = 0.0f;
	const U8 bits_per_pixel = 10;
	const U32 data_offset = 2048;

	FILE *file = NULL;
	size_t write_count;

	size_t image_buffer_size;

	// Try to open the DPX file in binary mode
	file = fopen(pathname, "wb");
	if (file == NULL) {
		return CODEC_ERROR_CREATE_FILE_FAILED;
	}

	// Use unbuffered writes
	setbuf(file, NULL);

	// Check that the header size is correct
	assert(total_header_size == 2048);

	// Initialize the file information header
	memset(&file_header, 0, sizeof(file_header));

	// Set the magic number to indicate that byte swapping is required
	file_header.magic_num = XPDS;

	file_header.offset = DPX_Swap32((U32)total_header_size);
	strcpy(file_header.vers, "V1.0");
	file_header.file_size = DPX_Swap32((U32)file_size);
	file_header.ditto_key = DPX_Swap32(ditto_key);
	file_header.gen_hdr_size = DPX_Swap32((U32)generic_header_size);
	file_header.ind_hdr_size = DPX_Swap32((U32)industry_header_size);

	file_header.key = 0xFFFFFFFF;

	// Write the file information header
	write_count = fwrite(&file_header, sizeof(file_header), 1, file);
	assert(write_count == 1);

	// Initialize the image information header
	memset(&image_header, 0, sizeof(image_header));
	image_header.orientation = 0;
	image_header.element_number = Swap16(1);
	image_header.pixels_per_line = DPX_Swap32((U32)image->width);
	image_header.lines_per_image_ele = DPX_Swap32((U32)image->height);

	// Initialize the members of the first image element
	image_header.image_element[0].data_sign = 0;
	image_header.image_element[0].ref_low_data = DPX_Swap32(ref_low_data);
	image_header.image_element[0].ref_low_quantity = DPX_Swap32f(ref_low_quantity);
	image_header.image_element[0].ref_high_data = DPX_Swap32(ref_high_data);
	image_header.image_element[0].ref_high_quantity = DPX_Swap32f(ref_high_quantity);
	image_header.image_element[0].descriptor = 50;
	image_header.image_element[0].bit_size = bits_per_pixel;
	image_header.image_element[0].packing = Swap16(1);
	image_header.image_element[0].data_offset = DPX_Swap32(data_offset);

	// Write the image information header
	write_count = fwrite(&image_header, sizeof(image_header), 1, file);
	assert(write_count == 1);

	// Initialize the image orientation header
	memset(&orientation_header, 0, sizeof(orientation_header));
	orientation_header.pixel_aspect[0] = UINT32_MAX;
	orientation_header.pixel_aspect[1] = UINT32_MAX;

	// Write the image orientation header
	write_count = fwrite(&orientation_header, sizeof(orientation_header), 1, file);
	assert(write_count == 1);

	// Initialize the film and motion picture industry header
	memset(&film_header, 0, sizeof(film_header));

	// Write the film and motion picture industry header
	write_count = fwrite(&film_header, sizeof(film_header), 1, file);
	assert(write_count == 1);

	// Initialize the television industry header
	memset(&television_header, 0, sizeof(television_header));

	// Write the television industry header
	write_count = fwrite(&television_header, sizeof(television_header), 1, file);
	assert(write_count == 1);

	// Write the image buffer
	image_buffer_size = image->height * image->pitch;
	write_count = fwrite(image->buffer, image_buffer_size, 1, file);
	assert(write_count == 1);

	fclose(file);

	return CODEC_ERROR_OKAY;
}
#endif

/*!
	@brief Pack the intermediate decoded image into the output image in DPX pixel format

	The only DPX pixel format supported is the most common pixel format of 10-bit RGB packed
	into a 32-bit word with two zero bits for padding.
*/
CODEC_ERROR PackBufferRowsToDPX0(PIXEL *input_buffer, size_t input_pitch,
								 PIXEL *output_buffer, size_t output_pitch,
								 DIMENSION width, DIMENSION height)
{
	int row;

	for (row = 0; row < height; row++)
	{
		uint8_t *input_row_ptr = (uint8_t *)input_buffer + row * input_pitch;
		uint8_t *output_row_ptr = (uint8_t *)output_buffer + row * output_pitch;

		uint16_t *R_input = (uint16_t *)input_row_ptr + 1 * width;
		uint16_t *G_input = (uint16_t *)input_row_ptr + 0 * width;
		uint16_t *B_input = (uint16_t *)input_row_ptr + 2 * width;

		uint32_t *output = (uint32_t *)output_row_ptr;

		int column;

		// Pack the rows of components into DPX pixels
		for (column = 0; column < width; column++)
		{
			int32_t R, G, B;

			R = R_input[column];
			G = G_input[column];
			B = B_input[column];

			output[column] = Pack10(R, G, B);
		}
	}

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Convert the intermediate decoded image into the output image in DPX pixel format

	This routine converts the decoded result of a sample that was YUV 4:2:2 encoded into the
	DPX pixel format of 10-bit RGB packed into a 32-bit word with two zero bits for padding.
*/
CODEC_ERROR ConvertBufferRowsToDPX0(PIXEL *input_buffer, size_t input_pitch,
									PIXEL *output_buffer, size_t output_pitch,
									DIMENSION width, DIMENSION height)
{
	uint8_t *input_row_ptr = (uint8_t *)input_buffer;
	size_t input_row_pitch = input_pitch;

	uint8_t *output_row_ptr = (uint8_t *)output_buffer;
	size_t output_row_pitch = output_pitch;

	// Color conversion coefficients for 709 full range
	uint32_t ymult = 8192;			// 1.0
	uint32_t r_vmult = 12616;		// 1.540
	uint32_t g_vmult = 3760;		// 0.459
	uint32_t g_umult = 1499;		// 0.183
	uint32_t b_umult = 14877;		// 1.816

	const uint32_t chroma_offset = (1 << 15);

	// Reduce the output precision to 16 bits
	const int shift = 13;

	int chroma_width = width / 2;

	int row;

	assert(input_buffer != NULL && output_buffer != NULL);

	for (row = 0; row < height; row++)
	{
		uint16_t *Y_input = (uint16_t *)input_row_ptr;
		uint16_t *V_input = (uint16_t *)input_row_ptr + width;
		uint16_t *U_input = (uint16_t *)input_row_ptr + width + chroma_width;

		uint32_t *output = (uint32_t *)output_row_ptr;

		int column;

		assert((width % 2) == 0);

		for (column = 0; column < width; column += 2)
		{
			int32_t Y1, Y2, U1, V1;
			int32_t R1, R2, G1, G2, B1, B2;

			// Load two pixels (four components)
			Y1 = Y_input[column];
			U1 = U_input[column/2];

			Y2 = Y_input[column + 1];
			V1 = V_input[column/2];

			// Remove the chroma offset
			U1 -= chroma_offset;
			V1 -= chroma_offset;

			//TODO: Add code to correctly convert the pixels to RGB
			R1 = ymult * Y1 + r_vmult * V1;
			R2 = ymult * Y2 + r_vmult * V1;
			B1 = ymult * Y1 + b_umult * U1;
			B2 = ymult * Y2 + b_umult * U1;
			G1 = ymult * Y1 + g_umult * U1 + g_vmult * V1;
			G2 = ymult * Y2 + g_umult * U1 + g_vmult * V1;

			R1 = clamp_uint16(R1 >> shift);
			G1 = clamp_uint16(G1 >> shift);
			B1 = clamp_uint16(B1 >> shift);
			R2 = clamp_uint16(R2 >> shift);
			G2 = clamp_uint16(G2 >> shift);
			B2 = clamp_uint16(B2 >> shift);

			// Pack 16-bit components into 32-bit words
			output[column + 0] = Pack10(R1, G1, B1);
			output[column + 1] = Pack10(R2, G2, B2);
		}

		input_row_ptr += input_row_pitch;
		output_row_ptr += output_row_pitch;
	}

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Pack the intermediate decoded image into the output image in DPX pixel format

	The only DPX pixel format supported is the most common pixel format of 10-bit RGB packed
	into a 32-bit word with two zero bits for padding.
*/
CODEC_ERROR PackBayerRowsToDPX0(PIXEL *input_buffer, size_t input_pitch,
								PIXEL *output_buffer, size_t output_pitch,
								DIMENSION width, DIMENSION height)
{
#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)

	// Define pointers to the rows for each input component
	uint16_t *GG_input_row_ptr;
	uint16_t *RG_input_row_ptr;
	uint16_t *BG_input_row_ptr;
	uint16_t *GD_input_row_ptr;

	size_t input_quarter_pitch;

	int row;

	// The input pitch must be divisible by four
	assert((input_pitch % 4) == 0);

	// Compute the size of each input component (in bytes)
	input_quarter_pitch = input_pitch / 4;

	for (row = 0; row < height; row++)
	{
		uint8_t *input_row_ptr = (uint8_t *)input_buffer + row * input_pitch;
		uint32_t *output_row_ptr = (uint32_t *)((uint8_t *)output_buffer + row * output_pitch);

		const int32_t midpoint = 32768;

		int column;

		GG_input_row_ptr = (uint16_t *)input_row_ptr;
		RG_input_row_ptr = (uint16_t *)(input_row_ptr + input_quarter_pitch);
		BG_input_row_ptr = (uint16_t *)(input_row_ptr + 2 * input_quarter_pitch);
		GD_input_row_ptr = (uint16_t *)(input_row_ptr + 3 * input_quarter_pitch);;

		// Pack the rows of Bayer components into the BYR4 pattern
		for (column = 0; column < width; column++)
		{
			int32_t GG, RG, BG, GD;
			int32_t G1, G2;
			int32_t R, G, B;

			GG = GG_input_row_ptr[column];
			RG = RG_input_row_ptr[column];
			BG = BG_input_row_ptr[column];
			GD = GD_input_row_ptr[column];

			// Convert unsigned values to signed values
			GD -= midpoint;
			RG -= midpoint;
			BG -= midpoint;

			R = (RG << 1) + GG;
			B = (BG << 1) + GG;
			G1 = GG + GD;
			G2 = GG - GD;

			// Average the green components
			G = (G1 + G2) / 2;

			R = clamp_uint16(R);
			G = clamp_uint16(G);
			B = clamp_uint16(B);

			output_row_ptr[column] = Pack10(R, G, B);
		}
	}

#else

	// Define pointers to the rows for each input component
	uint16_t *R1_input_row_ptr;
	uint16_t *G1_input_row_ptr;
	uint16_t *G2_input_row_ptr;
	uint16_t *B1_input_row_ptr;

	size_t input_quarter_pitch;

	int row;

	// The input pitch must be divisible by four
	assert((input_pitch % 4) == 0);

	// Compute the size of each input component (in bytes)
	input_quarter_pitch = input_pitch / 4;

	for (row = 0; row < height; row++)
	{
		uint8_t *input_row_ptr = (uint8_t *)input_buffer + row * input_pitch;
		uint32_t *output_row_ptr = (uint32_t *)((uint8_t *)output_buffer + row * output_pitch);

		//const int32_t midpoint = 32768;

		int column;

		R1_input_row_ptr = (uint16_t *)input_row_ptr;
		G1_input_row_ptr = (uint16_t *)(input_row_ptr + input_quarter_pitch);
		G2_input_row_ptr = (uint16_t *)(input_row_ptr + 2 * input_quarter_pitch);
		B1_input_row_ptr = (uint16_t *)(input_row_ptr + 3 * input_quarter_pitch);;

		// Pack the rows of Bayer components into 32-bit RGB pixels for the DPX file format
		for (column = 0; column < width; column++)
		{
			int32_t R1, G1, G2, B1;
			int32_t R, G, B;

			R1 = R1_input_row_ptr[column];
			G1 = G1_input_row_ptr[column];
			G2 = G2_input_row_ptr[column];
			B1 = B1_input_row_ptr[column];

			// Average the green components
			G1 = (G1 + G2) / 2;

			R = clamp_uint16(R1);
			G = clamp_uint16(G1);
			B = clamp_uint16(B1);

			output_row_ptr[column] = Pack10(R, G, B);
		}
	}

#endif

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Pack the lowpass bands from the component arrays into a DPX image

	This routine assumes that the component arrays correspond to an input image
	with the BYR4 pixel format.
*/
CODEC_ERROR PackLowpassBandsBYR4ToDPX0(void **lowpass_buffer_array,
									   size_t *lowpass_pitch_array,
									   DIMENSION width,
									   DIMENSION height,
									   int shift,
									   IMAGE *output)
{
	// Define pointers to the rows for each input component
	uint8_t *C1_input_buffer;
	uint8_t *C2_input_buffer;
	uint8_t *C3_input_buffer;
	uint8_t *C4_input_buffer;

	//TODO: Need to pass the wavelet level and prescale shifts as parameters
	//const int prescale_shift = 4;
	//const int wavelet_level = 3;
	//const int shift = 4 - (wavelet_level * 2) + prescale_shift;

	int row;

	C1_input_buffer = lowpass_buffer_array[0];
	C2_input_buffer = lowpass_buffer_array[1];
	C3_input_buffer = lowpass_buffer_array[2];
	C4_input_buffer = lowpass_buffer_array[3];

	for (row = 0; row < height; row++)
	{
		PIXEL *C1_input_row_ptr = (PIXEL *)(C1_input_buffer + row * lowpass_pitch_array[0]);
		PIXEL *C2_input_row_ptr = (PIXEL *)(C2_input_buffer + row * lowpass_pitch_array[1]);
		PIXEL *C3_input_row_ptr = (PIXEL *)(C3_input_buffer + row * lowpass_pitch_array[2]);
		PIXEL *C4_input_row_ptr = (PIXEL *)(C4_input_buffer + row * lowpass_pitch_array[3]);

		uint8_t *output_row_ptr = (uint8_t *)output->buffer + row * output->pitch;
		uint32_t *output_pixel_ptr = (uint32_t *)output_row_ptr;

		int column;

		// Pack the component values into DPX pixels
		for (column = 0; column < width; column++)
		{
			uint32_t C1, C2, C3, C4;
			uint32_t R, G, B;

			C1 = C1_input_row_ptr[column];
			C2 = C2_input_row_ptr[column];
			C3 = C3_input_row_ptr[column];
			C4 = C4_input_row_ptr[column];

			R = C1;
			G = (C2 + C3) / 2;
			B = C4;

			// Scale the component values to 16 bits
			R <<= shift;
			G <<= shift;
			B <<= shift;

			assert(0 <= R && R <= UINT16_MAX);
			assert(0 <= G && G <= UINT16_MAX);
			assert(0 <= B && B <= UINT16_MAX);

			*(output_pixel_ptr++) = Pack10(R, G, B);
		}
	}

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Pack the lowpass bands from the component arrays into a DPX image

	This routine assumes that the component arrays correspond to an input image
	with the RG48 pixel format.
*/
CODEC_ERROR PackLowpassBandsRG48ToDPX0(void **lowpass_buffer_array,
									   size_t *lowpass_pitch_array,
									   DIMENSION width,
									   DIMENSION height,
									   int shift,
									   IMAGE *output)
{
	// Define pointers to the rows for each input component
	uint8_t *R_input_buffer;
	uint8_t *G_input_buffer;
	uint8_t *B_input_buffer;

	//TODO: Need to pass the wavelet level and prescale shifts as parameters
	//const int prescale_shift = 4;
	//const int wavelet_level = 3;
	//const int shift = 4 - (wavelet_level * 2) + prescale_shift;

	int row;

	R_input_buffer = lowpass_buffer_array[0];
	G_input_buffer = lowpass_buffer_array[1];
	B_input_buffer = lowpass_buffer_array[2];

	for (row = 0; row < height; row++)
	{
		PIXEL *R_input_row_ptr = (PIXEL *)(R_input_buffer + row * lowpass_pitch_array[0]);
		PIXEL *G_input_row_ptr = (PIXEL *)(G_input_buffer + row * lowpass_pitch_array[1]);
		PIXEL *B_input_row_ptr = (PIXEL *)(B_input_buffer + row * lowpass_pitch_array[2]);

		uint8_t *output_row_ptr = (uint8_t *)output->buffer + row * output->pitch;
		uint32_t *output_pixel_ptr = (uint32_t *)output_row_ptr;

		int column;

		// Pack the component values into DPX pixels
		for (column = 0; column < width; column++)
		{
			uint32_t R, G, B;

			R = R_input_row_ptr[column];
			G = G_input_row_ptr[column];
			B = B_input_row_ptr[column];

			// Scale the component values to 16 bits
			R <<= shift;
			G <<= shift;
			B <<= shift;

			assert(0 <= R && R <= UINT16_MAX);
			assert(0 <= G && G <= UINT16_MAX);
			assert(0 <= B && B <= UINT16_MAX);

			*(output_pixel_ptr++) = Pack10(R, G, B);
		}
	}

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Pack the lowpass bands from the component arrays into a DPX image

	This routine assumes that the component arrays correspond to an input image
	with the DPX0 pixel format.
*/
CODEC_ERROR PackLowpassBandsDPX0ToDPX0(void **lowpass_buffer_array,
									   size_t *lowpass_pitch_array,
									   DIMENSION width,
									   DIMENSION height,
									   int shift,
									   IMAGE *output)
{
	// Define pointers to the rows for each input component
	uint8_t *R_input_buffer;
	uint8_t *G_input_buffer;
	uint8_t *B_input_buffer;

	//TODO: Need to pass the wavelet level and prescale shifts as parameters
	//const int prescale_shift = 4;
	//const int wavelet_level = 3;
	//const int shift = 4 - (wavelet_level * 2) + prescale_shift;

	int row;

	R_input_buffer = lowpass_buffer_array[0];
	G_input_buffer = lowpass_buffer_array[1];
	B_input_buffer = lowpass_buffer_array[2];

	for (row = 0; row < height; row++)
	{
		PIXEL *R_input_row_ptr = (PIXEL *)(R_input_buffer + row * lowpass_pitch_array[0]);
		PIXEL *G_input_row_ptr = (PIXEL *)(G_input_buffer + row * lowpass_pitch_array[1]);
		PIXEL *B_input_row_ptr = (PIXEL *)(B_input_buffer + row * lowpass_pitch_array[2]);

		uint8_t *output_row_ptr = (uint8_t *)output->buffer + row * output->pitch;
		uint32_t *output_pixel_ptr = (uint32_t *)output_row_ptr;

		int column;

		// Pack the component values into DPX pixels
		for (column = 0; column < width; column++)
		{
			uint32_t R, G, B;

			R = R_input_row_ptr[column];
			G = G_input_row_ptr[column];
			B = B_input_row_ptr[column];

			// Scale the component values to 16 bits
			R <<= shift;
			G <<= shift;
			B <<= shift;

			//assert(0 <= R && R <= UINT16_MAX);
			//assert(0 <= G && G <= UINT16_MAX);
			//assert(0 <= B && B <= UINT16_MAX);

			*(output_pixel_ptr++) = Pack10(R, G, B);
		}
	}

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Pack lowpass bands from a set of wavelets into an image in DPX0 format
*/
CODEC_ERROR PackLowpassBandsToDPX0(void **lowpass_buffer_array,
								   size_t *lowpass_pitch_array,
								   DIMENSION width,
								   DIMENSION height,
								   PIXEL_FORMAT format,
								   int shift,
								   IMAGE *output)
{
	switch (format)
	{
	case PIXEL_FORMAT_BYR4:
		return PackLowpassBandsBYR4ToDPX0(lowpass_buffer_array, lowpass_pitch_array, width, height, shift, output);
		break;

	case PIXEL_FORMAT_RG48:
		return PackLowpassBandsRG48ToDPX0(lowpass_buffer_array, lowpass_pitch_array, width, height, shift, output);
		break;

	case PIXEL_FORMAT_DPX0:
		return PackLowpassBandsDPX0ToDPX0(lowpass_buffer_array, lowpass_pitch_array, width, height, shift, output);
		break;

	default:
		// Unsupported pixel format
		assert(0);
		break;
	}

	// Could not handle the specified piexl format
	return CODEC_ERROR_PIXEL_FORMAT;
}

/*!
	@brief Prepare to write a DPX file

	This routine should be called before using any routines that pack
	images into the pixel format used for DPX files so that the byte
	swapping flag is set.
*/
CODEC_ERROR DPX_SetByteSwapFlag()
{
	// Set the global byte swapping flag
	byte_swap_flag = true;
	return CODEC_ERROR_OKAY;
}

/*!
	@brief Write an image to the specified file in DPX format
*/
CODEC_ERROR DPX_WriteImage(const IMAGE *image, const char *pathname)
{
	File_Information file_header;
	Image_Information image_header;
	Image_Orientation orientation_header;
	Motion_Picture_Film film_header;
	Television_Header television_header;

	size_t generic_header_size = sizeof(file_header) + sizeof(image_header) + sizeof(orientation_header);
	size_t industry_header_size = sizeof(film_header) + sizeof(television_header);
	size_t total_header_size = generic_header_size + industry_header_size;
	size_t image_size = image->width * image->height * sizeof(U32);
	size_t file_size = image_size + total_header_size;

	const U32 ditto_key = 1;
	const U32 ref_low_data = 0;
	const R32 ref_low_quantity = 0.0f;
	const U32 ref_high_data = 1023;
	const R32 ref_high_quantity = 0.0f;
	const U8 bits_per_pixel = 10;
	const U32 data_offset = 2048;

	FILE *file = NULL;
	size_t write_count;

	size_t image_buffer_size;

	// Try to open the DPX file in binary mode
	file = fopen(pathname, "wb");
	if (file == NULL) {
		return CODEC_ERROR_CREATE_FILE_FAILED;
	}

	// Use unbuffered writes
	setbuf(file, NULL);

	// Check that the header size is correct
	assert(total_header_size == 2048);

	// Initialize the file information header
	memset(&file_header, 0, sizeof(file_header));

	// Set the magic number that signals byte swapping
	if (byte_swap_flag)
	{
		file_header.magic_num = XPDS;
	}
	else
	{
		file_header.magic_num = SPDX;
	}

	file_header.offset = DPX_Swap32((U32)total_header_size);
	strcpy(file_header.vers, "V1.0");
	file_header.file_size = DPX_Swap32((U32)file_size);
	file_header.ditto_key = DPX_Swap32(ditto_key);
	file_header.gen_hdr_size = DPX_Swap32((U32)generic_header_size);
	file_header.ind_hdr_size = DPX_Swap32((U32)industry_header_size);

	file_header.key = 0xFFFFFFFF;

	// Write the file information header
	write_count = fwrite(&file_header, sizeof(file_header), 1, file);
	assert(write_count == 1);

	// Initialize the image information header
	memset(&image_header, 0, sizeof(image_header));
	image_header.orientation = 0;
	image_header.element_number = DPX_Swap16(1);
	image_header.pixels_per_line = DPX_Swap32((U32)image->width);
	image_header.lines_per_image_ele = DPX_Swap32((U32)image->height);

	// Initialize the members of the first image element
	image_header.image_element[0].data_sign = 0;
	image_header.image_element[0].ref_low_data = DPX_Swap32(ref_low_data);
	image_header.image_element[0].ref_low_quantity = DPX_Swap32f(ref_low_quantity);
	image_header.image_element[0].ref_high_data = DPX_Swap32(ref_high_data);
	image_header.image_element[0].ref_high_quantity = DPX_Swap32f(ref_high_quantity);
	image_header.image_element[0].descriptor = 50;
	image_header.image_element[0].bit_size = bits_per_pixel;
	image_header.image_element[0].packing = DPX_Swap16(1);
	image_header.image_element[0].data_offset = DPX_Swap32(data_offset);

	// Write the image information header
	write_count = fwrite(&image_header, sizeof(image_header), 1, file);
	assert(write_count == 1);

	// Initialize the image orientation header
	memset(&orientation_header, 0, sizeof(orientation_header));
	orientation_header.pixel_aspect[0] = UINT32_MAX;
	orientation_header.pixel_aspect[1] = UINT32_MAX;

	// Write the image orientation header
	write_count = fwrite(&orientation_header, sizeof(orientation_header), 1, file);
	assert(write_count == 1);

	// Initialize the film and motion picture industry header
	memset(&film_header, 0, sizeof(film_header));

	// Write the film and motion picture industry header
	write_count = fwrite(&film_header, sizeof(film_header), 1, file);
	assert(write_count == 1);

	// Initialize the television industry header
	memset(&television_header, 0, sizeof(television_header));

	// Write the television industry header
	write_count = fwrite(&television_header, sizeof(television_header), 1, file);
	assert(write_count == 1);

	// Write the image
	image_buffer_size = image->height * image->pitch;
	write_count = fwrite(image->buffer, image_buffer_size, 1, file);
	assert(write_count == 1);

	fclose(file);

	return CODEC_ERROR_OKAY;
}
