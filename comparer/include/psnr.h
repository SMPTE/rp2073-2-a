/*!	@file comparer/include/psnr.h

	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _PSNR_H
#define _PSNR_H


void ComputePSNR_DPX0(int width,
					  int height,
					  void *srca,		// Source A bitmap
					  void *srcb,		// Source B bitmap
					  int stats);

void ComputePSNR_RG48(int width,
					  int height,
					  void *srca,		// Source A bitmap
					  void *srcb,		// Source B bitmap
					  int stats);

void ComputePSNR_BYR3(int width,
					  int height,
					  void *srca,		// Source A bitmap
					  void *srcb,		// Source B bitmap
					  int stats);

void ComputePSNR_BYR4(int width,
					  int height,
					  void *srca,		// Source A bitmap
					  void *srcb,		// Source B bitmap
					  int stats);
#endif
