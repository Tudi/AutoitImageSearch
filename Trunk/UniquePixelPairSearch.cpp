#include "StdAfx.h"

// instead taking the color of a pixel, take into consideration the texture of the colors also. This would help in detecting edges even when there are very few colors
#define MATCH_REGION_SIZE			2
//#define MATCH_PRECISION_DECREASE	0
//#define MATCH_PRECISION_MASK		(~0x07)	// loose the last 3 bits to avoid shade effects / gradients / overlaps messing up our search
#define USED_COLOR_COUNT			31	// because of gradients and transparent overlays
#define TRANSPARENT_COLOR			(0x00FFFFFF)

void TakeScreenshotNext(int aLeft, int aTop, int aRight, int aBottom, int Cycle = 1)
{
	//take another screeenshot
	TakeScreenshot(aLeft, aTop, aRight, aBottom);
//	RemoveScreenshotAlphaChannel(CurScreenshot);
//	DecreaseColorPrecision(CurScreenshot, MATCH_PRECISION_DECREASE, MATCH_PRECISION_MASK); // no rounding is used. Might end up with lots of black areas
	DecreaseColorCount(CurScreenshot, USED_COLOR_COUNT);	// rounding is used. Edge(size) of the image might change form one screeenshot to another if gradients are present
	//flip current and prev pictures so that on next screenshot the initial picture will not get erased
	// CurScreenshot is THE screenshot we will save. Contains the "unique" image
	if (Cycle == 1)
		CycleScreenshots();
}

void WINAPI TakeScreenshotDetectUniquePixelsInitial(int aLeft, int aTop, int aRight, int aBottom)
{
	FileDebug("Started taking the screenshot in TakeScreenshotDetectUniqueFeaturesInitial");

	TakeScreenshotNext(aLeft, aTop, aRight, aBottom, 0);
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

// we expect that the region was taken from the same location !
void WINAPI TakeScreenshotRemoveDynamicPixels(int aLeft, int aTop, int aRight, int aBottom)
{
	FileDebug("Started taking the screenshot in TakeScreenshotRemoveDynamicPixels");

	// size of the new screenshot should be at least as the Curent screenshot size
	if (aRight - aLeft < CurScreenshot->GetWidth() )
		FileDebug("Error TakeScreenshotRemoveDynamicPixels : new screenshot width is smaller than the reference image !");
	if (aBottom - aTop < CurScreenshot->GetHeight())
		FileDebug("Error TakeScreenshotRemoveDynamicPixels : new screenshot height is smaller than the reference image !");

	TakeScreenshotNext(aLeft, aTop, aRight, aBottom);

	//remove non unique features from the previous one
	COLORREF TransparentColor = TRANSPARENT_COLOR;
	for (int y = 0; y < CurScreenshot->GetHeight(); y++)
		for (int x = 0; x < CurScreenshot->GetWidth(); x++)
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

void WINAPI TakeScreenshotKeepUniquePixels(int aLeft, int aTop, int aRight, int aBottom, int BlockSize)
{
	FileDebug("Started taking the screenshot in TakeScreenshotKeepUniquePixels");

	TakeScreenshotNext(aLeft, aTop, aRight, aBottom);

	//remove non unique features from the previous one
	COLORREF *PixelBlock = (COLORREF *)malloc(sizeof(COLORREF) * BlockSize);
	COLORREF TransparentColor = TRANSPARENT_COLOR;
	for (int y = 0; y < CurScreenshot->GetHeight() - MATCH_REGION_SIZE; y++)
		for (int x = 0; x < CurScreenshot->GetWidth() - MATCH_REGION_SIZE; x++)
		{
			//no need to search here if it is not a full block
			if (CurScreenshot->GetPixel(x, y) == TransparentColor)
				continue;

			//take a NxN pixel block form the intial subregion 
			for (int i = 0; i < MATCH_REGION_SIZE; i++)
				for (int j = 0; j < MATCH_REGION_SIZE; j++)
					PixelBlock[i*MATCH_REGION_SIZE+j] = CurScreenshot->GetPixel(x + j, y + i);

			//see if it is present in the new screenshot
			for (int y1 = 0; y1 < PrevScreenshot->GetHeight() - MATCH_REGION_SIZE; y1++)
				for (int x1 = 0; x1 < PrevScreenshot->GetWidth() - MATCH_REGION_SIZE; x1++)
				{
					int MatchCounter = 0;
					for (int i = 0; i < MATCH_REGION_SIZE; i++)
						for (int j = 0; j < MATCH_REGION_SIZE; j++)
							if (PixelBlock[i*MATCH_REGION_SIZE+j] == PrevScreenshot->GetPixel(x1 + j, y1 + i))
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
	if (PixelBlock)
		free(PixelBlock);
	return;
}

void WINAPI SaveScreenshotUniqueFeatures()
{
	SaveScreenshotCutTransparent();
}

//search for the best possible location. Will be used to remove dynamic background of images
int LeastErrorX, LeastErrorY, LeastErrorCount;

// We took a screenshot of our Button/Base/Name.... 
// we want to make sure we remove the dynamic pixels out of it ( burning/blinking..)
// we want to find a small portion of it that can not be found in other places of the game except our base
char* WINAPI ImageSearchOnScreenshotUniqueFeatures(char *aFilespec, int AcceptedErrorCount, int StopAfterNFullMatches)
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
	int TransparentColor = TRANSPARENT_COLOR;

	RemoveCacheAlphaChannel(cache);

//	RemoveScreenshotAlphaChannel(CurScreenshot);
//	DecreaseColorPrecision(CurScreenshot, MATCH_PRECISION_DECREASE, MATCH_PRECISION_MASK);
	DecreaseColorCount(CurScreenshot, USED_COLOR_COUNT);

	//DumpAsPPM( &CurScreenshot->Pixels[ 40 * Width + 40 ], 40, 40, Width );
	//DumpAsPPM( &cache->Pixels[ 0 ], 40, 40 );
	LeastErrorX = LeastErrorY = LeastErrorCount = 0x0FFFFFFF;
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
				if (LeastErrorCount > FoundErrors)
				{
					LeastErrorX = CurScreenshot->Left + x;
					LeastErrorY = CurScreenshot->Top + y;
					LeastErrorCount = FoundErrors;
				}
				MatchesFound++;
				if (MatchesFound >= StopAfterNFullMatches && StopAfterNFullMatches > 0 )
					goto docleanupandreturn2;
			}
		}
docleanupandreturn2:
	;
	sprintf_s(ReturnBuff, DEFAULT_STR_BUFFER_SIZE * 10, "%d%s", MatchesFound, ReturnBuff2);
	//on last position, write the best possible match ( only different if we searched for more than 1 match )
	sprintf_s(ReturnBuff2, DEFAULT_STR_BUFFER_SIZE * 10, "%s|%d|%d|%d", ReturnBuff2, LeastErrorX, LeastErrorY, LeastErrorCount);
	FileDebug(ReturnBuff2);
	FileDebug("\tImage search finished");
	return ReturnBuff;
}

void WINAPI TakeScreenshotRemoveDynamicPixelsAutoPosition(int aLeft, int aTop, int aRight, int aBottom)
{
	FileDebug("Started taking the screenshot in TakeScreenshotRemoveDynamicPixelsAuto");

	// since we do not have the region, we will take a full screenshot
//	int MaxWidth, MaxHeight;
//	GetMaxDesktopResolution(&MaxWidth, &MaxHeight);
//	TakeScreenshotNext(0, 0, MaxWidth, MaxHeight);
	TakeScreenshotNext(aLeft, aTop, aRight, aBottom);

	//search for the best match of our image in the full screenshot
	FileDebug("\tStarted searching in TakeScreenshotRemoveDynamicPixelsAuto");
	int CurScrWidth = CurScreenshot->GetWidth();
	int CurScrHeight = CurScreenshot->GetHeight();
	int PrevScrWidth = PrevScreenshot->GetWidth();
	int PrevScrHeight = PrevScreenshot->GetHeight();

	if (PrevScrWidth < CurScrWidth)
		FileDebug("Error TakeScreenshotRemoveDynamicPixels : new screenshot width is smaller than the reference image !");
	if (PrevScrHeight < CurScrHeight)
		FileDebug("Error TakeScreenshotRemoveDynamicPixels : new screenshot height is smaller than the reference image !");

	LeastErrorX = LeastErrorY = LeastErrorCount = 0x0FFFFFFF;
	int TransparentColor = TRANSPARENT_COLOR;
	for (int y = 0; y < PrevScrHeight - CurScrHeight; y += 1)
		for (int x = 0; x < PrevScrWidth - CurScrWidth; x += 1)
		{
			int ImageMatched = 1;
			int FoundErrors = 0;
			for (int y2 = 0; y2 < CurScrHeight; y2++)
			{
				for (int x2 = 0; x2 < CurScrWidth; x2++)
				{
					int PixelIndexDst = (0 + y2) * CurScrWidth + 0 + x2;
					COLORREF BGRDst = CurScreenshot->Pixels[PixelIndexDst];
					if (BGRDst == TransparentColor)
						continue;
					int PixelIndexSrc = (y + y2) * PrevScrWidth + x + x2;
					COLORREF BGRSrc = PrevScreenshot->Pixels[PixelIndexSrc];
					if (BGRSrc != BGRDst)
					{
						FoundErrors++;
						if (FoundErrors > LeastErrorCount)
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
				if (LeastErrorCount > FoundErrors)
				{
					LeastErrorX = x;
					LeastErrorY = y;
					LeastErrorCount = FoundErrors;
				}
			}
		}
	FileDebug("\tFinished searching in TakeScreenshotRemoveDynamicPixelsAuto");

	FileDebug("\tStarted removing changed pixels in TakeScreenshotRemoveDynamicPixelsAuto");
	//now remove the dynamic pixels in this "very good" match
	for (int y = 0; y < CurScreenshot->GetHeight(); y++)
		for (int x = 0; x < CurScreenshot->GetWidth(); x++)
		{
			COLORREF CurPixel = CurScreenshot->GetPixel(x, y);
			//no need to search here if it is not a full block
			if (CurPixel == TransparentColor)
				continue;
			//if it changed, consider it a dynamic pixel
			if (PrevScreenshot->GetPixel(x + LeastErrorX, y + LeastErrorY) != CurPixel)
				CurScreenshot->SetPixel(x, y, TransparentColor);
		}
	FileDebug("\tFinished removing changed pixels in TakeScreenshotRemoveDynamicPixelsAuto");
}