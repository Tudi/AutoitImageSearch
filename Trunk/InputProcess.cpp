#include "StdAfx.h"

LIBRARY_API ScreenshotStruct	ScreenshotCache[ NR_SCREENSHOTS_CACHED ];
LIBRARY_API ScreenshotStruct	*CurScreenshot, *PrevScreenshot;
LIBRARY_API int					ScreenshotStoreIndex;

void CycleScreenshots()
{
	PrevScreenshot = CurScreenshot;
	ScreenshotStoreIndex = ( ScreenshotStoreIndex + 1 ) % NR_SCREENSHOTS_CACHED;
	CurScreenshot = &ScreenshotCache[ ScreenshotStoreIndex ];
}

void WINAPI ReleaseScreenshot()
{
	if( CurScreenshot->Pixels )
		free( CurScreenshot->Pixels );
	CurScreenshot->Pixels = NULL;
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

	HDC sdc = NULL;
	HBITMAP hbitmap_screen = NULL;
	HGDIOBJ sdc_orig_select = NULL;
	COLORREF trans_color = CLR_NONE; // The default must be a value that can't occur naturally in an image.
    
	// Create an empty bitmap to hold all the pixels currently visible on the screen that lie within the search area:
	int search_width = aRight - aLeft;
	int search_height = aBottom - aTop;

	HDC hdc = GetDC(NULL);
	if( !hdc )
	{
		return;
	}

	if( !(sdc = CreateCompatibleDC(hdc)) || !(hbitmap_screen = CreateCompatibleBitmap(hdc, search_width, search_height)) )
		goto end;

	if( !(sdc_orig_select = SelectObject(sdc, hbitmap_screen))   )
		goto end;

	// Copy the pixels in the search-area of the screen into the DC to be searched:
	if( !(BitBlt(sdc, 0, 0, search_width, search_height, hdc, aLeft, aTop, SRCCOPY)) )
		goto end;

	LONG screen_width, screen_height;
	bool screen_is_16bit;
	if( !(CurScreenshot->Pixels = getbits(hbitmap_screen, sdc, screen_width, screen_height, screen_is_16bit)) )
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
	FileDebug( "Started taking the screenshot" );
	CycleScreenshots();
	ReleaseScreenshot();
	TakeNewScreenshot( aLeft, aTop, aRight, aBottom );

	FileDebug( "\tFinished taking the screenshot" );
//	if( CurScreenshot->Pixels == NULL )
//		FileDebug( "WARNING:Screenshot buffer is null when taking the screenshot!" );

	return ;
}

char ReturnBuff[DEFAULT_STR_BUFFER_SIZE*10];
char* WINAPI ImageSearchOnScreenshot( char *aFilespec, int TransparentColor, int AcceptedColorDiff, int AcceptedErrorCount, int StopAfterNMatches )
{
	char ReturnBuff2[DEFAULT_STR_BUFFER_SIZE*10];
	int MatchesFound = 0;
	ReturnBuff[0]=0;
	ReturnBuff2[0]=0;
	FileDebug( "Started Image search" );
	CachedPicture *cache = CachePicture( aFilespec );
	if( cache == NULL )
	{
		FileDebug( "Skipping Image search as image could not be loaded" );
		return "";
	}
	if( cache->Pixels == NULL )
	{
		FileDebug( "Skipping Image search as image pixels are missing" );
		return "";
	}
	if( cache->LoadedPicture == NULL )
	{
		FileDebug( "Skipping Image search as image is missing" );
		return "";
	}
	if( CurScreenshot->Pixels == NULL )
	{
		FileDebug( "Skipping Image search no screenshot is available" );
		return "";
	}

	int Width = CurScreenshot->Right - CurScreenshot->Left;
	int Height = CurScreenshot->Bottom - CurScreenshot->Top;
	if( AcceptedColorDiff > 0 )
	{
		CheckPrepareToleranceMaps( cache, AcceptedColorDiff, TransparentColor );
//DumpAsPPM( MinMap[0], MinMap[1], MinMap[2], cache->Width, cache->Height );
//DumpAsPPM( &CurScreenshot->Pixels[ 40 * Width + 40 ], 40, 40, Width );
//DumpAsPPM( MaxMap[0], MaxMap[1], MaxMap[2], cache->Width, cache->Height );
		for( int y = 0; y < Height; y +=1 )
			for( int x = 0; x < Width; x += 1 )
			{
				int ImageMatched = 1;
				int FoundErrors = 0;
				for( int y2=0;y2<cache->Height;y2++ )
				{
					for( int x2=0;x2<cache->Width;x2++)
					{
						int PixelIndexDst = ( 0 + y2 ) * cache->Width + 0 + x2;
						int PixelIndexSrc = ( y + y2 ) * Width + x + x2;
						COLORREF BGRSrc = CurScreenshot->Pixels[ PixelIndexSrc ];
						int Cols[3];
						Cols[0] = ( (int)BGRSrc >> 0 ) & 0xFF;
						if( cache->MinMap[0][PixelIndexDst] <= Cols[0] && Cols[0] <= cache->MaxMap[0][PixelIndexDst] )
						{
							Cols[1] = ( (int)BGRSrc >> 8 ) & 0xFF;
							if( cache->MinMap[1][PixelIndexDst] <= Cols[1] && Cols[1] <= cache->MaxMap[1][PixelIndexDst] )
							{
								Cols[2] = ( (int)BGRSrc >> 16 ) & 0xFF;
								if( cache->MinMap[2][PixelIndexDst] <= Cols[2] && Cols[2] <= cache->MaxMap[2][PixelIndexDst] )
								{
									//yaay it matches = nothig to do here
								}
								else
									FoundErrors++;
							}
							else
								FoundErrors++;
						}
						else
							FoundErrors++;

						if( FoundErrors > AcceptedErrorCount )
						{
							ImageMatched = 0;
							goto AbandonIMageInImageSearch;
						}
					}
				}
AbandonIMageInImageSearch:
				if( ImageMatched == 1 )
				{
					FileDebug( "Image search found a match" );
					sprintf_s( ReturnBuff2, DEFAULT_STR_BUFFER_SIZE*10, "%s|%d|%d", ReturnBuff, x, y );
					FileDebug( ReturnBuff2 );
					MatchesFound++;
					if( MatchesFound >= StopAfterNMatches )
						goto docleanupandreturn;
				}
			}
docleanupandreturn:
			;
	}
	else
	{
//DumpAsPPM( &CurScreenshot->Pixels[ 40 * Width + 40 ], 40, 40, Width );
//DumpAsPPM( &cache->Pixels[ 0 ], 40, 40 );
		for( int y = 0; y < Height; y +=1 )
			for( int x = 0; x < Width; x += 1 )
			{
				int ImageMatched = 1;
				int FoundErrors = 0;
				for( int y2=0;y2<cache->Height;y2++ )
				{
					for( int x2=0;x2<cache->Width;x2++)
					{
						int PixelIndexSrc = ( y + y2 ) * Width + x + x2;
						int PixelIndexDst = ( 0 + y2 ) * cache->Width + 0 + x2;
						COLORREF BGRSrc = CurScreenshot->Pixels[ PixelIndexSrc ];
						COLORREF BGRDst = cache->Pixels[ PixelIndexDst ];
						if( BGRDst == TransparentColor || BGRSrc == BGRDst )
						{
							//yaay it matches = nothig to do here
						}
						else
							FoundErrors++;

						if( FoundErrors > AcceptedErrorCount )
						{
							ImageMatched = 0;
							goto AbandonIMageInImageSearch2;
						}
					}
				}
AbandonIMageInImageSearch2:
				if( ImageMatched == 1 )
				{
					FileDebug( "Image search found a match" );
					sprintf_s( ReturnBuff2, DEFAULT_STR_BUFFER_SIZE*10, "%s|%d|%d", ReturnBuff, x, y );
					FileDebug( ReturnBuff2 );
					MatchesFound++;
					if( MatchesFound >= StopAfterNMatches )
						goto docleanupandreturn2;
				}
			}
docleanupandreturn2:
			;
	}
	sprintf_s( ReturnBuff, DEFAULT_STR_BUFFER_SIZE*10, "%d%s", MatchesFound, ReturnBuff2 );
	return ReturnBuff;
}

char* WINAPI ImageSearchOnScreenshotMasked( char *aFilespec, char *MaskFile, int TransparentColor, int AcceptedColorDiff, int AcceptedErrorCount, int StopAfterNMatches )
{
	char ReturnBuff2[DEFAULT_STR_BUFFER_SIZE*10];
	int MatchesFound = 0;
	ReturnBuff[0]=0;
	ReturnBuff2[0]=0;
	FileDebug( "Started Image search" );

	CachedPicture *cache = CachePicture( aFilespec );
	if( cache == NULL )
	{
		FileDebug( "Skipping Image search as image could not be loaded" );
		return "";
	}
	if( cache->Pixels == NULL )
	{
		FileDebug( "Skipping Image search as image pixels are missing" );
		return "";
	}
	if( cache->LoadedPicture == NULL )
	{
		FileDebug( "Skipping Image search as image is missing" );
		return "";
	}

	CachedPicture *mask = CachePicture( MaskFile );
	if( mask == NULL )
	{
		FileDebug( "Skipping Image search as mask could not be loaded" );
		return "";
	}
	if( mask->Pixels == NULL )
	{
		FileDebug( "Skipping Image search as mask pixels are missing" );
		return "";
	}
	if( mask->LoadedPicture == NULL )
	{
		FileDebug( "Skipping Image search as mask is missing" );
		return "";
	}

	if( CurScreenshot->Pixels == NULL )
	{
		FileDebug( "Skipping Image search no screenshot is available" );
		return "";
	}

	int Width = CurScreenshot->Right - CurScreenshot->Left;
	int Height = CurScreenshot->Bottom - CurScreenshot->Top;

	if( mask->Width != Width || mask->Height != Height )
	{
		FileDebug( "Warning : Image search mask does not have same size as screenshot" );
	}

	if( AcceptedColorDiff > 0 )
	{
		CheckPrepareToleranceMaps( cache, AcceptedColorDiff, TransparentColor );
//DumpAsPPM( MinMap[0], MinMap[1], MinMap[2], cache->Width, cache->Height );
//DumpAsPPM( &CurScreenshot->Pixels[ 40 * Width + 40 ], 40, 40, Width );
//DumpAsPPM( MaxMap[0], MaxMap[1], MaxMap[2], cache->Width, cache->Height );
		for( int y = 0; y < Height; y +=1 )
			for( int x = 0; x < Width; x += 1 )
			{
				int ImageMatched = 1;
				int FoundErrors = 0;
				for( int y2=0;y2<cache->Height;y2++ )
				{
					for( int x2=0;x2<cache->Width;x2++)
					{
						int PixelIndexDst = ( 0 + y2 ) * cache->Width + 0 + x2;
						int PixelIndexSrc = ( y + y2 ) * Width + x + x2;
						COLORREF BGRSrc = CurScreenshot->Pixels[ PixelIndexSrc ];
						COLORREF BGRSrcMask = mask->Pixels[ PixelIndexSrc ];
						if( BGRSrcMask == 0 )	//screenshot pixel value is hidden and should not be compared. Used for overlays
							continue;
						int Cols[3];
						Cols[0] = ( (int)BGRSrc >> 0 ) & 0xFF;
						if( cache->MinMap[0][PixelIndexDst] <= Cols[0] && Cols[0] <= cache->MaxMap[0][PixelIndexDst] )
						{
							Cols[1] = ( (int)BGRSrc >> 8 ) & 0xFF;
							if( cache->MinMap[1][PixelIndexDst] <= Cols[1] && Cols[1] <= cache->MaxMap[1][PixelIndexDst] )
							{
								Cols[2] = ( (int)BGRSrc >> 16 ) & 0xFF;
								if( cache->MinMap[2][PixelIndexDst] <= Cols[2] && Cols[2] <= cache->MaxMap[2][PixelIndexDst] )
								{
									//yaay it matches = nothig to do here
								}
								else
									FoundErrors++;
							}
							else
								FoundErrors++;
						}
						else
							FoundErrors++;

						if( FoundErrors > AcceptedErrorCount )
						{
							ImageMatched = 0;
							goto AbandonIMageInImageSearch;
						}
					}
				}
AbandonIMageInImageSearch:
				if( ImageMatched == 1 )
				{
					FileDebug( "Image search found a match" );
					sprintf_s( ReturnBuff2, DEFAULT_STR_BUFFER_SIZE*10, "%s|%d|%d", ReturnBuff, x, y );
					FileDebug( ReturnBuff2 );
					MatchesFound++;
					if( MatchesFound >= StopAfterNMatches )
						goto docleanupandreturn;
				}
			}
docleanupandreturn:
			;
	}
	else
	{
//DumpAsPPM( &CurScreenshot->Pixels[ 40 * Width + 40 ], 40, 40, Width );
//DumpAsPPM( &cache->Pixels[ 0 ], 40, 40 );
		for( int y = 0; y < Height; y +=1 )
			for( int x = 0; x < Width; x += 1 )
			{
				int ImageMatched = 1;
				int FoundErrors = 0;
				for( int y2=0;y2<cache->Height;y2++ )
				{
					for( int x2=0;x2<cache->Width;x2++)
					{
						int PixelIndexSrc = ( y + y2 ) * Width + x + x2;
						int PixelIndexDst = ( 0 + y2 ) * cache->Width + 0 + x2;
						COLORREF BGRSrc = CurScreenshot->Pixels[ PixelIndexSrc ];
						COLORREF BGRDst = cache->Pixels[ PixelIndexDst ];
						COLORREF BGRSrcMask = mask->Pixels[ PixelIndexSrc ];
						if( BGRSrcMask == 0 )	//screenshot pixel value is hidden and should not be compared. Used for overlays
							continue;
						if( BGRDst == TransparentColor || BGRSrc == BGRDst )
						{
							//yaay it matches = nothig to do here
						}
						else
							FoundErrors++;

						if( FoundErrors > AcceptedErrorCount )
						{
							ImageMatched = 0;
							goto AbandonIMageInImageSearch2;
						}
					}
				}
AbandonIMageInImageSearch2:
				if( ImageMatched == 1 )
				{
					FileDebug( "Image search found a match" );
					sprintf_s( ReturnBuff2, DEFAULT_STR_BUFFER_SIZE*10, "%s|%d|%d", ReturnBuff, x, y );
					FileDebug( ReturnBuff2 );
					MatchesFound++;
					if( MatchesFound >= StopAfterNMatches )
						goto docleanupandreturn2;
				}
			}
docleanupandreturn2:
			;
	}
	sprintf_s( ReturnBuff, DEFAULT_STR_BUFFER_SIZE*10, "%d%s", MatchesFound, ReturnBuff2 );
	return ReturnBuff;
}
