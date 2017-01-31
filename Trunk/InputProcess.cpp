#include "StdAfx.h"

LIBRARY_API ScreenshotStruct	ScreenshotCache[ NR_SCREENSHOTS_CACHED ];
LIBRARY_API ScreenshotStruct	*CurScreenshot, *PrevScreenshot;
LIBRARY_API int					ScreenshotStoreIndex;

void WINAPI CycleScreenshots()
{
	PrevScreenshot = CurScreenshot;
	ScreenshotStoreIndex = ( ScreenshotStoreIndex + 1 ) % NR_SCREENSHOTS_CACHED;
	CurScreenshot = &ScreenshotCache[ ScreenshotStoreIndex ];
}

void WINAPI ReleaseScreenshot()
{
	if (CurScreenshot->Pixels)
	{
		_aligned_free(CurScreenshot->Pixels);
		CurScreenshot->Pixels = NULL;
	}
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
}

void TakeNewScreenshot( int aLeft, int aTop, int aRight, int aBottom )
{
	if( CurScreenshot->Pixels != NULL )
	{
		FileDebug( "Can not take screenshot as older one was not yet released." );
		return;
	}
	CurScreenshot->Left = aLeft;
	CurScreenshot->Top = aTop;
	CurScreenshot->Right = aRight;
	CurScreenshot->Bottom = aBottom;
	CurScreenshot->NeedsSSCache = true;
	CurScreenshot->NeedsPSCache = true;
	CurScreenshot->NeedsAlphaRemoved = true;
	CurScreenshot->NeedsSplitChannelCache = true;
	CurScreenshot->BytesPerPixel = 4;

	HDC sdc = NULL;
	HBITMAP hbitmap_screen = NULL;
	HGDIOBJ sdc_orig_select = NULL;
	COLORREF trans_color = CLR_NONE; // The default must be a value that can't occur naturally in an image.
    
	// Create an empty bitmap to hold all the pixels currently visible on the screen that lie within the search area:
	int search_width = aRight - aLeft;
	int search_height = aBottom - aTop;

	int MaxWidth, MaxHeight;
	GetMaxDesktopResolution( &MaxWidth, &MaxHeight );
	if( aLeft + search_width > MaxWidth )
		search_width = MaxWidth - aLeft;
	if( aTop + search_height > MaxHeight )
		search_height = MaxHeight - aTop;

	//make sure we did not capture more than we could. Avoid illegal memory read errors on bad screenshot params
	if( CurScreenshot->Right - CurScreenshot->Left > search_width )
		CurScreenshot->Right = CurScreenshot->Left + search_width;
	if( CurScreenshot->Bottom - CurScreenshot->Top > search_height )
		CurScreenshot->Bottom = CurScreenshot->Top + search_height;

	HDC hdc = GetDC(NULL);
	if( !hdc )
	{
		return;
	}

	sdc = CreateCompatibleDC(hdc);
	if( !sdc )
		goto end;

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
	CurScreenshot->Pixels = getbits(hbitmap_screen, sdc, screen_width, screen_height, screen_is_16bit);
	if( !CurScreenshot->Pixels )
		goto end;

	LONG Pixels_count = screen_width * screen_height;

	// If either is 16-bit, convert *both* to the 16-bit-compatible 32-bit format:
	if( screen_is_16bit )
	{
		if (trans_color != CLR_NONE)
			trans_color &= 0x00F8F8F8; // Convert indicated trans-color to be compatible with the conversion below.
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

void WINAPI TakeScreenshot( int aLeft, int aTop, int aRight, int aBottom )
{
	char TBuff[2000];
	sprintf_s(TBuff, sizeof(TBuff), "Started taking the screenshot [%d,%d][%d,%d]", aLeft, aTop, aRight, aBottom);
	FileDebug(TBuff);

	CycleScreenshots();
	ReleaseScreenshot();
	TakeNewScreenshot( aLeft, aTop, aRight, aBottom );

	FileDebug( "\tFinished taking the screenshot" );
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
		return "0|0";
	}
	if( cache->Pixels == NULL )
	{
		FileDebug( "Skipping Image info as image pixels are missing" );
		return "0|0";
	}
	if( cache->LoadedPicture == NULL )
	{
		FileDebug( "Skipping Image info as image is missing" );
		return "0|0";
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
		return "0|0|0";
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
		return "0|0|0";
	}
	if( CurScreenshot->Left != PrevScreenshot->Left || CurScreenshot->Right != PrevScreenshot->Right || CurScreenshot->Top != PrevScreenshot->Top || CurScreenshot->Bottom != PrevScreenshot->Bottom )
	{
		FileDebug( "Screenshots were not taken from same place. Can't compare" );
		return "0|0|0";
	}
	if( StartY > EndY || StartX > EndX )
	{
		FileDebug( "Third / Fourth parameter does not seem to be width / height" );
		return "0|0|0";
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
	return "0|0|0";
}
