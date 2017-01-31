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
char * WINAPI ReadTextFromScreenshot( int StartX, int StartY, int EndX, int EndY );

//similar to image binarization. Will be used for OCR
void WINAPI KeepColorSetRest(int SetRest, int Color1);
void WINAPI KeepColor3SetBoth(int SetRest, int SetColors, int Color1, int Color2, int Color3);

extern int OCRMaxFontWidth;
extern int OCRMaxFontHeight;
extern int OCRTransparentColor;
#endif