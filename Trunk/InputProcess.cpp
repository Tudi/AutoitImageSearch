#include "StdAfx.h"

LIBRARY_API ScreenshotStruct	ScreenshotCache[NR_SCREENSHOTS_CACHED];
LIBRARY_API ScreenshotStruct	*CurScreenshot = NULL, *PrevScreenshot = NULL;
LIBRARY_API int					ScreenshotStoreIndex = 0;

LIBRARY_API SearchedRegionMinMax g_SearchedRegions = { 10000, 10000, -10000, -10000 };
#define MAX_RETURN_BUFF_SIZE		1024
static char*					g_FuncReturnBuff = NULL;

class ConstructorForStaticVars
{
public:
	ConstructorForStaticVars()
	{
		for (size_t i = 0; i < NR_SCREENSHOTS_CACHED; i++)
		{
			ScreenshotCache[i].Constuctor();
		}
		CurScreenshot = PrevScreenshot = NULL;
		ScreenshotStoreIndex = 0;
		g_SearchedRegions = { 10000, 10000, -10000, -10000 };
		g_FuncReturnBuff = (char*)MY_ALLOC(MAX_RETURN_BUFF_SIZE);
	}
};
// just in case the other init fails
static ConstructorForStaticVars g_InitGlobalVariables;

void WINAPI CycleScreenshots()
{
	FileDebug("Started CycleScreenshots");
	PrevScreenshot = CurScreenshot;
	ScreenshotStoreIndex = ( ScreenshotStoreIndex + 1 ) % NR_SCREENSHOTS_CACHED;
	CurScreenshot = &ScreenshotCache[ ScreenshotStoreIndex ];
	FileDebug("\tFinished CycleScreenshots");
}

void WINAPI ReleaseScreenshot()
{
#ifndef REDUCE_ALLOC_COUNT
	if (CurScreenshot->Pixels)
	{
		MY_FREE(CurScreenshot->Pixels);
		CurScreenshot->Pixels = NULL;
	}
#endif
	if (CurScreenshot->PSCache)
	{
		delete CurScreenshot->PSCache;
		CurScreenshot->PSCache = NULL;
	}
	if (CurScreenshot->SSCache)
	{
		delete CurScreenshot->SSCache;
		CurScreenshot->SSCache = NULL;
	}
	if (CurScreenshot->SCCache)
	{
		delete CurScreenshot->SCCache;
		CurScreenshot->SCCache = NULL;
	}
	FreeSADSUMScreenshot(&CurScreenshot->SADSums);
}

void TakeNewScreenshot( int aLeft, int aTop, int aRight, int aBottom )
{
#ifndef REDUCE_ALLOC_COUNT
	if( CurScreenshot->Pixels != NULL )
	{
		FileDebug( "Can not take screenshot as older one was not yet released." );
		return;
	}
#endif
	int MaxWidth, MaxHeight;
	GetMaxDesktopResolution(&MaxWidth, &MaxHeight);

	LPCOLORREF PixelsBeforeReset = CurScreenshot->Pixels;
	CurScreenshot->Constuctor(); 
	CurScreenshot->Left = aLeft;
	CurScreenshot->Top = aTop;
	CurScreenshot->Right = aRight;
	CurScreenshot->Bottom = aBottom;
	CurScreenshot->NeedsSSCache = true;
	CurScreenshot->NeedsPSCache = true;
	CurScreenshot->NeedsAlphaRemoved = true;
	CurScreenshot->NeedsSplitChannelCache = true;
	CurScreenshot->BytesPerPixel = 4;
	CurScreenshot->TimeStampTaken = GetTickCount();

	COLORREF trans_color = CLR_NONE; // The default must be a value that can't occur naturally in an image.
	HBITMAP hbitmap_screen = NULL;
	HGDIOBJ sdc_orig_select = NULL;
	HDC hdc = NULL;
	HDC sdc = NULL;
    
	// Create an empty bitmap to hold all the pixels currently visible on the screen that lie within the search area:
	int search_width = aRight - aLeft;
	int search_height = aBottom - aTop;

	if( aLeft + search_width > MaxWidth )
		search_width = MaxWidth - aLeft;
	if( aTop + search_height > MaxHeight )
		search_height = MaxHeight - aTop;

	//make sure we did not capture more than we could. Avoid illegal memory read errors on bad screenshot params
	if( CurScreenshot->Right - CurScreenshot->Left > search_width )
		CurScreenshot->Right = CurScreenshot->Left + search_width;
	if( CurScreenshot->Bottom - CurScreenshot->Top > search_height )
		CurScreenshot->Bottom = CurScreenshot->Top + search_height;

	hdc = GetDC(NULL);
	if( !hdc )
	{
		FileDebug("\nTakeScreenshot:Failed GetDC");
		return;
	}
	sdc = CreateCompatibleDC(hdc);
	if (!sdc)
	{
		FileDebug("\nTakeScreenshot:Failed CreateCompatibleDC");
		return;
	}

	hbitmap_screen = CreateCompatibleBitmap(hdc, search_width, search_height);
	if( !hbitmap_screen )
		goto end;

	sdc_orig_select = SelectObject(sdc, hbitmap_screen);
	if( !sdc_orig_select )
		goto end;

	// Copy the pixels in the search-area of the screen into the DC to be searched:
	if( !(BitBlt(sdc, 0, 0, search_width, search_height, hdc, aLeft, aTop, SRCCOPY)) )
		goto end;

	LONG screen_width, screen_height;
	bool screen_is_16bit;
#ifdef REDUCE_ALLOC_COUNT
	// alloc max possible size we can capture. We will be keeping this
	if (PixelsBeforeReset == NULL) {
		PixelsBeforeReset = (LPCOLORREF)MY_ALLOC(MaxWidth * (MaxHeight + SSE_PADDING) * sizeof(COLORREF));
	}
	CurScreenshot->Pixels = getbits(hbitmap_screen, sdc, screen_width, screen_height, screen_is_16bit, 8, PixelsBeforeReset);
#else
	CurScreenshot->Pixels = getbits(hbitmap_screen, sdc, screen_width, screen_height, screen_is_16bit, 8, PixelsBeforeReset);
#endif
	if( !CurScreenshot->Pixels )
		goto end;
	CurScreenshot->Width = screen_width;
	CurScreenshot->Height = screen_height;

	// If either is 16-bit, convert *both* to the 16-bit-compatible 32-bit format:
	if (screen_is_16bit)
	{
		if (trans_color != CLR_NONE)
			trans_color &= 0x00F8F8F8; // Convert indicated trans-color to be compatible with the conversion below.
		LONG Pixels_count = screen_width * screen_height;
		for (int i = 0; i < Pixels_count; ++i)
			CurScreenshot->Pixels[i] &= 0x00F8F8F8; // Highest order byte must be masked to zero for consistency with use of 0x00FFFFFF below.
	}

end:
	// If found==false when execution reaches here, ErrorLevel is already set to the right value, so just
	// clean up then return.
	ReleaseDC(NULL, hdc);
	if (sdc)
	{
		if (sdc_orig_select) // i.e. the original call to SelectObject() didn't fail.
			SelectObject(sdc, sdc_orig_select); // Probably necessary to prevent memory leak.
		DeleteDC(sdc);
	}
	if (hbitmap_screen)
		DeleteObject(hbitmap_screen);
}

// if you do a lot of operations you do not want to spend a lot of time taking screenshots
// example : multiple image searches on the same screenshot : make sure to reuse the same screenshot to take advantage of lookup maps
static int TakeScreenshotFPSLimit = 5;
void WINAPI SetScreehotFPSLimit(int newFPSLimit)
{
	TakeScreenshotFPSLimit = newFPSLimit;
}

void WINAPI TakeScreenshot( int aLeft, int aTop, int aRight, int aBottom )
{
	FileDebug("\nTakeScreenshot:");
	// use auto optimized based on previous frame statistics
	if (aLeft == -1)
	{
		if (g_SearchedRegions.aBottom != -10000)
		{
			aLeft = g_SearchedRegions.aLeft;
			aTop = g_SearchedRegions.aTop;
			aRight = g_SearchedRegions.aRight;
			aBottom = g_SearchedRegions.aBottom;
			FileDebug("TakeScreenshot:Auto adjusted screenshot region");
		}
		else
		{
			aLeft = aTop = aRight = aBottom = 0;
			FileDebug("TakeScreenshot:Unknown screenshot region");
		}
	}
	int MaxWidth, MaxHeight;
	GetMaxDesktopResolution(&MaxWidth, &MaxHeight);

	// if we want a full screen screenshot
	if (aBottom == 0) {
		aBottom = MaxHeight;
		FileDebug("TakeScreenshot:Bottom was too large, adjusted to max");
	}
	if (aRight == 0) {
		aRight = MaxWidth;
		FileDebug("TakeScreenshot:Right was 0, adjusted to max");
	}

	// sanity checks
	if (aLeft == aRight || aTop == aBottom) {
		FileDebug("TakeScreenshot:Width or height is zero");
		return;
	}

	// in case params are inversed. Flip them to avoid negative numbers
	if (aLeft > aRight) {
		int t = aLeft;
		aLeft = aRight;
		aRight = t;
		FileDebug("TakeScreenshot:Flipped x cords");
	}
	// top is the smaller number
	if (aTop > aBottom) {
		int t = aTop;
		aTop = aBottom;
		aBottom = t;
		FileDebug("TakeScreenshot:Flipped y cords");
	}
	if (aLeft < 0)
	{
		FileDebug("TakeScreenshot:Left was negative. Set it to 0");
		aLeft = 0;
	}
	if (aTop < 0)
	{
		FileDebug("TakeScreenshot:Top was negative. Set it to 0");
		aTop = 0;
	}
	if (aRight > MaxWidth)
	{
		FileDebug("TakeScreenshot:Right was too large. Set it to desktop");
		aRight = MaxWidth;
	}
	if (aBottom > MaxHeight)
	{
		FileDebug("TakeScreenshot:Bottom was too large. Set it to desktop");
		aBottom = MaxHeight;
	}

	size_t startStamp = GetTickCount();
	char TBuff[2000];
	sprintf_s(TBuff, sizeof(TBuff), "Started taking the screenshot [%d,%d][%d,%d]", aLeft, aTop, aRight, aBottom);
	FileDebug(TBuff);

	if (CurScreenshot != NULL && TakeScreenshotFPSLimit > 0)
	{
		if (CurScreenshot->Left <= aLeft && CurScreenshot->Top <= aTop &&
			CurScreenshot->Right >= aRight && CurScreenshot->Bottom >= aBottom) {
			if (CurScreenshot->TimeStampTaken + 1000 / TakeScreenshotFPSLimit > GetTickCount()) // limit to 5 FPS ? Every 200 ms ? This process takes time :(
			{
				FileDebug("\tFinished taking the screenshot. Skipped because recent screenshot is too fresh");
			}
		}
	}

	CycleScreenshots();
	ReleaseScreenshot();
	TakeNewScreenshot( aLeft, aTop, aRight, aBottom );

	size_t endStamp = GetTickCount();
	sprintf_s(TBuff, sizeof(TBuff), "\tFinished taking the screenshot. Took %d ms", (int)(endStamp - startStamp));

	FileDebug(TBuff);
//	if( CurScreenshot->Pixels == NULL )
//		FileDebug( "WARNING:Screenshot buffer is null when taking the screenshot!" );

	return ;
}


char* WINAPI GetImageSize( char *aImageFile )
{
	CachedPicture *cache = CachePicture( aImageFile );
	if( cache == NULL )
	{
		FileDebug( "Skipping Image info as image could not be loaded" );
		strcpy_s(g_FuncReturnBuff, MAX_RETURN_BUFF_SIZE, "0|0");
		return g_FuncReturnBuff;
	}
	if( cache->Pixels == NULL )
	{
		FileDebug( "Skipping Image info as image pixels are missing" );
		strcpy_s(g_FuncReturnBuff, MAX_RETURN_BUFF_SIZE, "0|0");
		return g_FuncReturnBuff;
	}
	if( cache->LoadedPicture == NULL )
	{
		FileDebug( "Skipping Image info as image is missing" );
		strcpy_s(g_FuncReturnBuff, MAX_RETURN_BUFF_SIZE, "0|0");
		return g_FuncReturnBuff;
	}

	sprintf_s( ReturnBuff, DEFAULT_STR_BUFFER_SIZE*10, "%d|%d", cache->Width, cache->Height );

	return ReturnBuff;
}

char ReturnBuffIAC[DEFAULT_STR_BUFFER_SIZE*10];
char* WINAPI IsAnythingChanced( int StartX, int StartY, int EndX, int EndY )
{
	FileDebug( "Started IsAnythingChanced" );
	ReturnBuffIAC[0] = 0;
	if( CurScreenshot->Pixels == NULL )
	{
		FileDebug( "Skipping change search as no screenshot is available" );
		strcpy_s(g_FuncReturnBuff, MAX_RETURN_BUFF_SIZE, "0|0|0");
		return g_FuncReturnBuff;
	}
	// probably too lazy to set cords, reuse from previous screenshot
	if (StartX == EndX || StartY == EndY)
	{
		// take the screenshot
		TakeScreenshot(CurScreenshot->Left, CurScreenshot->Top, CurScreenshot->Right, CurScreenshot->Bottom);
	}
	if (PrevScreenshot == NULL || PrevScreenshot->Pixels == NULL)
	{
		FileDebug( "Skipping change search as no secondary screenshot is available" );
		strcpy_s(g_FuncReturnBuff, MAX_RETURN_BUFF_SIZE, "0|0|0");
		return g_FuncReturnBuff;
	}
	if( CurScreenshot->Left != PrevScreenshot->Left || CurScreenshot->Right != PrevScreenshot->Right || CurScreenshot->Top != PrevScreenshot->Top || CurScreenshot->Bottom != PrevScreenshot->Bottom )
	{
		FileDebug( "Screenshots were not taken from same place. Can't compare" );
		strcpy_s(g_FuncReturnBuff, MAX_RETURN_BUFF_SIZE, "0|0|0");
		return g_FuncReturnBuff;
	}
	if( StartY > EndY || StartX > EndX )
	{
		FileDebug( "Third / Fourth parameter does not seem to be width / height" );
		strcpy_s(g_FuncReturnBuff, MAX_RETURN_BUFF_SIZE, "0|0|0");
		return g_FuncReturnBuff;
	}
	int Width = CurScreenshot->Right - CurScreenshot->Left;
//	int Height = CurScreenshot->Bottom - CurScreenshot->Top;
	for( int y = StartY; y < EndY; y++ )
		for( int x = StartX; x < EndX; x++ )
			if( CurScreenshot->Pixels[ y * Width + x ] != PrevScreenshot->Pixels[ y * Width + x ] )
			{
				sprintf_s( ReturnBuffIAC, DEFAULT_STR_BUFFER_SIZE*10, "1|%d|%d", x, y );
				FileDebug( ReturnBuffIAC );
				return ReturnBuffIAC;
			}
	FileDebug( "\tEnd IsAnythingChanced" );
	strcpy_s(g_FuncReturnBuff, MAX_RETURN_BUFF_SIZE, "0|0|0");
	return g_FuncReturnBuff;
}

#if 0
void ScreenshotStruct::ReplaceReadOnlyPixels()
{
	if (Pixels)
	{
		size_t buffByteCount = Width * (Height + SSE_PADDING) * sizeof(COLORREF);
		LPCOLORREF PixelsNew = (LPCOLORREF)MY_ALLOC(buffByteCount + SSE_PADDING);
	if (PixelsNew)
		{
			FileDebug("\tReplaceReadOnlyPixels: Done");
			bPixelsAreReadonly = false;
			memcpy(PixelsNew, Pixels, buffByteCount);
			Pixels = PixelsNew;
		}
	}
}
#endif