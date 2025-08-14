#pragma once

// do a sad between 2 images
// width / height is from the smaller image
// !!! width needs to be truncated to a multiple of 8 !!!
inline uint64_t ImageSadGrayScale(const uint8_t *pixels1, const size_t stride1, const uint8_t *pixels2, const size_t stride2, const size_t width, const size_t height)
{
	__m256i acc_sad = _mm256_setzero_si256();

	const uint8_t* __restrict AddrBig = pixels1;
	const uint8_t* __restrict AddrSmall = pixels2;
	for (size_t y2 = 0; y2 < (size_t)height; ++y2)
	{
		for (size_t x2 = 0; x2 < width; x2 += 32) // process 32 pixels
		{
			const __m256i l0 = _mm256_loadu_si256((__m256i*)(&AddrBig[x2]));
			const __m256i l1 = _mm256_loadu_si256((__m256i*)(&AddrSmall[x2]));
			__m256i line_sad = _mm256_sad_epu8(l0, l1);
			acc_sad = _mm256_add_epi64(acc_sad, line_sad);
		}
		AddrBig += stride1;
		AddrSmall += stride2;
	}

	alignas(32) uint64_t sums[4];
	_mm256_store_si256(reinterpret_cast<__m256i*>(sums), acc_sad);
	return sums[0] + sums[1] + sums[2] + sums[3];
}

/*
* 10 01
* 01 10
*/
template <int StageNr>
inline uint64_t ImageSad2StageGrayScale_(const uint8_t *pixels1, size_t stride1,
	const uint8_t *pixels2, size_t stride2,
	size_t width, size_t height)
{
	static_assert(StageNr >= 0 && StageNr <= 1);
	__m256i acc_sad = _mm256_setzero_si256();

	const uint8_t* __restrict AddrBig = pixels1;
	const uint8_t* __restrict AddrSmall = pixels2;
	for (size_t y2 = 0; y2 < (size_t)height; y2 += 1)
	{
		size_t x2;
		if constexpr (StageNr == 0) {
			x2 = (y2 & 1) * 32;
		}
		else if constexpr (StageNr == 1) {
			x2 = ((~y2) & 1) * 32;
		}
		for (; x2 < width; x2 += 64) // process 32 pixels + skip 32
		{
			const __m256i l0 = _mm256_loadu_si256((__m256i*)(&AddrBig[x2]));
			const __m256i l1 = _mm256_loadu_si256((__m256i*)(&AddrSmall[x2]));
			const __m256i line_sad = _mm256_sad_epu8(l0, l1);
			acc_sad = _mm256_add_epi64(acc_sad, line_sad);
		}
		AddrBig += stride1;
		AddrSmall += stride2;
	}

	alignas(32) uint64_t sums[4];
	_mm256_store_si256(reinterpret_cast<__m256i*>(sums), acc_sad);
	return sums[0] + sums[1] + sums[2] + sums[3];
}

inline uint64_t ImageSad2StageGrayScale(const uint8_t *pixels1, size_t stride1,
	const uint8_t *pixels2, size_t stride2,
	size_t width, size_t height, uint64_t* stage_sads)
{
	const size_t width_64 = width & (~(size_t)63);
	uint64_t sad0 = ImageSad2StageGrayScale_<0>(pixels1, stride1, pixels2, stride2, width_64, height);
	if (sad0 < stage_sads[0]) {
		uint64_t sad1 = sad0 + ImageSad2StageGrayScale_<1>(pixels1, stride1, pixels2, stride2, width_64, height);
		if (sad1 < stage_sads[1]) {
			stage_sads[0] = sad0;
			stage_sads[1] = sad1;
			return sad1;
		}
	}
	return ~(uint64_t)0;
}

inline uint64_t ImageSadGrayScale_s(const ScreenshotStruct* ss, const CachedPicture* cache, const size_t x, const size_t y, 
	const size_t width, const size_t height, uint64_t* stage_sads) {
	const size_t stride1 = ss->Width;
	const uint8_t *pixels1 = &ss->pGrayscalePixels[y * stride1 + x];
	const size_t stride2 = cache->Width;
	const uint8_t* pixels2 = &cache->pGrayscalePixels[0];
	const size_t width_32 = width & (~(size_t)31);
	uint64_t stage_1 = ImageSadGrayScale(pixels1, stride1, pixels2, stride2, width_32, height);
	if (stage_1 < stage_sads[0]) {
		const LPCOLORREF pixels11 = &ss->Pixels[y * stride1 + x];
		const LPCOLORREF pixels22 = &cache->Pixels[0];
		uint64_t stage_2 = ImageSad(pixels11, stride1, pixels22, stride2, width, height);
		if (stage_2 < stage_sads[1]) {
			stage_sads[0] = stage_1;
			stage_sads[1] = stage_2;
			return stage_2;
		}
	}
	return ~(uint64_t)0;
}

inline uint64_t ImageSad2StageGrayScale_s(const ScreenshotStruct* ss, const CachedPicture* cache, const size_t x, const size_t y,
	const size_t width, const size_t height, uint64_t* stage_sads) {
	const size_t stride1 = ss->Width;
	const uint8_t* pixels1 = &ss->pGrayscalePixels[y * stride1 + x];
	const size_t stride2 = cache->Width;
	const uint8_t* pixels2 = &cache->pGrayscalePixels[0];
	uint64_t stage_1 = ImageSad2StageGrayScale(pixels1, stride1, pixels2, stride2, width, height, stage_sads);
	if (stage_1 != (~(uint64_t)0)) { // means we found a better match than previously
		const LPCOLORREF pixels11 = &ss->Pixels[y * stride1 + x];
		const LPCOLORREF pixels22 = &cache->Pixels[0];
		uint64_t stage_2 = ImageSad(pixels11, stride1, pixels22, stride2, width, height);
		if (stage_2 < stage_sads[2]) {
			stage_sads[2] = stage_2;
			return stage_2;
		}
	}
	return ~(uint64_t)0;
}