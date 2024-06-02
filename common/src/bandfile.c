/*! @file common/src/bandfile.c

	Implementation of a module that writes wavelet bands to a file

	The band file is a custom file format for wavelet bands and can
	be used for debugging by comparing the bands computed by different
	versions of the codec.  The decoder does not free the wavelets
	after decoding a sample since they can be used for decoding the
	next sample, so after calling @ref DecodeSample selected wavelet
	bands can be written to a band file.

	A band file can contain bands for multiple frames (samples), one
	or more channels for each decoded sample, and any combination of
	wavelet bands in a channel.

	There is a header identified by a four character code for the start
	of the band file, frames, channels, wavelets, and bands.  A particular
	type of header is not repeated if the previous header is sufficient.
	For example, one wavelet header will be written in the band file for
	all of the bands in that wavelet.

	The file header contains that maximum dimensions and size of all of the
	bands written into the band file and can be used to allocate memory for
	storing bands that are read from the file.

	Typical usage is to use the routine @ref FindNextBand to find the next
	band in the file and then read the band data by calling the routine
	@ref ReadBandData.  The call to @ref FindNextBand updates the copy of
	the band parameters in the band file data structure using the values
	in the headers that it encounters while searching for the next chunk
	of band data.

	@todo Need to add a version number to the headers

	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#include <stdio.h>
#include <memory.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "bandfile.h"

#ifdef WIN32
#pragma warning(disable: 4996)
#endif

/*!
	@brief Four character codes for the band file headers

	@todo Byte swap the four character codes so that the letters
	appear in the correct order on little endian machines.
*/
typedef enum
{
	BAND_HEADER_FILE = 0x66696c65,		// 'file'
	BAND_HEADER_FRAME = 0x6672616d,		// 'fram'
	BAND_HEADER_CHANNEL = 0x6368616e,	// 'chan'
	BAND_HEADER_WAVELET = 0x77617665,	// 'wave'
	BAND_HEADER_DATA = 0x62616e64,		// 'band'

} BAND_HEADER_TYPE;


#pragma pack(push, 1)

struct header
{
	uint32_t type;
	uint32_t size;
};

struct file_header
{
	struct header h;
	uint16_t max_band_width;		// Maximum width of the wavelet bands
	uint16_t max_band_height;		// Maximum height of the wavelet bands
	uint32_t max_band_size;			// Size of the largest wavelet band (in bytes)
};

struct frame_header
{
	struct header h;
	uint32_t frame;
};

struct channel_header
{
	struct header h;
	uint16_t channel;
	uint16_t reserved;
};

struct wavelet_header
{
	struct header h;
	uint16_t wavelet;
	uint16_t reserved;
};

struct band_header
{
	struct header h;
	uint16_t band;
	uint16_t type;
	uint16_t width;
	uint16_t height;
	uint32_t size;
};

#pragma pack(pop)


//TODO: Need to document all of the bandfile routines


/*!
	@brief Open the band file for reading band data
*/
int OpenBandFile(BANDFILE *bandfile, const char *pathname)
{
	memset(bandfile, 0, sizeof(BANDFILE));
	bandfile->file = fopen(pathname, "rb");
	if (bandfile->file == NULL) {
		return BANDFILE_ERROR_OPEN_FAILED;
	}
	return BANDFILE_ERROR_OKAY;
}

/*!
	@brief Find the next band in the band file

	This routine is the recommended method for reading band data from
	a band file.  Any combination of bands can be stored in any order.
	This routine updates the band file data structure with the index
	of the next frame, channel, wavelet, and band and the type of band.

	After calling this routine, the calling application should call the
	routine @ref ReadBandData to read the actual band data.

*/
int FindNextBand(BANDFILE *bandfile)
{
	struct {
		struct header prefix;		// All headers start with a common prefix
		uint8_t payload[64];		// Allocate space for the maximum payload
	} header;

	//printf("Band file size: %d\n", sizeof(BANDFILE));

	memset(&header, 0, sizeof(header));

	while (header.prefix.type != BAND_HEADER_DATA)
	{
		size_t size = 0;
		size_t result;

		// Read the common prefix for all headers
		result = fread(&header.prefix, sizeof(header.prefix), 1, bandfile->file);
		if (result != 1) {
			if (feof(bandfile->file)) {
				return BANDFILE_ERROR_END_OF_DATA;
			}
			return BANDFILE_ERROR_READ_FAILED;
		}

		// Read the rest of the header
		size = header.prefix.size - sizeof(header.prefix);
		assert(size <= sizeof(header.payload));
		if (size > 0) {
			result = fread(header.payload, size, 1, bandfile->file);
			if (result != 1) {
				return BANDFILE_ERROR_READ_FAILED;
			}
		}

		switch (header.prefix.type)
		{
		case BAND_HEADER_FILE:
			bandfile->max_band_width = ((struct file_header *)&header)->max_band_width;
			bandfile->max_band_height = ((struct file_header *)&header)->max_band_height;
			bandfile->max_band_size = ((struct file_header *)&header)->max_band_size;
			break;

		case BAND_HEADER_FRAME:
			bandfile->frame = ((struct frame_header *)&header)->frame;
			break;

		case BAND_HEADER_CHANNEL:
			bandfile->channel = ((struct channel_header *)&header)->channel;
			break;

		case BAND_HEADER_WAVELET:
			bandfile->wavelet = ((struct wavelet_header *)&header)->wavelet;
			break;

		case BAND_HEADER_DATA:
			bandfile->band = ((struct band_header *)&header)->band;
			bandfile->type = ((struct band_header *)&header)->type;
			bandfile->width = ((struct band_header *)&header)->width;
			bandfile->height = ((struct band_header *)&header)->height;
			bandfile->size = ((struct band_header *)&header)->size;
			break;

		default:
			// Unknown header
			assert(0);
			return BANDFILE_ERROR_UNKNOWN_HEADER;
			break;
		}
	}

	return BANDFILE_ERROR_OKAY;
}

int ReadFileHeader(BANDFILE *bandfile)
{
	struct file_header header;
	size_t result;

	memset(&header, 0, sizeof(header));
	result = fread(&header, sizeof(header), 1, bandfile->file);
	if (result != 1) {
		return BANDFILE_ERROR_READ_FAILED;
	}
	assert(header.h.type == BAND_HEADER_FILE);
	assert(header.h.size == sizeof(header));
	return BANDFILE_ERROR_OKAY;
}

int ReadFrameHeader(BANDFILE *bandfile)
{
	struct frame_header header;
	size_t result;

	memset(&header, 0, sizeof(header));
	result = fread(&header, sizeof(header), 1, bandfile->file);
	if (result != 1) {
		return BANDFILE_ERROR_READ_FAILED;
	}
	assert(header.h.type == BAND_HEADER_FRAME);
	assert(header.h.size == sizeof(header));
	bandfile->frame = header.frame;
	return BANDFILE_ERROR_OKAY;
}

int ReadChannelHeader(BANDFILE *bandfile)
{
	struct channel_header header;
	size_t result;

	memset(&header, 0, sizeof(header));
	result = fread(&header, sizeof(header), 1, bandfile->file);
	if (result != 1) {
		return BANDFILE_ERROR_READ_FAILED;
	}
	assert(header.h.type == BAND_HEADER_CHANNEL);
	assert(header.h.size == sizeof(header));
	bandfile->channel = header.channel;
	return BANDFILE_ERROR_OKAY;
}

int ReadWaveletHeader(BANDFILE *bandfile)
{
	struct wavelet_header header;
	size_t result;

	memset(&header, 0, sizeof(header));
	result = fread(&header, sizeof(header), 1, bandfile->file);
	if (result != 1) {
		return BANDFILE_ERROR_READ_FAILED;
	}
	assert(header.h.type == BAND_HEADER_WAVELET);
	assert(header.h.size == sizeof(header));
	bandfile->wavelet = header.wavelet;
	return BANDFILE_ERROR_OKAY;
}

int ReadBandHeader(BANDFILE *bandfile)
{
	struct band_header header;
	size_t result;

	memset(&header, 0, sizeof(header));
	result = fread(&header, sizeof(header), 1, bandfile->file);
	if (result != 1) {
		return BANDFILE_ERROR_READ_FAILED;
	}
	assert(header.h.type == BAND_HEADER_DATA);
	assert(header.h.size == sizeof(header));
	bandfile->band = header.band;
	bandfile->type = header.type;
	bandfile->width = header.width;
	bandfile->height = header.height;
	bandfile->size = header.size;
	return BANDFILE_ERROR_OKAY;
}

/*!
	@brief Read the data for the next band from the band file
*/
int ReadBandData(BANDFILE *bandfile, void *data, size_t size)
{
#if 1
	size_t result = fread(data, size, 1, bandfile->file);
	if (result != 1) {
		return BANDFILE_ERROR_READ_FAILED;
	}
#else
	fseek(bandfile->file, size, SEEK_CUR);
#endif
	return BANDFILE_ERROR_OKAY;
}

/*!
	@brief Create a band file for storing band data
*/
int CreateBandFile(BANDFILE *bandfile, const char *pathname)
{
	memset(bandfile, 0, sizeof(BANDFILE));
	bandfile->file = fopen(pathname, "wb");
	if (bandfile->file == NULL) {
		return BANDFILE_ERROR_CREATE_FAILED;
	}
	return BANDFILE_ERROR_OKAY;
}

/*!
	@brief Write the band data to the band file

	This is the recommended method for writing band data to a file.
	Any headers that must be written to the file will be written
	before the band data.  For example, if the frame, channel, and
	wavelet numbers have not changed since the last call to this routine
	then the frame, channel, and wavelet headers will be be rewritten.
*/
int WriteWaveletBand(BANDFILE *bandfile,
					 int frame,
					 int channel,
					 int wavelet,
					 int band,
					 int type,
					 int width,
					 int height,
					 void *data,
					 size_t size)
{
	assert(bandfile->file_header_flag);

	if (!bandfile->frame_header_flag || bandfile->frame != (uint32_t)frame) {
		WriteFrameHeader(bandfile, frame);
	}

	if (!bandfile->channel_header_flag || bandfile->channel != channel) {
		WriteChannelHeader(bandfile, channel);
	}

	if (!bandfile->wavelet_header_flag || bandfile->wavelet != wavelet) {
		WriteWaveletHeader(bandfile, wavelet);
	}

	if (!bandfile->band_header_flag || bandfile->band != band || bandfile->type != type) {
		WriteBandHeader(bandfile, band, type, width, height, size);
	}

	WriteBandData(bandfile, data, size);

	return BANDFILE_ERROR_OKAY;
}

int WriteFileHeader(BANDFILE *bandfile, int max_band_width, int max_band_height)
{
	struct file_header header;
	size_t result;

	memset(&header, 0, sizeof(header));
	header.h.type = BAND_HEADER_FILE;
	header.h.size = sizeof(header);
	header.max_band_width = (uint16_t)max_band_width;
	header.max_band_height = (uint16_t)max_band_height;
	header.max_band_size = (max_band_width * max_band_height) * 2;
	result = fwrite(&header, sizeof(header), 1, bandfile->file);
	if (result != 1) {
		return BANDFILE_ERROR_WRITE_FAILED;
	}
	bandfile->file_header_flag = true;
	bandfile->frame_header_flag = false;
	return BANDFILE_ERROR_OKAY;
}

int WriteFrameHeader(BANDFILE *bandfile, int frame)
{
	struct frame_header header;
	size_t result;

	memset(&header, 0, sizeof(header));
	header.h.type = BAND_HEADER_FRAME;
	header.h.size = sizeof(header);\
	header.frame = frame;
	result = fwrite(&header, sizeof(header), 1, bandfile->file);
	if (result != 1) {
		return BANDFILE_ERROR_WRITE_FAILED;
	}
	bandfile->frame = frame;
	bandfile->frame_header_flag = true;
	bandfile->channel_header_flag = false;
	return BANDFILE_ERROR_OKAY;
}

int WriteChannelHeader(BANDFILE *bandfile, int channel)
{
	struct channel_header header;
	size_t result;

	memset(&header, 0, sizeof(header));
	header.h.type = BAND_HEADER_CHANNEL;
	header.h.size = sizeof(header);
	header.channel = (uint16_t)channel;
	result = fwrite(&header, sizeof(header), 1, bandfile->file);
	if (result != 1) {
		return BANDFILE_ERROR_WRITE_FAILED;
	}
	bandfile->channel = (uint16_t)channel;
	bandfile->channel_header_flag = true;
	bandfile->wavelet_header_flag = false;
	return BANDFILE_ERROR_OKAY;
}

int WriteWaveletHeader(BANDFILE *bandfile, int wavelet)
{
	struct wavelet_header header;
	size_t result;

	memset(&header, 0, sizeof(header));
	header.h.type = BAND_HEADER_WAVELET;
	header.h.size = sizeof(header);
	header.wavelet = (uint16_t)wavelet;
	result = fwrite(&header, sizeof(header), 1, bandfile->file);
	if (result != 1) {
		return BANDFILE_ERROR_WRITE_FAILED;
	}
	bandfile->wavelet = (uint16_t)wavelet;
	bandfile->wavelet_header_flag = true;
	bandfile->band_header_flag = false;
	return BANDFILE_ERROR_OKAY;
}

int WriteBandHeader(BANDFILE *bandfile, int band, int type, int width, int height, size_t size)
{
	struct band_header header;
	size_t result;

	memset(&header, 0, sizeof(header));
	header.h.type = BAND_HEADER_DATA;
	header.h.size = sizeof(header);
	header.band = (uint16_t)band;
	header.type = (uint16_t)type;
	header.width = (uint16_t)width;
	header.height = (uint16_t)height;
	header.size = (uint32_t)size;
	result = fwrite(&header, sizeof(header), 1, bandfile->file);
	if (result != 1) {
		return BANDFILE_ERROR_WRITE_FAILED;
	}
	bandfile->band = (uint16_t)band;
	bandfile->type = (uint16_t)type;
	bandfile->width = (uint16_t)width;
	bandfile->height = (uint16_t)height;
	bandfile->size = (uint32_t)size;
	bandfile->band_header_flag = true;
	return BANDFILE_ERROR_OKAY;
}

int WriteBandData(BANDFILE *bandfile, void *data, size_t size)
{
	size_t result = fwrite(data, size, 1, bandfile->file);
	if (result != 1) {
		return BANDFILE_ERROR_WRITE_FAILED;
	}
	return BANDFILE_ERROR_OKAY;
}

int CloseBandFile(BANDFILE *bandfile)
{
	if (bandfile->file) {
		fclose(bandfile->file);
		bandfile->file = NULL;
	}
	return BANDFILE_ERROR_OKAY;
}
