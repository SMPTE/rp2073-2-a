/*!	@file decoder/src/identifier.c
 
	Implementation of code for writing the unique image identifier.
 
	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
 */

#include "headers.h"

/*!
    @brief Parse the unique image identifier in a small chunk payload

    @todo Should the UMID instance number be a parameter to this routine?
 */
CODEC_ERROR ParseUniqueImageIdentifier(DECODER *decoder, BITSTREAM *stream, size_t identifier_length)
{
    const int UMID_length_byte = 0x13;
    const int UMID_instance_number = 0;
    
    // Total length of the unique image identifier chunk payload (in segments)
    const int identifier_chunk_payload_length = UMID_length + sequence_number_length;
    
    uint8_t byte_array[12];
    BITWORD length_byte;
    BITWORD instance_number;
    
    // Check that the chunk payload has the correct length (in segments)
    if (identifier_length != identifier_chunk_payload_length) {
        return CODEC_ERROR_SYNTAX_ERROR;
    }
    
    // The unique image identifier chunk should begin with a UMID label
    GetByteArray(stream, byte_array, sizeof(byte_array));
    if (memcmp(byte_array, UMID_label, sizeof(UMID_label)) != 0) {
        return CODEC_ERROR_UMID_LABEL;
    }
    
    // Check the UMID length byte
    length_byte = GetBits(stream, 8);
    if (length_byte != UMID_length_byte) {
        return CODEC_ERROR_SYNTAX_ERROR;
    }
    
    // Check the UMID instance number
    instance_number = GetBits(stream, 24);
    if (instance_number != UMID_instance_number) {
        return CODEC_ERROR_SYNTAX_ERROR;
    }
    
    // Read the image sequence identifier
    GetByteArray(stream, decoder->image_sequence_identifier, sizeof(decoder->image_sequence_identifier));
    
    // Read the image sequence number
    decoder->image_sequence_number = GetBits(stream, 32);
    
    return CODEC_ERROR_OKAY;
}
