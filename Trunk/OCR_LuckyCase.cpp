
// from starting position, search down to left until we manage to find a column with at least 1 pixel
#include "StdAfx.h"

int OCR_Statistics_TotalCompares = 0;
int OCR_Statistics_PixelCountMatchEliminations = 0;	// number of times we tought it woudll match
int OCR_Statistics_RealMatch = 0;					// number of times it really matched
int OCR_Statistics_SizeEliminations = 0;			// skipped check due to size differences
int OCR_Statistics_SimilarSearches = 0;				// when exact match fails
int OCR_Statistics_PixelCountEliminations = 0;		// skipped check due to pixel differences
int OCR_Statistics_HashEliminations = 0;			// skipped check due to hash differences
__int64 OCR_Statistics_PixelsCompared = 0;			// just to now how much room is there for improvement

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
	for (int y2 = BoxEndY - 1; y2 >= StartY; y2--) // has to go bottom->top or else i will have separate parts
	{
		int AllWasTransparent = 1;
		for (int x2 = StartX; x2 < EndX; x2++)
			if (Image[y2*Width + x2] != OCRTransparentColor)
			{
				StartY = y2 + 1;
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
//			if (Screenshot[y * Width + x] == Font[(y - StartY)*FontWidth + (x - StartX)])
				ret++;
	return ret;
}

void GenerateAvailableFontFilename(char *Buf, int len, char *TheChars)
{
	//generate a new valid filename
	char NewFilename[500], OldFilename[500];
	int FileIndex = -1;
	if (TheChars == 0)
		TheChars = "_";
	do{
		FileIndex++;
		sprintf_s(NewFilename, sizeof(NewFilename), "KCM_%s_%d.bmp", TheChars, FileIndex);
		if (_access(NewFilename, 0) == 0)
			continue;
		sprintf_s(OldFilename, sizeof(OldFilename), "%s/KCM_%s_%d.bmp", FontSetName, TheChars, FileIndex);
		if (_access(OldFilename, 0) == 0)
			continue;
		break;
	} while (1);
	strcpy_s(Buf, len, NewFilename);
}

OCRStore *FindExactMatchingFont(int *Img, int Width, int CharStartX, int CharStartY, int CharEndX, int CharEndY)
{
//#define USE_PIXEL_COUNT_FOR_IMG_MATCH
#ifdef USE_PIXEL_COUNT_FOR_IMG_MATCH	// hashing seems to replace this. About 0.01% less 1 to 1 image matches. Might not seem a lot, but we might be doing it millsions of times.
	// count the number of pixels in this region. Helps our mapping search
	int PixelCount = CountPixelsInArea(Img, Width, CharStartX, CharStartY, CharEndX, CharEndY);
	if (PixelCount == 0)
		return NULL;
#endif
	int CharWidth = CharEndX - CharStartX;
	int CharHeight = CharEndY - CharStartY;
	OCR_Statistics_PixelsCompared += CharWidth * CharHeight;
	unsigned int FontHash = GetImgAreaHash(Img, Width, CharStartX, CharStartY, CharEndX, CharEndY); // we will find a match here, we can afford to calculate hash form the start
	// iterate through all cached pictures and check which ones are fonts
	for (int c = 0; c < NrPicturesCached; c++)
	{
		//if this font would be the best match, at what location would it be at ?
		CachedPicture *FontCache = &PictureCache[c];
		if (FontCache->OCRCache == NULL || FontCache->OCRCache->FontSet != OCRActiveFontSet)
			continue;

		OCR_Statistics_TotalCompares++;

		//hash should be slightly better than simple pixel count check
		if (FontCache->OCRCache->Hash != FontHash)
		{
			OCR_Statistics_HashEliminations++;
			continue;
		}

#ifdef USE_PIXEL_COUNT_FOR_IMG_MATCH	// hashing seems to replace this
		// very simple comparison. Number of pixels match ?
		if (FontCache->OCRCache->PixelCount != PixelCount)
		{
			OCR_Statistics_PixelCountEliminations++;
			continue;
		}
#endif
		// if there is a big differnce in size, skip checking the whole font area
		if (CharWidth != FontCache->Width || CharHeight != FontCache->Height)
		{
			OCR_Statistics_SizeEliminations++;
			continue;
		}
		/**/
		//check if this image is a perfect match
		int MatchStrength = GetPixelMatchCount(Img, Width, CharStartX, CharStartY, CharEndX, CharEndY, (int*)FontCache->Pixels, FontCache->Width, FontCache->Height);
		OCR_Statistics_PixelsCompared += CharWidth * CharHeight;
		if (MatchStrength != FontCache->OCRCache->PixelCount) // font pixelcount might be a bit smaller as we try to merge multiple versions into 1
		{
			OCR_Statistics_PixelCountMatchEliminations++;
			continue;
		}
#ifdef MIGRATE_OLD_TO_NEW_ON_FILTER_CHANGE
		//if font is comming from a differenct directory than copy it to our Font folder. This happens when whe redo the font library and want to clean up unused ones
		if (FontCache->OCRCache->Migrated == 0 && strstr(FontCache->FileName, FontSetName) != FontCache->FileName)
		{
			//get the file name from src
			char Filename[500], Filename2[500];
			GenerateAvailableFontFilename(Filename, sizeof(Filename), FontCache->OCRCache->AssignedChars);
			sprintf_s(Filename2, sizeof(Filename2), "%s/%s", FontSetName, Filename);
			BOOL success = CopyFile(FontCache->FileName, Filename2, true);
			if (success == false)
				printf("failed to copy, debug me\n");
			FontCache->OCRCache->Migrated = 1;
		}
#endif
		OCR_Statistics_RealMatch++;
		return FontCache->OCRCache;
	}
	return NULL;
}

void OCR_FindMostSimilarFontAndSave(int *Img, int Width, int CharStartX, int CharStartY, int CharEndX, int CharEndY)
{
	int BestMatch = 0;
	CachedPicture *BestMatchFont = NULL;
	int CharWidth = CharEndX - CharStartX;
	int CharHeight = CharEndY - CharStartY;
	// iterate through all cached pictures and check which ones are fonts
	for (int c = 0; c < NrPicturesCached; c++)
	{
		//if this font would be the best match, at what location would it be at ?
		CachedPicture *FontCache = &PictureCache[c];
		if (FontCache->OCRCache == NULL)
			continue;

		OCR_Statistics_SimilarSearches++;

		if (abs(CharWidth - FontCache->Width) > 3 || abs(CharHeight - FontCache->Height) > 3)
			continue;

		//there is a chance the new old font location does not perfectly allign. Search in multiple locations for a lucky hit
		for (int y = -1; y <= 1; y++)
			for (int x = -1; x <= 1; x++)
			{
				int MatchStrength = GetPixelMatchCount(Img, Width, CharStartX+x, CharStartY+y, CharEndX+x, CharEndY+y, (int*)FontCache->Pixels, FontCache->Width, FontCache->Height);
				if (MatchStrength > BestMatch)
				{
					BestMatch = MatchStrength;
					BestMatchFont = FontCache;
				}
			}
	}
	//maybe it's just a small aberation of an already declared font. Some fontx are 8x8 pixels. 10% represents 6 pixel diference. Which can be enough to confuse it with another font. 'o' -'u' = 4 pixels ?
	if (BestMatchFont && abs( BestMatchFont->OCRCache->PixelCount - BestMatch ) < BestMatchFont->OCRCache->PixelCount * 10 / 100)
	{
		char NewFilename[500];
		GenerateAvailableFontFilename(NewFilename, sizeof(NewFilename), BestMatchFont->OCRCache->AssignedChars);
		SaveScreenshotArea(CharStartX, CharStartY, CharEndX, CharEndY, NewFilename);
	}
	else
	{
		char NewFilename[500];
		int FileIndex = 0;
		do{
			sprintf_s(NewFilename, sizeof(NewFilename), "KCM__%d.bmp", FileIndex++);
		} while (_access(NewFilename, 0) == 0 && FileIndex < 1000);
		SaveScreenshotArea(CharStartX, CharStartY, CharEndX, CharEndY, NewFilename);
	}
}

std::set<int> ReportedHashes;
void ExtractNewFont( int *Img, int Width, int CharStartX, int CharStartY, int CharEndX, int CharEndY)
{
	int Hash = GetImgAreaHash(Img, Width, CharStartX, CharStartY, CharEndX, CharEndY);
	if (ReportedHashes.find(Hash) != ReportedHashes.end())
		return;
	ReportedHashes.insert(Hash);
	OCR_FindMostSimilarFontAndSave(Img, Width, CharStartX, CharStartY, CharEndX, CharEndY);
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

int OCR_FoundNewFont;
//start X and Y should be the upper left part of the text
char * WINAPI OCR_ReadTextLeftToRightSaveUnknownChars(int StartX, int StartY, int EndX, int EndY)
{
	FileDebug("Started OCR_ReadTextLeftToRightSaveUnknownChars");
	OCR_FoundNewFont = 0;
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
	int PrevFontEnd = StartX;
	int FoundNewFont = 0;
	while (SearchRes == 1 && CharStartX < EndX)
	{
		CharStartY = StartY;
		CharEndX = EndX;
		CharEndY = EndY;
			// try to get 1 full character that is separated by the next one and it's close to the previous one
		if (FindCharacterEnclosingBox(Img, Width, CharStartX, CharStartY, CharEndX, CharEndY) == 0)
			break;
		//guessing there were " " characters between the 2 fonts
		if (CharStartX > PrevFontEnd + 5)
			ReturnBuff[WriteIndex++] = ' ';
		//maybe we get lucky and guess what this was
		OCRStore *Font = FindExactMatchingFont(Img, Width, CharStartX, CharStartY, CharEndX, CharEndY);
		if (Font != NULL)
		{
			char *ImgStr = Font->AssignedChars;
			while (*ImgStr != 0)
			{
				ReturnBuff[WriteIndex++] = *ImgStr;
				ImgStr++;
			}
		}
		else
		{
			ExtractNewFont(Img, Width, CharStartX, CharStartY, CharEndX, CharEndY);
//			FoundNewFont = 1;
			OCR_FoundNewFont = 1;
		}

		CharStartX = CharEndX + 1;
		PrevFontEnd = CharStartX;
	}
	FileDebug("Finished OCR_ReadTextLeftToRightSaveUnknownChars");
	ReturnBuff[WriteIndex++] = 0;
//	sprintf_s(ReturnBuff, DEFAULT_STR_BUFFER_SIZE * 10, "%s|%d|%d", ReturnBuff, CharStartX, FoundNewFont);
	return ReturnBuff;
}
