#ifndef _INPUT_PROCESS_H_
#define _INPUT_PROCESS_H_

void WINAPI TakeScreenshot(int aLeft, int aTop, int aRight, int aBottom);
void WINAPI ReleaseScreenshot( );
char* WINAPI ImageSearchOnScreenshot( char *aImageFile, int TransparentColor, int AcceptedColorDiff, int AcceptedErrorCount, int StopAfterNMatches );
// mask value 0 = screenshot pixel is hidden and should not be searched
// mask value 1 = screenshot pixel is visible and should be searched
char* WINAPI ImageSearchOnScreenshotMasked( char *ImageFile, char *MaskFile, int TransparentColor, int AcceptedColorDiff, int AcceptedErrorCount, int StopAfterNMatches );
char* WINAPI GetImageSize( char *aImageFile );

void CycleScreenshots();

struct ScreenshotStruct
{
	LPCOLORREF		Pixels;
	int				Left, Top, Right, Bottom;
	bool			IsDiffMap;	//width and height needs to be divided by 4
	SimilarSearch	*SSCache;
};

#define NR_SCREENSHOTS_CACHED	2

extern LIBRARY_API ScreenshotStruct ScreenshotCache[NR_SCREENSHOTS_CACHED];
extern LIBRARY_API ScreenshotStruct	*CurScreenshot, *PrevScreenshot;
extern LIBRARY_API int ScreenshotStoreIndex;

#endif