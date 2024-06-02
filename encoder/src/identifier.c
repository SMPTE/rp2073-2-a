/*!	@file encoder/src/identifier.c
 
	Implementation of code for writing the unique image identifier.
 
	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
 */

#include "headers.h"

/*!
    @brief Write the unique image identifier
 
    @todo Should the UMID instance number be a parameter to this routine?
*/
CODEC_ERROR WriteUniqueImageIdentifier(ENCODER *encoder, BITSTREAM *stream)
{
    const int UMID_length_byte = 0x13;
    const int UMID_instance_number = 0;
    
    // Total length of the unique image identifier chunk payload (in segments)
    const int identifier_chunk_payload_length = UMID_length + sequence_number_length;
    
    // Write the tag value pair for the small chunk element for the unique image identifier
    PutTagPairOptional(stream, CODEC_TAG_UniqueImageIdentifier, identifier_chunk_payload_length);
    
    // Write the UMID label
    PutByteArray(stream, UMID_label, sizeof(UMID_label));
    
    // Write the UMID length byte
    PutBits(stream, UMID_length_byte, 8);
    
    // Write the UMID instance number
    PutBits(stream, UMID_instance_number, 24);
    
    // Write the image sequence identifier
    PutByteArray(stream, encoder->image_sequence_identifier, sizeof(encoder->image_sequence_identifier));
    
    // Write the image sequence number
    PutLong(stream, encoder->image_sequence_number);
    
    return CODEC_ERROR_OKAY;
}

/*!
    @brief Initialize the unique image identifier with know values for testing
*/
CODEC_ERROR SetUniqueImageIdentifierTesting(ENCODER *encoder)
{
    int i;
    
    for (i = 0; i < sizeof(encoder->image_sequence_identifier); i++)
    {
        encoder->image_sequence_identifier[i] = (0x10 + i);
    }
    encoder->image_sequence_number = 0x0A0B0C0D;
    
    return CODEC_ERROR_OKAY;
}
