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

//similar to image binarization. Will be used for OCR
void WINAPI KeepColorSetRest(int SetRest, int SetColors, int Color1);
void WINAPI KeepColor3SetBoth(int SetRest, int SetColors, int Color1, int Color2, int Color3);
void WINAPI KeepColorsMinInRegion(int StartX = -1, int StartY = -1, int EndX = -1, int EndY = -1, int RMin = 0, int GMin = 0, int BMin = 0);

extern int OCRMaxFontWidth;
extern int OCRMaxFontHeight;
extern int OCRTransparentColor;
#endif