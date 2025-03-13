#pragma once


uint32_t satd_8x8_RGBA_SSE41(const uint8_t* src, const uint8_t* ref, int stride);
uint32_t satd_8x8_rgba_sse(const uint8_t* src, const uint8_t* ref, int stride);

uint32_t satd_8x8_rgba_avx2(const uint8_t* src, const uint8_t* ref, int stride);

uint32_t satd_8x8_rgba_avx512(const uint8_t* src, const uint8_t* ref, int stride);

size_t satd_8x8_rgb_scalar(const uint8_t* src, const uint8_t* ref, size_t stride_src, size_t stride_dst);
inline size_t satd_nxm(const LPCOLORREF src, const LPCOLORREF ref, size_t width, size_t height, size_t stride_src, size_t stride_dst)
{
	height = (height / 8) * 8;
	width = (width / 8) * 8;
	size_t satd_sum = 0;
	for (size_t row = 0; row < height; row += 8)
	{
		for (size_t col = 0; col < width; col += 8)
		{
			satd_sum += satd_8x8_rgb_scalar((uint8_t * )(src + col + row * stride_src), 
				(uint8_t *)(ref + col + row * stride_dst), stride_src, stride_dst);
		}
	}
	return satd_sum;
}
