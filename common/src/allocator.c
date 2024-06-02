/*!	@file common/src/allocator.c

	Implementation of a default memory allocator.

	The default memory allocator uses the routine malloc from the
	standard C library, but decoder implementations are free to
	replace the default allocator with an algorithm that implements
	a better memory allocation policy.

	See the discussion in the file documentation for allocator.h
	
	Some memory allocation schemes may choose to ignore a request to
	free memory and subsequent calls to @ref Alloc may reuse blocks
	that were previously allocated for decoding an earlier sample.

	Memory allocation by the decoder follows a simple pattern.  Blocks
	that are allocated during decoder initialization are used during the
	decoding of all samples in the video stream until the decoder is
	reinitialized, while a block that is allocated during decoding is
	always freed by the same routine that allocated the block and a block
	of the same size will be allocated and freed again by the same routine.

	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#include "headers.h"

/*!
	@brief Allocate a block with the specified size
*/
void *Alloc(ALLOCATOR *allocator, size_t size)
{
	(void)allocator;
	return malloc(size);
}

/*!
	@brief Free a block that was allocated by the specified allocator

	It is an error to free a block allocated by one allocator using a
	different allocator.
*/
void Free(ALLOCATOR *allocator, void *block)
{
	(void)allocator;
	if (block != NULL) {
		free(block);
	}
}
