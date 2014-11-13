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

void WINAPI RegisterOCRFont( char *aFilespec, int Font );
char * WINAPI ReadTextFromScreenshot( int StartX, int StartY, int EndX, int EndY );

#endif