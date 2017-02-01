
// from starting position, search down to left until we manage to find a column with at least 1 pixel
#include "StdAfx.h"

// this is a happy case when characters are separated from each other, the binarized image should have a 1 to 1 match with one of the fonts that we registered
int OCR_FindNextCharFirstColumn(int *Image, int Width, int &x, int y, int BoxEndX, int BoxEndY)
{
	for (int x2 = x; x2 < BoxEndX; x2++)
		//check if we can find 1 pixel on this column of pixels that is not transparent
		for (int y2 = y; y2 < BoxEndY; y2++)
			if (Image[y2*Width + x2] != OCRTransparentColor)
			{
				x = x2;
				return 1;
			}
	return 0;	//could not find one
}

int OCR_FindCurrentCharLastColumn(int *Image, int Width, int &StartX, int StartY, int BoxEndX, int BoxEndY)
{
	for (int x2 = StartX; x2 < BoxEndX; x2++)	// give a chance to detect new characters
	{
		//if the whole column was transparent, we presume the character ended
		int AllWasTransparent = 1;
		for (int y2 = StartY; y2 < BoxEndY; y2++)
			if (Image[y2*Width + x2] != OCRTransparentColor)
			{
				AllWasTransparent = 0;
				break;
			}
		if (AllWasTransparent == 1)
		{
			StartX = x2;
			return 1;
		}
	}
	return 0;	//could not find the end of this character. Could be unknown ?
}

int OCR_FindCurrentCharFirstRow(int *Image, int Width, int StartX, int EndX, int &StartY, int BoxEndY)
{
	for (int y2 = StartY; y2 < BoxEndY; y2++)
	{
		for (int x2 = StartX; x2 < EndX; x2++)
			if (Image[y2*Width + x2] != OCRTransparentColor)
			{
				StartY = y2;
				return 1;
			}
	}
	return 0;
}

int OCR_FindCurrentCharLastRow(int *Image, int Width, int StartX, int EndX, int &StartY, int BoxEndY)
{
	for (int y2 = StartY; y2 <BoxEndY; y2++) // give  a chance to detect new fonts
	{
		int AllWasTransparent = 1;
		for (int x2 = StartX; x2 < EndX; x2++)
			if (Image[y2*Width + x2] != OCRTransparentColor)
			{
				AllWasTransparent = 0;
				break;
			}
		if (AllWasTransparent == 1)
		{
			StartY = y2;
			return 1;
		}
	}
	return 0;
}

int CountPixelsInArea(int *Image, int Width, int StartX, int StartY, int EndX, int EndY)
{
	int ret = 0;
	for (int y = StartY; y < EndY; y++)
	{
		int *Pixels = (int*)&Image[y*Width];
		for (int x = StartX; x < EndX; x++)
			if (Pixels[x] != OCRTransparentColor)
				ret++;
	}
	return ret;
}

int GetPixelMatchCount(int *Screenshot, int Width, int StartX, int StartY, int Endx, int Endy, int *Font, int FontWidth, int FontHeight)
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
			if (Font[(y - StartY)*FontWidth + (x - StartX)] != OCRTransparentColor && Screenshot[y * Width + x] == Font[(y - StartY)*FontWidth + (x - StartX)])
				ret++;
	return ret;
}

char FindMatchingFont(int *Img, int Width, int CharStartX, int CharStartY, int CharEndX, int CharEndY)
{
	// count the number of pixels in this region. Helps our mapping search
	int PixelCount = CountPixelsInArea(Img, Width, CharStartX, CharStartY, CharEndX, CharEndY);
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
		int MatchStrength = GetPixelMatchCount(Img, Width, CharStartX, CharStartY, CharEndX, CharEndY, (int*)FontCache->Pixels, FontCache->Width, FontCache->Height);
		if (MatchStrength == FontCache->OCRCache->PixelCount) // font pixelcount might be a bit smaller as we try to merge multiple versions into 1
			return FontCache->OCRCache->AssignedChar;
	}
	return 0;
}

//no way safe, but maybe it helps us getting spammed by same character multiple times. Maybe.
//if there is hash collision, we will need to rerun the training program
int GetAreaHash(int *Img, int Width, int StartX, int StartY, int EndX, int EndY)
{
	int ret = 0;
	for (int y = StartY; y < EndY; y++)
		for (int x = StartX; x < EndX; x++)
		{
			int Local = (y - StartY) * (x - StartX) + Img[y * Width + x] * 1000;
			ret += Local;
		}
	return ret;
}
std::set<int> ReportedHashes;
void ExtractNewFont( int *Img, int Width, int CharStartX, int CharStartY, int CharEndX, int CharEndY)
{
	int Hash = GetAreaHash(Img, Width, CharStartX, CharStartY, CharEndX, CharEndY);
	if (ReportedHashes.find(Hash) != ReportedHashes.end())
		return;
	ReportedHashes.insert(Hash);
	SaveScreenshotArea(CharStartX, CharStartY, CharEndX, CharEndY);
}

int FindCharacterEnclosingBox(int *Img, int Width, int &CharStartX, int &CharStartY, int &CharEndX, int &CharEndY)
{
	int BoxEndX = CharEndX;
	int BoxEndY = CharEndY;
	if (OCR_FindNextCharFirstColumn(Img, Width, CharStartX, CharStartY, BoxEndX, BoxEndY) == 0)
		return 0;
	CharEndX = CharStartX;
	if (OCR_FindCurrentCharLastColumn(Img, Width, CharEndX, CharStartY, BoxEndX, BoxEndY) == 0)
		return 0;
	if (OCR_FindCurrentCharFirstRow(Img, Width, CharStartX, CharEndX, CharStartY, BoxEndY) == 0)
		return 0;
	CharEndY = CharStartY;
	if (OCR_FindCurrentCharLastRow(Img, Width, CharStartX, CharEndX, CharEndY, BoxEndY) == 0)
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
	int CharStartY;
	int CharEndX;
	int CharEndY;
	int SearchRes = 1;
	int *Img = (int*)CurScreenshot->Pixels;
	int Width = CurScreenshot->GetWidth();
	int Height = CurScreenshot->GetHeight();
	int WriteIndex = 0;
	ReturnBuff[0] = 0;
	while (SearchRes == 1 && CharStartX < EndX)
	{
		CharStartY = StartY;
		CharEndX = EndX;
		CharEndY = EndY;
			// try to get 1 full character that is separated by the next one and it's close to the previous one
		if (FindCharacterEnclosingBox(Img, Width, CharStartX, CharStartY, CharEndX, CharEndY) == 0)
			break;
		char Font = FindMatchingFont(Img, Width, CharStartX, CharStartY, CharEndX, CharEndY);
		if (Font != 0)
			ReturnBuff[WriteIndex++] = Font;
		else
			ExtractNewFont(Img, Width, CharStartX, CharStartY, CharEndX, CharEndY);

		CharStartX = CharEndX + 1;
	}
	FileDebug("Finished OCR_ReadTextLeftToRightSaveUnknownChars");
	ReturnBuff[WriteIndex++] = 0;
	sprintf_s(ReturnBuff, DEFAULT_STR_BUFFER_SIZE * 10, "%s|%d", ReturnBuff, CharStartX);
	return ReturnBuff;
}