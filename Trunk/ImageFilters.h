#ifndef _IMAGE_FILTERS_H_
#define _IMAGE_FILTERS_H_

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
#endif