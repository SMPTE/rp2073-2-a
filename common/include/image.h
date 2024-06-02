/*!	@file common/include/image.h

	Declaration of structures and functions for images

	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _IMAGE_H
#define _IMAGE_H


//! Data type for the values in a component array
typedef uint16_t COMPONENT_VALUE;


/*!
	@brief Data structure for an image input to the unpacking process

	This data structure is used to represent the image that is the input to the
	image unpacking process that unpacks an image into component arrays for encoding.
	Unlike the wavelet data structures, an image contains multiple color components,
	usually in a packed pixel format.
*/
typedef struct _packed_image
{
	DIMENSION width;			//!< Width of the frame (in pixels)
	DIMENSION height;			//!< Height of the frame
	size_t pitch;				//!< Distance between rows (in bytes)
	PIXEL_FORMAT format;		//!< Format of the pixels
	void *buffer;				//!< Address of the buffer for the frame
	size_t size;				//!< Allocated size of the buffer (in bytes)
	size_t offset;				//!< Offset to the start of the frame

} PACKED_IMAGE;

//! Short name for the packed image data type
typedef PACKED_IMAGE IMAGE;

/*!
	@brief Data structure for an array that contains a single type of component

	This data structure is used to represent the component array output by the image
	unpacking process.  The image unpacking process unpacks an image into component
	arrays for encoding.
*/
typedef struct _component_array
{
	DIMENSION width;			//!< Width of the image (in pixels)
	DIMENSION height;			//!< Height of the image
	size_t pitch;				//!< Distance between rows (in bytes)
	COMPONENT_VALUE *data;		//!< Buffer for the array of component values

	//! Number of bits per in each component value
	PRECISION bits_per_component;

} COMPONENT_ARRAY;

/*!
	@brief Image represented as an ordered set of component arrays

	The decoder outputs a set of component arrays that represent an image.

	The image repacking process can pack the component arrays output by the
	decoder into a packed image.
*/
typedef struct _unpacked_image
{
	//! Number of component arrays in the unpacked image
	int component_count;

	//! Vector of component arrays
	COMPONENT_ARRAY *component_array_list;

} UNPACKED_IMAGE;


#if VC5_ENABLED_PART(VC5_PART_LAYERS) || VC5_ENABLED_PART(VC5_PART_SECTIONS)

/*!
    @brief Ordered set of packed images
 
    A packed image list represents one or more images that comprise layers or
    the list of images that are encoded into the bitstream as image sections.
 
    Each layer image must have the same width and height, but there are no restrictions
    on the dimensions of images that are encoded as image sections since each image in an
    image section is independent of the other images encoded in the bitstream.
*/
typedef struct _packed_image_list
{
    //! Number of images in the list of images
    int image_count;
    
    //! List of packed images
    IMAGE *image_list[MAX_LAYER_COUNT];
    
} PACKED_IMAGE_LIST;

//! Short name for the packed image list data type
typedef PACKED_IMAGE_LIST IMAGE_LIST;

/*!
    @brief Ordered set of unpacked images
 */
typedef struct _unpacked_image_list
{
    //! Number of images in the list of images
    int image_count;
    
    //! List of packed images
    UNPACKED_IMAGE *image_list[MAX_LAYER_COUNT];

} UNPACKED_IMAGE_LIST;

#endif


/*!
	@brief Flags that describe the image structure
*/
typedef enum
{
	IMAGE_STRUCTURE_INTERLACED = 0x0001,			//!< Set the first bit if the image is interlaced
	IMAGE_STRUCTURE_BOTTOM_FIELD_FIRST = 0x0002,	//!< The bottom field is encoded before the top field
	IMAGE_STRUCTURE_BOTTOM_ROW_FIRST = 0x0010,		//!< The encoded image is upside down
} IMAGE_STRUCTURE;


#ifdef __cplusplus
extern "C" {
#endif

CODEC_ERROR InitImage(IMAGE *image);

IMAGE *CreateImage(ALLOCATOR *allocator, DIMENSION width, DIMENSION height, PIXEL_FORMAT format);

//CODEC_ERROR AllocImage(ALLOCATOR *allocator, IMAGE *image, DIMENSION width, DIMENSION height, PIXEL_FORMAT format);
CODEC_ERROR AllocImage(ALLOCATOR *allocator, IMAGE *image, DIMENSION width, DIMENSION height, PIXEL_FORMAT format);

CODEC_ERROR AllocImageSize(ALLOCATOR *allocator, IMAGE *image, size_t size);

CODEC_ERROR AllocImageCopy(ALLOCATOR *allocator, IMAGE *image, IMAGE *prototype, PIXEL_FORMAT format);

CODEC_ERROR ReleaseImage(ALLOCATOR *allocator, IMAGE *image);
    
CODEC_ERROR FreeImage(ALLOCATOR *allocator, IMAGE *image);

DIMENSION ImagePitch(DIMENSION width, PIXEL_FORMAT format);

CODEC_ERROR SetImageFormat(IMAGE *image,
						   DIMENSION width,
						   DIMENSION height,
						   DIMENSION pitch,
						   PIXEL_FORMAT format,
						   size_t offset);

void *ImageData(IMAGE *image);

void *RowAddress(IMAGE *image, DIMENSION row);

CODEC_ERROR PackOutputImage(void *buffer, size_t pitch, int encoded_format, IMAGE *image);

CODEC_ERROR PackBufferRowsToRG48(PIXEL *input_buffer, size_t input_pitch,
								 PIXEL *output_buffer, size_t output_pitch,
								 DIMENSION width, DIMENSION height);

CODEC_ERROR AllocateComponentArrays(ALLOCATOR *allocator,
									UNPACKED_IMAGE *image,
									int channel_count,
									DIMENSION max_channel_width,
									DIMENSION max_channel_height,
									PIXEL_FORMAT format,
									PRECISION bits_per_component);

CODEC_ERROR ReleaseComponentArrays(ALLOCATOR *allocator,
                                   UNPACKED_IMAGE *image,
                                   int channel_count);
    
CODEC_ERROR AllocateComponentArray(ALLOCATOR *allocator,
                                   COMPONENT_ARRAY *component_array,
								   DIMENSION width,
								   DIMENSION height,
								   PRECISION bits_per_component);

CODEC_ERROR InitUnpackedImage(UNPACKED_IMAGE *image);


CODEC_ERROR WriteUnpackedImage(const UNPACKED_IMAGE *image,
							   PIXEL_FORMAT pixel_format,
							   ENABLED_PARTS enabled_parts,
							   const char *pathname);

PRECISION MaxBitsPerComponent(const UNPACKED_IMAGE *image);


#if VC5_ENABLED_PART(VC5_PART_LAYERS) || VC5_ENABLED_PART(VC5_PART_SECTIONS)
    
CODEC_ERROR InitImageList(IMAGE_LIST *image_list, COUNT image_count);
    
CODEC_ERROR AllocListImage(ALLOCATOR *allocator, IMAGE_LIST *image_list, int image_index,
                           DIMENSION image_width, DIMENSION image_height, PIXEL_FORMAT pixel_format);

CODEC_ERROR PrintImageList(IMAGE_LIST *image_list);

bool CheckLayerImageList(IMAGE_LIST *image_list);

CODEC_ERROR InitUnpackedImageList(UNPACKED_IMAGE_LIST *unpacked_image_list, COUNT image_count);
    
CODEC_ERROR ReleaseUnpackedImageList(ALLOCATOR *allocator, UNPACKED_IMAGE_LIST *unpacked_image_list);

#endif

#ifdef __cplusplus
}
#endif

#endif
