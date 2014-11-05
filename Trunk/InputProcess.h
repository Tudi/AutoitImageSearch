#ifndef _INPUT_PROCESS_H_
#define _INPUT_PROCESS_H_

void WINAPI TakeScreenshot(int aLeft, int aTop, int aRight, int aBottom);
void WINAPI ReleaseScreenshot( );

extern LIBRARY_API LPCOLORREF ScreenshotPixels; 
extern LIBRARY_API int ScreenshotLeft, ScreenshotTop, ScreenshotRight, ScreenshotBottom;

#endif