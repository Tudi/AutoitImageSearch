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
		_aligned_free( CurScreenshot->Pixels );
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
	CurScreenshot->NeedsSSCache = true;
	CurScreenshot->NeedsPSCache = true;
	CurScreenshot->NeedsAlphaRemoved = true;
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

char ReturnBuff[DEFAULT_STR_BUFFER_SIZE*10];
char* WINAPI ImageSearchOnScreenshot( char *aFilespec, int TransparentColor, int AcceptedColorDiff, int AcceptedErrorCount, int StopAfterNFullMatches )
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
	if( CurScreenshot->Pixels == NULL )
	{
		FileDebug( "Skipping Image search no screenshot is available" );
		return "";
	}

	int Width = CurScreenshot->Right - CurScreenshot->Left;
	int Height = CurScreenshot->Bottom - CurScreenshot->Top;
	if( AcceptedColorDiff > 0 )
	{
		FileDebug( "\t Image search with tolerance" );
		CheckPrepareToleranceMaps( cache, AcceptedColorDiff, TransparentColor );
		FileDebug( "\t Image search generated tolerance maps" );
//DumpAsPPM( MinMap[0], MinMap[1], MinMap[2], cache->Width, cache->Height );
//DumpAsPPM( &CurScreenshot->Pixels[ 40 * Width + 40 ], 40, 40, Width );
//DumpAsPPM( MaxMap[0], MaxMap[1], MaxMap[2], cache->Width, cache->Height );
		for( int y = 0; y < Height - cache->Height; y +=1 )
		{
			for( int x = 0; x < Width - cache->Width; x += 1 )
			{
				int ImageMatched = 1;
				int FoundErrors = 0;
				for( int y2=0;y2<cache->Height;y2++ )
				{
//					if( y + y2 >= Height )
//						break;
					for( int x2=0;x2<cache->Width;x2++)
					{
//						if( x + x2 >= Width )
//							break;
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
					if( strlen( ReturnBuff2 ) < DEFAULT_STR_BUFFER_SIZE*10 - 2 * 10 )	//is this even supposed to crash with sprintf_s ?
						sprintf_s( ReturnBuff2, DEFAULT_STR_BUFFER_SIZE*10, "%s|%d|%d", ReturnBuff2, CurScreenshot->Left + x, CurScreenshot->Top + y );
					FileDebug( ReturnBuff2 );
					MatchesFound++;
					if( MatchesFound >= StopAfterNFullMatches )
						goto docleanupandreturn;
				}
			}
		}
docleanupandreturn:
			if( MatchesFound == 0 )
				FileDebug( "\t Image search found no matches" );
	}
	else
	{
		RemoveCacheAlphaChannel( cache );
		RemoveScreenshotAlphaChannel( CurScreenshot );
		if( TransparentColor > 0x00FFFFFF && AcceptedErrorCount == 0 && StopAfterNFullMatches == 1 )
		{
			for( int y = 0; y < Height - cache->Height; y +=1 )
				for( int x = 0; x < Width - cache->Width; x += 1 )
				{
					for( int y2=0;y2<cache->Height;y2++ )
					{
						int PixelIndexSrc = ( y + y2 ) * Width + x;
						int PixelIndexDst = ( 0 + y2 ) * cache->Width;
						for( int x2=0;x2<cache->Width;x2++)
						{
							if( CurScreenshot->Pixels[ PixelIndexSrc++ ] != cache->Pixels[ PixelIndexDst++ ] )
								goto AbandonIMageInImageSearch3;
						}
					}
					FileDebug( "Image search found a match" );
					sprintf_s( ReturnBuff2, DEFAULT_STR_BUFFER_SIZE*10, "%s|%d|%d", ReturnBuff2, CurScreenshot->Left + x, CurScreenshot->Top + y );
					FileDebug( ReturnBuff2 );
					MatchesFound = 1;
					goto docleanupandreturn3;
AbandonIMageInImageSearch3:
					;
				}
docleanupandreturn3:
				;
		}
		else
		{
//DumpAsPPM( &CurScreenshot->Pixels[ 40 * Width + 40 ], 40, 40, Width );
//DumpAsPPM( &cache->Pixels[ 0 ], 40, 40 );
			for( int y = 0; y < Height - cache->Height; y +=1 )
				for( int x = 0; x < Width - cache->Width; x += 1 )
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
								FoundErrors =  FoundErrors;
							}
							else
							{
								FoundErrors++;
								if( FoundErrors > AcceptedErrorCount )
								{
									ImageMatched = 0;
									goto AbandonIMageInImageSearch2;
								}
							}
						}
					}
AbandonIMageInImageSearch2:
					if( ImageMatched == 1 )
					{
						FileDebug( "Image search found a match" );
						sprintf_s( ReturnBuff2, DEFAULT_STR_BUFFER_SIZE*10, "%s|%d|%d", ReturnBuff2, CurScreenshot->Left + x, CurScreenshot->Top + y );
						FileDebug( ReturnBuff2 );
						MatchesFound++;
						if( MatchesFound >= StopAfterNFullMatches )
							goto docleanupandreturn2;
					}
				}
docleanupandreturn2:
				;
		}
	}
	sprintf_s( ReturnBuff, DEFAULT_STR_BUFFER_SIZE*10, "%d%s", MatchesFound, ReturnBuff2 );
	FileDebug( "\tImage search finished" );
	return ReturnBuff;
}

char* WINAPI ImageSearchOnScreenshotMasked( char *aFilespec, char *MaskFile, int TransparentColor, int AcceptedColorDiff, int AcceptedErrorCount, int StopAfterNFullMatches )
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
		for( int y = 0; y < Height - cache->Height; y +=1 )
			for( int x = 0; x < Width - cache->Width; x += 1 )
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
					sprintf_s( ReturnBuff2, DEFAULT_STR_BUFFER_SIZE*10, "%s|%d|%d", ReturnBuff2, CurScreenshot->Left + x, CurScreenshot->Top + y );
					FileDebug( ReturnBuff2 );
					MatchesFound++;
					if( MatchesFound >= StopAfterNFullMatches )
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
		for( int y = 0; y < Height - cache->Height; y +=1 )
			for( int x = 0; x < Width - cache->Width; x += 1 )
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
					sprintf_s( ReturnBuff2, DEFAULT_STR_BUFFER_SIZE*10, "%s|%d|%d", ReturnBuff2, CurScreenshot->Left + x, CurScreenshot->Top + y );
					FileDebug( ReturnBuff2 );
					MatchesFound++;
					if( MatchesFound >= StopAfterNFullMatches )
						goto docleanupandreturn2;
				}
			}
docleanupandreturn2:
			;
	}
	sprintf_s( ReturnBuff, DEFAULT_STR_BUFFER_SIZE*10, "%d%s", MatchesFound, ReturnBuff2 );
	return ReturnBuff;
}

char* WINAPI ImageSearchOnScreenshotBest( char *aFilespec )
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
	if( CurScreenshot->Pixels == NULL )
	{
		FileDebug( "Skipping Image search no screenshot is available" );
		return "";
	}

	int Width = CurScreenshot->Right - CurScreenshot->Left;
	int Height = CurScreenshot->Bottom - CurScreenshot->Top;

	int retx = -1;
	int rety = -1;
	unsigned int BestSAD = 0x7FFFFFFF;
	{
//DumpAsPPM( MinMap[0], MinMap[1], MinMap[2], cache->Width, cache->Height );
//DumpAsPPM( &CurScreenshot->Pixels[ 40 * Width + 40 ], 40, 40, Width );
//DumpAsPPM( MaxMap[0], MaxMap[1], MaxMap[2], cache->Width, cache->Height );
		for( int y = 0; y < Height - cache->Height; y +=1 )
		{
			for( int x = 0; x < Width - cache->Width; x += 1 )
			{
/*				int RDiffSum = 0;
				int GDiffSum = 0;
				int BDiffSum = 0;

//				int pixelcount1 = 0;
				for( int y2=0;y2<cache->Height;y2++ )
				{
					LPCOLORREF AddrBig = &CurScreenshot->Pixels[ ( y + y2 ) * Width + x ];
					LPCOLORREF AddSmall = &cache->Pixels[ ( 0 + y2 ) * cache->Width + 0 ];
//					for( int x2=0;x2<cache->Width;x2++)
					for( int x2=0;x2<(cache->Width & ~0x03);x2++)
					{
						COLORREF BGRDst = AddSmall[ x2 ];
						if( BGRDst == TransparentColor )
							continue;

						COLORREF BGRSrc = AddrBig[ x2 ];

						int RDiff = ( ( BGRSrc & 0x0000FF ) >> 0  ) - ( ( BGRDst & 0x0000FF ) >> 0  );
						int GDiff = ( ( BGRSrc & 0x00FF00 ) >> 8  ) - ( ( BGRDst & 0x00FF00 ) >> 8  );
						int BDiff = ( ( BGRSrc & 0xFF0000 ) >> 16 ) - ( ( BGRDst & 0xFF0000 ) >> 16 );

						RDiff = abs( RDiff ) ;
						GDiff = abs( GDiff ) ;
						BDiff = abs( BDiff ) ;

						RDiffSum += RDiff;
						GDiffSum += GDiff;
						BDiffSum += BDiff;
//						pixelcount1++;
					}
				} 
				sad = RDiffSum + GDiffSum + BDiffSum;
				*/

//				int pixelcount2 = 0;
				unsigned int sad_array[4];
				__m128i l0, l1, line_sad, acc_sad, alpha_mask;

				acc_sad = _mm_setzero_si128();
				alpha_mask = _mm_setzero_si128();

				alpha_mask.m128i_u32[0] = 0x00FFFFFF;
				alpha_mask.m128i_u32[1] = 0x00FFFFFF;
				alpha_mask.m128i_u32[2] = 0x00FFFFFF;
				alpha_mask.m128i_u32[3] = 0x00FFFFFF;

				for( int y2=0;y2<cache->Height;y2++ )
				{
					LPCOLORREF AddrBig = &CurScreenshot->Pixels[ ( y + y2 ) * Width + x ];
					LPCOLORREF AddSmall = &cache->Pixels[ ( 0 + y2 ) * cache->Width + 0 ];
					for( int x2=0;x2<(cache->Width & ~0x03); x2+= 4 )
//					for( int x2=0;x2<4;x2+=4)
					{
//assert( ( AddrBig[0] & 0xFF000000 ) == 0 && ( AddrBig[1] & 0xFF000000 ) == 0 && ( AddrBig[2] & 0xFF000000 ) == 0 && ( AddrBig[3] & 0xFF000000 ) == 0 );
//assert( ( AddSmall[0] & 0xFF000000 ) == 0 && ( AddSmall[1] & 0xFF000000 ) == 0 && ( AddSmall[2] & 0xFF000000 ) == 0 && ( AddSmall[3] & 0xFF000000 ) == 0 );
						l0 = _mm_loadu_si128((__m128i*)AddrBig);
						l0 = _mm_and_si128( l0, alpha_mask );
						l1 = _mm_loadu_si128((__m128i*)AddSmall);
						line_sad = _mm_sad_epu8( l0, l1 );
						acc_sad = _mm_add_epi32( acc_sad, line_sad );

						AddrBig += 4;
						AddSmall += 4;
//						pixelcount2 += 4;
					}
				}

				_mm_storeu_si128((__m128i*)(&sad_array[0]), acc_sad);

				unsigned int sad = sad_array[0]+sad_array[2];

//				assert( sad == RDiffSum + GDiffSum + BDiffSum );

				if( BestSAD > sad )
				{
					BestSAD = sad;
					retx = x + CurScreenshot->Left;
					rety = y + CurScreenshot->Top;
					//exact match ? I doubt it will ever happen...
					if( BestSAD == 0 )
						goto docleanupandreturn;
				}
			}
		}
docleanupandreturn:
			if( MatchesFound == 0 )
				FileDebug( "\t Image search found no matches" );
	}

	sprintf_s( ReturnBuff, DEFAULT_STR_BUFFER_SIZE*10, "1|%d|%d|%d", retx, rety, BestSAD );
	FileDebug( "\tImage search finished" );
	return ReturnBuff;
}

char* WINAPI ImageSearchOnScreenshotBestTransparent( char *aFilespec )
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
	if( CurScreenshot->Pixels == NULL )
	{
		FileDebug( "Skipping Image search no screenshot is available" );
		return "";
	}

	int Width = CurScreenshot->Right - CurScreenshot->Left;
	int Height = CurScreenshot->Bottom - CurScreenshot->Top;

	int retx = -1;
	int rety = -1;
	unsigned int BestSAD = 0x7FFFFFFF;
	unsigned int TransparentColor = 0x00FFFFFF;
	{
//DumpAsPPM( MinMap[0], MinMap[1], MinMap[2], cache->Width, cache->Height );
//DumpAsPPM( &CurScreenshot->Pixels[ 40 * Width + 40 ], 40, 40, Width );
//DumpAsPPM( MaxMap[0], MaxMap[1], MaxMap[2], cache->Width, cache->Height );
		for( int y = 0; y < Height - cache->Height; y +=1 )
		{
			for( int x = 0; x < Width - cache->Width; x += 1 )
			{
				int RDiffSum = 0;
				int GDiffSum = 0;
				int BDiffSum = 0;

				for( int y2=0;y2<cache->Height;y2++ )
				{
					LPCOLORREF AddrBig = &CurScreenshot->Pixels[ ( y + y2 ) * Width + x ];
					LPCOLORREF AddSmall = &cache->Pixels[ ( 0 + y2 ) * cache->Width + 0 ];
					for( int x2=0;x2<cache->Width;x2++)
					{
						COLORREF BGRDst = AddSmall[ x2 ];
						if( BGRDst == TransparentColor )
							continue;

						COLORREF BGRSrc = AddrBig[ x2 ];

						int RDiff = ( ( BGRSrc & 0x0000FF ) >> 0  ) - ( ( BGRDst & 0x0000FF ) >> 0  );
						int GDiff = ( ( BGRSrc & 0x00FF00 ) >> 8  ) - ( ( BGRDst & 0x00FF00 ) >> 8  );
						int BDiff = ( ( BGRSrc & 0xFF0000 ) >> 16 ) - ( ( BGRDst & 0xFF0000 ) >> 16 );

						RDiff = abs( RDiff ) ;
						GDiff = abs( GDiff ) ;
						BDiff = abs( BDiff ) ;

						RDiffSum += RDiff;
						GDiffSum += GDiff;
						BDiffSum += BDiff;
					}
				} 
				unsigned int sad = RDiffSum + GDiffSum + BDiffSum;
				if( BestSAD > sad )
				{
					BestSAD = sad;
					retx = x + CurScreenshot->Left;
					rety = y + CurScreenshot->Top;
					//exact match ? I doubt it will ever happen...
					if( BestSAD == 0 )
						goto docleanupandreturn;
				}
			}
		}
docleanupandreturn:
			if( MatchesFound == 0 )
				FileDebug( "\t Image search found no matches" );
	}

	sprintf_s( ReturnBuff, DEFAULT_STR_BUFFER_SIZE*10, "1|%d|%d|%d", retx, rety, BestSAD );
	FileDebug( "\tImage search finished" );
	return ReturnBuff;
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
	if( PrevScreenshot == NULL || PrevScreenshot->Pixels == NULL )
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

void WINAPI ApplyColorBitmask(int Mask)
{
	int PixelCount = CurScreenshot->GetWidth() * CurScreenshot->GetHeight();
	for (int i = 0; i < PixelCount; i++)
		CurScreenshot->Pixels[i] = CurScreenshot->Pixels[i] & Mask;
}

void RemoveScreenshotAlphaChannel( ScreenshotStruct *cache )
{
	if( cache->NeedsAlphaRemoved == true )
	{
		cache->NeedsAlphaRemoved = false;
		int PixelCount = cache->GetWidth() * cache->GetHeight();
		for( int i=0;i<PixelCount;i++)
			cache->Pixels[ i ] = cache->Pixels[ i ] & 0x00FFFFFF;
	}
}

void DecreaseColorPrecision(ScreenshotStruct *cache, unsigned int Div, unsigned int And)
{
	int PixelCount = cache->GetWidth() * cache->GetHeight();
	for (int i = 0; i < PixelCount; i++)
	{
		int Colors[3];
		Colors[0] = GetRValue(cache->Pixels[i]);
		Colors[1] = GetGValue(cache->Pixels[i]);
		Colors[2] = GetBValue(cache->Pixels[i]);
		if (Div != 0)
		{
			Colors[0] = Colors[0] / Div;
			Colors[1] = Colors[1] / Div;
			Colors[2] = Colors[2] / Div;
		}
		if (And != 0)
		{
			Colors[0] = Colors[0] & And;
			Colors[1] = Colors[1] & And;
			Colors[2] = Colors[2] & And;
		}
		cache->Pixels[i] = RGB(Colors[0], Colors[1], Colors[2]);
	}
}

void DecreaseColorCount(ScreenshotStruct *cache, unsigned int ColorsPerChannel)
{
	int PixelCount = cache->GetWidth() * cache->GetHeight();
	int ColorStep = 255 / ColorsPerChannel; // only valid for 8bpp. Which we intend ot use
	int ColorStepHalf = ColorStep / 2; // because we use rounding
	for (int i = 0; i < PixelCount; i++)
	{
		int Colors[3];
		Colors[0] = GetRValue(cache->Pixels[i]);
		Colors[1] = GetGValue(cache->Pixels[i]);
		Colors[2] = GetBValue(cache->Pixels[i]);
		for (int j = 0; j < 3; j++)
		{
			int NewC = (Colors[j] / ColorsPerChannel) * ColorsPerChannel;
			if (Colors[j] - NewC >= ColorStepHalf)
				Colors[j] = NewC + 1; // round up
			else
				Colors[j] = NewC;
		}
		cache->Pixels[i] = RGB(Colors[0], Colors[1], Colors[2]);
	}
}
