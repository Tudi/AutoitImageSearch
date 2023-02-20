#include "StdAfx.h"
#include <stdlib.h>

#ifdef WOULD_BE_NICE_IF_WORKED
void ComputeSAD32x32Map_(LPCOLORREF pixels, int in_width, int in_height, LPCOLORREF* out_sumsads)
{
	*out_sumsads = (LPCOLORREF)MY_ALLOC(in_width * in_height * sizeof(LPCOLORREF));

	if (*out_sumsads == NULL)
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

			(*out_sumsads)[y * in_width + x] = sad_array[0] + sad_array[2] + sad_array[4] + sad_array[6];
		}
	}

	// from 8x1 to 8x4
	for (size_t y = 0; y < in_height; y += 1)
	{
		for (size_t x = 0; x < in_width; x += 1)
		{
				(*out_sumsads)[y * in_width + x] = (*out_sumsads)[y * in_width + x] 
					+ (*out_sumsads)[(y + 1) * in_width + x]
					+ (*out_sumsads)[(y + 2) * in_width + x]
					+ (*out_sumsads)[(y + 3) * in_width + x];
		}
	}

	// from 8x4 to 8x8
	for (size_t y = 0; y < in_height; y += 1)
	{
		for (size_t x = 0; x < in_width; x += 1)
		{
			(*out_sumsads)[y * in_width + x] = (*out_sumsads)[y * in_width + x]
				+ (*out_sumsads)[(y + 4) * in_width + x];
		}
	}

	// convert to 16x16 from 8x8
	for (size_t y = 0; y < in_height; y += 1)
	{
		for (size_t x = 0; x < in_width; x += 1)
		{
			(*out_sumsads)[y * in_width + x] = (*out_sumsads)[y * in_width + x] 
				+ (*out_sumsads)[(y + 0) * in_width + x + 8]
				+ (*out_sumsads)[(y + 8) * in_width + x + 0]
				+ (*out_sumsads)[(y + 8) * in_width + x + 8];
		}
	}

	// convert to 32x32 from 16x16
	for (size_t y = 0; y < in_height; y += 1)
	{
		for (size_t x = 0; x < in_width; x += 1)
		{
			(*out_sumsads)[y * in_width + x] = (*out_sumsads)[y * in_width + x]
				+ (*out_sumsads)[(y + 0) * in_width + x + 16]
				+ (*out_sumsads)[(y + 16) * in_width + x + 0]
				+ (*out_sumsads)[(y + 16) * in_width + x + 16];
		}
	}

	// convert to 64x64 from 32x32
	for (size_t y = 0; y < in_height; y += 1)
	{
		for (size_t x = 0; x < in_width; x += 1)
		{
			(*out_sumsads)[y * in_width + x] = (*out_sumsads)[y * in_width + x]
				+ (*out_sumsads)[(y + 0) * in_width + x + 32]
				+ (*out_sumsads)[(y + 32) * in_width + x + 0]
				+ (*out_sumsads)[(y + 32) * in_width + x + 32];
		}
	}
}
#endif

#ifdef OUTDATEDCODE
void ComputeSAD16x16Map(LPCOLORREF pixels, int in_width, int in_height, LPCOLORREF* out_sumsads)
{
	*out_sumsads = (LPCOLORREF)MY_ALLOC(in_width * in_height * sizeof(LPCOLORREF));

	if (*out_sumsads == NULL)
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
		LPCOLORREF writeTo = &((*out_sumsads)[y * in_width + 0]);
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
		LPCOLORREF writeTo = &((*out_sumsads)[y * in_width + 0]);
		LPCOLORREF readFrom2 = &((*out_sumsads)[(y + 1) * in_width + 0]);
		LPCOLORREF readFrom3 = &((*out_sumsads)[(y + 2) * in_width + 0]);
		LPCOLORREF readFrom4 = &((*out_sumsads)[(y + 3) * in_width + 0]);
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
		LPCOLORREF writeTo = &((*out_sumsads)[y * in_width + 0]);
		LPCOLORREF readFrom2 = &((*out_sumsads)[(y + 4) * in_width + 0]);
		LPCOLORREF readFrom3 = &((*out_sumsads)[(y + 8) * in_width + 0]);
		LPCOLORREF readFrom4 = &((*out_sumsads)[(y + 12) * in_width + 0]);
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

void ComputeSAD32x32Map(LPCOLORREF pixels, int in_width, int in_height, LPCOLORREF* out_sumsads)
{
	*out_sumsads = (LPCOLORREF)MY_ALLOC(in_width * in_height * sizeof(LPCOLORREF));

	if (*out_sumsads == NULL)
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
		LPCOLORREF writeTo = &((*out_sumsads)[y * in_width + 0]);
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
		LPCOLORREF writeTo = &((*out_sumsads)[y * in_width + 0]);
		LPCOLORREF readFrom2 = &((*out_sumsads)[(y + 1) * in_width + 0]);
		LPCOLORREF readFrom3 = &((*out_sumsads)[(y + 2) * in_width + 0]);
		LPCOLORREF readFrom4 = &((*out_sumsads)[(y + 3) * in_width + 0]);
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
		LPCOLORREF writeTo = &((*out_sumsads)[y * in_width + 0]);
		LPCOLORREF readFrom2 = &((*out_sumsads)[(y + 4) * in_width + 0]);
		LPCOLORREF readFrom3 = &((*out_sumsads)[(y + 8) * in_width + 0]);
		LPCOLORREF readFrom4 = &((*out_sumsads)[(y + 12) * in_width + 0]);
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
		LPCOLORREF writeTo = &((*out_sumsads)[y * in_width + 0]);
		LPCOLORREF readFrom2 = &((*out_sumsads)[(y + 16) * in_width + 0]);
		for (size_t x = 0; x < in_width; x += 1)
		{
			*writeTo = *writeTo + *readFrom2;
			writeTo++;
			readFrom2++;
		}
	}
}

void ComputeSAD64x64Map(LPCOLORREF pixels, int in_width, int in_height, LPCOLORREF* out_sumsads)
{
	*out_sumsads = (LPCOLORREF)MY_ALLOC(in_width * in_height * sizeof(LPCOLORREF));

	if (*out_sumsads == NULL)
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
		LPCOLORREF writeTo = &((*out_sumsads)[y * in_width + 0]);
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
		LPCOLORREF writeTo = &((*out_sumsads)[y * in_width + 0]);
		LPCOLORREF readFrom2 = &((*out_sumsads)[(y + 1) * in_width + 0]);
		LPCOLORREF readFrom3 = &((*out_sumsads)[(y + 2) * in_width + 0]);
		LPCOLORREF readFrom4 = &((*out_sumsads)[(y + 3) * in_width + 0]);
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
		LPCOLORREF writeTo = &((*out_sumsads)[y * in_width + 0]);
		LPCOLORREF readFrom2 = &((*out_sumsads)[(y + 4) * in_width + 0]);
		LPCOLORREF readFrom3 = &((*out_sumsads)[(y + 8) * in_width + 0]);
		LPCOLORREF readFrom4 = &((*out_sumsads)[(y + 12) * in_width + 0]);
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
		LPCOLORREF writeTo = &((*out_sumsads)[y * in_width + 0]);
		LPCOLORREF readFrom2 = &((*out_sumsads)[(y + 16) * in_width + 0]);
		LPCOLORREF readFrom3 = &((*out_sumsads)[(y + 32) * in_width + 0]);
		LPCOLORREF readFrom4 = &((*out_sumsads)[(y + 48) * in_width + 0]);
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
	MY_FREE(sss->sumSAD16x16);
	MY_FREE(sss->sumSAD32x32);
	MY_FREE(sss->sumSAD64x64);
}

void ComputeSADSumScreenshot(LPCOLORREF pixels, int in_width, int in_height, SADSumStoreScreenshot* out_sumsads)
{
	size_t stride = in_width;

	out_sumsads->sumSAD16x16 = (LPCOLORREF)MY_ALLOC(in_width * in_height * sizeof(LPCOLORREF));
	out_sumsads->sumSAD32x32 = (LPCOLORREF)MY_ALLOC(in_width * in_height * sizeof(LPCOLORREF));
	out_sumsads->sumSAD64x64 = (LPCOLORREF)MY_ALLOC(in_width * in_height * sizeof(LPCOLORREF));

	// last 64 columns / rows would contain bad data
	in_width -= 64;
	in_height -= 64;

	// first we process just collumns
	for (size_t y = 0; y < in_height; y += 1)
	{
		unsigned char* pixels2 = (unsigned char*)&pixels[y * stride];
		LPCOLORREF writeTo = &(out_sumsads->sumSAD16x16[y * stride + 0]);
		writeTo[0] = 0;
		for (size_t tx = 0; tx < 16 * 4; tx += 4) // sum 16 pixels
		{
			writeTo[0] += pixels2[tx + 0] + pixels2[tx + 1] + pixels2[tx + 2];
		}
		for (size_t x = 1; x < in_width; x += 1)
		{
			// from previous sum of colors, substract the first column and add the next column
			writeTo[1] = writeTo[0]
				- (pixels2[0] + pixels2[1] + pixels2[2]) // substract first
				+ (pixels2[16 * 4 + 0] + pixels2[16 * 4 + 1] + pixels2[16 * 4 + 2]); // add next
			pixels2 += 4;
			writeTo += 1;
		}
	}

	// from 16x1 to 16x4
	for (size_t y = 0; y < in_height; y += 1)
	{
		LPCOLORREF writeTo = &(out_sumsads->sumSAD16x16[y * stride + 0]);
		LPCOLORREF readFrom2 = &(out_sumsads->sumSAD16x16[(y + 1) * stride + 0]);
		LPCOLORREF readFrom3 = &(out_sumsads->sumSAD16x16[(y + 2) * stride + 0]);
		LPCOLORREF readFrom4 = &(out_sumsads->sumSAD16x16[(y + 3) * stride + 0]);
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
		LPCOLORREF writeTo = &(out_sumsads->sumSAD16x16[y * stride + 0]);
		LPCOLORREF readFrom2 = &(out_sumsads->sumSAD16x16[(y + 4) * stride + 0]);
		LPCOLORREF readFrom3 = &(out_sumsads->sumSAD16x16[(y + 8) * stride + 0]);
		LPCOLORREF readFrom4 = &(out_sumsads->sumSAD16x16[(y + 12) * stride + 0]);
		for (size_t x = 0; x < in_width; x += 1)
		{
			*writeTo = *writeTo + *readFrom2 + *readFrom3 + *readFrom4;
			writeTo++;
			readFrom2++;
			readFrom3++;
			readFrom4++;
		}
	}

	// from 16x16 to 32x32
	for (size_t y = 0; y < in_height; y += 1)
	{
		LPCOLORREF writeTo = &(out_sumsads->sumSAD32x32[y * stride + 0]);
		LPCOLORREF readFrom1 = &(out_sumsads->sumSAD16x16[(y + 0) * stride + 0]);
		LPCOLORREF readFrom2 = &(out_sumsads->sumSAD16x16[(y + 0) * stride + 16]);
		LPCOLORREF readFrom3 = &(out_sumsads->sumSAD16x16[(y + 16) * stride + 0]);
		LPCOLORREF readFrom4 = &(out_sumsads->sumSAD16x16[(y + 16) * stride + 16]);
		for (size_t x = 0; x < in_width; x += 1)
		{
			*writeTo = *readFrom1 + *readFrom2 + *readFrom3 + *readFrom4;
			writeTo++;
			readFrom1++;
			readFrom2++;
			readFrom3++;
			readFrom4++;
		}
	}

	// from 32x32 to 64x64
	for (size_t y = 0; y < in_height; y += 1)
	{
		LPCOLORREF writeTo = &(out_sumsads->sumSAD64x64[y * stride + 0]);
		LPCOLORREF readFrom1 = &(out_sumsads->sumSAD32x32[(y + 0) * stride + 0]);
		LPCOLORREF readFrom2 = &(out_sumsads->sumSAD32x32[(y + 0) * stride + 32]);
		LPCOLORREF readFrom3 = &(out_sumsads->sumSAD32x32[(y + 32) * stride + 0]);
		LPCOLORREF readFrom4 = &(out_sumsads->sumSAD32x32[(y + 32) * stride + 32]);
		for (size_t x = 0; x < in_width; x += 1)
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

void InitSADSUMCached(SADSumStoreCached* sss)
{
	memset(sss, 0, sizeof(SADSumStoreCached));
}

void FreeSADSUMCached(SADSumStoreCached* sss)
{
}

void ComputeSADSumCached(LPCOLORREF pixels, int in_width, int in_height, SADSumStoreCached* out_sumsads)
{
	size_t stride = in_width;
	if (in_width >= 64 && in_height >= 64)
	{
		in_width = 64;
		in_height = 64;
		out_sumsads->widt_height = 64;
	}
	else if (in_width >= 32 && in_height >= 32)
	{
		in_width = 32;
		in_height = 32;
		out_sumsads->widt_height = 32;
	}
	else if (in_width >= 16 && in_height >= 16)
	{
		in_width = 16;
		in_height = 16;
		out_sumsads->widt_height = 16;
	}
	out_sumsads->sumSAD = 0;
	for (size_t y = 0; y < in_height; y++)
	{
		unsigned char *readFrom = (unsigned char*)&(pixels[(y + 0) * stride + 0]);
		for (size_t x = 0; x < in_width; x++)
		{
			out_sumsads->sumSAD += readFrom[0] + readFrom[1] + readFrom[2];
			readFrom += 4;
		}
	}
}

char* WINAPI ImageSearch_SAD_Limit(char* aImageFile, int SAD_Limit)
{
	// if you do not wish to use sad limit, you should not use this function :P
	if (SAD_Limit < 0)
	{
		return ImageSearch_SAD(aImageFile);
	}

	size_t startStamp = GetTickCount();

	CachedPicture* cache = CachePicture(aImageFile);
	if (cache == NULL)
	{
		FileDebug("Skipping Image search as image could not be loaded");
		return "";
	}

	// not yet initialized ?
	if (cache->SADSums.widt_height == 0)
	{
		ComputeSADSumCached(cache->Pixels, cache->Width, cache->Height, &cache->SADSums);
	}

	size_t big_width = CurScreenshot->GetWidth();
	size_t big_height = CurScreenshot->GetHeight();
	size_t stride = big_width;
	if (CurScreenshot->SADSums.sumSAD16x16 == NULL)
	{
		ComputeSADSumScreenshot(CurScreenshot->Pixels, big_width, big_height, &CurScreenshot->SADSums);
	}

	int retx = -1;
	int rety = -1;
	int bestSAD = 0x7FFFFFFF;
	int bestSumSAD = 0x7FFFFFFF;
	LPCOLORREF bigSADSums = NULL;

	if (cache->SADSums.widt_height == 16)
	{
		big_height -= 16;
		big_width -= 16;
		bigSADSums = CurScreenshot->SADSums.sumSAD16x16;
	}
	else if (cache->SADSums.widt_height == 32)
	{
		big_height -= 32;
		big_width -= 32;
		bigSADSums = CurScreenshot->SADSums.sumSAD32x32;
	}
	else if (cache->SADSums.widt_height == 64)
	{
		big_height -= 64;
		big_width -= 64;
		bigSADSums = CurScreenshot->SADSums.sumSAD64x64;
	}

#if defined( _DEBUG ) && defined(_CONSOLE)
	size_t improved_result = 0;
	size_t sadCalcs = 0;
#endif
//	const int uncoveredAreaSAD = (cache->Width - cache->SADSums.widt_height) * (cache->Height - cache->SADSums.widt_height) * (255 + 255 + 255);
	const int small_SAD = cache->SADSums.sumSAD;
	int SAD_MIN = (int)small_SAD - (int)SAD_Limit;
	int SAD_MAX = small_SAD + SAD_Limit;
	for (size_t y = 0; y < big_height; y++)
	{
		LPCOLORREF bigSADSumsRow = &bigSADSums[y * stride];
		for (size_t x = 0; x < big_width; x++)
		{
			if (SAD_MIN <= (int)(*bigSADSumsRow) && (int)(*bigSADSumsRow) <= SAD_MAX)
			{
				size_t sadSums = abs((int)small_SAD - (int)*bigSADSumsRow);
				if (sadSums < bestSumSAD)
				{
					// get the real sad at this point
					int sadAlmostPrecise = ImgSAD(CurScreenshot->Pixels, CurScreenshot->GetWidth(), CurScreenshot->GetHeight(), CurScreenshot->GetWidth(),
						cache->Pixels, cache->Width, cache->Height, cache->Width,
						x, y);
#if defined( _DEBUG ) && defined(_CONSOLE)
					sadCalcs++;
#endif
					if (sadAlmostPrecise < bestSAD && sadAlmostPrecise <= SAD_Limit)
					{
						// these 2 will not be the same because SumSAD only captures a portion of the cached image
						// SumSAD should be smaller than bestSAD
						bestSAD = sadAlmostPrecise;
//						bestSumSAD = sadSums + uncoveredAreaSAD;
						bestSumSAD = sadSums;
						SAD_MIN = small_SAD - sadSums;
						SAD_MAX = small_SAD + sadSums;
						retx = x;
						rety = y;
#if defined( _DEBUG ) && defined(_CONSOLE)
						improved_result++;
#endif
					}
				}
			}
			bigSADSumsRow++;
		}
	}

	ReturnBuff[0] = 0;
	sprintf_s(ReturnBuff, DEFAULT_STR_BUFFER_SIZE * 10, "1|%d|%d|%d", retx, rety, bestSAD);
	size_t endStamp = GetTickCount();

#if defined( _DEBUG ) && defined(_CONSOLE)
	char dbgmsg[DEFAULT_STR_BUFFER_SIZE];
	if (rety == -1) { rety = 0; retx = 0; }
	unsigned char* AddrBig = (unsigned char*)&CurScreenshot->Pixels[rety * stride + retx];
	unsigned char* AddSmall = (unsigned char*)&cache->Pixels[0];
	sprintf_s(dbgmsg, sizeof(dbgmsg), "\tImage search finished in %d ms. Name %s. Sad calcs %d, Improved match %d. Returning %s . pixel 0 0 : %d %d %d %d - %d %d %d %d",
		(int)(endStamp - startStamp), cache->FileName, (int)sadCalcs, (int)improved_result, ReturnBuff, AddrBig[0], AddrBig[1], AddrBig[2], AddrBig[3], AddSmall[0], AddSmall[1], AddSmall[2], AddSmall[3]);
	FileDebug(dbgmsg);
#endif

	return ReturnBuff;
}