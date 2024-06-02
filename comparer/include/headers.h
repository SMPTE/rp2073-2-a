/*! @file encoder/include/headers.h

	This header file includes all of the headers and declarations that
	are used by the reference encoder.  Note that some header files are
	only used by the main program that calls the codec or are only used
	for debugging are not included by this file.  Only headers that are
	part of the reference encoder are included by this file.

	Including a single header file in all reference encoder source files
	ensures that all modules see the same header files in the same order.

	This file can be used for creating a pre-compiled header if the
	compiler supports that capabilities.

	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _HEADERS_H
#define _HEADERS_H

// Work around problem on Macintosh
#define	_LANGINFO_H_


#include "platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>

//#include "version.h"
#include "profile.h"
#include "config.h"
#include "timer.h"
#include "types.h"
#include "macros.h"
#include "error.h"
#include "allocator.h"
#include "pixel.h"
#include "color.h"
#include "unpack.h"
#include "image.h"
#include "bayer.h"
#include "dpxfile.h"
#include "convert.h"
#include "wavelet.h"
//#include "bitstream.h"

//HACK: Work around build problems
typedef struct _bitstream BITSTREAM;
typedef uint_fast8_t BITCOUNT;

#include "syntax.h"
#include "stream.h"
#include "swap.h"
//#include "forward.h"
//#include "companding.h"
//#include "quantize.h"
#include "codec.h"

// #if VC5_ENABLED_PART(VC5_PART_SECTIONS)
// #include "sections.h"
// #endif

#include "parameters.h"
#include "interlaced.h"
#include "vlc.h"
//#include "codeset.h"
//#include "codebooks.h"
//#include "bandfile.h"
//#include "metadata.h"
//#include "transperm.h"
//#include "component.h"

// #if VC5_ENABLED_PART(VC5_PART_LAYERS)
// #include "layers.h"
// #endif

//#include "encoder.h"
#include "utilities.h"
//#include "unique.h"
//#include "identifier.h"
#include "dump.h"

#endif
