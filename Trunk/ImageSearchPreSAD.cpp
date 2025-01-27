#include "StdAfx.h"
#include <stdlib.h>

#ifdef WOULD_BE_NICE_IF_WORKED
void ComputeSAD32x32Map_(LPCOLORREF pixels, int in_width, int in_height, LPCOLORREF* out_SADSums)
{
	*out_SADSums = (LPCOLORREF)MY_ALLOC(in_width * in_height * sizeof(LPCOLORREF));

	if (*out_SADSums == NULL)
	{
		return;
	}

	// last 32 columns / rows would contain bad data
	in_width -= 32;
	in_height -= 32;

	// first we process just collumns
	for (size_t y = 0; y < in_height; y += 1)
	{
		for (size_t x = 0; x < in_width; x += 1)
		{
			unsigned int sad_array[8];
			__m256i l0, line_sad, acc_sad;

			acc_sad = _mm256_setzero_si256();

			// prcess 8 pixels => 4*8=32 bytes
			l0 = _mm256_loadu_si256((__m256i*)(&pixels[y * in_width + x]));
			line_sad = _mm256_sad_epu8(l0, _mm256_setzero_si256());
			acc_sad = _mm256_add_epi32(acc_sad, line_sad);

			_mm256_storeu_si256((__m256i*)(&sad_array[0]), acc_sad);

			(*out_SADSums)[y * in_width + x] = sad_array[0] + sad_array[2] + sad_array[4] + sad_array[6];
		}
	}

	// from 8x1 to 8x4
	for (size_t y = 0; y < in_height; y += 1)
	{
		for (size_t x = 0; x < in_width; x += 1)
		{
				(*out_SADSums)[y * in_width + x] = (*out_SADSums)[y * in_width + x] 
					+ (*out_SADSums)[(y + 1) * in_width + x]
					+ (*out_SADSums)[(y + 2) * in_width + x]
					+ (*out_SADSums)[(y + 3) * in_width + x];
		}
	}

	// from 8x4 to 8x8
	for (size_t y = 0; y < in_height; y += 1)
	{
		for (size_t x = 0; x < in_width; x += 1)
		{
			(*out_SADSums)[y * in_width + x] = (*out_SADSums)[y * in_width + x]
				+ (*out_SADSums)[(y + 4) * in_width + x];
		}
	}

	// convert to 16x16 from 8x8
	for (size_t y = 0; y < in_height; y += 1)
	{
		for (size_t x = 0; x < in_width; x += 1)
		{
			(*out_SADSums)[y * in_width + x] = (*out_SADSums)[y * in_width + x] 
				+ (*out_SADSums)[(y + 0) * in_width + x + 8]
				+ (*out_SADSums)[(y + 8) * in_width + x + 0]
				+ (*out_SADSums)[(y + 8) * in_width + x + 8];
		}
	}

	// convert to 32x32 from 16x16
	for (size_t y = 0; y < in_height; y += 1)
	{
		for (size_t x = 0; x < in_width; x += 1)
		{
			(*out_SADSums)[y * in_width + x] = (*out_SADSums)[y * in_width + x]
				+ (*out_SADSums)[(y + 0) * in_width + x + 16]
				+ (*out_SADSums)[(y + 16) * in_width + x + 0]
				+ (*out_SADSums)[(y + 16) * in_width + x + 16];
		}
	}

	// convert to 64x64 from 32x32
	for (size_t y = 0; y < in_height; y += 1)
	{
		for (size_t x = 0; x < in_width; x += 1)
		{
			(*out_SADSums)[y * in_width + x] = (*out_SADSums)[y * in_width + x]
				+ (*out_SADSums)[(y + 0) * in_width + x + 32]
				+ (*out_SADSums)[(y + 32) * in_width + x + 0]
				+ (*out_SADSums)[(y + 32) * in_width + x + 32];
		}
	}
}
#endif

#ifdef OUTDATEDCODE
void ComputeSAD16x16Map(LPCOLORREF pixels, int in_width, int in_height, LPCOLORREF* out_SADSums)
{
	*out_SADSums = (LPCOLORREF)MY_ALLOC(in_width * in_height * sizeof(LPCOLORREF));

	if (*out_SADSums == NULL)
	{
		return;
	}

	// last 32 columns / rows would contain bad data
	in_width -= 16;
	in_height -= 16;

	// first we process just collumns
	for (size_t y = 0; y < in_height; y += 1)
	{
		unsigned char* pixels2 = (unsigned char*)&pixels[y * in_width];
		LPCOLORREF writeTo = &((*out_SADSums)[y * in_width + 0]);
		writeTo[0] = 0;
		for (size_t tx = 0; tx < 16; tx += 4)
		{
			writeTo[0] += pixels2[tx + 0] + pixels2[tx + 1] + pixels2[tx + 2];
		}
		for (size_t x = 1; x < in_width; x += 1)
		{
			// from previous sum of colors, substract the first column and add the next column
			writeTo[1] = writeTo[0]
				- (pixels2[0] + pixels2[1] + pixels2[2]) // substract first
				+ (pixels2[16] + pixels2[17] + pixels2[18]); // add next
			pixels2 += 4;
			writeTo += 1;
		}
	}

	// from 16x1 to 16x4
	for (size_t y = 0; y < in_height; y += 1)
	{
		LPCOLORREF writeTo = &((*out_SADSums)[y * in_width + 0]);
		LPCOLORREF readFrom2 = &((*out_SADSums)[(y + 1) * in_width + 0]);
		LPCOLORREF readFrom3 = &((*out_SADSums)[(y + 2) * in_width + 0]);
		LPCOLORREF readFrom4 = &((*out_SADSums)[(y + 3) * in_width + 0]);
		for (size_t x = 0; x < in_width; x += 1)
		{
			*writeTo = *writeTo + *readFrom2 + *readFrom3 + *readFrom4;
			writeTo++;
			readFrom2++;
			readFrom3++;
			readFrom4++;
		}
	}

	// from 16x4 to 16x16
	for (size_t y = 0; y < in_height; y += 1)
	{
		LPCOLORREF writeTo = &((*out_SADSums)[y * in_width + 0]);
		LPCOLORREF readFrom2 = &((*out_SADSums)[(y + 4) * in_width + 0]);
		LPCOLORREF readFrom3 = &((*out_SADSums)[(y + 8) * in_width + 0]);
		LPCOLORREF readFrom4 = &((*out_SADSums)[(y + 12) * in_width + 0]);
		for (size_t x = 0; x < in_width; x += 1)
		{
			*writeTo = *writeTo + *readFrom2 + *readFrom3 + *readFrom4;
			writeTo++;
			readFrom2++;
			readFrom3++;
			readFrom4++;
		}
	}
}

void ComputeSAD32x32Map(LPCOLORREF pixels, int in_width, int in_height, LPCOLORREF* out_SADSums)
{
	*out_SADSums = (LPCOLORREF)MY_ALLOC(in_width * in_height * sizeof(LPCOLORREF));

	if (*out_SADSums == NULL)
	{
		return;
	}

	// last 32 columns / rows would contain bad data
	in_width -= 32;
	in_height -= 32;

	// first we process just collumns
	for (size_t y = 0; y < in_height; y += 1)
	{
		unsigned char* pixels2 = (unsigned char*)&pixels[y * in_width];
		LPCOLORREF writeTo = &((*out_SADSums)[y * in_width + 0]);
		writeTo[0] = 0;
		for (size_t tx = 0; tx < 32; tx += 4)
		{
			writeTo[0] += pixels2[tx + 0] + pixels2[tx + 1] + pixels2[tx + 2];
		}
		for (size_t x = 1; x < in_width; x += 1)
		{
			// from previous sum of colors, substract the first column and add the next column
			writeTo[1] = writeTo[0]
				- (pixels2[0] + pixels2[1] + pixels2[2]) // substract first
				+ (pixels2[32] + pixels2[33] + pixels2[34]); // add next
			pixels2 += 4;
			writeTo += 1;
		}
	}

	// from 32x1 to 32x4
	for (size_t y = 0; y < in_height; y += 1)
	{
		LPCOLORREF writeTo = &((*out_SADSums)[y * in_width + 0]);
		LPCOLORREF readFrom2 = &((*out_SADSums)[(y + 1) * in_width + 0]);
		LPCOLORREF readFrom3 = &((*out_SADSums)[(y + 2) * in_width + 0]);
		LPCOLORREF readFrom4 = &((*out_SADSums)[(y + 3) * in_width + 0]);
		for (size_t x = 0; x < in_width; x += 1)
		{
			*writeTo = *writeTo + *readFrom2 + *readFrom3 + *readFrom4;
			writeTo++;
			readFrom2++;
			readFrom3++;
			readFrom4++;
		}
	}

	// from 32x4 to 32x16
	for (size_t y = 0; y < in_height; y += 1)
	{
		LPCOLORREF writeTo = &((*out_SADSums)[y * in_width + 0]);
		LPCOLORREF readFrom2 = &((*out_SADSums)[(y + 4) * in_width + 0]);
		LPCOLORREF readFrom3 = &((*out_SADSums)[(y + 8) * in_width + 0]);
		LPCOLORREF readFrom4 = &((*out_SADSums)[(y + 12) * in_width + 0]);
		for (size_t x = 0; x < in_width; x += 1)
		{
			*writeTo = *writeTo + *readFrom2 + *readFrom3 + *readFrom4;
			writeTo++;
			readFrom2++;
			readFrom3++;
			readFrom4++;
		}
	}

	// from 32x16 to 32x32
	for (size_t y = 0; y < in_height; y += 1)
	{
		LPCOLORREF writeTo = &((*out_SADSums)[y * in_width + 0]);
		LPCOLORREF readFrom2 = &((*out_SADSums)[(y + 16) * in_width + 0]);
		for (size_t x = 0; x < in_width; x += 1)
		{
			*writeTo = *writeTo + *readFrom2;
			writeTo++;
			readFrom2++;
		}
	}
}

void ComputeSAD64x64Map(LPCOLORREF pixels, int in_width, int in_height, LPCOLORREF* out_SADSums)
{
	*out_SADSums = (LPCOLORREF)MY_ALLOC(in_width * in_height * sizeof(LPCOLORREF));

	if (*out_SADSums == NULL)
	{
		return;
	}

	// last 64 columns / rows would contain bad data
	in_width -= 64;
	in_height -= 64;

	// first we process just collumns
	for (size_t y = 0; y < in_height; y += 1)
	{
		unsigned char* pixels2 = (unsigned char*)&pixels[y * in_width];
		LPCOLORREF writeTo = &((*out_SADSums)[y * in_width + 0]);
		writeTo[0] = 0;
		for (size_t tx = 0; tx < 64; tx+=4)
		{
			writeTo[0] += pixels2[tx + 0] + pixels2[tx + 1] + pixels2[tx + 2];
		}
		for (size_t x = 1; x < in_width; x += 1)
		{
			// from previous sum of colors, substract the first column and add the next column
			writeTo[1] = writeTo[0]
				- (pixels2[0] + pixels2[1] + pixels2[2]) // substract first
				+ (pixels2[64] + pixels2[65] + pixels2[66]); // add next
			pixels2 += 4;
			writeTo += 1;
		}
	}

	// from 64x1 to 64x4
	for (size_t y = 0; y < in_height; y += 1)
	{
		LPCOLORREF writeTo = &((*out_SADSums)[y * in_width + 0]);
		LPCOLORREF readFrom2 = &((*out_SADSums)[(y + 1) * in_width + 0]);
		LPCOLORREF readFrom3 = &((*out_SADSums)[(y + 2) * in_width + 0]);
		LPCOLORREF readFrom4 = &((*out_SADSums)[(y + 3) * in_width + 0]);
		for (size_t x = 0; x < in_width; x += 1)
		{
			*writeTo = *writeTo + *readFrom2 + *readFrom3 + *readFrom4;
			writeTo++;
			readFrom2++;
			readFrom3++;
			readFrom4++;
		}
	}

	// from 64x4 to 64x16
	for (size_t y = 0; y < in_height; y += 1)
	{
		LPCOLORREF writeTo = &((*out_SADSums)[y * in_width + 0]);
		LPCOLORREF readFrom2 = &((*out_SADSums)[(y + 4) * in_width + 0]);
		LPCOLORREF readFrom3 = &((*out_SADSums)[(y + 8) * in_width + 0]);
		LPCOLORREF readFrom4 = &((*out_SADSums)[(y + 12) * in_width + 0]);
		for (size_t x = 0; x < in_width; x += 1)
		{
			*writeTo = *writeTo + *readFrom2 + *readFrom3 + *readFrom4;
			writeTo++;
			readFrom2++;
			readFrom3++;
			readFrom4++;
		}
	}

	// from 64x16 to 64x64
	for (size_t y = 0; y < in_height; y += 1)
	{
		LPCOLORREF writeTo = &((*out_SADSums)[y * in_width + 0]);
		LPCOLORREF readFrom2 = &((*out_SADSums)[(y + 16) * in_width + 0]);
		LPCOLORREF readFrom3 = &((*out_SADSums)[(y + 32) * in_width + 0]);
		LPCOLORREF readFrom4 = &((*out_SADSums)[(y + 48) * in_width + 0]);
		for (size_t x = 0; x < in_width; x += 1)
		{
			*writeTo = *writeTo + *readFrom2 + *readFrom3 + *readFrom4;
			writeTo++;
			readFrom2++;
			readFrom3++;
			readFrom4++;
		}
	}
}
#endif

void InitSADSUMScreenshot(SADSumStoreScreenshot* sss)
{
	memset(sss, 0, sizeof(SADSumStoreScreenshot));
}

void FreeSADSUMScreenshot(SADSumStoreScreenshot* sss)
{
	for (size_t color = 0; color < 3; color++)
	{
		MY_FREE(sss->SADSum16x16[color]);
		MY_FREE(sss->SADSum32x32[color]);
		MY_FREE(sss->SADSum64x64[color]);
		MY_FREE(sss->SADSum128x128[color]);
	}
}

void ComputeSADSumScreenshot(LPCOLORREF pixels, int in_width, int in_height, SADSumStoreScreenshot* out_SADSums, SADSumAvailableSizes requiredSize)
{
	size_t stride = in_width;
	size_t in_width2;
	size_t in_height2;


	for (size_t color = 0; color < 3; color++)
	{
		if (out_SADSums->SADSum16x16[color] == NULL)
		{
			out_SADSums->SADSum16x16[color] = (LPCOLORREF)MY_ALLOC(in_width * in_height * sizeof(LPCOLORREF));


			// last 64 columns / rows would contain bad data
			in_width2 = in_width - 8;
			in_height2 = in_height - 8;

			// first we process just collumns
//			size_t AvgPrev = 128;
			for (size_t y = 0; y < in_height2; y += 1)
			{
				LPCOLORREF writeTo = &(out_SADSums->SADSum16x16[color][y * stride + 0]);
//				writeTo[0] = 0;
				for (size_t x = 0; x < in_width2; x += 1)
				{
					unsigned char* pixels2 = (unsigned char*)&pixels[y * stride + x];
					pixels2 += color;
//					size_t sum = 0;
					unsigned __int64 hash1 = 0; // relative to prev value
					if (pixels2[0] < pixels2[4]) hash1 |= (1 << (0));
					if (pixels2[4] < pixels2[8]) hash1 |= (1 << (1));
					if (pixels2[8] < pixels2[12]) hash1 |= (1 << (2));
					if (pixels2[12] < pixels2[16]) hash1 |= (1 << (3));
					pixels2 += (stride * 4);
					if (pixels2[0] < pixels2[4]) hash1 |= (1 << (4));
					if (pixels2[4] < pixels2[8]) hash1 |= (1 << (5));
					if (pixels2[8] < pixels2[12]) hash1 |= (1 << (6));
					if (pixels2[12] < pixels2[16]) hash1 |= (1 << (7));
					pixels2 += (stride * 4);
					if (pixels2[0] < pixels2[4]) hash1 |= (1 << (8));
					if (pixels2[4] < pixels2[8]) hash1 |= (1 << (9));
					if (pixels2[8] < pixels2[12]) hash1 |= (1 << (10));
					if (pixels2[12] < pixels2[16]) hash1 |= (1 << (11));
					pixels2 += (stride * 4);
					if (pixels2[0] < pixels2[4]) hash1 |= (1 << (12));
					if (pixels2[4] < pixels2[8]) hash1 |= (1 << (13));
					if (pixels2[8] < pixels2[12]) hash1 |= (1 << (14));
					if (pixels2[12] < pixels2[16]) hash1 |= (1 << (15));
#if 0
//					unsigned __int64 hash2 = 0; // relative to avg value
					for (size_t y2 = 0; y2 < 4; y2++)
					{
						for (size_t x2 = 0; x2 < 4; x2++)
						{
//							sum += pixels2[x2];
							if (pixels2[x2] > pixels2[x2 - 1])
								hash1 |= ( 1 << (y2 * 4 + x2));

/*							if (pixels2[x2] > AvgPrev)
							{
								hash2 = (hash2 << 1) | 1;
							}
							else
							{
								hash2 = (hash2 << 1);
							}*/
						}
						pixels2 += 8 * 4;
					}
//					AvgPrev = sum / (8 * 8);
#endif
					writeTo[0] = (DWORD)hash1;
//					writeTo[1] = (DWORD)hash2;
					writeTo++;
				}
			}


			// last 64 columns / rows would contain bad data
			in_width2 = in_width - 16;
			in_height2 = in_height - 4;

			// from 16x1 to 16x4
			for (size_t y = 0; y < in_height2; y += 1)
			{
				LPCOLORREF writeTo = &(out_SADSums->SADSum16x16[color][(y + 0) * stride + 0]);
				LPCOLORREF readFrom2 = &(out_SADSums->SADSum16x16[color][(y + 1) * stride + 0]);
				LPCOLORREF readFrom3 = &(out_SADSums->SADSum16x16[color][(y + 2) * stride + 0]);
				LPCOLORREF readFrom4 = &(out_SADSums->SADSum16x16[color][(y + 3) * stride + 0]);
				for (size_t x = 0; x < in_width2; x += 1)
				{
					*writeTo = *writeTo + *readFrom2 + *readFrom3 + *readFrom4;
					writeTo++;
					readFrom2++;
					readFrom3++;
					readFrom4++;
				}
			}

			// from 16x4 to 16x16
			in_width2 = in_width - 16;
			in_height2 = in_height - 12;
			for (size_t y = 0; y < in_height2; y += 1)
			{
				LPCOLORREF writeTo = &(out_SADSums->SADSum16x16[color][(y + 0) * stride + 0]);
				LPCOLORREF readFrom2 = &(out_SADSums->SADSum16x16[color][(y + 4) * stride + 0]);
				LPCOLORREF readFrom3 = &(out_SADSums->SADSum16x16[color][(y + 8) * stride + 0]);
				LPCOLORREF readFrom4 = &(out_SADSums->SADSum16x16[color][(y + 12) * stride + 0]);
				for (size_t x = 0; x < in_width2; x += 1)
				{
					*writeTo = *writeTo + *readFrom2 + *readFrom3 + *readFrom4;
					writeTo++;
					readFrom2++;
					readFrom3++;
					readFrom4++;
				}
			}
		}

		if (out_SADSums->SADSum32x32[color] == NULL && requiredSize >= SSAS_32x32)
		{
			out_SADSums->SADSum32x32[color] = (LPCOLORREF)MY_ALLOC(in_width * in_height * sizeof(LPCOLORREF));

			// from 16x16 to 32x32
			in_width2 = in_width - 16;
			in_height2 = in_height - 16;
			for (size_t y = 0; y < in_height2; y += 1)
			{
				LPCOLORREF writeTo = &(out_SADSums->SADSum32x32[color][(y + 0) * stride + 0]);
				LPCOLORREF readFrom1 = &(out_SADSums->SADSum16x16[color][(y + 0) * stride + 0]);
				LPCOLORREF readFrom2 = &(out_SADSums->SADSum16x16[color][(y + 0) * stride + 16]);
				LPCOLORREF readFrom3 = &(out_SADSums->SADSum16x16[color][(y + 16) * stride + 0]);
				LPCOLORREF readFrom4 = &(out_SADSums->SADSum16x16[color][(y + 16) * stride + 16]);
				for (size_t x = 0; x < in_width2; x += 1)
				{
					*writeTo = *readFrom1 + *readFrom2 + *readFrom3 + *readFrom4;
					writeTo++;
					readFrom1++;
					readFrom2++;
					readFrom3++;
					readFrom4++;
				}
			}
		}

		if (out_SADSums->SADSum64x64[color] == NULL && requiredSize >= SSAS_64x64)
		{
			out_SADSums->SADSum64x64[color] = (LPCOLORREF)MY_ALLOC(in_width * in_height * sizeof(LPCOLORREF));
			// from 32x32 to 64x64
			in_width2 = in_width - 32;
			in_height2 = in_height - 32;
			for (size_t y = 0; y < in_height2; y += 1)
			{
				LPCOLORREF writeTo = &(out_SADSums->SADSum64x64[color][y * stride + 0]);
				LPCOLORREF readFrom1 = &(out_SADSums->SADSum32x32[color][(y + 0) * stride + 0]);
				LPCOLORREF readFrom2 = &(out_SADSums->SADSum32x32[color][(y + 0) * stride + 32]);
				LPCOLORREF readFrom3 = &(out_SADSums->SADSum32x32[color][(y + 32) * stride + 0]);
				LPCOLORREF readFrom4 = &(out_SADSums->SADSum32x32[color][(y + 32) * stride + 32]);
				for (size_t x = 0; x < in_width2; x += 1)
				{
					*writeTo = *readFrom1 + *readFrom2 + *readFrom3 + *readFrom4;
					writeTo++;
					readFrom1++;
					readFrom2++;
					readFrom3++;
					readFrom4++;
				}
			}
		}

		if (out_SADSums->SADSum128x128[color] == NULL && requiredSize >= SSAS_128x128)
		{
			out_SADSums->SADSum128x128[color] = (LPCOLORREF)MY_ALLOC(in_width * in_height * sizeof(LPCOLORREF));
			// from 64x64 to 128x128
			in_width2 = in_width - 64;
			in_height2 = in_height - 64;
			for (size_t y = 0; y < in_height2; y += 1)
			{
				LPCOLORREF writeTo = &(out_SADSums->SADSum128x128[color][y * stride + 0]);
				LPCOLORREF readFrom1 = &(out_SADSums->SADSum64x64[color][(y + 0) * stride + 0]);
				LPCOLORREF readFrom2 = &(out_SADSums->SADSum64x64[color][(y + 0) * stride + 64]);
				LPCOLORREF readFrom3 = &(out_SADSums->SADSum64x64[color][(y + 64) * stride + 0]);
				LPCOLORREF readFrom4 = &(out_SADSums->SADSum64x64[color][(y + 64) * stride + 64]);
				for (size_t x = 0; x < in_width2; x += 1)
				{
					*writeTo = *readFrom1 + *readFrom2 + *readFrom3 + *readFrom4;
					writeTo++;
					readFrom1++;
					readFrom2++;
					readFrom3++;
					readFrom4++;
				}
			}
		}
	}
}

void InitSADSUMCached(SADSumStoreCached* sss)
{
	memset(sss, 0, sizeof(SADSumStoreCached));
}

void FreeSADSUMCached(SADSumStoreCached* sss)
{
}

void ComputeSADSumCached(LPCOLORREF pixels, int in_width, int in_height, SADSumStoreCached* out_SADSums)
{
	size_t stride = in_width;

	// one time init 
	if (out_SADSums->width_height != 0)
	{
		return;
	}

	FileDebug("Prepare SADSum for cached image");

	if (in_width >= 128 && in_height >= 128)
	{
		in_width = 128;
		in_height = 128;
		out_SADSums->width_height = 128;
	}
	else if (in_width >= 64 && in_height >= 64)
	{
		in_width = 64;
		in_height = 64;
		out_SADSums->width_height = 64;
	}
	else if (in_width >= 32 && in_height >= 32)
	{
		in_width = 32;
		in_height = 32;
		out_SADSums->width_height = 32;
	}
	else if (in_width >= 16 && in_height >= 16)
	{
		in_width = 16;
		in_height = 16;
		out_SADSums->width_height = 16;
	}
	else
	{
		out_SADSums->width_height = 0;
		return;
	}

	memset(out_SADSums->SADSum16x16, 0, sizeof(out_SADSums->SADSum16x16));
	memset(out_SADSums->SADSum, 0, sizeof(out_SADSums->SADSum));
	for (size_t y = 0; y < (size_t)in_height; y++)
	{
		unsigned char *readFrom = (unsigned char*)&(pixels[(y + 0) * stride + 0]);
		for (size_t x = 0; x < (size_t)in_width; x++)
		{
			size_t y16x16 = y / 16;
			size_t x16x16 = x / 16;
			out_SADSums->SADSum16x16[0][y16x16][x16x16] += readFrom[0];
			out_SADSums->SADSum16x16[1][y16x16][x16x16] += readFrom[1];
			out_SADSums->SADSum16x16[2][y16x16][x16x16] += readFrom[2];
			out_SADSums->SADSum[0] += readFrom[0];
			out_SADSums->SADSum[1] += readFrom[1];
			out_SADSums->SADSum[2] += readFrom[2];
			readFrom += 4;
		}
	}
}

void ImageSearch_SAD_BestMatchAlwaysFind(CachedPicture* cache, const size_t big_height, const size_t big_width, 
	LPCOLORREF* bigSADSums, const size_t stride, const __int64 *small_SAD, int &retx, int &rety)
{
	__int64 bestSAD = 0x7FFFFFFFFFFFFFFF;
	__int64 bestSADSum = 0x7FFFFFFFFFFFFFFF;
	for (size_t y = 0; y < big_height; y++)
	{
		LPCOLORREF bigSADSumsRow_0 = &bigSADSums[0][y * stride];
		LPCOLORREF bigSADSumsRow_1 = &bigSADSums[1][y * stride];
		LPCOLORREF bigSADSumsRow_2 = &bigSADSums[2][y * stride];
		for (size_t x = 0; x < big_width; x++)
		{
			__int64 sadSums_0 = (__int64)small_SAD[0] - (__int64)*bigSADSumsRow_0;
			__int64 sadSums_1 = (__int64)small_SAD[1] - (__int64)*bigSADSumsRow_1;
			__int64 sadSums_2 = (__int64)small_SAD[2] - (__int64)*bigSADSumsRow_2;
			__int64 sadExp = sadSums_0 * sadSums_0 + sadSums_1 * sadSums_1 + sadSums_2 * sadSums_2;
			if (sadExp > bestSADSum)
			{
				bigSADSumsRow_0++;
				bigSADSumsRow_1++;
				bigSADSumsRow_2++;
				continue;
			}

			// get the real sad at this point
			__int64 sadAlmostPrecise = ImgSAD(CurScreenshot->Pixels, CurScreenshot->GetWidth(), CurScreenshot->GetHeight(), CurScreenshot->GetWidth(),
				cache->Pixels, cache->Width, cache->Height, cache->Width,
				x, y);

			if (sadAlmostPrecise < bestSAD)
			{
				// these 2 will not be the same because SADSum only captures a portion of the cached image
				// SADSum should be smaller than bestSAD
				bestSAD = sadAlmostPrecise;
				bestSADSum = sadExp;
				retx = (int)x;
				rety = (int)y;
			}

			bigSADSumsRow_0++;
			bigSADSumsRow_1++;
			bigSADSumsRow_2++;
		}
	}
}

// !! this expects the cached image to be at least 16x16
__forceinline __int64 IsSADSum16x16OSmallEnough(CachedPicture* cache, size_t atX, size_t atY, size_t stride, __int64 sad_limit_1_channel_16x16)
{
	const size_t nr16 = cache->SADSums.width_height/16;
	for (size_t color = 0; color < 3; color++)
	{
		for (size_t y = 0; y < nr16; y++)
		{
			LPCOLORREF bigSADSumsRow = &CurScreenshot->SADSums.SADSum16x16[color][(atY + y * 16) * stride + atX];
			for (size_t x = 0; x < nr16; x++)
			{
				__int64 sadSums = (__int64)cache->SADSums.SADSum16x16[color][y][x] - (__int64)*bigSADSumsRow;
				if (sadSums > sad_limit_1_channel_16x16)
				{
					return 0;
				}
			}
		}
	}
	return 1;
}

static char g_FuncReturnBuff[MAX_PATH];

char* WINAPI ImageSearch_Similar(char* aImageFile, float differencePCT)
{
	size_t startStamp = GetTickCount();

	CachedPicture* cache = CachePicture(aImageFile);
	g_FuncReturnBuff[0] = 0;
	if (cache == NULL)
	{
		FileDebug("Skipping Image search as image could not be loaded");
		return g_FuncReturnBuff;
	}

	// not yet initialized ?
	if (cache->SADSums.width_height == 0)
	{
		ComputeSADSumCached(cache->Pixels, cache->Width, cache->Height, &cache->SADSums);
	}
	// image is smaller than 16x16
	if (cache->SADSums.width_height == 0 || cache->SADSums.width_height > 128)
	{
		return ImageSearch_SAD(aImageFile);
	}

	size_t big_width = CurScreenshot->GetWidth();
	size_t big_height = CurScreenshot->GetHeight();
	size_t stride = big_width;
	ComputeSADSumScreenshot(CurScreenshot->Pixels, (int)big_width, (int)big_height, &CurScreenshot->SADSums, (SADSumAvailableSizes)cache->SADSums.width_height);

	int retx = -1;
	int rety = -1;
	LPCOLORREF *bigSADSums = NULL;
	const __int64 small_SAD[3] = { cache->SADSums.SADSum[0], cache->SADSums.SADSum[1], cache->SADSums.SADSum[2] };

	if (cache->SADSums.width_height == 16)
	{
		big_height -= 16;
		big_width -= 16;
		bigSADSums = CurScreenshot->SADSums.SADSum16x16;
	}
	else if (cache->SADSums.width_height == 32)
	{
		big_height -= 32;
		big_width -= 32;
		bigSADSums = CurScreenshot->SADSums.SADSum32x32;
	}
	else if (cache->SADSums.width_height == 64)
	{
		big_height -= 64;
		big_width -= 64;
		bigSADSums = CurScreenshot->SADSums.SADSum64x64;
	}
	else if (cache->SADSums.width_height == 128)
	{
		big_height -= 128;
		big_width -= 128;
		bigSADSums = CurScreenshot->SADSums.SADSum128x128;
	}

#if defined( _DEBUG ) && defined(_CONSOLE)
	size_t improved_result = 0;
	size_t sadCalcs = 0;
	size_t sadSkipps = 0;
	size_t sadSkipps2 = 0;
#endif

	const float SADPerPixelChannel = 255.0f * differencePCT / 100.0f;
	const __int64 totalArea = cache->Width * cache->Height;
	__int64 bestSAD = (__int64)(totalArea * 3.0f * SADPerPixelChannel);
	const __int64 totalAreaPrecached = (__int64)cache->SADSums.width_height * (__int64)cache->SADSums.width_height;
	const float areaPrecachedRatio = (float)totalAreaPrecached / (float)totalArea; // best case it's 1
	__int64 sad_limit_1_channel = (__int64)(totalAreaPrecached * SADPerPixelChannel);
	__int64 sad_limit_1_channel_16x16 = (__int64)(16.0f * 16.0f * SADPerPixelChannel);
	// sad limit is unknown and at this point it has no meaning to make a check against it
	if (differencePCT >= 100)
	{
		ImageSearch_SAD_BestMatchAlwaysFind(cache, big_height, big_width, bigSADSums, stride, small_SAD, retx, rety);
	}
	else
	{
		for (size_t y = 0; y < big_height; y++)
		{
			LPCOLORREF bigSADSumsRow_0 = &bigSADSums[0][y * stride];
			LPCOLORREF bigSADSumsRow_1 = &bigSADSums[1][y * stride];
			LPCOLORREF bigSADSumsRow_2 = &bigSADSums[2][y * stride];
			for (size_t x = 0; x < big_width; x++)
			{
				// !! small sads are not precise. example values : 16*16*128 != 8*16*64+8*16*192 when SAD is calculated ! position of values is lost !
				// !! small sad can be A LOT smaller or equal to precise SAD
				__int64 sadSums_0 = (__int64)small_SAD[0] - (__int64)*bigSADSumsRow_0;
				sadSums_0 = abs(sadSums_0);
				if (sadSums_0 > sad_limit_1_channel)
				{
					bigSADSumsRow_0++;
					bigSADSumsRow_1++;
					bigSADSumsRow_2++;
#if defined( _DEBUG ) && defined(_CONSOLE)
					sadSkipps++;
#endif
					continue;
				}
				__int64 sadSums_1 = (__int64)small_SAD[1] - (__int64)*bigSADSumsRow_1;
				sadSums_1 = abs(sadSums_1);
				if (sadSums_1 > sad_limit_1_channel)
				{
					bigSADSumsRow_0++;
					bigSADSumsRow_1++;
					bigSADSumsRow_2++;
#if defined( _DEBUG ) && defined(_CONSOLE)
					sadSkipps++;
#endif
					continue;
				}
				__int64 sadSums_2 = (__int64)small_SAD[2] - (__int64)*bigSADSumsRow_2;
				sadSums_2 = abs(sadSums_2);
				if (sadSums_2 > sad_limit_1_channel)
				{
					bigSADSumsRow_0++;
					bigSADSumsRow_1++;
					bigSADSumsRow_2++;
#if defined( _DEBUG ) && defined(_CONSOLE)
					sadSkipps++;
#endif
					continue;
				}

				if (sad_limit_1_channel_16x16 > 0 && IsSADSum16x16OSmallEnough(cache, x, y, stride, sad_limit_1_channel_16x16) == 0)
				{
					bigSADSumsRow_0++;
					bigSADSumsRow_1++;
					bigSADSumsRow_2++;
#if defined( _DEBUG ) && defined(_CONSOLE)
					sadSkipps2++;
#endif
					continue;
				}

				// get the real sad at this point
				__int64 sadAlmostPrecise = ImgSAD(CurScreenshot->Pixels, CurScreenshot->GetWidth(), CurScreenshot->GetHeight(), CurScreenshot->GetWidth(),
					cache->Pixels, cache->Width, cache->Height, cache->Width,
					x, y);

#if defined( _DEBUG ) && defined(_CONSOLE)
				sadCalcs++;
#endif
				if (sadAlmostPrecise < bestSAD)
				{
					bestSAD = sadAlmostPrecise;
					float newBestSADPerPixel = (float)bestSAD / (float)totalArea / 3.0f;
					sad_limit_1_channel = (__int64)(totalAreaPrecached * newBestSADPerPixel); // does not need to be true though. Could be that 2 channels are great and one is very bad
					sad_limit_1_channel_16x16 = (__int64)(16.0 * 16.0 * newBestSADPerPixel); // just hoping for the best
					retx = (int)x;
					rety = (int)y;
#if defined( _DEBUG ) && defined(_CONSOLE)
					improved_result++;
#endif
				}

				bigSADSumsRow_0++;
				bigSADSumsRow_1++;
				bigSADSumsRow_2++;
			}
		}
	}

	ReturnBuff[0] = 0;
	sprintf_s(ReturnBuff, DEFAULT_STR_BUFFER_SIZE * 10, "1|%d|%d|%d", retx, rety, (int)bestSAD);
	size_t endStamp = GetTickCount();

#if defined( _DEBUG ) && defined(_CONSOLE)
	char dbgmsg[DEFAULT_STR_BUFFER_SIZE];
	if (rety == -1) { rety = 0; retx = 0; }
	unsigned char* AddrBig = (unsigned char*)&CurScreenshot->Pixels[rety * stride + retx];
	unsigned char* AddSmall = (unsigned char*)&cache->Pixels[0];
	sprintf_s(dbgmsg, sizeof(dbgmsg), "\tImageSearch_SAD_Limit finished in %d ms. Name %s. Sad calcs %d, Sad skipps %d, Sad2 skipps %d, Improved match %d. Returning %s . pixel 0 0 : %d %d %d %d - %d %d %d %d",
		(int)(endStamp - startStamp), cache->FileName, (int)sadCalcs, (int)sadSkipps, (int)sadSkipps2, (int)improved_result, ReturnBuff, AddrBig[0], AddrBig[1], AddrBig[2], AddrBig[3], AddSmall[0], AddSmall[1], AddSmall[2], AddSmall[3]);
	FileDebug(dbgmsg);
#endif

	return ReturnBuff;
}