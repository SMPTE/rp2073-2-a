/*!	@file comparer/src/psnr.c

	(c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "swap.h"
#include "psnr.h"

#define sqr(x) ((x) * (x))


void ComputePSNR_DPX0(int width, int height, void *srca, void *srcb, int stats)     
{
	double diff;
	double ymse = 0.0;
	double rmse = 0.0;
	double gmse = 0.0;
	double bmse = 0.0;
	double mse = 0.0;
	double y_psnr, r_psnr, g_psnr, b_psnr;
	double y_rmse, r_rmse, g_rmse, b_rmse;
	//double offr=0,offg=0,offb=0;
	double r_offset = 0.0;
	double g_offset = 0.0;
	double b_offset = 0.0;
	double y_total = 0.0;
	double r_total = 0.0;
	double g_total = 0.0;
	double b_total = 0.0;
	double total = 0.0;
	uint32_t *ptra, *ptrb;
	int y, x, count = 0;
	int lowcount = 0;
	int highcount = 0;

	//int val,average1,average2;

	int once = 0;
	long curve[1024];
	long curvecount[1024];

	const int x_border = 0;
	const int y_border = 0;

	const double MAXVALUE = 1024.0;

	double g_y_psnr = 0.0, g_r_psnr = 0.0, g_g_psnr = 0.0, g_b_psnr = 0.0;
	double g_ytotal = 0.0; 
	double g_rtotal = 0.0; 
	double g_gtotal = 0.0; 
	double g_btotal = 0.0;

	int r_shift = 22;
	int g_shift = 12;
	int b_shift = 2;

	int rgb_mask = 0x3FF;

//oncemore:

	ymse = 0.0,rmse = 0.0,gmse = 0.0,bmse = 0.0,mse = 0.0;
	y_total = 0.0;
	total = 0.0;
	r_total = 0.0;
	g_total = 0.0;
	b_total = 0.0;
	count = 0;
	lowcount=0, highcount=0;


	for(y=0;y<1024;y++)
	{
		curve[y] = 0;
		curvecount[y] = 0;
	}

	for (y = y_border; y < height - y_border; y++)
	{
		ptra = (unsigned int *)srca;
		ptra += y * width;
		ptrb = (unsigned int *)srcb;
		ptrb += y * width;

		for (x = x_border; x < width - x_border; x++)
		{
			uint32_t dpxvala;
			uint32_t dpxvalb;
			double r1,g1,b1,y1;
			double r2,g2,b2,y2;
			int r3,g3,b3;
			int ir1,ig1,ib1;
			int ir2,ig2,ib2;

			dpxvala = Swap32(*ptra);
			dpxvalb = Swap32(*ptrb);

			ir1 = ((dpxvala >> r_shift) & rgb_mask);
			ig1 = ((dpxvala >> g_shift) & rgb_mask);
			ib1 = ((dpxvala >> b_shift) & rgb_mask);
		
			r1 = (double)ir1;
			g1 = (double)ig1;
			b1 = (double)ib1;

			ir2 = (int)(((dpxvalb >> r_shift) & rgb_mask) + r_offset);
			ig2 = (int)(((dpxvalb >> g_shift) & rgb_mask) + g_offset);
			ib2 = (int)(((dpxvalb >> b_shift) & rgb_mask) + b_offset);
		
			r2 = (double)ir2;
			g2 = (double)ig2;
			b2 = (double)ib2;

			curve[ir1] += ir2; curvecount[ir1]++;
			curve[ig1] += ig2; curvecount[ig1]++;
			curve[ib1] += ib2; curvecount[ib1]++;

			diff = r1 - r2;
			r3  = (int)fabs(diff);
			r_total += diff;
			rmse += (diff * diff);

			diff = -(((double)(g1)) - ((double)(g2)));
			g3  = (int)fabs(diff);
			g_total += diff;
			gmse += (diff * diff);

			diff = -(((double)(b1)) - ((double)(b2)));
			b3  = (int)fabs(diff);
			b_total += diff;
			bmse += (diff * diff);
			
			y1 = (r1*213 + g1*715 + b1*72);
			y2 = (r2*213 + g2*715 + b2*72);			
			diff = -(((double)(y1)) - ((double)(y2)));
			diff /= 1000.0;
			y_total += diff;			
			ymse += (diff * diff);
			
			count++;

			if(!stats)
			{
				dpxvala = r3<<22 | g3<<12 | b3<<2;
				*ptra++ = Swap32(dpxvala);
			}
			else
			{
				ptra++;
			}
			ptrb++;
		}
	}

	r_total /= (double)(count);
	g_total /= (double)(count);
	b_total /= (double)(count);
	y_total /= (double)(count);

	rmse /=  (double)(count);
	gmse /=  (double)(count);
	bmse /=  (double)(count);
	ymse /=  (double)(count);

	y_rmse = sqrt(ymse);
	r_rmse = sqrt(rmse);
	g_rmse = sqrt(gmse);
	b_rmse = sqrt(bmse);

	y_psnr = 20.0*log10(MAXVALUE/y_rmse);
	r_psnr = 20.0*log10(MAXVALUE/r_rmse);
	g_psnr = 20.0*log10(MAXVALUE/g_rmse);
	b_psnr = 20.0*log10(MAXVALUE/b_rmse);

	
	for(y=0;y<1024;y++)
	{
		if(curvecount[y])
			curve[y] /= curvecount[y];
	}

	if(stats)
	{		
		printf("%2.2f,%2.2f,%2.2f,%2.2f\n", y_psnr, r_psnr, g_psnr, b_psnr);
		//printf("%2.2f,%2.2f,%2.2f,%2.2f\n", ytotal, rtotal, gtotal, btotal); //offset

		r_offset = r_total;
		g_offset = g_total;
		b_offset = b_total;
#if 0
		if(once == 0)
		{
			once = 1;
			goto oncemore;
		}
#endif
	}
	else
	{
		printf("%2.2f,%2.2f,%2.2f,%2.2f\n", y_psnr, r_psnr, g_psnr, b_psnr);
	}

	g_y_psnr += y_psnr;
	g_r_psnr += r_psnr;
	g_g_psnr += g_psnr;
	g_b_psnr += b_psnr;

	g_ytotal += y_total; 
	g_rtotal += r_total; 
	g_gtotal += g_total;
	g_btotal += b_total; 
}


void ComputePSNR_RG48(int width, int height, void *srca, void *srcb, int stats)     
{
	double diff;
	double ymse = 0.0;
	double rmse = 0.0;
	double gmse = 0.0;
	double bmse = 0.0;
	double mse = 0.0;
	double y_psnr, r_psnr, g_psnr, b_psnr;
	double y_rmse, r_rmse, g_rmse, b_rmse;
	//double offr=0,offg=0,offb=0;
	double r_offset = 0.0;
	double g_offset = 0.0;
	double b_offset = 0.0;
	double y_total = 0.0;
	double r_total = 0.0;
	double g_total = 0.0;
	double b_total = 0.0;
	double total = 0.0;
	//uint32_t *ptra, *ptrb;
	uint16_t *ptra, *ptrb;
	int y, x, count = 0;
	int lowcount = 0;
	int highcount = 0;

	//int val,average1,average2;

	int once = 0;
	static long curve[1 << 16];
	static long curvecount[1 << 16];

	const int x_border = 0;
	const int y_border = 0;

	const double MAXVALUE = UINT16_MAX;

	double g_y_psnr = 0.0, g_r_psnr = 0.0, g_g_psnr = 0.0, g_b_psnr = 0.0;
	double g_ytotal = 0.0; 
	double g_rtotal = 0.0; 
	double g_gtotal = 0.0; 
	double g_btotal = 0.0;

	int r_shift = 22;
	int g_shift = 12;
	int b_shift = 2;

	int rgb_mask = 0x3FF;

//oncemore:

	ymse = 0.0,rmse = 0.0,gmse = 0.0,bmse = 0.0,mse = 0.0;
	y_total = 0.0;
	total = 0.0;
	r_total = 0.0;
	g_total = 0.0;
	b_total = 0.0;
	count = 0;
	lowcount=0, highcount=0;


	for(y=0;y<1024;y++)
	{
		curve[y] = 0;
		curvecount[y] = 0;
	}

	for (y = y_border; y < height - y_border; y++)
	{
		ptra = (uint16_t *)srca;
		ptra += y * width * 3;

		ptrb = (uint16_t *)srcb;
		ptrb += y * width * 3;

		for (x = x_border; x < width - x_border; x++)
		{
			//uint32_t dpxvala;
			//uint32_t dpxvalb;
			double r1,g1,b1,y1;
			double r2,g2,b2,y2;
			int r3,g3,b3;
			int ir1,ig1,ib1;
			int ir2,ig2,ib2;

			//dpxvala = Swap32(*ptra);
			//dpxvalb = Swap32(*ptrb);
#if 0
			ir1 = ((dpxvala >> r_shift) & rgb_mask);
			ig1 = ((dpxvala >> g_shift) & rgb_mask);
			ib1 = ((dpxvala >> b_shift) & rgb_mask);
#else
			ir1 = ptra[3 * x + 0];
			ig1 = ptra[3 * x + 1];
			ib1 = ptra[3 * x + 2];
#endif
			r1 = (double)ir1;
			g1 = (double)ig1;
			b1 = (double)ib1;
#if 0
			ir2 = (int)(((dpxvalb >> r_shift) & rgb_mask) + r_offset);
			ig2 = (int)(((dpxvalb >> g_shift) & rgb_mask) + g_offset);
			ib2 = (int)(((dpxvalb >> b_shift) & rgb_mask) + b_offset);
#else
			ir2 = ptrb[3 * x + 0] + (int)r_offset;
			ig2 = ptrb[3 * x + 1] + (int)g_offset;
			ib2 = ptrb[3 * x + 2] + (int)b_offset;
#endif
			r2 = (double)ir2;
			g2 = (double)ig2;
			b2 = (double)ib2;

			curve[ir1] += ir2; curvecount[ir1]++;
			curve[ig1] += ig2; curvecount[ig1]++;
			curve[ib1] += ib2; curvecount[ib1]++;

			diff = r1 - r2;
			r3  = (int)fabs(diff);
			r_total += diff;
			rmse += (diff * diff);

			diff = -(((double)(g1)) - ((double)(g2)));
			g3  = (int)fabs(diff);
			g_total += diff;
			gmse += (diff * diff);

			diff = -(((double)(b1)) - ((double)(b2)));
			b3  = (int)fabs(diff);
			b_total += diff;
			bmse += (diff * diff);
			
			y1 = (r1*213 + g1*715 + b1*72);
			y2 = (r2*213 + g2*715 + b2*72);			
			diff = -(((double)(y1)) - ((double)(y2)));
			diff /= 1000.0;
			y_total += diff;			
			ymse += (diff * diff);
			
			count++;
#if 0
			if(!stats)
			{
				dpxvala = r3<<22 | g3<<12 | b3<<2;
				*ptra++ = Swap32(dpxvala);
			}
			else
			{
				ptra++;
			}
			ptrb++;
#endif
		}
	}

	r_total /= (double)(count);
	g_total /= (double)(count);
	b_total /= (double)(count);
	y_total /= (double)(count);

	rmse /=  (double)(count);
	gmse /=  (double)(count);
	bmse /=  (double)(count);
	ymse /=  (double)(count);

	y_rmse = sqrt(ymse);
	r_rmse = sqrt(rmse);
	g_rmse = sqrt(gmse);
	b_rmse = sqrt(bmse);

	y_psnr = 20.0*log10(MAXVALUE/y_rmse);
	r_psnr = 20.0*log10(MAXVALUE/r_rmse);
	g_psnr = 20.0*log10(MAXVALUE/g_rmse);
	b_psnr = 20.0*log10(MAXVALUE/b_rmse);

	for(y=0;y<1024;y++)
	{
		if(curvecount[y])
			curve[y] /= curvecount[y];
	}

	if(stats)
	{		
		printf("%2.2f,%2.2f,%2.2f,%2.2f\n", y_psnr, r_psnr, g_psnr, b_psnr);
		//printf("%2.2f,%2.2f,%2.2f,%2.2f\n", ytotal, rtotal, gtotal, btotal); //offset

		r_offset = r_total;
		g_offset = g_total;
		b_offset = b_total;
#if 0
		if(once == 0)
		{
			once = 1;
			goto oncemore;
		}
#endif
	}
	else
	{
		printf("%2.2f,%2.2f,%2.2f,%2.2f\n", y_psnr, r_psnr, g_psnr, b_psnr);
	}

	g_y_psnr += y_psnr;
	g_r_psnr += r_psnr;
	g_g_psnr += g_psnr;
	g_b_psnr += b_psnr;

	g_ytotal += y_total; 
	g_rtotal += r_total; 
	g_gtotal += g_total;
	g_btotal += b_total; 
}


void ComputePSNR_BYR3(int width, int height, void *srca, void *srcb, int stats)     
{
#if 0
	double r_offset = 0.0;
	double g_offset = 0.0;
	double b_offset = 0.0;
	double y_total = 0.0;
	double r_total = 0.0;
	double g_total = 0.0;
	double b_total = 0.0;
	double total = 0.0;
#endif
	uint16_t *ptra_r1, *ptra_g1, *ptra_g2, *ptra_b1;
	uint16_t *ptrb_r1, *ptrb_g1, *ptrb_g2, *ptrb_b1;

	int x, y;
	//int lowcount = 0;
	//int highcount = 0;

	//int val,average1,average2;

	//int once = 0;
	//long curve[1024];
	//long curvecount[1024];

	const int x_border = 0;
	const int y_border = 0;

	const double maximum_value = 1023;

#if 0
	double g_y_psnr = 0.0, g_r_psnr = 0.0, g_g_psnr = 0.0, g_b_psnr = 0.0;
	double g_ytotal = 0.0; 
	double g_rtotal = 0.0; 
	double g_gtotal = 0.0; 
	double g_btotal = 0.0;
#endif

	double r1_total = 0;
	double g1_total = 0;
	double g2_total = 0;
	double b1_total = 0;

	double r1_rmse = 0.0;
	double g1_rmse = 0.0;
	double g2_rmse = 0.0;
	double b1_rmse = 0.0;

	double r1_psnr;
	double g1_psnr;
	double g2_psnr;
	double b1_psnr;

	int count = 0;

#if 0
	for(y=0;y<1024;y++)
	{
		curve[y] = 0;
		curvecount[y] = 0;
	}
#endif

	for (y = y_border; y < height - y_border; y++)
	{
		ptra_r1 = (uint16_t *)srca;
		ptra_r1 += y * width * 4;
		ptra_g1 = ptra_r1 + width;
		ptra_g2 = ptra_g1 + width;
		ptra_b1 = ptra_g2 + width;

		ptrb_r1 = (uint16_t *)srcb;
		ptrb_r1 += y * width * 4;
		ptrb_g1 = ptrb_r1 + width;
		ptrb_g2 = ptrb_g1 + width;
		ptrb_b1 = ptrb_g2 + width;

		for (x = x_border; x < width - x_border; x++)
		{
			uint16_t srca_r1;
			uint16_t srca_g1;
			uint16_t srca_g2;
			uint16_t srca_b1;

			uint16_t srcb_r1;
			uint16_t srcb_g1;
			uint16_t srcb_g2;
			uint16_t srcb_b1;

			double r1a, r1b;
			double g1a, g1b;
			double g2a, g2b;
			double b1a, b1b;

			double r1_delta;
			double g1_delta;
			double g2_delta;
			double b1_delta;

			srca_r1 = ptra_r1[x];
			srca_g1 = ptra_g1[x];
			srca_g2 = ptra_g2[x];
			srca_b1 = ptra_b1[x];

			srcb_r1 = ptrb_r1[x];
			srcb_g1 = ptrb_g1[x];
			srcb_g2 = ptrb_g2[x];
			srcb_b1 = ptrb_b1[x];
#if 0
			curve[srca_r1] += srcb_r1 + r1_offset;	curvecount[srca_r1]++;
			curve[srca_g1] += srcb_g1 + g1_offset;	curvecount[srca_g1]++;
			curve[srca_g2] += srcb_g2 + g2_offset;	curvecount[srca_g2]++;
			curve[srca_b1] += srcb_b1 + b1_offset;	curvecount[srca_b1]++;
#endif
			r1a = srca_r1;
			g1a = srca_g1;
			g2a = srca_g2;
			b1a = srca_b1;

			r1b = srcb_r1;
			g1b = srcb_g1;
			g2b = srcb_g2;
			b1b = srcb_b1;

			r1_delta = r1a - r1b;
			r1_total += r1_delta;
			r1_rmse += sqr(r1_delta);

			g1_delta = g1a - g1b;
			g1_total += g1_delta;
			g1_rmse += sqr(g1_delta);

			g2_delta = g2a - g2b;
			g2_total += g2_delta;
			g2_rmse += sqr(g2_delta);

			b1_delta = b1a - b1b;
			b1_total += b1_delta;
			b1_rmse += sqr(b1_delta);
#if 0			
			y1 = (r1*213 + g1*715 + b1*72);
			y2 = (r2*213 + g2*715 + b2*72);			
			diff = -(((double)(y1)) - ((double)(y2)));
			diff /= 1000.0;
			y_total += diff;			
			ymse += (diff * diff);
#endif		
			count++;
#if 0
			if(!stats)
			{
				dpxvala = r3<<22 | g3<<12 | b3<<2;
				*ptra++ = Swap32(dpxvala);
			}
			else
			{
				ptra++;
			}
			ptrb++;
#else

#endif
		}
	}

	r1_total /= count;
	g1_total /= count;
	g2_total /= count;
	b1_total /= count;
	//y_total /= (double)(count);

	r1_rmse /= count;
	g1_rmse /= count;
	g2_rmse /= count;
	b1_rmse /= count;

	//y_rmse = sqrt(ymse);
	r1_rmse = sqrt(r1_rmse);
	g1_rmse = sqrt(g1_rmse);
	g2_rmse = sqrt(g2_rmse);
	b1_rmse = sqrt(b1_rmse);

	//y_psnr = 20.0*log10(MAXVALUE/y_rmse);
	r1_psnr = (r1_rmse > 0.0) ? 20.0 * log10(maximum_value / r1_rmse) : 0.0;
	g1_psnr = (g1_rmse > 0.0) ? 20.0 * log10(maximum_value / g1_rmse) : 0.0;
	g2_psnr = (g2_rmse > 0.0) ? 20.0 * log10(maximum_value / g2_rmse) : 0.0;
	b1_psnr = (b1_rmse > 0.0) ? 20.0 * log10(maximum_value / b1_rmse) : 0.0;

#if 0
	for(y=0;y<1024;y++)
	{
		if(curvecount[y]) {
			curve[y] /= curvecount[y];
		}
	}
#endif

#if 0
	if(stats)
	{		
		printf("%2.2f,%2.2f,%2.2f,%2.2f\n", r1_psnr, g1_psnr, g2_psnr, b1_psnr);
		//printf("%2.2f,%2.2f,%2.2f,%2.2f\n", ytotal, rtotal, gtotal, btotal); //offset

		r_offset = r_total;
		g_offset = g_total;
		b_offset = b_total;
#if 0
		if(once == 0)
		{
			once = 1;
			goto oncemore;
		}
#endif
	}
	else
#endif
	{
		printf("%2.2f,%2.2f,%2.2f,%2.2f\n", r1_psnr, g1_psnr, g2_psnr, b1_psnr);
	}

#if 0
	g_y_psnr += y_psnr;
	g_r_psnr += r_psnr;
	g_g_psnr += g_psnr;
	g_b_psnr += b_psnr;
#endif
#if 0
	g_ytotal += y_total;
	g_rtotal += r_total;
	g_gtotal += g_total;
	g_btotal += b_total;
#endif
}


void ComputePSNR_BYR4(int width, int height, void *srca, void *srcb, int stats)     
{
#if 0
	double r_offset = 0.0;
	double g_offset = 0.0;
	double b_offset = 0.0;
	double y_total = 0.0;
	double r_total = 0.0;
	double g_total = 0.0;
	double b_total = 0.0;
	double total = 0.0;
#endif
	uint16_t *ptra1, *ptra2;
	uint16_t *ptrb1, *ptrb2;

	int x, y;
	//int lowcount = 0;
	//int highcount = 0;

	//int val,average1,average2;

	//int once = 0;
	//long curve[1024];
	//long curvecount[1024];

	const int x_border = 0;
	const int y_border = 0;

	const double maximum_value = UINT16_MAX;

#if 0
	double g_y_psnr = 0.0, g_r_psnr = 0.0, g_g_psnr = 0.0, g_b_psnr = 0.0;
	double g_ytotal = 0.0; 
	double g_rtotal = 0.0; 
	double g_gtotal = 0.0; 
	double g_btotal = 0.0;
#endif

	double r1_total = 0;
	double g1_total = 0;
	double g2_total = 0;
	double b1_total = 0;

	double r1_rmse = 0.0;
	double g1_rmse = 0.0;
	double g2_rmse = 0.0;
	double b1_rmse = 0.0;

	double r1_psnr;
	double g1_psnr;
	double g2_psnr;
	double b1_psnr;

	int count = 0;

	// The width and height must be in units of pattern elements
	width /= 2;
	height /= 2;

#if 0
	for(y=0;y<1024;y++)
	{
		curve[y] = 0;
		curvecount[y] = 0;
	}
#endif

	for (y = y_border; y < height - y_border; y++)
	{
		ptra1 = (uint16_t *)srca;
		ptra1 += y * width * 4;
		ptra2 = ptra1 + width * 2;

		ptrb1 = (uint16_t *)srcb;
		ptrb1 += y * width * 4;
		ptrb2 = ptrb1 + width * 2;

		for (x = x_border; x < width - x_border; x++)
		{
			uint16_t srca_r1;
			uint16_t srca_g1;
			uint16_t srca_g2;
			uint16_t srca_b1;

			uint16_t srcb_r1;
			uint16_t srcb_g1;
			uint16_t srcb_g2;
			uint16_t srcb_b1;

			double r1a, r1b;
			double g1a, g1b;
			double g2a, g2b;
			double b1a, b1b;

			double r1_delta;
			double g1_delta;
			double g2_delta;
			double b1_delta;

			srca_r1 = ptra1[2 * x + 0];
			srca_g1 = ptra1[2 * x + 1];
			srca_g2 = ptra2[2 * x + 0];
			srca_b1 = ptra2[2 * x + 1];

			srcb_r1 = ptrb1[2 * x + 0];
			srcb_g1 = ptrb1[2 * x + 1];
			srcb_g2 = ptrb2[2 * x + 0];
			srcb_b1 = ptrb2[2 * x + 1];
#if 0
			curve[srca_r1] += srcb_r1 + r1_offset;	curvecount[srca_r1]++;
			curve[srca_g1] += srcb_g1 + g1_offset;	curvecount[srca_g1]++;
			curve[srca_g2] += srcb_g2 + g2_offset;	curvecount[srca_g2]++;
			curve[srca_b1] += srcb_b1 + b1_offset;	curvecount[srca_b1]++;
#endif
			r1a = srca_r1;
			g1a = srca_g1;
			g2a = srca_g2;
			b1a = srca_b1;

			r1b = srcb_r1;
			g1b = srcb_g1;
			g2b = srcb_g2;
			b1b = srcb_b1;

			r1_delta = r1a - r1b;
			r1_total += r1_delta;
			r1_rmse += sqr(r1_delta);

			g1_delta = g1a - g1b;
			g1_total += g1_delta;
			g1_rmse += sqr(g1_delta);

			g2_delta = g2a - g2b;
			g2_total += g2_delta;
			g2_rmse += sqr(g2_delta);

			b1_delta = b1a - b1b;
			b1_total += b1_delta;
			b1_rmse += sqr(b1_delta);
#if 0			
			y1 = (r1*213 + g1*715 + b1*72);
			y2 = (r2*213 + g2*715 + b2*72);			
			diff = -(((double)(y1)) - ((double)(y2)));
			diff /= 1000.0;
			y_total += diff;			
			ymse += (diff * diff);
#endif		
			count++;
#if 0
			if(!stats)
			{
				dpxvala = r3<<22 | g3<<12 | b3<<2;
				*ptra++ = Swap32(dpxvala);
			}
			else
			{
				ptra++;
			}
			ptrb++;
#else

#endif
		}
	}

	r1_total /= count;
	g1_total /= count;
	g2_total /= count;
	b1_total /= count;
	//y_total /= (double)(count);

	r1_rmse /= count;
	g1_rmse /= count;
	g2_rmse /= count;
	b1_rmse /= count;

	//y_rmse = sqrt(ymse);
	r1_rmse = sqrt(r1_rmse);
	g1_rmse = sqrt(g1_rmse);
	g2_rmse = sqrt(g2_rmse);
	b1_rmse = sqrt(b1_rmse);

	//y_psnr = 20.0*log10(MAXVALUE/y_rmse);
	r1_psnr = (r1_rmse > 0.0) ? 20.0 * log10(maximum_value / r1_rmse) : 0.0;
	g1_psnr = (g1_rmse > 0.0) ? 20.0 * log10(maximum_value / g1_rmse) : 0.0;
	g2_psnr = (g2_rmse > 0.0) ? 20.0 * log10(maximum_value / g2_rmse) : 0.0;
	b1_psnr = (b1_rmse > 0.0) ? 20.0 * log10(maximum_value / b1_rmse) : 0.0;

#if 0
	for(y=0;y<1024;y++)
	{
		if(curvecount[y]) {
			curve[y] /= curvecount[y];
		}
	}
#endif

#if 0
	if(stats)
	{		
		printf("%2.2f,%2.2f,%2.2f,%2.2f\n", r1_psnr, g1_psnr, g2_psnr, b1_psnr);
		//printf("%2.2f,%2.2f,%2.2f,%2.2f\n", ytotal, rtotal, gtotal, btotal); //offset

		r_offset = r_total;
		g_offset = g_total;
		b_offset = b_total;
#if 0
		if(once == 0)
		{
			once = 1;
			goto oncemore;
		}
#endif
	}
	else
#endif
	{
		printf("%2.2f,%2.2f,%2.2f,%2.2f\n", r1_psnr, g1_psnr, g2_psnr, b1_psnr);
	}

#if 0
	g_y_psnr += y_psnr;
	g_r_psnr += r_psnr;
	g_g_psnr += g_psnr;
	g_b_psnr += b_psnr;
#endif
#if 0
	g_ytotal += y_total;
	g_rtotal += r_total;
	g_gtotal += g_total;
	g_btotal += b_total;
#endif
}
