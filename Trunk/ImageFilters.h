#pragma once

void WINAPI BlurrImage( int HalfKernelSize );
void WINAPI ErrodeDiffMap( int HalfKernelSize );
void WINAPI EdgeDetect( int HalfKernelSize );
void WINAPI ApplyColorBitmask(int Mask);
void WINAPI ApplyColorBitmaskCache(const char* aFilespec, int Mask);
void WINAPI DecreaseColorCount(unsigned int ColorsPerChannel);
void WINAPI DecreaseColorCount(unsigned int ColorsPerChannel);
void WINAPI ErrodeRegionToTransparent(int StartX = -1, int StartY = -1, int EndX = -1, int EndY = -1, int RequiredNeighbourCount = 2);
void WINAPI KeepColorRangesInRegion(int StartX = -1, int StartY = -1, int EndX = -1, int EndY = -1, int RMin = 0, int RMax = 255, int GMin = 0, int GMax = 255, int BMin = 0, int BMax = 255);

void RemoveScreenshotAlphaChannel(ScreenshotStruct *cache);
void DecreaseColorPrecision(ScreenshotStruct *cache, unsigned int Div, unsigned int And);
void DecreaseColorCount_(ScreenshotStruct *cache, unsigned int ColorsPerChannel);
void GetUniqueColorsInRegion(int StartX, int StartY, int EndX, int EndY);
LPCOLORREF BlurrImage_(int HalfKernelSize, int MiddleFactor, LPCOLORREF Pixels, int Width, int Height);
void ApplyColorBitmask_(LPCOLORREF Pixels, int Width, int Height, uint32_t Mask);


// wanted to benchmark to see if there is a difference
inline void Blurr3x3_2(const LPCOLORREF Pixels, const size_t Stride, LPCOLORREF out_pixel) {
	uint64_t ShiftedRSum = 0;
	uint64_t ShiftedGSum = 0;
	uint64_t ShiftedBSum = 0;

	const COLORREF* row0 = Pixels;
	const COLORREF* row1 = Pixels + Stride;
	const COLORREF* row2 = Pixels + 2 * Stride;
#define SUM3(p) do { \
        ShiftedBSum += (p[0] & 0x000000FF) + (p[1] & 0x000000FF) + (p[2] & 0x000000FF); \
        ShiftedGSum += (p[0] & 0x0000FF00) + (p[1] & 0x0000FF00) + (p[2] & 0x0000FF00); \
        ShiftedRSum += (p[0] & 0x00FF0000) + (p[1] & 0x00FF0000) + (p[2] & 0x00FF0000); \
    } while(0);
	SUM3(row0);
	SUM3(row1);
	SUM3(row2);
#undef SUM3
	ShiftedRSum = (ShiftedRSum / 9) & 0x00FF0000;
	ShiftedGSum = (ShiftedGSum / 9) & 0x0000FF00;
	ShiftedBSum = (ShiftedBSum / 9) & 0x000000FF;
	*out_pixel = (COLORREF)(ShiftedRSum | ShiftedGSum | ShiftedBSum);
}

// 30% times faster than Blurr3x3_2 
inline void Blurr3x3(const LPCOLORREF Pixels, const size_t Stride, LPCOLORREF out_pixel) {
	uint64_t rSum = 0;
	uint64_t gSum = 0;
	uint64_t bSum = 0;

	const uint8_t* row0 = (uint8_t*)(Pixels);
	const uint8_t* row1 = (uint8_t*)(Pixels + Stride);
	const uint8_t* row2 = (uint8_t*)(Pixels + 2 * Stride);
#define SUM3(p) do { \
        rSum += (uint64_t)p[0] + (uint64_t)p[4] + (uint64_t)p[8]; \
        gSum += (uint64_t)p[1] + (uint64_t)p[5] + (uint64_t)p[9]; \
        bSum += (uint64_t)p[2] + (uint64_t)p[6] + (uint64_t)p[10]; \
    } while(0);
	SUM3(row0);
	SUM3(row1);
	SUM3(row2);
#undef SUM3
	uint64_t r = (rSum / 9);
	uint64_t g = (gSum / 9);
	uint64_t b = (bSum / 9);
	*out_pixel = COLORREF(r | (g << 8) | (b << 16));
}

template <const __int64 HalfKernelSize, const double MiddleFactor>
LPCOLORREF BlurrImage2_(const LPCOLORREF Pixels, const size_t Width, const size_t Height, const size_t Stride)
{
	LPCOLORREF new_Pixels = (COLORREF*)MY_ALLOC(Width * Height * sizeof(COLORREF));
	if (new_Pixels == NULL)
	{
		FileDebug("Error:Could not allocate buffer for blur!");
		return NULL;
	}
	constexpr __int64 HalfKernelSize_ = HalfKernelSize;
	constexpr double MiddleFactor_ = MiddleFactor;
	if constexpr (MiddleFactor_ == 1.0 && HalfKernelSize_ == 1) {
		for (size_t y = 1; y < Height - 1; y += 1) {
			for (size_t x = 1; x < Width - 1; x += 1) {
				Blurr3x3(&Pixels[(y - 1) * Stride + x - 1], Stride, &new_Pixels[y * Width + x]);
			}
		}
	}
	else {
		const size_t CharWidth = Stride * 4;
		const size_t kernel_pixel_count = (2 * HalfKernelSize_ + 1) * (2 * HalfKernelSize_ + 1);
		const double divider = (kernel_pixel_count + MiddleFactor_ - 1.0);
		for (size_t y = HalfKernelSize_; y < Height - HalfKernelSize_; y += 1)
			for (size_t x = HalfKernelSize_; x < Width - HalfKernelSize_; x += 1)
			{
				unsigned char* RowStart = (unsigned char*)&Pixels[y * Stride + x];
				size_t SumOfValuesR = (size_t)(MiddleFactor_ * RowStart[0]);
				size_t SumOfValuesG = (size_t)(MiddleFactor_ * RowStart[1]);
				size_t SumOfValuesB = (size_t)(MiddleFactor_ * RowStart[2]);
				for (__int64 ky = -HalfKernelSize_; ky <= HalfKernelSize_; ky++)
					for (__int64 kx = -HalfKernelSize_ * 4; kx <= HalfKernelSize_ * 4; kx += 4)
					{
						SumOfValuesR += RowStart[ky * CharWidth + kx + 0];
						SumOfValuesG += RowStart[ky * CharWidth + kx + 1];
						SumOfValuesB += RowStart[ky * CharWidth + kx + 2];
					}
				new_Pixels[y * Width + x] = RGB(SumOfValuesR / divider, SumOfValuesG / divider, SumOfValuesB / divider);
			}
	}
	return new_Pixels;
}

inline LPCOLORREF BoxBlur3x3_AIMade(
	const COLORREF* __restrict src, // LPCOLORREF
	size_t width,
	size_t height,
	size_t stride
) {
	COLORREF* __restrict dst = (COLORREF*)MY_ALLOC(width * height * sizeof(COLORREF));
	if (dst == NULL)
	{
		FileDebug("Error:Could not allocate buffer for blur!");
		return NULL;
	}


	const size_t W = width;

	// Thread-local scratch reused across calls to avoid allocations.
	struct Buffers {
		std::unique_ptr<uint16_t[]> hB[3], hG[3], hR[3]; // ring buffer of 3 rows
		std::unique_ptr<uint16_t[]> vB, vG, vR;          // vertical sums for current 3 rows
		size_t cap = 0;
	};
	static thread_local Buffers buf;

	auto ensure_capacity = [&](size_t need) {
		if (buf.cap >= need) return;
		for (int i = 0; i < 3; ++i) {
			buf.hB[i].reset(new uint16_t[need]);
			buf.hG[i].reset(new uint16_t[need]);
			buf.hR[i].reset(new uint16_t[need]);
		}
		buf.vB.reset(new uint16_t[need]);
		buf.vG.reset(new uint16_t[need]);
		buf.vR.reset(new uint16_t[need]);
		buf.cap = need;
		};
	ensure_capacity(W);

	// Helper: compute horizontal rolling 3-sums for row y into ring slot s (only interior cols written).
	auto horz3 = [&](size_t y, uint16_t* __restrict outB,
		uint16_t* __restrict outG,
		uint16_t* __restrict outR) {
			const COLORREF* __restrict row = src + y * stride;

			// Seed window at x=1: pixels [0,1,2]
			uint32_t p0 = row[0], p1 = row[1], p2 = row[2];
			uint16_t sumB = (uint16_t)((p0 & 0xFF) + (p1 & 0xFF) + (p2 & 0xFF));
			uint16_t sumG = (uint16_t)(((p0 >> 8) & 0xFF) + ((p1 >> 8) & 0xFF) + ((p2 >> 8) & 0xFF));
			uint16_t sumR = (uint16_t)(((p0 >> 16) & 0xFF) + ((p1 >> 16) & 0xFF) + ((p2 >> 16) & 0xFF));

			outB[1] = sumB; outG[1] = sumG; outR[1] = sumR;

			for (size_t x = 2; x + 1 < W; ++x) {
				uint32_t addp = row[x + 1];
				uint32_t subp = row[x - 2];

				sumB = (uint16_t)(sumB + (uint16_t)(addp & 0xFF) - (uint16_t)(subp & 0xFF));
				sumG = (uint16_t)(sumG + (uint16_t)((addp >> 8) & 0xFF) - (uint16_t)((subp >> 8) & 0xFF));
				sumR = (uint16_t)(sumR + (uint16_t)((addp >> 16) & 0xFF) - (uint16_t)((subp >> 16) & 0xFF));

				outB[x] = sumB; outG[x] = sumG; outR[x] = sumR;
			}
		};

	// Precompute first three rows of horizontal 3-sums
	horz3(0, buf.hB[0].get(), buf.hG[0].get(), buf.hR[0].get());
	horz3(1, buf.hB[1].get(), buf.hG[1].get(), buf.hR[1].get());
	horz3(2, buf.hB[2].get(), buf.hG[2].get(), buf.hR[2].get());

	// Initialize vertical sums v = h(0)+h(1)+h(2) for interior columns
	for (size_t x = 1; x + 1 < W; ++x) {
		buf.vB[x] = (uint16_t)(buf.hB[0][x] + buf.hB[1][x] + buf.hB[2][x]);
		buf.vG[x] = (uint16_t)(buf.hG[0][x] + buf.hG[1][x] + buf.hG[2][x]);
		buf.vR[x] = (uint16_t)(buf.hR[0][x] + buf.hR[1][x] + buf.hR[2][x]);
	}

	// Output row y=1 using current vertical sums
	{
		COLORREF* __restrict out = dst + 1 * width;
		for (size_t x = 1; x + 1 < W; ++x) {
			uint8_t b = (uint8_t)((buf.vB[x]) / 9);
			uint8_t g = (uint8_t)((buf.vG[x]) / 9);
			uint8_t r = (uint8_t)((buf.vR[x]) / 9);
			out[x] = (uint32_t)r << 16 | (uint32_t)g << 8 | (uint32_t)b;
		}
	}

	// Process remaining rows with vertical rolling update:
	// At each step, compute horizontal sums for new row (y+2),
	// v += newRow - oldRow, then write row y.
	int top = 0, mid = 1, bot = 2;
	for (size_t y = 2; y + 1 < height; ++y) {
		// next row that enters the 3x window
		size_t newRow = y + 1;

		// Reuse the 'top' slot for the new horizontal sums (ring rotate)
		horz3(newRow, buf.hB[top].get(), buf.hG[top].get(), buf.hR[top].get());

		// Update vertical sums for interior columns: v = v + h[new] - h[old(top_before_overwrite)]
		// Note: we overwrote 'top' with new row, so the "old" row to subtract is 'mid' (previous top row).
		// To avoid confusion, rotate indices AFTER using old pointers:
		int old = mid;

		for (size_t x = 1; x + 1 < W; ++x) {
			buf.vB[x] = (uint16_t)(buf.vB[x] + buf.hB[top][x] - buf.hB[old][x]);
			buf.vG[x] = (uint16_t)(buf.vG[x] + buf.hG[top][x] - buf.hG[old][x]);
			buf.vR[x] = (uint16_t)(buf.vR[x] + buf.hR[top][x] - buf.hR[old][x]);
		}

		// Write output for row y
		COLORREF* __restrict out = dst + y * width;
		for (size_t x = 1; x + 1 < W; ++x) {
			uint8_t b = (uint8_t)(buf.vB[x]/9);
			uint8_t g = (uint8_t)(buf.vG[x]/9);
			uint8_t r = (uint8_t)(buf.vR[x]/9);
			out[x] = (uint32_t)r << 16 | (uint32_t)g << 8 | (uint32_t)b;
		}

		// Rotate ring indices: (top,mid,bot) <- (mid,bot,top)
		int newTop = mid, newMid = bot, newBot = top;
		top = newTop; mid = newMid; bot = newBot;
	}

	return (LPCOLORREF)dst;
}
