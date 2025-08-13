#pragma once

// do a sad between 2 images
// width / height is from the smaller image
// !!! width needs to be truncated to a multiple of 8 !!!
inline uint64_t ImageSad(const LPCOLORREF pixels1, const size_t stride1, const LPCOLORREF pixels2, const size_t stride2, const size_t width, const size_t height)
{
#if defined(_DEBUG) || defined(_CONSOLE)
	if (((width/8) * 8) != width)
	{
		assert(false);
	}
#endif
	__m256i acc_sad = _mm256_setzero_si256();

	COLORREF * __restrict AddrBig = pixels1;
	COLORREF * __restrict AddrSmall = pixels2;
	for (size_t y2 = 0; y2 < (size_t)height; ++y2)
	{
		for (size_t x2 = 0; x2 < width; x2 += 8) // prcess 8 pixels each 4 bytes => 4*8=32 bytes
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

inline uint64_t ImageSadNoSSE(const LPCOLORREF pixels1, const size_t stride1, const LPCOLORREF pixels2, const size_t stride2, const size_t width, const size_t height)
{
	uint64_t acc_sad = 0;

	size_t RoundedWidth = width & (~7);
	for (size_t y2 = 0; y2 < (size_t)height; y2++)
	{
		const unsigned char* AddrBig = (unsigned char*)&pixels1[y2 * stride1];
		const unsigned char* AddrSmall = (unsigned char*)&pixels2[y2 * stride2];
		for (size_t x2 = 0; x2 < RoundedWidth * 4; x2 += 4)
		{
			acc_sad += abs(AddrBig[0] - AddrSmall[0]);
			acc_sad += abs(AddrBig[1] - AddrSmall[1]);
			acc_sad += abs(AddrBig[2] - AddrSmall[2]);
			AddrBig += 4;
			AddrSmall += 4;
		}
	}

	return acc_sad;
}

/*
* 10 01
* 01 10
*/
template <int StageNr>
inline uint64_t ImageSad2Stage_(const LPCOLORREF pixels1, size_t stride1,
	const LPCOLORREF pixels2, size_t stride2,
	size_t width, size_t height)
{
	static_assert( StageNr >= 0 && StageNr <= 1);
	__m256i acc_sad = _mm256_setzero_si256();

	const COLORREF* __restrict AddrBig = pixels1;
	const COLORREF* __restrict AddrSmall = pixels2;
	for (size_t y2 = 0; y2 < (size_t)height; y2 += 1)
	{
		size_t x2;
		if constexpr (StageNr == 0) {
			x2 = (y2 & 1) * 8;
		}
		else if constexpr (StageNr == 1) {
			x2 = ((~y2) & 1) * 8;
		}
		for (; x2 < width; x2 += 16) // process 8 pixels each 4 bytes => 4*8=32 bytes
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

inline uint64_t ImageSad2Stage(const LPCOLORREF pixels1, size_t stride1,
	const LPCOLORREF pixels2, size_t stride2,
	size_t width, size_t height, uint64_t* stage_sads)
{
#if defined(_DEBUG) || defined(_CONSOLE)
	assert((width % 8) == 0);
#endif
	uint64_t sad0 = ImageSad2Stage_<0>(pixels1, stride1, pixels2, stride2, width, height);
	if (sad0 < stage_sads[0]) {
		uint64_t sad1 = sad0 + ImageSad2Stage_<1>(pixels1, stride1, pixels2, stride2, width, height);
		if (sad1 < stage_sads[1]) {
			stage_sads[0] = sad0;
			stage_sads[1] = sad1;
			return sad1;
		}
	}
	return ~(uint64_t)0;
}

// Quarter = 0..3 -> quarter partitions as described above
/*
*	4 stages
1010	0000	0101	0000
0000	1010	0000	0101
1010	0000	0101	0000
0000	1010	0000	0101
*/
template <int Quarter>
inline uint64_t ImageSad4Stage_(const LPCOLORREF pixels1, size_t stride1,
	const LPCOLORREF pixels2, size_t stride2,
	size_t width, size_t height)
{
	__m256i acc_sad = _mm256_setzero_si256();

	const COLORREF* __restrict AddrBig;
	const COLORREF* __restrict AddrSmall;
	if constexpr (Quarter == 0) {
		AddrBig = pixels1;
		AddrSmall = pixels2;
	}
	else if constexpr (Quarter == 1) {
		AddrBig = pixels1 + stride1;
		AddrSmall = pixels2 + stride2;
		height -= 1;
	}
	else if constexpr (Quarter == 2) {
		AddrBig = pixels1 + 8;
		AddrSmall = pixels2 + 8;
		width -= 8;
	}
	else if constexpr (Quarter == 3) {
		AddrBig = pixels1 + 8 + stride1;
		AddrSmall = pixels2 + 8 + stride2;
		height -= 1;
		width -= 8;
	}
	for (size_t y2 = 0; y2 < (size_t)height; y2 += 2)
	{
		for (size_t x2 = 0; x2 < width; x2 += 16) // process 8 pixels each 4 bytes => 4*8=32 bytes
		{
			const __m256i l0 = _mm256_loadu_si256((__m256i*)(&AddrBig[x2]));
			const __m256i l1 = _mm256_loadu_si256((__m256i*)(&AddrSmall[x2]));
			const __m256i line_sad = _mm256_sad_epu8(l0, l1);
			acc_sad = _mm256_add_epi64(acc_sad, line_sad);
		}
		AddrBig += stride1 * 2;
		AddrSmall += stride2 * 2;
	}

	alignas(32) uint64_t sums[4];
	_mm256_store_si256(reinterpret_cast<__m256i*>(sums), acc_sad);
	return sums[0] + sums[1] + sums[2] + sums[3];
}

inline uint64_t ImageSad4Stage(const LPCOLORREF pixels1, size_t stride1,
	const LPCOLORREF pixels2, size_t stride2,
	size_t width, size_t height, uint64_t* stage_sads)
{
#if defined(_DEBUG) || defined(_CONSOLE)
	assert((width % 8) == 0);
#endif
	uint64_t sad0 = ImageSad4Stage_<0>(pixels1, stride1, pixels2, stride2, width, height);
	if (sad0 < stage_sads[0]) {
		uint64_t sad1 = sad0 + ImageSad4Stage_<1>(pixels1, stride1, pixels2, stride2, width, height);
		if (sad1 < stage_sads[1]) {
			uint64_t sad2 = sad1 + ImageSad4Stage_<2>(pixels1, stride1, pixels2, stride2, width, height);
			if (sad2 < stage_sads[2]) {
				uint64_t sad3 = sad2 + ImageSad4Stage_<3>(pixels1, stride1, pixels2, stride2, width, height);
				if (sad3 < stage_sads[3]) {
					stage_sads[0] = sad0;
					stage_sads[1] = sad1;
					stage_sads[2] = sad2;
					stage_sads[3] = sad3;
					return sad3;
				}
			}
		}
	}
	return ~(uint64_t)0;
}

// nines = 0..8 -> partitions as described below
/*
*	9 stages
100100 000000 000000 | 010010 000000 000000	| 001001 000000	000000
000000 010010 000000 | 000000 001001 000000	| 000000 100100	000000
000000 000000 001001 | 000000 000000 010010	| 000000 000000	001001
001001 000000 000000 | 010010 000000 000000	| 001001 000000	000000
000000 010010 000000 | 000000 001001 000000	| 000000 010010	000000
000000 000000 001001 | 000000 000000 010010	| 000000 000000	100100
*/
template <int nines>
inline uint64_t ImageSad9Stage_(const LPCOLORREF pixels1, size_t stride1,
	const LPCOLORREF pixels2, size_t stride2,
	size_t width, size_t height)
{
	static_assert(nines >= 0 && nines < 9); // you should not call this function with other settings

	__m256i acc_sad = _mm256_setzero_si256();
	const COLORREF* __restrict AddrBig;
	const COLORREF* __restrict AddrSmall;
	if constexpr (nines == 0) {
		AddrBig = pixels1;
		AddrSmall = pixels2;
	}
	else if constexpr (nines == 1) {
		AddrBig = pixels1 + 8 + stride1;
		AddrSmall = pixels2 + 8 + stride2;
		width -= 8;
		height -= 1;
	}
	else if constexpr (nines == 2) {
		AddrBig = pixels1 + 16 + stride1 * 2;
		AddrSmall = pixels2 + 16 + stride2 * 2;
		width -= 16;
		height -= 2;
	}

	else if constexpr (nines == 3) {
		AddrBig = pixels1 + 8;
		AddrSmall = pixels2 + 8;
		width -= 8;
	}
	else if constexpr (nines == 4) {
		AddrBig = pixels1 + 16 + stride1;
		AddrSmall = pixels2 + 16 + stride2;
		width -= 16;
		height -= 1;
	}
	else if constexpr (nines == 5) {
		AddrBig = pixels1 + stride1 * 2;
		AddrSmall = pixels2 + stride2 * 2;
		height -= 2;
	}

	else if constexpr (nines == 6) {
		AddrBig = pixels1 + 16;
		AddrSmall = pixels2 + 16;
		width -= 16;
	}
	else if constexpr (nines == 7) {
		AddrBig = pixels1 + stride1;
		AddrSmall = pixels2 + stride2;
		height -= 1;
	}
	else if constexpr (nines == 8) {
		AddrBig = pixels1 + 8 + stride1 * 2;
		AddrSmall = pixels2 + 8 + stride2 * 2;
		height -= 2;
		width -= 8;
	}
	const size_t stride1_ = stride1 * 3;
	const size_t stride2_ = stride2 * 3;
	for (size_t y2 = 0; y2 < height; y2 += 3)
	{
		for (size_t x2 = 0; x2 < width; x2 += 24)
		{
			const __m256i l0 = _mm256_loadu_si256((__m256i*)(&AddrBig[x2]));
			const __m256i l1 = _mm256_loadu_si256((__m256i*)(&AddrSmall[x2]));
			const __m256i line_sad = _mm256_sad_epu8(l0, l1);
			acc_sad = _mm256_add_epi64(acc_sad, line_sad);
		}
		AddrBig += stride1_;
		AddrSmall += stride2_;
	}

	alignas(32) uint64_t sums[4];
	_mm256_store_si256(reinterpret_cast<__m256i*>(sums), acc_sad);
	return sums[0] + sums[1] + sums[2] + sums[3];
}

inline uint64_t ImageSad9Stage(const LPCOLORREF pixels1, size_t stride1,
	const LPCOLORREF pixels2, size_t stride2,
	size_t width, size_t height, uint64_t*stage_sads)
{
#if defined(_DEBUG) || defined(_CONSOLE)
	assert((width % 8) == 0);
#endif
	uint64_t sad[9];
	sad[0] = ImageSad9Stage_<0>(pixels1, stride1, pixels2, stride2, width, height);
	if (sad[0] > stage_sads[0]) {
		return ~(uint64_t)0;
	}
	sad[1] = sad[0] + ImageSad9Stage_<1>(pixels1, stride1, pixels2, stride2, width, height);
	if (sad[1] > stage_sads[1]) {
		return ~(uint64_t)0;
	}
	sad[2] = sad[1] + ImageSad9Stage_<2>(pixels1, stride1, pixels2, stride2, width, height);
	if (sad[2] > stage_sads[2]) {
		return ~(uint64_t)0;
	}
	sad[3] = sad[2] + ImageSad9Stage_<3>(pixels1, stride1, pixels2, stride2, width, height);
	if (sad[3] > stage_sads[3]) {
		return ~(uint64_t)0;
	}
	sad[4] = sad[3] + ImageSad9Stage_<4>(pixels1, stride1, pixels2, stride2, width, height);
	if (sad[4] > stage_sads[4]) {
		return ~(uint64_t)0;
	}
	sad[5] = sad[4] + ImageSad9Stage_<5>(pixels1, stride1, pixels2, stride2, width, height);
	if (sad[5] > stage_sads[5]) {
		return ~(uint64_t)0;
	}
	sad[6] = sad[5] + ImageSad9Stage_<6>(pixels1, stride1, pixels2, stride2, width, height);
	if (sad[6] > stage_sads[6]) {
		return ~(uint64_t)0;
	}
	sad[7] = sad[6] + ImageSad9Stage_<7>(pixels1, stride1, pixels2, stride2, width, height);
	if (sad[7] > stage_sads[7]) {
		return ~(uint64_t)0;
	}
	sad[8] = sad[7] + ImageSad9Stage_<8>(pixels1, stride1, pixels2, stride2, width, height);
	if (sad[8] > stage_sads[8]) {
		return ~(uint64_t)0;
	}
	memcpy(stage_sads, sad, sizeof(uint64_t) * 9);
	return sad[8];
}

// this is the same, but it's here due to code cleanup
unsigned int ImgSAD(LPCOLORREF bigImg, size_t bigWidth, size_t bigHeight, size_t bigStride,
	LPCOLORREF smallImg, size_t smallWidth, size_t smallHeight, size_t smallStride,
	size_t atX, size_t atY);
unsigned int GetSADAtPos2(const unsigned char* RGB1, const int Widthx3, const unsigned char* A2, const unsigned char* RGB2, const int CacheWidthx3, const int CacheStride, const int CacheHeight);
