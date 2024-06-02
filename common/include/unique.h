/*! @file common/include/unique.h
 
	Declaration of data structures for the unique image identifier.
 
	(c) 2015 Society of Motion Picture & Television Engineers LLC and GoPro, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
 */

#ifndef _UNIQUE_H
#define _UNIQUE_H

// Basic UMID label using the UUID method

static const uint8_t UMID_label[] = {
    0x06, 0x0A, 0x2B, 0x34, 0x01, 0x01, 0x01, 0x05, 0x01, 0x01, 0x01, 0x20,
};

// Length of the UMID (in bytes)
#define UMID_size 32

// Length of the unique identifier chunk payload (in segments)
#define UMID_length (UMID_size/sizeof(SEGMENT))

// Length of the image sequence number (in bytes)
#define sequence_number_size sizeof(uint32_t)

// Length of the image sequence number (in segments)
#define sequence_number_length (sequence_number_size/sizeof(SEGMENT))

#endif
