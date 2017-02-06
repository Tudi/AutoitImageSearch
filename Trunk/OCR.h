#ifndef _OCR_H_
#define _OCR_H_

struct OCRStore
{
	int AssignedChar;
	int LastSearchHitCount;
	int	LastSearchMissCount;
	int LastSearchX,LastSearchY;
	int SumR,SumG,SumB,CountR,CountG,CountB,PixelCount;
};

void WINAPI OCR_RegisterFont( char *aFilespec, int Font );
void WINAPI OCR_LoadFontsFromFile(char *aFilespec);
void WINAPI OCR_LoadFontsFromDir(char *Path, char *SkipFileNameStart);
void WINAPI OCR_SetMaxFontSize(int Width, int Height);	// if you put it too large, it will read data from next row. If you put it too small, it might not be able to detect new characters
char * WINAPI ReadTextFromScreenshot( int StartX, int StartY, int EndX, int EndY );

extern int OCRMaxFontWidth;
extern int OCRMaxFontHeight;
extern int OCRTransparentColor;
#endif