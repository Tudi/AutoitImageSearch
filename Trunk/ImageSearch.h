#ifndef _IMAGE_SEARCH_H_
#define _IMAGE_SEARCH_H_

enum SADSearchRegionFlags : int
{
	SSRF_ST_NO_FLAGS = 0,
	SSRF_ST_PROCESS_INLCUDE_DIFF_INFO = 1 << 0,
	SSRF_ST_ENFORCE_SAD_WITH_HASH = 1 << 1,
};

char* WINAPI ImageSearchOnScreenshot(const char *aImageFile, int TransparentColor, int AcceptedColorDiff, int AcceptedErrorCount, int StopAfterNFullMatches);
char* WINAPI ImageSearchOnScreenshotBest_SAD(const char *aImageFile);
char* WINAPI ImageSearchOnScreenshotBestTransparent(const char *aImageFile);
// mask value 0 = screenshot pixel is hidden and should not be searched
// mask value 1 = screenshot pixel is visible and should be searched
char* WINAPI ImageSearchOnScreenshotMasked(const char *ImageFile, char *MaskFile, int TransparentColor, int AcceptedColorDiff, int AcceptedErrorCount, int StopAfterNFullMatches);
char* WINAPI ImageSearch_SAD(const char *aImageFile);
char* WINAPI ImageSearch_SAD_Region(const char* aImageFile, int aLeft, int aTop, int aRight, int aBottom, SADSearchRegionFlags uSearchFlags); // when we want to search only a subreagion. Used for IsImageAt
char* WINAPI ImageSearch_Multiple_ExactMatch(const char *aImageFile);
char* WINAPI ImageSearch_Multiple_Transparent(const char *aImageFile);
char* WINAPI ImageSearch_Multiple_PixelCount(int Color, int Percent, int AreaWidth, int AreaHeight);
char* WINAPI ImageSearch_Multipass_PixelCount(int Color, int PercentStart, int PercentMin, int PercentStep, int AreaWidth, int AreaHeight);
// this is a lot slower than doing it step by step !
char* WINAPI ImageSearch_Multiple_Gradient(int Color, int GradientMatchPercent, int CountInAreaPercent, int AreaWidth, int AreaHeight);


unsigned int ImgSAD(LPCOLORREF bigImg, size_t bigWidth, size_t bigHeight, size_t bigStride,
	LPCOLORREF smallImg, size_t smallWidth, size_t smallHeight, size_t smallStride,
	size_t atX, size_t atY);

// ignore these. Using them for a specific case
void ImageSearch_Multipass_PixelCount2(int Color, int PercentMax, int PercentMin, int PercentStep, int AreaWidth, int AreaHeight, int OneSearchInRadius);
void ImageSearch_Multipass_PixelCount3(int Color, int PercentMin, int AreaWidth, int AreaHeight, int OneSearchInRadius);
extern int SearchResultCount;
extern int SearchResultXYSAD[500][3];
#endif
