/*!	@file buffer.h

	Definitions for buffer allocation macros that hide compiler dependencies
*/

#ifndef _BUFFER_H
#define _BUFFER_H

#ifdef _MSC_VER

// Require use of alloca for variable-length arrays (Windows only)
#include <malloc.h>

// The Windows compiler does not support variable-length arrays
#define ALLOC_BUFFER(_array, _length) uint8_t * _array = alloca(_length)

#define ALLOC_STRING(_string, _length) char * _string = alloca(_length)

#else

// Can use variable-length arrays on other platforms
#define ALLOC_BUFFER(_array, _Length) uint8_t _array[_Length]

#define ALLOC_STRING(_string, _Length) char _string[_Length]

#endif

#endif
