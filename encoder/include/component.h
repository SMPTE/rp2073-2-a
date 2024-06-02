/*! @file encoder/include/component.h
 
	Declaration of routines for the inverse component transform and permutation.
 
	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
 */

#ifndef _COMPONENT_H
#define _COMPONENT_H

#ifdef __cplusplus
extern "C" {
#endif
    
    CODEC_ERROR InitComponentTransform(COMPONENT_TRANSFORM *transform);

    CODEC_ERROR InitComponentPermutation(COMPONENT_PERMUTATION *permutation);
    
    CODEC_ERROR AllocateComponentTransform(ALLOCATOR *allocator,
                                           COMPONENT_TRANSFORM *transform,
                                           int component_count);

    CODEC_ERROR AllocateComponentPermutation(ALLOCATOR *allocator,
                                             COMPONENT_PERMUTATION *permutation,
                                             int component_count);

    CODEC_ERROR ReleaseComponentTransform(ALLOCATOR *allocator,
                                          COMPONENT_TRANSFORM *transform);
    
    CODEC_ERROR ReleaseComponentPermutation(ALLOCATOR *allocator,
                                            COMPONENT_PERMUTATION *permutation);
    
    CODEC_ERROR InitComponentTransformIdentity(COMPONENT_TRANSFORM *transform,
                                               int component_count,
                                               ALLOCATOR *allocator);

    CODEC_ERROR InitComponentPermutationIdentity(COMPONENT_PERMUTATION *permutation,
                                                 int component_count,
                                                 ALLOCATOR *allocator);
    
    CODEC_ERROR InitComponentTransformTesting(COMPONENT_TRANSFORM *transform,
                                              int component_count,
                                              ALLOCATOR *allocator);

    CODEC_ERROR InitComponentPermutationTesting(COMPONENT_PERMUTATION *permutation,
                                                int component_count,
                                                ALLOCATOR *allocator);

    bool IsComponentTransformIdentity(COMPONENT_TRANSFORM *transform);

    bool IsComponentPermutationIdentity(COMPONENT_PERMUTATION *permutation);

    CODEC_ERROR WriteComponentTransform(COMPONENT_TRANSFORM *transform, BITSTREAM *stream);

    CODEC_ERROR WriteComponentPermutation(COMPONENT_PERMUTATION *permutation, BITSTREAM *stream);

#ifdef __cplusplus
}
#endif

#endif
