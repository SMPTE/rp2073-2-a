/*!	@file common/source/arguments.c

	Routines for parsing the values of command-line arguments

	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#include <ctype.h>

#include "headers.h"
//#include "getopt.h"
#include "utilities.h"
#include "arguments.h"
//#include "parseargs.h"

/*!
	@brief Convert a command-line argument to an image dimension
*/
bool GetDimension(const char *string, DIMENSION *dimension_out)
{
	int value;
	if (string != NULL && dimension_out != NULL && sscanf(string, "%d", &value) == 1) {
		*dimension_out = (DIMENSION)value;
		return true;
	}
	return false;
}

/*!
	@brief Convert a command-line argument to a pixel format
*/
bool GetPixelFormat(const char *string, PIXEL_FORMAT *format_out)
{
	if (string != NULL && format_out != NULL)
	{
		PIXEL_FORMAT format = PixelFormat(string);
		if (format != PIXEL_FORMAT_UNKNOWN) {
			*format_out = format;
		}
		return true;
	}
	return false;
}

#if VC5_ENABLED_PART(VC5_PART_IMAGE_FORMATS)
/*!
	@brief Convert a command-line argument to an image format
*/
bool GetImageFormat(const char *string, IMAGE_FORMAT *format_out)
{
	int value;
	if (string != NULL && format_out != NULL && sscanf(string, "%d", &value) == 1) {
		*format_out = (IMAGE_FORMAT)value;
		return true;
	}
	return false;
}
#endif

/*!
	@brief Convert a command-line argument to precision in bits
*/
bool GetPrecision(const char *string, PRECISION *precision_out)
{
	int value;
	if (string != NULL && precision_out != NULL && sscanf(string, "%d", &value) == 1) {
		*precision_out = (PRECISION)value;
		return true;
	}
	return false;
}

/*!
	@brief Set the values in the quantization table

	The string contains the quantization values separated by commas.
*/
bool GetQuantization(const char *string, QUANT *quant)
{
	if (string != NULL && quant != NULL)
	{
		int count = sscanf(string,
						   "%d,%d,%d,%d,%d,%d,%d,%d,%d",
						   &quant[1], &quant[2], &quant[3],
						   &quant[4], &quant[5], &quant[6],
						   &quant[7], &quant[8], &quant[9]);
		quant[0] = 1;
		return (count == (MAX_SUBBAND_COUNT - 1));
	}

	return false;
}

/*!
	@brief Set the order in which channels are encoded into the bitstream

	The string contains the channel numbers in the order in which the channnels
	are encoded into the bitstream.  First channel by number is channel zero.

	Note that this facility allows the user to specify that only a subset of the
	component arrays are encoded as channels in the bitstream.
*/
bool GetChannelOrder(const char *string, CHANNEL *channel_order_table, int *channel_order_count, int channel_order_table_length)
{
	if (string != NULL && channel_order_table != NULL && channel_order_count != NULL)
	{
		const char *p = string;
		int i;
		for (i = 0; i < channel_order_table_length; i++)
		{
			char *q = NULL;
			long channel;
			if (!isdigit(*p)) break;
			channel = strtol(p, &q, 10);
			assert(0 <= channel && channel < MAX_CHANNEL_COUNT);
			channel_order_table[i] = (CHANNEL)channel;
			p = (*q != '\0') ? q + 1 : q;
		}

		// Set the count of channel numbers parsed from the niput string
		*channel_order_count = i;

		// Should have parsed all channel numbers in the input string
		assert(*p == '\0');
		return true;
	}

	return false;
}

/*!
	@brief Set the flags that indicate which parts of VC-5 are enabled

	The argument is a list of comma-separated integers for the part numbers
    in the VC-5 standard that are enabled for this invocation of the encoder.

	Part 1 is always enabled.  Part 2 is never enabled (and is ignored).

    Note: Enabling a part at runtime has no effect unless support for that part
    is compiled into the program by enabling the corresponding compile-time switch.
*/
bool GetEnabledParts(const char *string, uint32_t *enabled_parts_out)
{
	if (string != NULL && enabled_parts_out != NULL)
	{
		// The elementary stream is always enabled
		//ENABLED_PARTS enabled_parts = VC5_PART_MASK(VC5_PART_ELEMENTARY);
        ENABLED_PARTS enabled_parts = VC5_PART_MASK_NONE;

		const char *p = string;
		assert(p != NULL);
		while (*p != '\0')
		{
			char *q = NULL;
			long part_number;
			if (!isdigit(*p)) break;
			part_number = strtol(p, &q, 10);

			//TODO: Increase the upper range as more parts are added to this implementation
			//if (VC5_PART_ELEMENTARY <= part_number && part_number <= VC5_PART_SECTIONS)
			if (VC5_PART_ELEMENTARY <= part_number && part_number <= VC5_PART_METADATA)
			{
				if (part_number == VC5_PART_CONFORMANCE) {
					// Conformance does not affect what capabilities are included in the codec
					continue;
				}

				// Set the bit that corresponds to this part number
				enabled_parts |= VC5_PART_MASK(part_number);

				// Advance to the next part number in the command-line argument
				p = (*q != '\0') ? q + 1 : q;
			}
			else
			{
				// Invalid part number
				assert(0);
				return false;
			}
		}

		// Return the bit mask for the enabled parts
		*enabled_parts_out = enabled_parts;

		// Should have parsed all part numbers in the argument string
		assert(*p == '\0');
		return true;
	}

	// Invalid input arguments
	return false;
}

/*!
	@brief Parse the bandfile information

	The bandfile information is a string of fields delimited by commas.  The first field is the
	pathname of the bandfile, the second and third fields are the channel mask and subband mask
	as hexadecimal numbers.  The masks specify which channels and subbands to write to the bandfile.
	
	@todo Add a lowpass mask as an optional fourth argument in the band file information string
	to specify which lowpass bands in the intermediate wavelets are included in the bandfile or
	use extra bits in the subband mask.
*/
bool GetBandfileInfo(const char *string, BANDFILE_INFO *bandfile)
{
	char *p, *q;
	size_t n;

	// Assume that all subbands will be written
	bandfile->channel_mask = UINT32_MAX;
	bandfile->subband_mask = UINT32_MAX;
	
	// Clear the bandfile pathname
	memset(bandfile->pathname, 0, sizeof(bandfile->pathname));

	// Find the end of the bandfile pathname
	p = (char *)string;
	q = strchr(p, ',');

	if (q == NULL)
	{
		// The entire argument is the bandfile pathname
		strcpy(bandfile->pathname, string);
		return true;
	}

	// Copy the bandfile pathname up to but not including the comma
	n = q - p;
	strncpy(bandfile->pathname, string, n);

	// Advance to the channel mask
	p = q + 1;

	// Convert the next substring to the channel mask (hexadecimal)
	bandfile->channel_mask = (uint32_t)strtol(p, &q, 16);
	if (*q != ',') {
		return true;
	}

	// Advance to the subband mask
	p = q + 1;

	// Convert the next substring to the subband mask (hexadecimal)
	bandfile->subband_mask = (uint32_t)strtol(p, &q, 16);

	return true;
}


#if VC5_ENABLED_PART(VC5_PART_LAYERS)

#if 0
/*!
    @brief Copy a command-line argument string into a character array
 
    Copy the nul-terminated command-line argument to the destination array and
    terminate the destination array with a nul character.  Return true if the
    source string fit in destination buffer with room for the terimating nul.
  */
bool GetStringParameter(const char *string, char *parameter, size_t length)
{
    size_t count = strlcpy(parameter, string, length);
    if (count < length) {
        return true;
    }
    return false;
}
#endif

/*!
	@brief Convert a command-line argument to the number of layers
 */
bool GetLayerCount(const char *string, COUNT *layer_count_out)
{
    int value;
    if (string != NULL && layer_count_out != NULL && sscanf(string, "%d", &value) == 1) {
        *layer_count_out = (COUNT)value;
        return true;
    }
    return false;
}
#endif


#if VC5_ENABLED_PART(VC5_PART_SECTIONS)

/*!
	@brief Set the flags that indicate which sections in VC-5 Part 6 are enabled
 
	The argument is a list of comma-separated integers for the section numbers
    in the VC-5 Part 2 conformance specification that are enabled for this invocation
    of the encoder.
 
    Note: Enabling sections at runtime has no effect unless support for sections
    is compiled into the program by enabling the corresponding compile-time switch
    for VC-5 part 6 (sections).
 */
bool GetEnabledSections(const char *string, uint32_t *enabled_sections_out)
{
    if (string != NULL && enabled_sections_out != NULL)
    {
        // No sections are enabled by default
        ENABLED_SECTIONS enabled_sections = 0;
        
        const char *p = string;
        assert(p != NULL);
        while (*p != '\0')
        {
            char *q = NULL;
            long section_number;
            if (!isdigit(*p)) break;
            section_number = strtol(p, &q, 10);
            
            // Is the section number in bounds?
            if (SECTION_NUMBER_MINIMUM <= section_number && section_number <= SECTION_NUMBER_MAXIMUM)
            {
                // Set the bit that corresponds to this section number
                enabled_sections |= SECTION_NUMBER_MASK(section_number);
                
                // Advance to the next section number in the command-line argument
                p = (*q != '\0') ? q + 1 : q;
            }
            else
            {
                // Invalid section number
                assert(0);
                return false;
            }
        }
        
        // Return the bit mask for the enabled sections
        *enabled_sections_out = enabled_sections;
        
        // Should have parsed all section numbers in the argument string
        assert(*p == '\0');
        return true;
    }
    
    // Invalid input arguments
    return false;
}

#endif
