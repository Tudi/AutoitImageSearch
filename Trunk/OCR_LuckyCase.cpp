
// from starting position, search down to left until we manage to find a column with at least 1 pixel
#include "StdAfx.h"

#define UnknownCharMaxPixelsX  (OCRMaxFontWidth*2)
#define UnknownCharMaxPixelsY  (OCRMaxFontHeight*2)

// this is a happy case when characters are separated from each other, the binarized image should have a 1 to 1 match with one of the fonts that we registered
int OCR_FindNextCharFirstColumn(char *Image, int Width4x, int &x, int y)
{
	for (int x2 = x; x2 < x + UnknownCharMaxPixelsX; x2++)
		//check if we can find 1 pixel on this column of pixels that is not transparent
		for (int y2 = y; y2 < y + UnknownCharMaxPixelsY; y2++)
			if (*(int*)&Image[y2*Width4x + x2 * 4] != OCRTransparentColor)
			{
				x = x2;
				return 1;
			}
	return 0;	//could not find one
}

int OCR_FindCurrentCharLastColumn(char *Image, int Width4x, int &x, int y)
{
	for (int x2 = x; x2 < x + UnknownCharMaxPixelsX; x2++)	// give a chance to detect new characters
	{
		//if the whole column was transparent, we presume the character ended
		int AllWasTransparent = 1;
		for (int y2 = y; y2 < y + UnknownCharMaxPixelsY; y2++)
			if (*(int*)&Image[y2*Width4x + x2 * 4] != OCRTransparentColor)
			{
				AllWasTransparent = 0;
				break;
			}
		if (AllWasTransparent == 1)
		{
			x = x2;
			return 1;
		}
	}
	return 0;	//could not find the end of this character. Could be unknown ?
}

int OCR_FindCurrentCharFirstRow(char *Image, int Width4x, int StartX, int EndX, int &y)
{
	for (int y2 = y; y2 < y + OCRMaxFontHeight; y2++)
	{
		for (int x2 = StartX; x2 < EndX; x2++)
			if (*(int*)&Image[y2*Width4x + x2 * 4] != OCRTransparentColor)
			{
				y = y2;
				return 1;
			}
	}
	return 0;
}

int OCR_FindCurrentCharLastRow(char *Image, int Width4x, int StartX, int EndX, int &y)
{
	for (int y2 = y; y2 < y + OCRMaxFontHeight * 2; y2++) // give  a chance to detect new fonts
	{
		for (int x2 = StartX; x2 < EndX; x2++)
			if (*(int*)&Image[y2*Width4x + x2 * 4] != OCRTransparentColor)
			{
				y = y2;
				return 1;
			}
	}
	return 0;
}

int CountPixelsInArea(char *Image, int Width4x, int StartX, int StartY, int EndX, int EndY)
{
	int ret = 0;
	for (int y = StartY; y <= EndY; y++)
	{
		int *Pixels = (int*)&Image[y*Width4x];
		for (int x = StartX; x < EndX; x++)
			if (Pixels[x])
				ret++;
	}
	return ret;
}

int GetPixelMatchCount(char *Screenshot, int Width4x, int StartX, int StartY, int Endx, int Endy, char *Font, int FontWidth, int FontHeight)
{
	// in a perfect world these should be exact matches
	// font might be a bit larger due to lousy cut out by manual initial fonts
	if (Endy - StartY > FontHeight)
		return 0;
	if (Endx - StartX > FontWidth)
		return 0;
	int ret = 0;
	for (int y = StartY; y < Endy; y++)
		for (int x = StartX; x < Endx; x++)
			if (Screenshot[y * Width4x + x * 4] == Font[(y - StartY)*FontWidth + (x - StartX)])
				ret++;
	return ret;
}

char FindMatchingFont(char *Img, int Width4x, int CharStartX, int CharStartY, int CharEndX, int CharEndY)
{
	// count the number of pixels in this region. Helps our mapping search
	int PixelCount = CountPixelsInArea(Img, Width4x, CharStartX, CharStartY, CharEndX, CharEndY);
	if (PixelCount == 0)
		return 0;
	// iterate through all cached pictures and check which ones are fonts
	for (int c = 0; c < NrPicturesCached; c++)
	{
		//if this font would be the best match, at what location would it be at ?
		CachedPicture *FontCache = &PictureCache[c];
		if (FontCache->OCRCache == NULL || FontCache->OCRCache->PixelCount != PixelCount)
			continue;
		//check if this image is a perfect match
		int MatchStrength = GetPixelMatchCount(Img, Width4x, CharStartX, CharStartY, CharEndX, CharEndY, (char*)FontCache->Pixels, FontCache->Width, FontCache->Height);
		if (MatchStrength == PixelCount)
			return FontCache->OCRCache->AssignedChar;
	}
	return 0;
}

void ExtractNewFont( char *Img, int Width4x, int CharStartX, int CharStartY, int CharEndX, int CharEndY)
{
	SaveScreenshotArea(CharStartX, CharStartY, CharEndX, CharEndY);
}

int FindCharacterEnclosingBox(char *Img, int Width4x, int &CharStartX, int &CharStartY, int &CharEndX, int &CharEndY)
{
	if (OCR_FindNextCharFirstColumn(Img, Width4x, CharStartX, CharStartY) == 0)
		return 0;
	CharEndX = CharStartX;
	if (OCR_FindCurrentCharLastColumn(Img, Width4x, CharEndX, CharStartY) == 0)
		return 0;
	if (OCR_FindCurrentCharFirstRow(Img, Width4x, CharStartX, CharEndX, CharStartY) == 0)
		return 0;
	CharEndY = CharStartY;
	if (OCR_FindCurrentCharLastRow(Img, Width4x, CharStartX, CharEndX, CharEndY) == 0)
		return 0;
	return 1;
}
//start X and Y should be the upper left part of the text
char * WINAPI OCR_ReadTextLeftToRightSaveUnknownChars(int StartX, int StartY, int EndX, int EndY)
{
	FileDebug("Started OCR_ReadTextLeftToRightSaveUnknownChars");
	if (CurScreenshot->Pixels == NULL)
	{
		FileDebug("\tOCR has no screenshot to work on");
		return "|0";
	}
	int CharStartX = StartX;
	int CharStartY = StartY;
	int CharEndX = CharStartX + OCRMaxFontWidth;
	int CharEndY = CharStartY + OCRMaxFontHeight;
	int SearchRes = 1;
	char *Img = (char*)CurScreenshot->Pixels;
	int Width4x = CurScreenshot->GetWidth();
	int Height = CurScreenshot->GetHeight();
	int WriteIndex = 0;
	ReturnBuff[0] = 0;
	while (SearchRes == 1 && CharStartX < EndX)
	{
		// try to get 1 full character that is separated by the next one and it's close to the previous one
		if (FindCharacterEnclosingBox(Img, Width4x, CharStartX, CharStartY, CharEndX, CharEndY) == 0)
			break;
		char Font = FindMatchingFont(Img, Width4x, CharStartX, CharStartY, CharEndX, CharEndY);
		if (Font != 0)
			ReturnBuff[WriteIndex++] = Font;
		else
			ExtractNewFont(Img, Width4x, CharStartX, CharStartY, CharEndX, CharEndY);

		CharStartX = CharEndX + 1;
	}
	FileDebug("Finished OCR_ReadTextLeftToRightSaveUnknownChars");
	ReturnBuff[WriteIndex++] = 0;
	sprintf_s(ReturnBuff, DEFAULT_STR_BUFFER_SIZE * 10, "%s|%d", ReturnBuff, CharStartX);
	return ReturnBuff;
}