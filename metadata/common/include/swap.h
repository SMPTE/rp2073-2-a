/*! @file common/include/swap.h

	Platform-independent byte swapping routines.

	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _SWAP_H
#define _SWAP_H

inline static uint16_t Swap16(uint16_t word)
{
	uint8_t buffer[2];
	buffer[0] = (uint8_t)((word >>  8) & 0xFF);
	buffer[1] = (uint8_t)(word & 0xFF);

	return (buffer[1] << 8) | buffer[0];
}

inline static uint32_t Swap32(uint32_t word)
{
	uint16_t buffer[2];
	buffer[0] = (uint16_t)((word >> 16) & 0xFFFF);
	buffer[1] = (uint16_t)(word & 0xFFFF);

	return ((uint32_t)Swap16(buffer[1]) << 16) | Swap16(buffer[0]);
}

inline static uint64_t Swap64(uint64_t word)
{
	uint32_t buffer[2];
	buffer[0] = (uint32_t)((word >> 32) & 0xFFFFFFFF);
	buffer[1] = (uint32_t)(word & 0xFFFFFFFF);

	return ((uint64_t)Swap32(buffer[1]) << 32) | Swap32(buffer[0]);
}

inline static float SwapFloat32(float number)
{
	// Define unions to access floating-point numbers as a string of bytes
	union _float_bytes {
		float number;
		uint8_t byte[sizeof(float)];
	} word, swapped;

	word.number = number;

	//fprintf(stderr, "Float: %02X %02X %02X %02X\n", word.byte[0], word.byte[1], word.byte[2], word.byte[3]);

	for (int i = 0; i < sizeof(float); i++)
	{
		swapped.byte[i] = word.byte[sizeof(float) - i - 1];
	}

	//fprintf(stderr, "Swapped: %02X %02X %02X %02X\n", swapped.byte[0], swapped.byte[1], swapped.byte[2], swapped.byte[3]);

	return swapped.number;
}

inline static double SwapFloat64(double number)
{
	// Define unions to access floating-point numbers as a string of bytes
	union _double_bytes {
		double number;
		uint8_t byte[sizeof(double)];
	} word, swapped;

	word.number = number;

	//fprintf(stderr, "Float: %02X %02X %02X %02X\n", word.byte[0], word.byte[1], word.byte[2], word.byte[3]);

	for (int i = 0; i < sizeof(double); i++)
	{
		swapped.byte[i] = word.byte[sizeof(double) - i - 1];
	}

	//fprintf(stderr, "Swapped: %02X %02X %02X %02X\n", swapped.byte[0], swapped.byte[1], swapped.byte[2], swapped.byte[3]);

	return swapped.number;
}

#endif
