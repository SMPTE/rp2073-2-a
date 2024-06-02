/*! @file interlaced.c

	Implementation of the functions for handling interlaced frames.

	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#include "headers.h"

CODEC_ERROR UnpackFields(IMAGE *input_image, IMAGE *top_field, IMAGE *bottom_field)
{
	int row;

	assert(input_image->height == top_field->height * 2);
	assert(input_image->height == bottom_field->height * 2);

	for (row = 0; row < top_field->height; row++)
	{
		uint8_t *field1_row_ptr = (uint8_t *)input_image->buffer + 2 * row * input_image->pitch;
		uint8_t *field2_row_ptr = field1_row_ptr + input_image->pitch;

		uint8_t *top_field_row_ptr = (uint8_t *)top_field->buffer + row * top_field->pitch;
		uint8_t *bottom_field_row_ptr = (uint8_t *)bottom_field->buffer + row * bottom_field->pitch;

		memcpy(top_field_row_ptr, field1_row_ptr, input_image->pitch);
		memcpy(bottom_field_row_ptr, field2_row_ptr, input_image->pitch);
	}

	return CODEC_ERROR_OKAY;
}

CODEC_ERROR DecomposeFields(IMAGE *interlaced_frame, IMAGE *image_array[], int frame_count)
{
	DIMENSION image_width;
	DIMENSION image_height;
	DIMENSION field_height;
	int frame_index;

	assert(interlaced_frame != NULL && frame_count == 2);

	image_width = interlaced_frame->width;
	image_height = interlaced_frame->height;
	field_height = image_height / 2;

	// Allocate the output frames (if necessary)
	for (frame_index = 0; frame_index < frame_count; frame_index++)
	{
		if (image_array[frame_index] != NULL)
		{
			assert(image_array[frame_index]->width == image_width && image_array[frame_index]->height == field_height);
			if (! (image_array[frame_index]->width == image_width && image_array[frame_index]->height == field_height)) {
				return CODEC_ERROR_IMAGE_DIMENSIONS;
			}
		}
		else
		{
			image_array[frame_index] = CreateImage(NULL, image_width, field_height, interlaced_frame->format);
		}
	}

	return UnpackFields(interlaced_frame, image_array[0], image_array[1]);
}

CODEC_ERROR ComposeFields(IMAGE frame_array[], int frame_count, IMAGE *output_frame)
{
	IMAGE *field1;
	IMAGE *field2;
	int field_height;
	int row;

	assert(frame_count == 2);

	field1 = &frame_array[0];
	field2 = &frame_array[1];

	assert(output_frame->height == field1->height * 2);
	assert(output_frame->height == field2->height * 2);

	field_height = field1->height;

	for (row = 0; row < field_height; row++)
	{
		uint8_t *field1_row_ptr = (uint8_t *)field1->buffer + row * field1->pitch;
		uint8_t *field2_row_ptr = (uint8_t *)field2->buffer + row * field2->pitch;

		uint8_t *output_row_ptr = (uint8_t *)output_frame->buffer + 2 * row * output_frame->pitch;

		memcpy(output_row_ptr, field1_row_ptr, output_frame->pitch);
		output_row_ptr += output_frame->pitch;

		memcpy(output_row_ptr, field2_row_ptr, output_frame->pitch);
	}

	return CODEC_ERROR_OKAY;
}
