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



template <__int64 HalfKernelSize>
LPCOLORREF BlurrImage2_(const LPCOLORREF Pixels, const size_t Width, const size_t Height, const size_t Stride, double MiddleFactor)
{
	LPCOLORREF new_Pixels = (COLORREF*)MY_ALLOC(Stride * Height * sizeof(COLORREF) + SSE_PADDING);
	if (new_Pixels == NULL)
	{
		FileDebug("Error:Could not allocate buffer for blur!");
		return NULL;
	}
	const size_t CharWidth = Stride * 4;
	const size_t kernel_pixel_count = (2 * HalfKernelSize + 1) * (2 * HalfKernelSize + 1);
	for (size_t y = HalfKernelSize; y < Height - HalfKernelSize; y += 1)
		for (size_t x = HalfKernelSize; x < Width - HalfKernelSize; x += 1)
		{
			unsigned char* RowStart = (unsigned char*)&Pixels[y * Stride + x];
			size_t SumOfValuesR = (size_t)(MiddleFactor * RowStart[0]);
			size_t SumOfValuesG = (size_t)(MiddleFactor * RowStart[1]);
			size_t SumOfValuesB = (size_t)(MiddleFactor * RowStart[2]);
			for (__int64 ky = -HalfKernelSize; ky <= HalfKernelSize; ky++)
				for (__int64 kx = -HalfKernelSize * 4; kx <= HalfKernelSize * 4; kx += 4)
				{
					SumOfValuesR += RowStart[ky * CharWidth + kx + 0];
					SumOfValuesG += RowStart[ky * CharWidth + kx + 1];
					SumOfValuesB += RowStart[ky * CharWidth + kx + 2];
				}
			new_Pixels[y * Stride + x] = RGB(SumOfValuesR / (kernel_pixel_count + MiddleFactor), SumOfValuesG / (kernel_pixel_count + MiddleFactor), SumOfValuesB / (kernel_pixel_count + MiddleFactor));
		}
	return new_Pixels;
}
#endif