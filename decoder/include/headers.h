/*! @file decoder/include/headers.h

	This file includes all of the header files that are used by the decoder.

	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _HEADERS_H
#define _HEADERS_H

/*! @file headers.h

	This header file includes all of the headers and declarations that
	are used by the reference decoder.  Note that some header files are
	only used by the main program that calls the codec or are only used
	for debugging are not included by this file.  Only headers that are
	part of the reference decoder are included by this file.

	Including a single header file in all reference decoder source files
	ensures that all modules see the same header files in the same order.

	This file can be used for creating a pre-compiled header if the
	compiler supports that capabilities.
*/

#include "platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <memory.h>
#include <string.h>

#include "types.h"
#include "config.h"
#include "macros.h"
#include "error.h"
#include "allocator.h"
#include "filelist.h"
#include "color.h"
#include "pixel.h"
#include "image.h"
#include "dpxfile.h"
#include "bayer.h"
#include "convert.h"
#include "wavelet.h"
#include "stream.h"
#include "bitstream.h"
#include "syntax.h"
#include "swap.h"
#include "inverse.h"
#include "companding.h"
#include "dequantize.h"
//#include "metadata.h"
#include "codec.h"
#include "parameters.h"
#include "interlaced.h"
#include "vlc.h"
#include "codeset.h"
#include "transperm.h"

#if VC5_ENABLED_PART(VC5_PART_METADATA)

// Include the header file for the Mini-XML library
#include "mxml.h"

#include "params.h"
#include "base64.h"
#include "buffer.h"
#include "metadata.h"

// Include the header file for the metadata database
#include "database.h"

#else

//! Define an opaque pointer if the metadata database code is not enabled
typedef struct _database DATABASE;

#endif

#include "decoder.h"

#include "component.h"
#include "utilities.h"
#include "unique.h"
#include "identifier.h"
#include "dump.h"

#endif
