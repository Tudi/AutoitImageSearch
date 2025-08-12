#pragma once

// just to test that output matches with expected
size_t satd_8x8_rgb_reference(const uint8_t* src, const uint8_t* ref, size_t stride_src, size_t stride_dst);

size_t satd_8x8_rgb_avx2_v1(const uint8_t* src, const uint8_t* ref, size_t stride_src, size_t stride_dst);
// don't try to use this. It's unfinished and not sure if it will be worth finishing ever
size_t satd_8x8_rgb_avx2_v2(const uint8_t* src, const uint8_t* ref, size_t stride_src, size_t stride_dst);

inline size_t satd_nxm(const LPCOLORREF src, const LPCOLORREF ref, size_t width, size_t height, size_t stride_src, size_t stride_dst, size_t abort_if_larger = ~0)
{
	height = (height / 8) * 8;
	width = (width / 8) * 8;
	const size_t byte_stride_src = stride_src * 4; // param value is in pixels
	const size_t byte_stride_dst = stride_dst * 4; // param value is in pixels
	size_t satd_sum = 0;
	for (size_t row = 0; row < height; row += 8)
	{
		for (size_t col = 0; col < width; col += 8)
		{
			satd_sum += satd_8x8_rgb_avx2_v1((uint8_t * )(src + row * stride_src + col),
				(uint8_t *)(ref + row * stride_dst + col), byte_stride_src, byte_stride_dst);
			if (satd_sum >= abort_if_larger) {
				return satd_sum;
			}
		}
	}
	return satd_sum;
}
