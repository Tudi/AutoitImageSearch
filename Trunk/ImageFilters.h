#ifndef _IMAGE_FILTERS_H_
#define _IMAGE_FILTERS_H_

void WINAPI BlurrImage( int HalfKernelSize );
void WINAPI ErrodeDiffMap( int HalfKernelSize );
void WINAPI EdgeDetect( int HalfKernelSize );
void WINAPI ApplyColorBitmask(int Mask);
void WINAPI DecreaseColorCount(unsigned int ColorsPerChannel);
void WINAPI DecreaseColorCount(unsigned int ColorsPerChannel);

void RemoveScreenshotAlphaChannel(ScreenshotStruct *cache);
void DecreaseColorPrecision(ScreenshotStruct *cache, unsigned int Div, unsigned int And);
void DecreaseColorCount_(ScreenshotStruct *cache, unsigned int ColorsPerChannel);
void GetUniqueColorsInRegion(int StartX, int StartY, int EndX, int EndY);
#endif