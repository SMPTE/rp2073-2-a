/*! @file common/include/transperm.h
 
	Declaration of the inverse component transform and permutation.
 
	(c) 2015 Society of Motion Picture & Television Engineers LLC and GoPro, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
 */

#ifndef _TRANSPERM_H
#define _TRANSPERM_H

/*!
 @brief Data structure for the component transform (16 bit precision)
 */
typedef struct _component_transform
{
    uint16_t component_count;
    uint16_t *transform_matrix;
    uint16_t *transform_offset;
    uint16_t *transform_scale;
    
} COMPONENT_TRANSFORM;

/*!
 @brief Data structure for the component permutation
 */
typedef struct _component_permutation
{
    uint16_t component_count;
    uint16_t *permutation_array;
    
} COMPONENT_PERMUTATION;

#endif
