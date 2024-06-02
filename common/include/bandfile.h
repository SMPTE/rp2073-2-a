/*!	@file common/include/bandfile.h

	Declaration of the data structures and functions for reading
	and writing binary files that contain band data (for debugging).

	The band file can contain one or more channels and any combination
	of subbands can be written for each channel.

	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _BANDFILE_H
#define _BANDFILE_H

/*!
	@brief Declaration of the band file data structure

	@todo Replace the Boolean flags that are currently defined to
	be one byte with a 32-bit word using one bit per flag.
*/
#pragma pack(push, 1)

typedef struct _bandfile
{
	FILE *file;					//!< Current open band file
	uint32_t frame;				//!< Most recent frame number
	uint16_t channel;			//!< Most recent channel index
	uint16_t wavelet;			//!< Most recent wavelet index
	uint16_t band;				//!< Most recent band index
	uint16_t type;				//!< Data type of the most recent band
	uint32_t size;				//!< Size of the most recent band (in bytes)

	uint16_t width;				//!< Width of the most recent band
	uint16_t height;			//!< Height of the most recent band

	uint16_t max_band_width;	//!< Maximum width of a band in the band file
	uint16_t max_band_height;	//!< Maximum height of a band in the band file

	uint32_t max_band_size;		//!< Maximum size of the bands in the band file

	// Flags that indicate the state of the band data file
	uint8_t file_header_flag;		//!< Has the file header been written?
	uint8_t frame_header_flag;		//!< Has the frame header been written?
	uint8_t channel_header_flag;	//!< Has the channel header been written?
	uint8_t wavelet_header_flag;	//!< Has the wavelet header been written?
	uint8_t band_header_flag;		//!< Has the band header been written?

	uint8_t padding[3];				//!< Pad the structure to a multiple of four bytes

} BANDFILE;

#pragma pack(pop)

/*!
	@brief Data type of the data for a band in the band file
*/
typedef enum
{
	BAND_TYPE_UINT16 = 0,
	BAND_TYPE_SINT16,

	// Reserve a block of values for encoded bands
	BAND_TYPE_ENCODED = 16,
	BAND_TYPE_ENCODED_RUNLENGTHS,		// Encoding method used by the codec

} BAND_TYPE;

/*!
	@brief Error codes returned by the band file module
*/
//typedef enum
enum
{
	BANDFILE_ERROR_OKAY = 0,
	BANDFILE_ERROR_OPEN_FAILED,
	BANDFILE_ERROR_READ_FAILED,
	BANDFILE_ERROR_UNKNOWN_HEADER,
	BANDFILE_ERROR_CREATE_FAILED,
	BANDFILE_ERROR_WRITE_FAILED,
	BANDFILE_ERROR_END_OF_DATA,
};

typedef int BANDFILE_ERROR;


#ifdef __cplusplus
extern "C" {
#endif

int OpenBandFile(BANDFILE *bandfile, const char *pathname);

int FindNextBand(BANDFILE *bandfile);

int ReadFileHeader(BANDFILE *bandfile);

int ReadFrameHeader(BANDFILE *bandfile);

int ReadChannelHeader(BANDFILE *bandfile);

int ReadWaveletHeader(BANDFILE *bandfile);

int ReadBandHeader(BANDFILE *bandfile);

int ReadBandData(BANDFILE *bandfile, void *data, size_t size);

int CreateBandFile(BANDFILE *bandfile, const char *pathname);

int WriteFileHeader(BANDFILE *bandfile, int max_band_width, int max_band_height);

int WriteWaveletBand(BANDFILE *bandfile,
					 int frame,
					 int channel,
					 int wavelet,
					 int band,
					 int type,
					 int width,
					 int height,
					 void *data,
					 size_t size);

int WriteFrameHeader(BANDFILE *bandfile, int frame);

int WriteChannelHeader(BANDFILE *bandfile, int channel);

int WriteWaveletHeader(BANDFILE *bandfile, int wavelet);

int WriteBandHeader(BANDFILE *bandfile, int band, int type, int width, int height, size_t size);

int WriteBandData(BANDFILE *bandfile, void *data, size_t size);

int CloseBandFile(BANDFILE *bandfile);

#ifdef __cplusplus
}
#endif

#endif
