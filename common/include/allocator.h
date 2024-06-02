/*!	@file common/include/allocator.h

	The allocator hides the actual routines and data structures that are
	used for memory allocation so that the decoder can be easily adapted
	to use different memory allocation schemes.
	
	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _ALLOCATOR_H
#define _ALLOCATOR_H

/*!
	@brief Opaque data type for the default memory allocator

	It is expected that actual decoder implementations will define more
	sophisticated memory allocators.
*/
typedef void *ALLOCATOR;

#ifdef __cplusplus
extern "C" {
#endif

void *Alloc(ALLOCATOR *allocator, size_t size);
void Free(ALLOCATOR *allocator, void *block);

#ifdef __cplusplus
}
#endif

#endif
