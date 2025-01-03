#ifndef _IMAGE_SEARCH_H_
#define _IMAGE_SEARCH_H_

char* WINAPI ImageSearchOnScreenshot(char *aImageFile, int TransparentColor, int AcceptedColorDiff, int AcceptedErrorCount, int StopAfterNFullMatches);
char* WINAPI ImageSearchOnScreenshotBest_SAD(char *aImageFile);
char* WINAPI ImageSearchOnScreenshotBestTransparent(char *aImageFile);
// mask value 0 = screenshot pixel is hidden and should not be searched
// mask value 1 = screenshot pixel is visible and should be searched
char* WINAPI ImageSearchOnScreenshotMasked(char *ImageFile, char *MaskFile, int TransparentColor, int AcceptedColorDiff, int AcceptedErrorCount, int StopAfterNFullMatches);
char* WINAPI ImageSearch_SAD(char *aImageFile);
char* WINAPI ImageSearch_SAD_Region(char* aImageFile, int aLeft, int aTop, int aRight, int aBottom); // when we want to search only a subreagion. Used for IsImageAt
char* WINAPI ImageSearch_Multiple_ExactMatch(char *aImageFile);
char* WINAPI ImageSearch_Multiple_Transparent(char *aImageFile);
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
