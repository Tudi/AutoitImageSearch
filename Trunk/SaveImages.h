#ifndef _SAVE_IMAGE_H_
#define _SAVE_IMAGE_H_

void WINAPI SaveScreenshot();
void WINAPI SaveDiffMap();
void WINAPI SaveScreenshotDiffMask( int LowLimit );
void WINAPI SaveEdgeMap();
void WINAPI SaveScreenshotCutTransparent();

void DumpAsPPM( unsigned char *R,unsigned char *G,unsigned char *B, int Width, int Height );
void DumpAsPPM( LPCOLORREF RGB, int Width, int Height );
void DumpAsPPM( LPCOLORREF RGB, int Width, int Height, int Stride );
void DumpAsPPMBGR( LPCOLORREF BGR, int Width, int Height );

#endif