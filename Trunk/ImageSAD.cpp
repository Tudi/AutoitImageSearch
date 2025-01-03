#include "stdafx.h"

unsigned int ImgSAD(LPCOLORREF bigImg, size_t bigWidth, size_t bigHeight, size_t bigStride,
	LPCOLORREF smallImg, size_t smallWidth, size_t smallHeight, size_t smallStride,
	size_t atX, size_t atY)
{
	// can't compute SAD if over the limit
	if (atY + smallHeight > bigHeight)
	{
		return 0x7FFFFFFF;
	}
	if (atX + smallWidth > bigWidth)
	{
		return 0x7FFFFFFF;
	}

	smallWidth = smallWidth - 8; // or else SSE instruction would read out of bounds !!
	unsigned int sad_array[8];
	__m256i l0, l1, line_sad, acc_sad;

	acc_sad = _mm256_setzero_si256();

	for (size_t y2 = 0; y2 < smallHeight; y2++)
	{
		LPCOLORREF AddrBig = &bigImg[(atY + y2) * bigStride + atX];
		LPCOLORREF AddSmall = &smallImg[(0 + y2) * smallStride + 0];
		for (size_t x2 = 0; x2 < smallWidth; x2 += 8) // prcess 8 pixels => 4*8=32 bytes
		{
			l0 = _mm256_loadu_si256((__m256i*)(&AddrBig[x2]));
			l1 = _mm256_loadu_si256((__m256i*)(&AddSmall[x2]));
			line_sad = _mm256_sad_epu8(l0, l1);
			acc_sad = _mm256_add_epi32(acc_sad, line_sad);
		}
	}

	_mm256_storeu_si256((__m256i*)(&sad_array[0]), acc_sad);

	unsigned int sad = sad_array[0] + sad_array[2] + sad_array[4] + sad_array[6];

	return sad;
}

unsigned int GetSADAtPos2(const unsigned char* RGB1, const int Widthx3, const unsigned char* A2, const unsigned char* RGB2, const int CacheWidthx3, const int CacheStride, const int CacheHeight)
{
	unsigned int sad_array[4];
	__m128i l0, l1, line_sad, acc_sad, alpha_mask;
	acc_sad = _mm_setzero_si128();

	for (int y2 = 0; y2 < CacheHeight; y2++)
	{
		for (int x2 = 0; x2 < CacheWidthx3; x2 += 16) // 5 pixels at a time
		{
			// load the alpha mask
			alpha_mask = _mm_load_si128((__m128i*)(A2 + x2));

			// load 16 bytes of RGB values
			l0 = _mm_loadu_si128((__m128i*)(RGB1 + x2));
			l1 = _mm_load_si128((__m128i*)(RGB2 + x2)); // we could skip 1 byte to make an alligned read ?
			// apply mask to not count transparent values
			l0 = _mm_and_si128(l0, alpha_mask);
			l1 = _mm_and_si128(l1, alpha_mask);
			// sad RGB values
			line_sad = _mm_sad_epu8(l0, l1);
			// add to acc
			acc_sad = _mm_add_epi32(acc_sad, line_sad);
		}

		RGB1 += Widthx3;
		A2 += CacheStride;
		RGB2 += CacheStride;
	}

	_mm_storeu_si128((__m128i*)(&sad_array[0]), acc_sad);

	unsigned int sad = sad_array[0] + sad_array[2];
	return sad;
}
