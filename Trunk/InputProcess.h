#ifndef _INPUT_PROCESS_H_
#define _INPUT_PROCESS_H_

void WINAPI TakeScreenshot(int aLeft, int aTop, int aRight, int aBottom);
void WINAPI ReleaseScreenshot( );
char* WINAPI ImageSearchOnScreenshot( char *aImageFile, int TransparentColor, int AcceptedColorDiff, int AcceptedErrorCount, int StopAfterNMatches );

extern LIBRARY_API LPCOLORREF ScreenshotPixels; 
extern LIBRARY_API int ScreenshotLeft, ScreenshotTop, ScreenshotRight, ScreenshotBottom;

#endif