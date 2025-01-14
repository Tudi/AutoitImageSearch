#pragma once

// do a sad between 2 images
// width / height is from the smaller image
// !!! width needs to be truncated to a multiple of 8 !!!
inline uint64_t ImageSad(const LPCOLORREF pixels1, const size_t stride1, const LPCOLORREF pixels2, const size_t stride2, const size_t width, const size_t height)
{
	__m256i acc_sad = _mm256_setzero_si256();

	for (size_t y2 = 0; y2 < (size_t)height; y2++)
	{
		const LPCOLORREF AddrBig = &pixels1[y2 * stride1];
		const LPCOLORREF AddSmall = &pixels2[y2 * stride2];
		for (size_t x2 = 0; x2 < width; x2 += 8) // prcess 8 pixels each 4 bytes => 4*8=32 bytes
		{
			const __m256i l0 = _mm256_loadu_si256((__m256i*)(&AddrBig[x2]));
			const __m256i l1 = _mm256_loadu_si256((__m256i*)(&AddSmall[x2]));
			__m256i line_sad = _mm256_sad_epu8(l0, l1);
			acc_sad = _mm256_add_epi32(acc_sad, line_sad);
		}
	}

	unsigned int sad_array[8]; // 8 is enough
	_mm256_storeu_si256((__m256i*)(&sad_array[0]), acc_sad);

	uint64_t sad = sad_array[0] + sad_array[2] + sad_array[4] + sad_array[6];
	return sad;
}

inline uint64_t ImageSadNoSSE(const LPCOLORREF pixels1, const size_t stride1, const LPCOLORREF pixels2, const size_t stride2, const size_t width, const size_t height)
{
	uint64_t acc_sad = 0;

	size_t RoundedWidth = width & (~7);
	for (size_t y2 = 0; y2 < (size_t)height; y2++)
	{
		const unsigned char* AddrBig = (unsigned char*)&pixels1[y2 * stride1];
		const unsigned char* AddSmall = (unsigned char*)&pixels2[y2 * stride2];
		for (size_t x2 = 0; x2 < RoundedWidth * 4; x2 += 4)
		{
			acc_sad += abs(AddrBig[0] - AddSmall[0]);
			acc_sad += abs(AddrBig[1] - AddSmall[1]);
			acc_sad += abs(AddrBig[2] - AddSmall[2]);
			AddrBig += 4;
			AddSmall += 4;
		}
	}

	return acc_sad;
}

// this is the same, but it's here due to code cleanup
unsigned int ImgSAD(LPCOLORREF bigImg, size_t bigWidth, size_t bigHeight, size_t bigStride,
	LPCOLORREF smallImg, size_t smallWidth, size_t smallHeight, size_t smallStride,
	size_t atX, size_t atY);
unsigned int GetSADAtPos2(const unsigned char* RGB1, const int Widthx3, const unsigned char* A2, const unsigned char* RGB2, const int CacheWidthx3, const int CacheStride, const int CacheHeight);
