/*! @file common/include/config.h

    Parameters that control the configuration of the codec.

    Note: Only VC-5 Part 1 is supported by this implementation.

    (c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
    All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _CONFIG_H
#define _CONFIG_H

// Version number of both the encoder and decoder (major.minor.revision)
#define CODEC_VERSION_NUMBER 4.4
#define CODEC_VERSION_SUFFIX "release"


/*** Define the parts of the VC-5 standards suite supported by this version of the codec ***/

// Definitions of VC-5 parts
#define VC5_PART_NONE           0       //!< No parts are enabled
#define VC5_PART_ELEMENTARY     1       //!< Support for the elementary bitstream is enabled
#define VC5_PART_CONFORMANCE    2       //!< Enabling conformance testing has no effect on the codec
#define VC5_PART_IMAGE_FORMATS  3       //!< Enable support for common image formats
#define VC5_PART_COLOR_SAMPLING 4       //!< Enable support for subsampled color components
#define VC5_PART_LAYERS         5       //!< Enable support for layers
#define VC5_PART_SECTIONS       6       //!< Enable support for sections
#define VC5_PART_METADATA       7       //!< Metadata is currently not supported

//! Convert a part number into a part mask
#define VC5_PART_MASK(n)        (1 << ((n)-1))

//! Part number mask that represents no parts enabled
#define VC5_PART_MASK_NONE      0

//! Define the limits on the VC-5 part numbers (revise when new parts are added)
#define VC5_PART_NUMBER_MIN		1
#define VC5_PART_NUMBER_MAX		7


/*** Build the codec with all parts enabled at compile-time by default ***/

#ifndef _PARTS

//! Define the parts supported by this codec implementation
#define VC5_ENABLED_PARTS       (VC5_PART_MASK(VC5_PART_ELEMENTARY)     | \
                                 VC5_PART_MASK(VC5_PART_IMAGE_FORMATS)  | \
                                 VC5_PART_MASK(VC5_PART_COLOR_SAMPLING) | \
                                 VC5_PART_MASK(VC5_PART_LAYERS)         | \
                                 VC5_PART_MASK(VC5_PART_SECTIONS)       | \
                                 VC5_PART_MASK(VC5_PART_METADATA))
#else

//! Define the parts enabled by this code implementation using a command-line parameter
#define VC5_ENABLED_PARTS (_PARTS)

#endif


//! Compile code if any of the parts in the specified mask are supported
#define VC5_ENABLED_MASK(m)     ((VC5_ENABLED_PARTS & (m)) != 0)

//! Macro for testing presence of support for a specific part of the VC-5 standard
#define VC5_ENABLED_PART(n)     (VC5_ENABLED_MASK(VC5_PART_MASK(n)))


/*** Set the compile-time parameters that determine the maximum size of key data structures ***/

//! Maximum number of color channels
#define MAX_CHANNEL_COUNT   4

//! Maximum number of wavelets per channel
#define MAX_WAVELET_COUNT   3

//! Maximum number of bands per wavelet
#define MAX_BAND_COUNT      4

//! Maximum number of subbands in all wavelets (including the lowpass band in the first wavelet)
#define MAX_SUBBAND_COUNT   10

//! The number of prescale values that are encoded into the bitstream
#define MAX_PRESCALE_COUNT  8

//! Default internal precision of the intermediate results after unpacking
static const int default_internal_precision = 12;

//TODO: Change the global variable from internal_precision to encoded_precision?


#if VC5_ENABLED_PART(VC5_PART_LAYERS)
//! Maximum number of layers supported by the reference codec
#define MAX_LAYER_COUNT     10
#endif

#if VC5_ENABLED_PART(VC5_PART_SECTIONS)
//! Maximum number of image sections
#define MAX_IMAGE_SECTIONS  10
#endif

#if VC5_ENABLED_PART(VC5_PART_METADATA)
// Set the compile-time switches for building the metadata database

#if _DECODER

// Include the code for the metadata database in the decoder
#ifndef _DATABASE
#define _DATABASE	1
#endif

#endif

#endif

#if defined(_DEBUG) && !defined(DEBUG)
#define DEBUG _DEBUG
#endif

#endif
