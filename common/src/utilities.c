/*!	@file common/src/utilities.c

	The utilities routines in this modile are not part of the reference
	codec and are only included to allow the codec to be tested.

	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#include <string.h>
#include <sys/stat.h>
#include "headers.h"
#include "bandfile.h"
#include "dpxfile.h"
#include "timer.h"
#include "utilities.h"


#ifdef __GNUC__
#define _stat stat
#define _fileno fileno
#define _fstat fstat
#define stricmp strcasecmp
#endif


// Forward reference
CODEC_ERROR RAW_ReadImage(IMAGE *image, const char *pathname);
CODEC_ERROR DPX_ReadImage(IMAGE *image, const char *pathname);


//! List of printable names for the VC-5 parts indexed by part number
static const char *vc5_part_names[] =  {
	"",
	"elementary",
	"conformance",
	"images",
	"sampling",
	"layers",
	"sections",
	"metadata"

	//TODO: Add single-word names for new parts (no embedded whitespace)
};


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
	struct _stat info;
	int fd;
	size_t size;
	size_t result;

	// Open the file that contains the image
	file = fopen(pathname, "rb");
	if (file == NULL) {
		return CODEC_ERROR_OPEN_FILE_FAILED;
	}

	// Get the size of the file
	fd = _fileno(file);
	result = _fstat(fd, &info);
	if (result != 0) {
		return CODEC_ERROR_FILE_SIZE_FAILED;
	}

	// The image buffer must be large enough for the entire file
	assert(image->size >= (size_t)info.st_size);
	size = info.st_size;
	if (size > image->size) {
		size = image->size;
	}

	result = fread(image->buffer, size, 1, file);
	if (result != 1) {
		return CODEC_ERROR_READ_FILE_FAILED;
	}
    
    fclose(file);

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Read a single image from a DPX file

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
	pitch = ImagePitch((DIMENSION)info.width, info.format);

	// Set the image dimensions and format
	SetImageFormat(image, (DIMENSION)info.width, (DIMENSION)info.height, pitch, info.format, info.offset);

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Read the file into the image data structure passed as an argument

	The file extension may provide sufficient information to determine the format
	of the image in the file and some files contain information that defines the
	dimensions and format of the image in the file.

	If the image file does not contain the image dimensions, then the width and
	height must be provided as command-line options.  In most cases, the pixel format
	can be deduced from the file extension, otherwise it is necessary to provide the
	pixel format as a command-line option.
*/
CODEC_ERROR ReadImageFile(IMAGE *image,
						  DIMENSION image_width,
						  DIMENSION image_height,
						  PIXEL_FORMAT pixel_format,
						  const char *pathname)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;
	FILE_INFO info;

	InitImage(image);

	// Get the pixel format and precision of the input image
	GetFileInfo(pathname, &info);

	if (info.type == FILE_TYPE_RAW)
	{
		// The caller must provide the image dimensions and pixel format
		assert(image_width > 0 && image_height > 0 && pixel_format != PIXEL_FORMAT_UNKNOWN);

		// Allocate a buffer for the input image
		AllocImage(NULL, image, image_width, image_height, pixel_format);
	}
	else
	{
		// Must be a DPX image in which case the file header specifies the dimensions and format
		assert(info.type == FILE_TYPE_DPX);
		if (! (info.type == FILE_TYPE_DPX)) {
			return CODEC_ERROR_BAD_ARGUMENT;
		}
	}

	// Read the input image (the call may reallocate the image)
	error = ReadImage(image, pathname);
	if (error != CODEC_ERROR_OKAY) {
		fprintf(stderr, "Could not read input image: %s\n", pathname);
	}

	return error;
}

/*!
	@brief Check that the enabled parts are correct
*/
CODEC_ERROR CheckEnabledParts(ENABLED_PARTS *enabled_parts_ref)
{
    // Table of VC-5 parts that are implicitly enabled indexed by part number
    static ENABLED_PARTS enabled_parts_list[] =
    {
        VC5_PART_MASK_NONE,
        VC5_PART_MASK_NONE,
        VC5_PART_MASK_NONE,
        VC5_PART_MASK_NONE,
        VC5_PART_MASK(VC5_PART_IMAGE_FORMATS),      // Image formats are enabled if color sampling is enabled
        VC5_PART_MASK(VC5_PART_IMAGE_FORMATS),      // Image formats are enabled if layers are enabled
        VC5_PART_MASK(VC5_PART_IMAGE_FORMATS),      // Image formats are enabled if sections are enabled
        VC5_PART_MASK(VC5_PART_IMAGE_FORMATS),		// Image formats are required to test intrinsic metadata
        
    };
    const int enabled_parts_list_length = sizeof(enabled_parts_list)/sizeof(enabled_parts_list[0]);
    
	ENABLED_PARTS enabled_parts = (*enabled_parts_ref);
    
    int vc5_part_number;

	// The elementary bitstream is always enabled
	enabled_parts |= VC5_PART_MASK(VC5_PART_ELEMENTARY);

	// The conformance specification is never enabled
	enabled_parts &= ~((uint32_t)VC5_PART_MASK(VC5_PART_CONFORMANCE));

    for (vc5_part_number = 0; vc5_part_number < enabled_parts_list_length; vc5_part_number++)
    {
        // Enable the parts that are implicitly enabled if this part number is enabled
        enabled_parts |= enabled_parts_list[vc5_part_number];
    }

	// Check that the enabled parts were built at compile-time
	assert((enabled_parts & VC5_ENABLED_PARTS) == enabled_parts);
	if (! ((enabled_parts & VC5_ENABLED_PARTS) == enabled_parts)) {
		return CODEC_ERROR_ENABLED_PARTS;
	}

	// Return the modified enabled parts mask
	*enabled_parts_ref = enabled_parts;
	return CODEC_ERROR_OKAY;
}

/*!
	@brief Verify that the enabled parts are correct
*/
CODEC_ERROR VerifyEnabledParts(ENABLED_PARTS enabled_parts)
{
	// The elementary bitstream must always be enabled
	if ((enabled_parts & VC5_PART_MASK(VC5_PART_ELEMENTARY)) == 0) {
		return CODEC_ERROR_ENABLED_PARTS;
	}

	// The conformance specification must not be enabled
	if ((enabled_parts & VC5_PART_MASK(VC5_PART_CONFORMANCE)) != 0) {
		return CODEC_ERROR_ENABLED_PARTS;
	}

	// Image formats must be enabled if subsampled color differences are enabled
	if ((enabled_parts & VC5_PART_MASK(VC5_PART_COLOR_SAMPLING)) != 0 &&
		(enabled_parts & VC5_PART_MASK(VC5_PART_IMAGE_FORMATS)) == 0) {
		return CODEC_ERROR_ENABLED_PARTS;
	}

	// All enabled parts must be compiled into this codec implementation
	if ((enabled_parts & VC5_ENABLED_PARTS) != enabled_parts) {
		return CODEC_ERROR_ENABLED_PARTS;
	}

	// This codec implementation supports the enabled parts of the VC-5 standard
	return CODEC_ERROR_OKAY;
}


/*!
	@brief Print the enabled parts in a readable format
*/
CODEC_ERROR PrintEnabledParts(ENABLED_PARTS enabled_parts)
{
	printf("Enabled parts:");

	for (int part = VC5_PART_NUMBER_MIN; part <= VC5_PART_NUMBER_MAX; part++)
	{
		if (enabled_parts & VC5_PART_MASK(part)) {
			printf(" %s", vc5_part_names[part]);
		}
	}

	printf("\n");

	return CODEC_ERROR_OKAY;
}

