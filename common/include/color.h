/*!	@file common/include/color.h

	Declaration of the data structures and constants used to designate color spaces.

	@todo The color format should be a struct that includes the pixel format
	and the color space.  The pixel format only specifies the packing of the
	color components into each pixel, including the chroma sampling scheme.

	The color format may include more detailed information on the sampling
	such as the location of the chroma values relative to the luma values.

	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _COLOR_H
#define _COLOR_H

/*!
	@brief Definition of flags for the characteristics of the color space
*/
typedef enum _color_space
{
	COLOR_SPACE_UNKNOWN = 0,	//!< The color space is unknown
	COLOR_SPACE_BT_601 = 1,		//!< BT 601 color space for a YUV source
	COLOR_SPACE_BT_709 = 2,		//!< BT 709 color space for a YUV source
	COLOR_SPACE_VS_RGB = 4,		//!< RGB that ranges normally from 16 to 235 just like luma
	//COLOR_SPACE_422_TO_444 = 8,
	//COLOR_SPACE_8_PIXEL_PLANAR = 16,

	// Four common combinations of the color space flags
	COLOR_SPACE_VS_709 = (COLOR_SPACE_BT_709 | COLOR_SPACE_VS_RGB),		//!< Video safe 709
	COLOR_SPACE_VS_601 = (COLOR_SPACE_BT_601 | COLOR_SPACE_VS_RGB),		//!< Video safe 601
	COLOR_SPACE_CG_709 = (COLOR_SPACE_BT_709),							//!< Full range 709
	COLOR_SPACE_CG_601 = (COLOR_SPACE_BT_601),							//!< Full range 601

} COLOR_SPACE;

#endif
