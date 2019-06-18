#ifndef _MOTION_ESTIMATION_H_
#define _MOTION_ESTIMATION_H_

/*
converts a 4x4 pixel block into 1 single SAD value
*/
int WINAPI GenerateDiffMap();

extern LIBRARY_API ScreenshotStruct MotionDiff;

unsigned int GenerateDiffMap(LPCOLORREF Pix1, LPCOLORREF Pix2, int Width, int Height, unsigned char *DiffMapOutput);

#endif