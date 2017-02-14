#ifndef _OCR_H_
#define _OCR_H_

#define MIGRATE_OLD_TO_NEW_ON_FILTER_CHANGE

struct OCRStore
{
	char AssignedChars[10];	// sadly some chars can't be safely separated. One example is 'ej'. Using some fonts the J will swallow the e
	int LastSearchHitCount;
	int	LastSearchMissCount;
	int LastSearchX,LastSearchY;
	int SumR,SumG,SumB,CountR,CountG,CountB,PixelCount;
	int FontSet;	//in case we use multiple font types
#ifdef MIGRATE_OLD_TO_NEW_ON_FILTER_CHANGE
	int Migrated;
#endif
};

#ifdef MIGRATE_OLD_TO_NEW_ON_FILTER_CHANGE
	extern char FontSetName[150];	//just a lucky guess where these fonts are getting loaded
#endif

void WINAPI OCR_RegisterFont(char *aFilespec, char *Str);
void WINAPI OCR_LoadFontsFromFile(char *aFilespec);
void WINAPI OCR_LoadFontsFromDir(char *Path, char *SkipFileNameStart);
void WINAPI OCR_SetMaxFontSize(int Width, int Height);	// if you put it too large, it will read data from next row. If you put it too small, it might not be able to detect new characters
char * WINAPI ReadTextFromScreenshot( int StartX, int StartY, int EndX, int EndY );
void WINAPI OCR_SetActiveFontSet(int FontSet, char *Name);

extern int OCRMaxFontWidth;
extern int OCRMaxFontHeight;
extern int OCRTransparentColor;
extern int OCRActiveFontSet;
#endif