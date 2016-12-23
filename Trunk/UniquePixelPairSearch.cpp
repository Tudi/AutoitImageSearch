#include "StdAfx.h"

// instead taking the color of a pixel, take into consideration the texture of the colors also. This would help in detecting edges even when there are very few colors
#define MATCH_REGION_SIZE			4
#define MATCH_PRECISION_DECREASE	0
#define MATCH_PRECISION_MASK		(~0x07)	// loose the last 3 bits to avoid shade effects / gradients / overlaps messing up our search

void WINAPI TakeScreenshotDetectUniquePixelsInitial(int aLeft, int aTop, int aRight, int aBottom)
{
	FileDebug("Started taking the screenshot in TakeScreenshotDetectUniqueFeaturesInitial");

	TakeScreenshot(aLeft, aTop, aRight, aBottom);
	RemoveScreenshotAlphaChannel(CurScreenshot);
	DecreaseColorPrecision(CurScreenshot, MATCH_PRECISION_DECREASE, MATCH_PRECISION_MASK);
}

/*
void WINAPI LoadImageDetectUniquePixelsInitial(char *aImageFile)
{
	CachedPicture *cache = CachePicture(aImageFile);
	if (cache == NULL)
	{
		FileDebug("Skipping Image load as image could not be loaded");
		return;
	}
	if (cache->Pixels == NULL)
	{
		FileDebug("Skipping Image load as image pixels are missing");
		return;
	}
	//copy over the pixels
} */

void WINAPI TakeScreenshotKeepUniquePixels(int aLeft, int aTop, int aRight, int aBottom)
{
	FileDebug("Started taking the screenshot in TakeScreenshotKeepUniquePixels");

	//take another screeenshot
	TakeScreenshot(aLeft, aTop, aRight, aBottom);
	RemoveScreenshotAlphaChannel(CurScreenshot);
	DecreaseColorPrecision(CurScreenshot, MATCH_PRECISION_DECREASE, MATCH_PRECISION_MASK);
	//flip current and prev pictures so that on next screenshot the initial picture will not get erased
	CycleScreenshots();

	//remove non unique features from the previous one
	COLORREF TransparentColor = 0x00FFFFFF;
	for (int y = 0; y < CurScreenshot->GetHeight() - MATCH_REGION_SIZE; y++)
		for (int x = 0; x < CurScreenshot->GetWidth() - MATCH_REGION_SIZE; x++)
		{
			//no need to search here if it is not a full block
			if (CurScreenshot->GetPixel(x, y) == TransparentColor)
				continue;

			//take a 2x2 pixel block form the intial subregion 
			COLORREF PixelBlock[MATCH_REGION_SIZE][MATCH_REGION_SIZE];
			for (int i = 0; i < MATCH_REGION_SIZE; i++)
				for (int j = 0; j < MATCH_REGION_SIZE; j++)
					PixelBlock[i][j] = CurScreenshot->GetPixel(x + j, y + i) ;

			//see if it is present in the new screenshot
			for (int y1 = 0; y1 < PrevScreenshot->GetHeight() - MATCH_REGION_SIZE; y1++)
				for (int x1 = 0; x1 < PrevScreenshot->GetWidth() - MATCH_REGION_SIZE; x1++)
				{
					int MatchCounter = 0;
					for (int i = 0; i < MATCH_REGION_SIZE; i++)
						for (int j = 0; j < MATCH_REGION_SIZE; j++)
							if (PixelBlock[i][j] == PrevScreenshot->GetPixel(x1 + j, y1 + i))
								MatchCounter++;

					if (MatchCounter == MATCH_REGION_SIZE*MATCH_REGION_SIZE)
					{
						//mark on the initial image this as a non usable match color
						CurScreenshot->SetPixel(x, y, TransparentColor);
						goto FINISHED_NEW_IMAGE_SEARCH;
					}
				}
FINISHED_NEW_IMAGE_SEARCH:
			;
		}
	return;
}

void WINAPI SaveScreenshotUniqueFeatures()
{
	SaveScreenshotCutTransparent();
}

// We took a screenshot of our Button/Base/Name.... 
// we want to make sure we remove the dynamic pixels out of it ( burning/blinking..)
// we want to find a small portion of it that can not be found in other places of the game except our base
char* WINAPI ImageSearchOnScreenshotUniqueFeatures(char *aFilespec, int AcceptedErrorCount, int StopAfterNMatches)
{
	char ReturnBuff2[DEFAULT_STR_BUFFER_SIZE * 10];
	int MatchesFound = 0;
	ReturnBuff[0] = 0;
	ReturnBuff2[0] = 0;
	FileDebug("Started Image search in ImageSearchOnScreenshotUniqueFeatures");
	CachedPicture *cache = CachePicture(aFilespec);
	if (cache == NULL)
	{
		FileDebug("Skipping Image search as image could not be loaded");
		return "";
	}
	if (cache->Pixels == NULL)
	{
		FileDebug("Skipping Image search as image pixels are missing");
		return "";
	}
	if (CurScreenshot->Pixels == NULL)
	{
		FileDebug("Skipping Image search no screenshot is available");
		return "";
	}

	int Width = CurScreenshot->Right - CurScreenshot->Left;
	int Height = CurScreenshot->Bottom - CurScreenshot->Top;
	int TransparentColor = 0x00FFFFFF;

	RemoveCacheAlphaChannel(cache);

	RemoveScreenshotAlphaChannel(CurScreenshot);
	DecreaseColorPrecision(CurScreenshot, MATCH_PRECISION_DECREASE, MATCH_PRECISION_MASK);

	//DumpAsPPM( &CurScreenshot->Pixels[ 40 * Width + 40 ], 40, 40, Width );
	//DumpAsPPM( &cache->Pixels[ 0 ], 40, 40 );
	for (int y = 0; y < Height - cache->Height; y += 1)
		for (int x = 0; x < Width - cache->Width; x += 1)
		{
			int ImageMatched = 1;
			int FoundErrors = 0;
			for (int y2 = 0; y2<cache->Height; y2++)
			{
				for (int x2 = 0; x2<cache->Width; x2++)
				{
					int PixelIndexDst = (0 + y2) * cache->Width + 0 + x2;
					COLORREF BGRDst = cache->Pixels[PixelIndexDst];
					if (BGRDst == TransparentColor)
						continue;
					int PixelIndexSrc = (y + y2) * Width + x + x2;
					COLORREF BGRSrc = CurScreenshot->Pixels[PixelIndexSrc];
					if (BGRSrc != BGRDst)
					{
						FoundErrors++;
						if (FoundErrors > AcceptedErrorCount)
						{
							ImageMatched = 0;
							goto AbandonIMageInImageSearch2;
						}
					}
				}
			}
AbandonIMageInImageSearch2:
			if (ImageMatched == 1)
			{
				FileDebug("Image search found a match");
				sprintf_s(ReturnBuff2, DEFAULT_STR_BUFFER_SIZE * 10, "%s|%d|%d", ReturnBuff2, CurScreenshot->Left + x, CurScreenshot->Top + y);
				FileDebug(ReturnBuff2);
				MatchesFound++;
				if (MatchesFound >= StopAfterNMatches)
					goto docleanupandreturn2;
			}
		}
docleanupandreturn2:
	;
	sprintf_s(ReturnBuff, DEFAULT_STR_BUFFER_SIZE * 10, "%d%s", MatchesFound, ReturnBuff2);
	FileDebug("\tImage search finished");
	return ReturnBuff;
}

// we expect that the region was taken from the same location !
void WINAPI TakeScreenshotRemoveDynamicPixels(int aLeft, int aTop, int aRight, int aBottom)
{
	FileDebug("Started taking the screenshot in TakeScreenshotRemoveDynamicPixels");

	//take another screeenshot
	TakeScreenshot(aLeft, aTop, aRight, aBottom);
	RemoveScreenshotAlphaChannel(CurScreenshot);
	DecreaseColorPrecision(CurScreenshot, MATCH_PRECISION_DECREASE, MATCH_PRECISION_MASK);
	//flip current and prev pictures so that on next screenshot the initial picture will not get erased
	CycleScreenshots();

	//remove non unique features from the previous one
	COLORREF TransparentColor = 0x00FFFFFF;
	for (int y = 0; y < CurScreenshot->GetHeight() - MATCH_REGION_SIZE; y++)
		for (int x = 0; x < CurScreenshot->GetWidth() - MATCH_REGION_SIZE; x++)
		{
			COLORREF CurPixel = CurScreenshot->GetPixel(x, y);
			//no need to search here if it is not a full block
			if (CurPixel == TransparentColor)
				continue;
			//if it changed, consider it a dynamic pixel
			if (PrevScreenshot->GetPixel(x, y) != CurPixel)
				CurScreenshot->SetPixel(x, y, TransparentColor);
		}
	return;
}

