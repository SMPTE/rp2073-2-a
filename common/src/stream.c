/*! @file common/src/stream.c

	This module implements a byte stream abstraction that hides the details
	of how a stream of bytes is read or written on demand by the bitstream.
	The	byte stream can be bound to a binary file opened for reading (writing),
	to a buffer in memory, or to a module that reads (writes) a video track
	in a media container.

	This module is not part of the reference encoder but is provided to that
	the bitstream module can write bytes to a file or a memory buffer.

	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#include "headers.h"

// Local functions
CODEC_ERROR GetBlockFile(STREAM *stream, void *buffer, size_t size, size_t offset);
CODEC_ERROR PutBlockFile(STREAM *stream, void *buffer, size_t size, size_t offset);
CODEC_ERROR GetBlockMemory(STREAM *stream, void *buffer, size_t size, size_t offset);
CODEC_ERROR PutBlockMemory(STREAM *stream, void *buffer, size_t size, size_t offset);

/*!
	@brief Open a stream for reading bytes from the specified file

*/
CODEC_ERROR OpenStream(STREAM *stream, const char *pathname)
{
	assert(stream != NULL);
	if (! (stream != NULL)) {
		return CODEC_ERROR_NULLPTR;
	}

	// Clear all members of the stream data structure
	memset(stream, 0, sizeof(STREAM));

	// Open the file and bind it to the stream
	stream->location.file.iobuf = fopen(pathname, "rb");
	assert(stream->location.file.iobuf != NULL);
	if (! (stream->location.file.iobuf != NULL)) {
		return CODEC_ERROR_OPEN_FILE_FAILED;
	}

	// Set the stream type and access
	stream->type = STREAM_TYPE_FILE;
	stream->access = STREAM_ACCESS_READ;

	// Clear the number of bytes read from the stream
	stream->byte_count = 0;

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Create a stream for reading bytes from the specified file

*/
CODEC_ERROR CreateStream(STREAM *stream, const char *pathname)
{
	assert(stream != NULL);
	if (! (stream != NULL)) {
		return CODEC_ERROR_NULLPTR;
	}

	// Clear all members of the stream data structure
	memset(stream, 0, sizeof(STREAM));

	// Open the file and bind it to the stream
	stream->location.file.iobuf = fopen(pathname, "wb+");
	assert(stream->location.file.iobuf != NULL);
	if (! (stream->location.file.iobuf != NULL)) {
		return CODEC_ERROR_CREATE_FILE_FAILED;
	}

	// Set the stream type and access
	stream->type = STREAM_TYPE_FILE;
	stream->access = STREAM_ACCESS_WRITE;

	// Clear the number of bytes written to the stream
	stream->byte_count = 0;

	return CODEC_ERROR_OKAY;
}

/*!
    @brief Close the stream
*/
CODEC_ERROR CloseStream(STREAM *stream)
{
    if (stream != NULL && stream->location.file.iobuf != NULL)
    {
        fclose(stream->location.file.iobuf);
        stream->location.file.iobuf = NULL;
    }
    
    return CODEC_ERROR_OKAY;
}

/*!
	@brief Read a word from a byte stream

	This routine is used by the bitstream to read a word from a byte stream.
	A word is the number of bytes that can be stored in the internal buffer
	used by the bitstream.

	@todo Need to modify the routine to return an indication of end of file
	or an error reading from the byte stream.
*/
uint32_t GetWord(STREAM *stream)
{
	uint32_t buffer;
	size_t count = fread(&buffer, sizeof(buffer), 1, stream->location.file.iobuf);
    if (count != 1)
    {
        stream->error = STREAM_ERROR_EOF;
        return 0;
    }
    
	stream->byte_count += sizeof(buffer);
	return buffer;
}

/*!
	@brief Read a byte from a byte stream
*/
uint8_t GetByte(STREAM *stream)
{
	int byte = fgetc(stream->location.file.iobuf);
	stream->byte_count++;
	assert(byte >= 0 && (byte & ~0xFF) == 0);
	return (uint8_t)byte;
}

/*!
	@brief Write a word to a byte stream

	This routine is used by the bitstream to write a word to a byte stream.
	A word is the number of bytes that can be stored in the internal buffer
	used by the bitstream.

	@todo Need to modify the routine to return an indication of an error
	writing to the byte stream.
*/
CODEC_ERROR PutWord(STREAM *stream, uint32_t word)
{
	assert(stream != NULL);
	if (! (stream != NULL)) {
		return CODEC_ERROR_NULLPTR;
	}

	switch (stream->type)
	{
	case STREAM_TYPE_FILE:
		fwrite(&word, sizeof(word), 1, stream->location.file.iobuf);
		break;

	case STREAM_TYPE_MEMORY:
		memcpy((uint8_t *)stream->location.memory.buffer + stream->location.memory.count, &word, sizeof(word));
		stream->location.memory.count += sizeof(word);
		break;

	default:
		assert(0);
		break;
	}

	stream->byte_count += sizeof(word);

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Write a byte to a byte stream
*/
CODEC_ERROR PutByte(STREAM *stream, uint8_t byte)
{
	assert(stream != NULL);
	if (! (stream != NULL)) {
		return CODEC_ERROR_NULLPTR;
	}

	//assert(byte >= 0 && (byte & ~0xFF) == 0);

	switch (stream->type)
	{
	case STREAM_TYPE_FILE:
		fputc(byte,stream->location.file.iobuf);
		break;

	case STREAM_TYPE_MEMORY:
		((uint8_t *)stream->location.memory.buffer)[stream->location.memory.count++] = byte;
		break;

	default:
		assert(0);
		break;
	}

	stream->byte_count++;

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Rewind the stream to the beginning of the buffer or file
*/
//CODEC_ERROR ResetStream(STREAM *stream)
CODEC_ERROR RewindStream(STREAM *stream)
{
	assert(stream != NULL);
	if (! (stream != NULL)) {
		return CODEC_ERROR_NULLPTR;
	}
	
	switch (stream->type)
	{
	case STREAM_TYPE_FILE:
		if (stream->location.file.iobuf != NULL) {
			assert(fseek(stream->location.file.iobuf, 0, SEEK_SET) == 0);
		}
		break;

	case STREAM_TYPE_MEMORY:
		stream->location.memory.count = 0;
		break;

	default:
		assert(0);
		break;
	}

	stream->byte_count = 0;

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Skip the specified number of bytes in the stream
*/
CODEC_ERROR SkipBytes(STREAM *stream, size_t size)
{
	for (; size > 0; size--)
	{
		(void)GetByte(stream);
	}
	return CODEC_ERROR_OKAY;
}

/*!
	@brief Pad the specified number of bytes in the stream
*/
CODEC_ERROR PadBytes(STREAM *stream, size_t size)
{
	const uint8_t byte = 0;
	for (; size > 0; size--)
	{
		PutByte(stream, byte);
	}
	return CODEC_ERROR_OKAY;
}

/*!
	@brief Write the stream buffer to the file
*/
CODEC_ERROR FlushStream(STREAM *stream)
{
	assert(stream != NULL);
	if (! (stream != NULL)) {
		return CODEC_ERROR_NULLPTR;
	}

	if (stream->type == STREAM_TYPE_FILE)
	{
		int result = fflush(stream->location.file.iobuf);
		if (result != 0) {
			return CODEC_ERROR_FILE_FLUSH_FAILED;
		}
	}

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Create a byte stream for a memory buffer
*/
CODEC_ERROR CreateStreamBuffer(STREAM *stream, void *buffer, size_t size)
{
	assert(stream != NULL);
	if (! (stream != NULL)) {
		return CODEC_ERROR_NULLPTR;
	}

	// Clear all members of the stream data structure
	memset(stream, 0, sizeof(STREAM));

	// Bind the stream to the buffer
	stream->location.memory.buffer = buffer;
	stream->location.memory.size = size;
	stream->location.memory.count = 0;

	// Set the stream type and access
	stream->type = STREAM_TYPE_MEMORY;
	stream->access = STREAM_ACCESS_WRITE;

	// Clear the number of bytes written to the stream
	stream->byte_count = 0;

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Return the starting address and number of bytes in a buffer

	This routine is used to get the address and count of the bytes written
	to a memory stream (buffer).
*/
CODEC_ERROR GetStreamBuffer(STREAM *stream, void **buffer_out, size_t *size_out)
{
	assert(stream != NULL);
	if (! (stream != NULL)) {
		return CODEC_ERROR_NULLPTR;
	}

	assert(stream->type == STREAM_TYPE_MEMORY);

	if (buffer_out != NULL) {
		*buffer_out = stream->location.memory.buffer;
	}

	if (size_out != NULL) {
		*size_out = stream->location.memory.count;
	}

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Read a block of data at the specified offset in the byte stream
*/
CODEC_ERROR GetBlock(STREAM *stream, void *buffer, size_t size, size_t offset)
{
	switch (stream->type)
	{
	case STREAM_TYPE_FILE:
		return GetBlockFile(stream, buffer, size, offset);
		break;

	case STREAM_TYPE_MEMORY:
		return GetBlockMemory(stream, buffer, size, offset);
		break;

	case STREAM_TYPE_UNKNOWN:
		assert(0);
		break;
	}

	return CODEC_ERROR_UNEXPECTED;
}

/*!
	@brief Read a block of data from a file stream
*/
CODEC_ERROR GetBlockFile(STREAM *stream, void *buffer, size_t size, size_t offset)
{
	FILE *file = stream->location.file.iobuf;
	fpos_t position;

	// Save the current position in the file
	if (fgetpos(file, &position) != 0) {
		return CODEC_ERROR_FILE_GET_POSITION;
	}

	// Seek to the specified offset
	assert(0 <= offset && offset <= LONG_MAX);
	if (fseek(file, (long)offset, SEEK_SET) != 0) {
		return CODEC_ERROR_FILE_SEEK;
	}

	// Read data from the file
	if (fread(buffer, size, 1, file) != 1) {
		return CODEC_ERROR_FILE_READ;
	}

	// Return to the previous position in the file
	// if (fseek(file, (long)position, SEEK_SET) != 0) {
	if (fsetpos(file, &position) != 0) {
		return CODEC_ERROR_FILE_SEEK;
	}

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Read a block of data from a memory stream (buffer)
*/
CODEC_ERROR GetBlockMemory(STREAM *stream, void *buffer, size_t size, size_t offset)
{
	uint8_t *block = (uint8_t *)stream->location.memory.buffer + offset;
	memcpy(buffer, block, size);
	return CODEC_ERROR_OKAY;
}

/*!
	@brief Write a block of data at the specified offset in the byte stream
*/
CODEC_ERROR PutBlock(STREAM *stream, void *buffer, size_t size, size_t offset)
{
	switch (stream->type)
	{
	case STREAM_TYPE_FILE:
		return PutBlockFile(stream, buffer, size, offset);
		break;

	case STREAM_TYPE_MEMORY:
		return PutBlockMemory(stream, buffer, size, offset);
		break;

	case STREAM_TYPE_UNKNOWN:
		assert(0);
		break;
	}

	return CODEC_ERROR_UNEXPECTED;
}

/*!
	@brief Write a block of data at the specified offset in a file stream
*/
CODEC_ERROR PutBlockFile(STREAM *stream, void *buffer, size_t size, size_t offset)
{
	FILE *file = stream->location.file.iobuf;
	fpos_t position;

	// Save the current position in the file
	if (fgetpos(file, &position) != 0) {
		return CODEC_ERROR_FILE_GET_POSITION;
	}

	// Seek to the specified offset and write to the file
	assert(0 <= offset && offset <= LONG_MAX);
	if (fseek(file, (long)offset, SEEK_SET) != 0) {
		return CODEC_ERROR_FILE_SEEK;
	}

	// Write data to the file
	if (fwrite(buffer, size, 1, file) != 1) {
		return CODEC_ERROR_FILE_WRITE;
	}

	// Return to the previous position in the file
	// if (fseek(file, (long)position, SEEK_SET) != 0) {
	if (fsetpos(file, &position) != 0) {
		return CODEC_ERROR_FILE_SEEK;
	}

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Write a block of data at the specified offset in a memory stream
*/
CODEC_ERROR PutBlockMemory(STREAM *stream, void *buffer, size_t size, size_t offset)
{
	uint8_t *block = (uint8_t *)stream->location.memory.buffer + offset;
	memcpy(block, buffer, size);
	return CODEC_ERROR_OKAY;
}

/*!
	@brief Return true if end of stream
*/
bool EndOfStream(STREAM *stream)
{
	switch (stream->type)
	{
	case STREAM_TYPE_FILE:
		return feof(stream->location.file.iobuf);
		break;

	case STREAM_TYPE_MEMORY:
		return (stream->location.memory.count >= stream->location.memory.size);
		break;

	case STREAM_TYPE_UNKNOWN:
		assert(0);
		break;
	}

	// Assume end of stream if the stream type is unknown
	return true;
}
