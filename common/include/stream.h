/*! @file common/include/stream.h

	The stream abstracts the methods used by bitstreams to output bytes

	A stream may be bound to a binary file opened for writing, or a
	buffer in memory, or code that writes a byte stream into a track
	in a media container.

	Bitstreams write bytes to a stream from a buffer that holds partially
	written words and is internal to the bitstream.

	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _STREAM_H
#define _STREAM_H


//! Type of stream (binary file or memory buffer)
typedef enum _stream_type
{
	STREAM_TYPE_UNKNOWN = 0,		//!< Unknown type of stream
	STREAM_TYPE_FILE,				//!< Simple binary file
	STREAM_TYPE_MEMORY,				//!< Buffer in memory
	//STREAM_TYPE_AVI,				//!< Track in an AVI file
	//STREAM_TYPE_MOV,				//!< Track in a QuickTime file

} STREAM_TYPE;

/*!
	@brief Stream access (read or write)

	The stream provided with the reference decoder only supports
	read access.
*/
typedef enum _stream_access
{
	STREAM_ACCESS_UNKNOWN = 0,
	STREAM_ACCESS_READ,
	STREAM_ACCESS_WRITE,

} STREAM_ACCESS;

/*!
    @brief Error codes for byte streams
*/
typedef enum _stream_error
{
    STREAM_ERROR_OKAY = 0,      //!< No error
    STREAM_ERROR_EOF,           //!< Could not obtain more bytes from the stream
    
} STREAM_ERROR;


/*!
	@brief Declaration of the data structure for a byte stream

	The byte stream encapsulates the location of encoded images and the
	means for reading (writing) encoded images samples.  The byte stream
	could be a binary file that has been opened for reading (writing) or
	a buffer in memory.  The reference codec uses a binary file as the byte
	stream so the functionality for streams attached to memory buffers has
	not been tested.

	It is intended that the byte stream can be enhanced to read (write)
	encoded images from (into) a track in a media container.
*/
typedef struct _stream
{
	STREAM_TYPE type;		//!< Type of stream (file or memory buffer)
	STREAM_ACCESS access;	//!< Type of access (read or write)
    STREAM_ERROR error;     //!< Most recent error code

	//! Union of parameters for different types of streams
	union _location
	{
		//! Parameters for a binary file stream
		struct _file
		{
			FILE *iobuf;	//!< Binary file that contains the stream

		} file;			//!< Parameters for a stream in a binary file

		//! Parameters for a stream bound to a memory buffer
		struct _memory
		{
			void *buffer;	//!< Memory buffer that contains the stream
			size_t size;	//!< Length of the stream (in bytes)
			size_t count;	//!< Number of bytes that have been written

		} memory;		//!< Parameters for a stream in a memory buffer

		//TODO: Add other stream types for media containers

	} location;		//!< Location of the byte stream (file or memory buffer)

	size_t byte_count;		//!< Number of bytes read or written to the stream

} STREAM;

CODEC_ERROR OpenStream(STREAM *stream, const char *pathname);

CODEC_ERROR CreateStream(STREAM *stream, const char *pathname);

CODEC_ERROR CloseStream(STREAM *stream);

//CODEC_ERROR ResetStream(STREAM *stream);
CODEC_ERROR RewindStream(STREAM *stream);

uint32_t GetWord(STREAM *stream);

uint8_t GetByte(STREAM *stream);

CODEC_ERROR SkipBytes(STREAM *stream, size_t size);

CODEC_ERROR PutWord(STREAM *stream, uint32_t word);

CODEC_ERROR PutByte(STREAM *stream, uint8_t byte);

CODEC_ERROR PadBytes(STREAM *stream, size_t size);

CODEC_ERROR FlushStream(STREAM *stream);

CODEC_ERROR CreateStreamBuffer(STREAM *stream, void *buffer, size_t size);

CODEC_ERROR GetStreamBuffer(STREAM *stream, void **buffer_out, size_t *size_out);

CODEC_ERROR GetBlock(STREAM *stream, void *buffer, size_t size, size_t offset);

CODEC_ERROR PutBlock(STREAM *stream, void *buffer, size_t size, size_t offset);

bool EndOfStream(STREAM *stream);

#endif
