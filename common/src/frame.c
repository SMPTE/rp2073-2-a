/*!	@file image.c

	Implementation of the data structure for the image that is input to the
	image unpacking process.

	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#include "headers.h"
#include "dpxfile.h"
#include "fileinfo.h"
#include "convert.h"

/*!
	@brief Initialize the fields in an image data structure

	This routine is the constructor for the image data type that
	initializes an image instance to default values.
*/
CODEC_ERROR InitImage(IMAGE *image)
{
	if (image != NULL)
	{
		image->width = 0;
		image->height = 0;
		image->pitch = 0;
		image->offset = 0;
		image->format = PIXEL_FORMAT_UNKNOWN;
		image->buffer = NULL;
		image->size = 0;
		return CODEC_ERROR_OKAY;
	}

	return CODEC_ERROR_NULLPTR;
}

/*!

*/
IMAGE *CreateImage(ALLOCATOR *allocator, DIMENSION width, DIMENSION height, PIXEL_FORMAT format)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;

	IMAGE *image = Alloc(allocator, sizeof(IMAGE));
	if (image == NULL) {
		return NULL;
	}

	error = AllocImage(allocator, image, width, height, format);
	if (error != CODEC_ERROR_OKAY)
	{
		Free(allocator, image);
		return NULL;
	}

	return image;
}


/*!
	@brief Allocate the buffer for a image with the sepcified dimensions and format.

	This routine calculates the image pitch from the width and format, rounding up
	the pitch to satisfy memory alignment requirements in the codec.

	This routine will cause a memory leak if it is called with a image that has
	already been allocated.
*/
CODEC_ERROR AllocImage(ALLOCATOR *allocator, IMAGE *image, DIMENSION width, DIMENSION height, PIXEL_FORMAT format)
{
	size_t size = 0;
	DIMENSION pitch = 0;

	assert(image != NULL);

	// Clear the members of the image data structure
	InitImage(image);

	// Compute the pitch from the width and format
	pitch = ImagePitch(width, format);
	assert(pitch > 0);

	// Compute the size of the image
	if (format == PIXEL_FORMAT_NV12)
	{
		// The size includes the full height luma plane and hald-height color difference plane
		size = (3 * height * pitch) / 2;
	}
	else
	{
		size = height * pitch;
	}
	assert(size > 0);

	// Allocate the image buffer
	image->buffer = Alloc(allocator, size);
	if (image->buffer != NULL)
	{
		image->width = width;
		image->height = height;
		image->pitch = pitch;
		image->format = format;
		image->offset = 0;
		image->size = size;
		return CODEC_ERROR_OKAY;
	}

	return CODEC_ERROR_OUTOFMEMORY;
}

/*!
	@brief Allocate a image with the specified size in bytes

	@todo Free the buffer if it has already been allocated.
*/
CODEC_ERROR AllocImageSize(ALLOCATOR *allocator, IMAGE *image, size_t size)
{
	assert(image != NULL);
	InitImage(image);
	image->buffer = Alloc(allocator, size);
	if (image->buffer != NULL) {
		image->size = size;
		return CODEC_ERROR_OKAY;
	}

	return CODEC_ERROR_OUTOFMEMORY;
}

/*!
	@brief Allocate an image with the specified pixel format using a prototype

	The prototype image provides the dimensions for the new image, but note that the
	dimensions may be adjusted if the prototype image is Bayer since a Bayer image is
	converted to another image without applying a demosaic filter.  Likewise, if the
	new image is Bayer but the prototype image is not Bayer, then the dimensions of the
	new image will be doubled.
*/
CODEC_ERROR AllocImageCopy(ALLOCATOR *allocator, IMAGE *image, IMAGE *prototype, PIXEL_FORMAT format)
{
	DIMENSION width = prototype->width;
	DIMENSION height = prototype->height;

	if (IsBayerFormat(prototype->format) && !IsBayerFormat(format))
	{
		// Adjust the dimensions to the size of the grid of Bayer pattern elements
		width /= 2;
		height /= 2;
	}
	else if (!IsBayerFormat(prototype->format) && IsBayerFormat(format))
	{
		width *= 2;
		height *= 2;
	}

	return AllocImage(allocator, image, width, height, format);
}

/*!
	@brief Deallocate the buffer in a image data structure

	This routine does not deallocate the image data structure itself.
	In the typical use case, the image data structure is allocated on
	the stack and teh decoder allocates the buffer for the output image
	after determining the size of the image.  The image data structure
	is deallocated automatically by a calling routine.
*/
CODEC_ERROR ReleaseImage(ALLOCATOR *allocator, IMAGE *image)
{
	Free(allocator, image->buffer);
	return CODEC_ERROR_OKAY;
}

/*!
	@brief Compute the image pitch for the specified width and pixel format

	The pitch of the image (in bytes) is computed for a image with the width
	and pixel format that is specified.  The pitch is rounted up to satisfy
	memory alignment constraints required by the codec.

	The return value is zero if the pitch could not be computed for the
	specified width and format.
*/
DIMENSION ImagePitch(DIMENSION width, PIXEL_FORMAT format)
{
	DIMENSION pitch = 0;

	switch (format)
	{
	case PIXEL_FORMAT_BYR3:
	case PIXEL_FORMAT_BYR4:
		// Half the width of the image times 2 samples times 2 bytes per sample
		pitch = width * sizeof(uint16_t);
		break;

	case PIXEL_FORMAT_RG48:
		// RGB pixel with 16 bits per component
		pitch = width * 3 * sizeof(uint16_t);
		break;

	case PIXEL_FORMAT_B64A:
		// ARGB pixel with 16 bits per component
		pitch = width * 4 * sizeof(uint16_t);
		break;

	case PIXEL_FORMAT_DPX_50:
		// RGB 10-bit values packed into a 32-bit word
		pitch = width * sizeof(uint32_t);
		break;

	case PIXEL_FORMAT_NV12:
		// Planar luma and color difference wiht 8-bit color components
		pitch = width * sizeof(uint8_t);
		break;

	default:
		assert(0);
	}

	return pitch;
}

/*!
	case PIXEL_FORMAT_YUY2:
		// Two bytes per pixel due to 4:2:2 sampling
		pitch = 2 * width * sizeof(uint8_t);
		break;

	case PIXEL_FORMAT_RG48:
		// Two bytes for each of the three components
		pitch = 3 * width * sizeof(uint16_t);
		break;

	default:
		assert(0);
	}

	return pitch;
}

/*!
	@brief Set the dimensions and pixel format of a image

	This routine is used to set the dimensions and format of a image
	that was allocated with unknown parameters.  For example, an entire
	DPX file is read into a image buffer that was allocated to be the
	size of the file.  The image dimensions, format, and offset to the
	image in the buffer are set after the DPX file header is parsed.
*/
CODEC_ERROR SetImageFormat(IMAGE *image,
						   DIMENSION width,
						   DIMENSION height,
						   DIMENSION pitch,
						   PIXEL_FORMAT format,
						   size_t offset)
{
	assert(image != NULL);

	image->width = width;
	image->height = height;
	image->pitch = pitch;
	image->format = format;
	image->offset = offset;

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Return the address of the image in the buffer

	This routine should be used to obtain the starting address of
	a image in a image buffer since the image may be offset from the
	beginning of the buffer.
*/
void *ImageData(IMAGE *image)
{
	uint8_t *buffer = image->buffer;
	buffer += image->offset;
	return buffer;
}

/*!
	@brief Return the address of the specified row in the image

	The routine returns NULL if the image argument is null or the
	specified row is out of bounds.
*/
void *RowAddress(IMAGE *image, DIMENSION row)
{
	size_t pitch = image->pitch;

	if (image != NULL && image->pitch != 0)
	{
		if (0 <= row && row < image->height)
		{
			void *address = (void *)((uintptr_t)ImageData(image) + row * pitch);
			return address;
		}
	}

	// Could not compute the address of a valid row
	return NULL;
}

#if _DECODER
/*!
	@brief Pack decoded component arrays into the output image

	The reference decoder uses an internal representation for the
	decoded frame.  This routine converts the internal frame into
	the output frame.

	The internal format uses rows of 16 bit components.  Each row of
	components is followed by the corresponding row of the next component.
	This representation is between packed and planar and is an efficient
	method for storing and processing any combination of color components
	at the same	time.
*/
CODEC_ERROR PackOutputImage(void *buffer, size_t pitch, int encoded_format, IMAGE *image)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;

	switch (image->format)
	{
	case PIXEL_FORMAT_BYR3:
		error = PackBufferRowsToBYR3(buffer, pitch, image->buffer, image->pitch, image->width, image->height);
		break;

	case PIXEL_FORMAT_BYR4:
		error = PackBufferRowsToBYR4(buffer, pitch, image->buffer, image->pitch, image->width, image->height);
		break;

	case PIXEL_FORMAT_DPX0:
		switch (encoded_format)
		{
		case ENCODED_FORMAT_YUV_422:
			error = ConvertBufferRowsToDPX0(buffer, pitch, image->buffer, image->pitch, image->width, image->height);
			break;

		case ENCODED_FORMAT_RGB_444:
			error = PackBufferRowsToDPX0(buffer, pitch, image->buffer, image->pitch, image->width, image->height);
			break;

		case ENCODED_FORMAT_BAYER:
			error = PackBayerRowsToDPX0(buffer, pitch, image->buffer, image->pitch, image->width, image->height);
			break;

		default:
			assert(0);
			error = CODEC_ERROR_UNSUPPORTED_FORMAT;
			break;
		}
		break;

	case PIXEL_FORMAT_RG48:
		switch (encoded_format)
		{
		case ENCODED_FORMAT_RGB_444:
			error = PackBufferRowsToRG48(buffer, pitch, image->buffer, image->pitch, image->width, image->height);
			break;

		default:
			assert(0);
			error = CODEC_ERROR_UNSUPPORTED_FORMAT;
			break;
		}
		break;

	default:
		assert(0);
		error = CODEC_ERROR_UNSUPPORTED_FORMAT;
		break;
	}

	return error;
}
#endif
#if _DECODER
CODEC_ERROR PackBufferRowsToRG48(PIXEL *input_buffer, size_t input_pitch,
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

		uint16_t *output = (uint16_t *)output_row_ptr;

		int column;

		// Pack the rows of components into DPX pixels
		for (column = 0; column < width; column++)
		{
			uint16_t R, G, B;

			R = R_input[column];
			G = G_input[column];
			B = B_input[column];

			output[3 * column + 0] = R;
			output[3 * column + 1] = G;
			output[3 * column + 2] = B;
		}
	}

	return CODEC_ERROR_OKAY;
}
#endif

CODEC_ERROR AllocateComponentArrays(ALLOCATOR *allocator,
									UNPACKED_IMAGE *image,
									int channel_count,
									DIMENSION max_channel_width,
									DIMENSION max_channel_height,
									PIXEL_FORMAT format,
									int bits_per_component)
{
	int channel;

	// Allocate the vector of component arrays
	size_t size = channel_count * sizeof(COMPONENT_ARRAY);
	image->component_array_list = Alloc(allocator, size);
	if (image->component_array_list == NULL) {
		return CODEC_ERROR_OUTOFMEMORY;
	}

	// Clear the component array information so that the state is consistent
	image->component_count = 0;
	memset(image->component_array_list, 0, size);

	// Initialize each component array
	for (channel = 0; channel < channel_count; channel++)
	{
		DIMENSION channel_width = max_channel_width;
		DIMENSION channel_height = max_channel_height;
		size_t component_pitch;
		size_t component_size;

		if (format == PIXEL_FORMAT_NV12 && channel > 0)
		{
			// The NV12 format uses 4:2:0 color difference component sampling
			channel_width = max_channel_width / 2;
			channel_height = max_channel_height / 2;
		}

		// Allocate space for the data in the component array
		component_pitch = channel_width * sizeof(COMPONENT_VALUE);
		component_size = channel_height * component_pitch;
		image->component_array_list[channel].data = Alloc(allocator, component_size);
		if (image->component_array_list[channel].data == NULL) {
			return CODEC_ERROR_OUTOFMEMORY;
		}

		// Initialize the dimensions and format of the component array
		image->component_array_list[channel].width = channel_width;
		image->component_array_list[channel].height = channel_height;
		image->component_array_list[channel].pitch = component_pitch;
		image->component_array_list[channel].bits_per_component = (uint_least8_t)bits_per_component;
	}

	// Set the number of component arrays
	image->component_count = channel_count;

	return CODEC_ERROR_OKAY;
}

CODEC_ERROR AllocateComponentArray(COMPONENT_ARRAY *component_array,
								   DIMENSION width,
								   DIMENSION height,
								   int bits_per_component,
								   ALLOCATOR *allocator)
{
	//COMPONENT_ARRAY *component_array = Alloc(allocator, sizeof(COMPONENT_ARRAY));

	// Allocate space for the data in the component array
	size_t pitch = width * sizeof(COMPONENT_VALUE);
	size_t size = height * pitch;
	void *buffer = Alloc(allocator, size);
	if (buffer == NULL) {
		return CODEC_ERROR_OUTOFMEMORY;
	}

	component_array->width = width;
	component_array->height = height;
	component_array->pitch = pitch;
	component_array->data = buffer;
	component_array->bits_per_component = (PRECISION)bits_per_component;

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Initialize the data structure for the unpacked image

	Note that the component arrays will be allocated after the bitstream
	has been decoded and the dimensions of the component arrys are known.
*/
CODEC_ERROR InitUnpackedImage(UNPACKED_IMAGE *unpacked_image)
{
	if (unpacked_image == NULL) {
		return CODEC_ERROR_UNEXPECTED;
	}

	// Clear the fields in the unpacked iamge
	memset(unpacked_image, 0, sizeof(UNPACKED_IMAGE));

	return CODEC_ERROR_OKAY;
}


/*!
	Write the unpacked image components to a DPX file

	This routine is only intended for debugging as the unpacked image is usually
	passed to the image repacking process that packs the component arrays into a
	common image format.
*/
CODEC_ERROR WriteUnpackedImage(const UNPACKED_IMAGE *image,
							   PIXEL_FORMAT pixel_format,
							   const char *pathname)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;

	IMAGE output;

	DIMENSION width;
	DIMENSION height;

	int file_type = GetFileType(pathname);
	if (file_type != FILE_TYPE_DPX) {
		return CODEC_ERROR_UNSUPPORTED_FILE_TYPE;
	}

	width = image->component_array_list[0].width;
	height = image->component_array_list[0].height;

	// Allocate the DPX output image
	AllocImage(NULL, &output, width, height, PIXEL_FORMAT_DPX_50);

	// Pack the components into an image of DPX pixels
	ConvertComponentsToDPX0(image, pixel_format, &output);

	// Write the DPX image to the output file
	error = DPX_WriteImage(&output, pathname);

	ReleaseImage(NULL, &output);

	return error;
}
