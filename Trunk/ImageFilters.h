#ifndef _IMAGE_FILTERS_H_
#define _IMAGE_FILTERS_H_

void WINAPI BlurrImage( int HalfKernelSize );
void WINAPI ErrodeDiffMap( int HalfKernelSize );
void WINAPI EdgeDetect( int HalfKernelSize );
void WINAPI ApplyColorBitmask(int Mask);
void WINAPI DecreaseColorCount(unsigned int ColorsPerChannel);
void WINAPI DecreaseColorCount(unsigned int ColorsPerChannel);
void WINAPI ErrodeRegionToTransparent(int StartX = -1, int StartY = -1, int EndX = -1, int EndY = -1, int RequiredNeighbourCount = 2);
void WINAPI KeepColorRangesInRegion(int StartX = -1, int StartY = -1, int EndX = -1, int EndY = -1, int RMin = 0, int RMax = 255, int GMin = 0, int GMax = 255, int BMin = 0, int BMax = 255);

void RemoveScreenshotAlphaChannel(ScreenshotStruct *cache);
void DecreaseColorPrecision(ScreenshotStruct *cache, unsigned int Div, unsigned int And);
void DecreaseColorCount_(ScreenshotStruct *cache, unsigned int ColorsPerChannel);
void GetUniqueColorsInRegion(int StartX, int StartY, int EndX, int EndY);
#endif