#include "StdAfx.h"

char ReturnBuff[DEFAULT_STR_BUFFER_SIZE * 10];
char* WINAPI ImageSearchOnScreenshot(char *aFilespec, int TransparentColor, int AcceptedColorDiff, int AcceptedErrorCount, int StopAfterNFullMatches)
{
	char ReturnBuff2[DEFAULT_STR_BUFFER_SIZE * 10];
	int MatchesFound = 0;
	ReturnBuff[0] = 0;
	ReturnBuff2[0] = 0;
	FileDebug("Started Image search");
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
	if (AcceptedColorDiff > 0)
	{
		FileDebug("\t Image search with tolerance");
		CheckPrepareToleranceMaps(cache, AcceptedColorDiff, TransparentColor);
		FileDebug("\t Image search generated tolerance maps");
		//DumpAsPPM( MinMap[0], MinMap[1], MinMap[2], cache->Width, cache->Height );
		//DumpAsPPM( &CurScreenshot->Pixels[ 40 * Width + 40 ], 40, 40, Width );
		//DumpAsPPM( MaxMap[0], MaxMap[1], MaxMap[2], cache->Width, cache->Height );
		for (int y = 0; y < Height - cache->Height; y += 1)
		{
			for (int x = 0; x < Width - cache->Width; x += 1)
			{
				int ImageMatched = 1;
				int FoundErrors = 0;
				for (int y2 = 0; y2<cache->Height; y2++)
				{
					//					if( y + y2 >= Height )
					//						break;
					for (int x2 = 0; x2<cache->Width; x2++)
					{
						//						if( x + x2 >= Width )
						//							break;
						int PixelIndexDst = (0 + y2) * cache->Width + 0 + x2;
						int PixelIndexSrc = (y + y2) * Width + x + x2;
						COLORREF BGRSrc = CurScreenshot->Pixels[PixelIndexSrc];
						int Cols[3];
						Cols[0] = ((int)BGRSrc >> 0) & 0xFF;
						if (cache->MinMap[0][PixelIndexDst] <= Cols[0] && Cols[0] <= cache->MaxMap[0][PixelIndexDst])
						{
							Cols[1] = ((int)BGRSrc >> 8) & 0xFF;
							if (cache->MinMap[1][PixelIndexDst] <= Cols[1] && Cols[1] <= cache->MaxMap[1][PixelIndexDst])
							{
								Cols[2] = ((int)BGRSrc >> 16) & 0xFF;
								if (cache->MinMap[2][PixelIndexDst] <= Cols[2] && Cols[2] <= cache->MaxMap[2][PixelIndexDst])
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

						if (FoundErrors > AcceptedErrorCount)
						{
							ImageMatched = 0;
							goto AbandonIMageInImageSearch;
						}
					}
				}
			AbandonIMageInImageSearch:
				if (ImageMatched == 1)
				{
					FileDebug("Image search found a match");
					if (strlen(ReturnBuff2) < DEFAULT_STR_BUFFER_SIZE * 10 - 2 * 10)	//is this even supposed to crash with sprintf_s ?
						sprintf_s(ReturnBuff2, DEFAULT_STR_BUFFER_SIZE * 10, "%s|%d|%d", ReturnBuff2, CurScreenshot->Left + x, CurScreenshot->Top + y);
					FileDebug(ReturnBuff2);
					MatchesFound++;
					if (MatchesFound >= StopAfterNFullMatches)
						goto docleanupandreturn;
				}
			}
		}
	docleanupandreturn:
		if (MatchesFound == 0)
			FileDebug("\t Image search found no matches");
	}
	else
	{
		RemoveCacheAlphaChannel(cache);
		RemoveScreenshotAlphaChannel(CurScreenshot);
		if (TransparentColor > TRANSPARENT_COLOR && AcceptedErrorCount == 0 && StopAfterNFullMatches == 1)
		{
			for (int y = 0; y < Height - cache->Height; y += 1)
				for (int x = 0; x < Width - cache->Width; x += 1)
				{
					for (int y2 = 0; y2<cache->Height; y2++)
					{
						int PixelIndexSrc = (y + y2) * Width + x;
						int PixelIndexDst = (0 + y2) * cache->Width;
						for (int x2 = 0; x2<cache->Width; x2++)
						{
							if (CurScreenshot->Pixels[PixelIndexSrc++] != cache->Pixels[PixelIndexDst++])
								goto AbandonIMageInImageSearch3;
						}
					}
					FileDebug("Image search found a match");
					sprintf_s(ReturnBuff2, DEFAULT_STR_BUFFER_SIZE * 10, "%s|%d|%d", ReturnBuff2, CurScreenshot->Left + x, CurScreenshot->Top + y);
					FileDebug(ReturnBuff2);
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
			for (int y = 0; y < Height - cache->Height; y += 1)
				for (int x = 0; x < Width - cache->Width; x += 1)
				{
					int ImageMatched = 1;
					int FoundErrors = 0;
					for (int y2 = 0; y2<cache->Height; y2++)
					{
						for (int x2 = 0; x2<cache->Width; x2++)
						{
							int PixelIndexSrc = (y + y2) * Width + x + x2;
							int PixelIndexDst = (0 + y2) * cache->Width + 0 + x2;
							COLORREF BGRSrc = CurScreenshot->Pixels[PixelIndexSrc];
							COLORREF BGRDst = cache->Pixels[PixelIndexDst];
							if (BGRDst == TransparentColor || BGRSrc == BGRDst)
							{
								//yaay it matches = nothig to do here
								FoundErrors = FoundErrors;
							}
							else
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
						if (MatchesFound >= StopAfterNFullMatches)
							goto docleanupandreturn2;
					}
				}
		docleanupandreturn2:
			;
		}
	}
	sprintf_s(ReturnBuff, DEFAULT_STR_BUFFER_SIZE * 10, "%d%s", MatchesFound, ReturnBuff2);
	FileDebug("\tImage search finished");
	return ReturnBuff;
}

char* WINAPI ImageSearchOnScreenshotMasked(char *aFilespec, char *MaskFile, int TransparentColor, int AcceptedColorDiff, int AcceptedErrorCount, int StopAfterNFullMatches)
{
	char ReturnBuff2[DEFAULT_STR_BUFFER_SIZE * 10];
	int MatchesFound = 0;
	ReturnBuff[0] = 0;
	ReturnBuff2[0] = 0;
	FileDebug("Started Image search");

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
	if (cache->LoadedPicture == NULL)
	{
		FileDebug("Skipping Image search as image is missing");
		return "";
	}

	CachedPicture *mask = CachePicture(MaskFile);
	if (mask == NULL)
	{
		FileDebug("Skipping Image search as mask could not be loaded");
		return "";
	}
	if (mask->Pixels == NULL)
	{
		FileDebug("Skipping Image search as mask pixels are missing");
		return "";
	}
	if (mask->LoadedPicture == NULL)
	{
		FileDebug("Skipping Image search as mask is missing");
		return "";
	}

	if (CurScreenshot->Pixels == NULL)
	{
		FileDebug("Skipping Image search no screenshot is available");
		return "";
	}

	int Width = CurScreenshot->Right - CurScreenshot->Left;
	int Height = CurScreenshot->Bottom - CurScreenshot->Top;

	if (mask->Width != Width || mask->Height != Height)
	{
		FileDebug("Warning : Image search mask does not have same size as screenshot");
	}

	if (AcceptedColorDiff > 0)
	{
		CheckPrepareToleranceMaps(cache, AcceptedColorDiff, TransparentColor);
		//DumpAsPPM( MinMap[0], MinMap[1], MinMap[2], cache->Width, cache->Height );
		//DumpAsPPM( &CurScreenshot->Pixels[ 40 * Width + 40 ], 40, 40, Width );
		//DumpAsPPM( MaxMap[0], MaxMap[1], MaxMap[2], cache->Width, cache->Height );
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
						int PixelIndexSrc = (y + y2) * Width + x + x2;
						COLORREF BGRSrc = CurScreenshot->Pixels[PixelIndexSrc];
						COLORREF BGRSrcMask = mask->Pixels[PixelIndexSrc];
						if (BGRSrcMask == 0)	//screenshot pixel value is hidden and should not be compared. Used for overlays
							continue;
						int Cols[3];
						Cols[0] = ((int)BGRSrc >> 0) & 0xFF;
						if (cache->MinMap[0][PixelIndexDst] <= Cols[0] && Cols[0] <= cache->MaxMap[0][PixelIndexDst])
						{
							Cols[1] = ((int)BGRSrc >> 8) & 0xFF;
							if (cache->MinMap[1][PixelIndexDst] <= Cols[1] && Cols[1] <= cache->MaxMap[1][PixelIndexDst])
							{
								Cols[2] = ((int)BGRSrc >> 16) & 0xFF;
								if (cache->MinMap[2][PixelIndexDst] <= Cols[2] && Cols[2] <= cache->MaxMap[2][PixelIndexDst])
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

						if (FoundErrors > AcceptedErrorCount)
						{
							ImageMatched = 0;
							goto AbandonIMageInImageSearch;
						}
					}
				}
			AbandonIMageInImageSearch:
				if (ImageMatched == 1)
				{
					FileDebug("Image search found a match");
					sprintf_s(ReturnBuff2, DEFAULT_STR_BUFFER_SIZE * 10, "%s|%d|%d", ReturnBuff2, CurScreenshot->Left + x, CurScreenshot->Top + y);
					FileDebug(ReturnBuff2);
					MatchesFound++;
					if (MatchesFound >= StopAfterNFullMatches)
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
		for (int y = 0; y < Height - cache->Height; y += 1)
			for (int x = 0; x < Width - cache->Width; x += 1)
			{
				int ImageMatched = 1;
				int FoundErrors = 0;
				for (int y2 = 0; y2<cache->Height; y2++)
				{
					for (int x2 = 0; x2<cache->Width; x2++)
					{
						int PixelIndexSrc = (y + y2) * Width + x + x2;
						int PixelIndexDst = (0 + y2) * cache->Width + 0 + x2;
						COLORREF BGRSrc = CurScreenshot->Pixels[PixelIndexSrc];
						COLORREF BGRDst = cache->Pixels[PixelIndexDst];
						COLORREF BGRSrcMask = mask->Pixels[PixelIndexSrc];
						if (BGRSrcMask == 0)	//screenshot pixel value is hidden and should not be compared. Used for overlays
							continue;
						if (BGRDst == TransparentColor || BGRSrc == BGRDst)
						{
							//yaay it matches = nothig to do here
						}
						else
							FoundErrors++;

						if (FoundErrors > AcceptedErrorCount)
						{
							ImageMatched = 0;
							goto AbandonIMageInImageSearch2;
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
					if (MatchesFound >= StopAfterNFullMatches)
						goto docleanupandreturn2;
				}
			}
	docleanupandreturn2:
		;
	}
	sprintf_s(ReturnBuff, DEFAULT_STR_BUFFER_SIZE * 10, "%d%s", MatchesFound, ReturnBuff2);
	return ReturnBuff;
}

char* WINAPI ImageSearchOnScreenshotBest_SAD(char *aFilespec)
{
	char ReturnBuff2[DEFAULT_STR_BUFFER_SIZE * 10];
	int MatchesFound = 0;
	ReturnBuff[0] = 0;
	ReturnBuff2[0] = 0;
	FileDebug("Started Image search");
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

	int retx = -1;
	int rety = -1;
	unsigned int BestSAD = 0x7FFFFFFF;
	{
		//DumpAsPPM( MinMap[0], MinMap[1], MinMap[2], cache->Width, cache->Height );
		//DumpAsPPM( &CurScreenshot->Pixels[ 40 * Width + 40 ], 40, 40, Width );
		//DumpAsPPM( MaxMap[0], MaxMap[1], MaxMap[2], cache->Width, cache->Height );
		for (int y = 0; y < Height - cache->Height; y += 1)
		{
			for (int x = 0; x < Width - cache->Width; x += 1)
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

				alpha_mask.m128i_u32[0] = REMOVE_ALPHA_CHANNEL_MASK;
				alpha_mask.m128i_u32[1] = REMOVE_ALPHA_CHANNEL_MASK;
				alpha_mask.m128i_u32[2] = REMOVE_ALPHA_CHANNEL_MASK;
				alpha_mask.m128i_u32[3] = REMOVE_ALPHA_CHANNEL_MASK;

				for (int y2 = 0; y2<cache->Height; y2++)
				{
					LPCOLORREF AddrBig = &CurScreenshot->Pixels[(y + y2) * Width + x];
					LPCOLORREF AddSmall = &cache->Pixels[(0 + y2) * cache->Width + 0];
					for (int x2 = 0; x2<(cache->Width & ~0x03); x2 += 4)
						//					for( int x2=0;x2<4;x2+=4)
					{
						//assert( ( AddrBig[0] & 0xFF000000 ) == 0 && ( AddrBig[1] & 0xFF000000 ) == 0 && ( AddrBig[2] & 0xFF000000 ) == 0 && ( AddrBig[3] & 0xFF000000 ) == 0 );
						//assert( ( AddSmall[0] & 0xFF000000 ) == 0 && ( AddSmall[1] & 0xFF000000 ) == 0 && ( AddSmall[2] & 0xFF000000 ) == 0 && ( AddSmall[3] & 0xFF000000 ) == 0 );
						l0 = _mm_loadu_si128((__m128i*)AddrBig);
						l0 = _mm_and_si128(l0, alpha_mask);
						l1 = _mm_loadu_si128((__m128i*)AddSmall);
						line_sad = _mm_sad_epu8(l0, l1);
						acc_sad = _mm_add_epi32(acc_sad, line_sad);

						AddrBig += 4;
						AddSmall += 4;
						//						pixelcount2 += 4;
					}
				}

				_mm_storeu_si128((__m128i*)(&sad_array[0]), acc_sad);

				unsigned int sad = sad_array[0] + sad_array[2];

				//				assert( sad == RDiffSum + GDiffSum + BDiffSum );

				if (BestSAD > sad)
				{
					BestSAD = sad;
					retx = x + CurScreenshot->Left;
					rety = y + CurScreenshot->Top;
					//exact match ? I doubt it will ever happen...
					if (BestSAD == 0)
						goto docleanupandreturn;
				}
			}
		}
	docleanupandreturn:
		if (MatchesFound == 0)
			FileDebug("\t Image search found no matches");
	}

	sprintf_s(ReturnBuff, DEFAULT_STR_BUFFER_SIZE * 10, "1|%d|%d|%d", retx, rety, BestSAD);
	FileDebug("\tImage search finished");
	return ReturnBuff;
}

char* WINAPI ImageSearch_SAD(char *aFilespec)
{
	char ReturnBuff2[DEFAULT_STR_BUFFER_SIZE * 10];
	int MatchesFound = 0;
	ReturnBuff[0] = 0;
	ReturnBuff2[0] = 0;
	FileDebug("Started Image search");
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

	int retx = -1;
	int rety = -1;
	unsigned int BestSAD = 0x7FFFFFFF;
	{
		//DumpAsPPM( MinMap[0], MinMap[1], MinMap[2], cache->Width, cache->Height );
		//DumpAsPPM( &CurScreenshot->Pixels[ 40 * Width + 40 ], 40, 40, Width );
		//DumpAsPPM( MaxMap[0], MaxMap[1], MaxMap[2], cache->Width, cache->Height );
		for (int y = 0; y < Height - cache->Height; y += 1)
		{
			for (int x = 0; x < Width - cache->Width; x += 1)
			{
				unsigned int sad_array[4];
				__m128i l0, l1, line_sad, acc_sad;

				acc_sad = _mm_setzero_si128();

				for (int y2 = 0; y2<cache->Height; y2++)
				{
					LPCOLORREF AddrBig = &CurScreenshot->Pixels[(y + y2) * Width + x];
					LPCOLORREF AddSmall = &cache->Pixels[(0 + y2) * cache->Width + 0];
					for (int x2 = 0; x2<(cache->Width & ~0x03); x2 += 4)
					{
						l0 = _mm_loadu_si128((__m128i*)(&AddrBig[x2]));
						l1 = _mm_load_si128((__m128i*)(&AddSmall[x2]));
						line_sad = _mm_sad_epu8(l0, l1);
						acc_sad = _mm_add_epi32(acc_sad, line_sad);
					}
				}

				_mm_storeu_si128((__m128i*)(&sad_array[0]), acc_sad);

				unsigned int sad = sad_array[0] + sad_array[2];

				if (BestSAD > sad)
				{
					BestSAD = sad;
					retx = x + CurScreenshot->Left;
					rety = y + CurScreenshot->Top;
					//exact match ? I doubt it will ever happen...
#ifndef _CONSOLE
					if (BestSAD == 0)
						goto docleanupandreturn;
#endif
				}
			}
		}
	docleanupandreturn:
		if (MatchesFound == 0)
			FileDebug("\t Image search found no matches");
	}

	sprintf_s(ReturnBuff, DEFAULT_STR_BUFFER_SIZE * 10, "1|%d|%d|%d", retx, rety, BestSAD);
	FileDebug("\tImage search finished");
	return ReturnBuff;
}

char* WINAPI ImageSearch_Multiple_ExactMatch(char *aFilespec)
{
	char ReturnBuff2[DEFAULT_STR_BUFFER_SIZE * 10];
	int MatchesFound = 0;
	ReturnBuff[0] = 0;
	ReturnBuff2[0] = 0;
	FileDebug("Started Image search");
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

	int CacheWidthRoundDown = (cache->Width & ~0x03);
	for (int y = 0; y < Height - cache->Height; y += 1)
	{
		for (int x = 0; x < Width - CacheWidthRoundDown; x += 1)
		{
			//#define USE_SSE 1
			// do the sad for the picture
#if defined( USE_SSE )
			unsigned int sad_array[4];
			__m128i acc_sad = _mm_setzero_si128();
			for (int y2 = 0; y2<cache->Height; y2++)
			{
				LPCOLORREF AddrBig = &CurScreenshot->Pixels[(y + y2) * Width + x];
				LPCOLORREF AddSmall = &cache->Pixels[(0 + y2) * cache->Width + 0];
				for (int x2 = 0; x2<CacheWidthRoundDown; x2 += 4)
				{
					__m128i l0 = _mm_loadu_si128((__m128i*)(&AddrBig[x2]));
					__m128i l1 = _mm_load_si128((__m128i*)(&AddSmall[x2]));
					__m128i line_sad = _mm_sad_epu8(l0, l1);
					acc_sad = _mm_add_epi32(acc_sad, line_sad);
				}
			}
			_mm_storeu_si128((__m128i*)(&sad_array[0]), acc_sad);
			unsigned int sad = sad_array[0] + sad_array[2];
#elif defined( USE_SSE_EXACT_MATCH )
			unsigned int sad_array[4];
			__m128i acc_eq = _mm_setzero_si128();
			for (int y2 = 0; y2<cache->Height; y2++)
			{
				LPCOLORREF AddrBig = &CurScreenshot->Pixels[(y + y2) * Width + x];
				LPCOLORREF AddSmall = &cache->Pixels[(0 + y2) * cache->Width + 0];
				for (int x2 = 0; x2<CacheWidthRoundDown; x2 += 4)
				{
					__m128i l0 = _mm_loadu_si128((__m128i*)(&AddrBig[x2]));
					__m128i l1 = _mm_load_si128((__m128i*)(&AddSmall[x2]));
					__m128i line_eq = _mm_andnot_si128(l0, l1); // 0 where values are equal
					acc_eq = _mm_or_si128(acc_eq, line_eq);		// 0 where values are equal
				}
			}
			_mm_storeu_si128((__m128i*)(&sad_array[0]), acc_eq);
			unsigned int sad;
			if (sad_array[0] == 0 && sad_array[1] == 0 && sad_array[2] == 0 && sad_array[3] == 0)
				sad = 0;
			else
				sad = 1;
#else
			int sad = 0; // count the number of different pixels
			for (int y2 = 0; y2<cache->Height; y2++)
			{
				LPCOLORREF AddrBig = &CurScreenshot->Pixels[(y + y2) * Width + x];
				LPCOLORREF AddSmall = &cache->Pixels[(0 + y2) * cache->Width + 0];
				for (int x2 = 0; x2<CacheWidthRoundDown; x2++)
					if (AddrBig[x2] != AddSmall[x2])
					{
						sad = 1;
						// due to early exit, this should be fast when only a few examples are present on the screen
						// worst case scenario, this is like 5 times slower than with SSE
						goto NO_MATCH_NO_MORE_SEARCH;
					}
			}
		NO_MATCH_NO_MORE_SEARCH:
#endif

			if (sad == 0)
			{
				MatchesFound++;
				int retx = x + CurScreenshot->Left;
				int rety = y + CurScreenshot->Top;
				sprintf_s(ReturnBuff2, DEFAULT_STR_BUFFER_SIZE * 10, "%s|%d|%d", ReturnBuff2, retx, rety);
			}
		}
	}
	if (MatchesFound == 0)
		FileDebug("\t Image search found no matches");

	sprintf_s(ReturnBuff, DEFAULT_STR_BUFFER_SIZE * 10, "%d|%s", MatchesFound, ReturnBuff2);
	FileDebug("\tImage search finished");
	return ReturnBuff;
}

char* WINAPI ImageSearch_Multiple_Transparent(char *aFilespec)
{
	char ReturnBuff2[DEFAULT_STR_BUFFER_SIZE * 10];
	int MatchesFound = 0;
	ReturnBuff[0] = 0;
	ReturnBuff2[0] = 0;
	FileDebug("Started Image search");
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

	int CacheWidthRoundDown = (cache->Width & ~0x03);
	for (int y = 0; y < Height - cache->Height; y += 1)
	{
		for (int x = 0; x < Width - CacheWidthRoundDown; x += 1)
		{
			// do the sad for the picture
			for (int y2 = 0; y2<cache->Height; y2++)
			{
				LPCOLORREF AddrBig = &CurScreenshot->Pixels[(y + y2) * Width + x];
				LPCOLORREF AddSmall = &cache->Pixels[(0 + y2) * cache->Width + 0];
				for (int x2 = 0; x2<CacheWidthRoundDown; x2++)
					if (AddSmall[x2] != TRANSPARENT_COLOR && AddrBig[x2] != AddSmall[x2])
					{
						// due to early exit, this should be fast when only a few examples are present on the screen
						// worst case scenario, this is like 5 times slower than with SSE
						goto NO_MATCH_NO_MORE_SEARCH2;
					}
			}
			// If we got here, there is a match
			MatchesFound++;
			int retx = x + CurScreenshot->Left;
			int rety = y + CurScreenshot->Top;
			sprintf_s(ReturnBuff2, DEFAULT_STR_BUFFER_SIZE * 10, "%s|%d|%d", ReturnBuff2, retx, rety);
NO_MATCH_NO_MORE_SEARCH2:;
		}
	}
	if (MatchesFound == 0)
		FileDebug("\t Image search found no matches");

	sprintf_s(ReturnBuff, DEFAULT_STR_BUFFER_SIZE * 10, "%d|%s", MatchesFound, ReturnBuff2);
	FileDebug("\tImage search finished");
	return ReturnBuff;
}

char* WINAPI ImageSearchOnScreenshotBestTransparent(char *aFilespec)
{
	char ReturnBuff2[DEFAULT_STR_BUFFER_SIZE * 10];
	int MatchesFound = 0;
	ReturnBuff[0] = 0;
	ReturnBuff2[0] = 0;
	FileDebug("Started Image search");
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

	int retx = -1;
	int rety = -1;
	unsigned int BestSAD = 0x7FFFFFFF;
	unsigned int TransparentColor = TRANSPARENT_COLOR;
	{
		//DumpAsPPM( MinMap[0], MinMap[1], MinMap[2], cache->Width, cache->Height );
		//DumpAsPPM( &CurScreenshot->Pixels[ 40 * Width + 40 ], 40, 40, Width );
		//DumpAsPPM( MaxMap[0], MaxMap[1], MaxMap[2], cache->Width, cache->Height );
		for (int y = 0; y < Height - cache->Height; y += 1)
		{
			for (int x = 0; x < Width - cache->Width; x += 1)
			{
				int RDiffSum = 0;
				int GDiffSum = 0;
				int BDiffSum = 0;

				for (int y2 = 0; y2<cache->Height; y2++)
				{
					LPCOLORREF AddrBig = &CurScreenshot->Pixels[(y + y2) * Width + x];
					LPCOLORREF AddSmall = &cache->Pixels[(0 + y2) * cache->Width + 0];
					for (int x2 = 0; x2<cache->Width; x2++)
					{
						COLORREF BGRDst = AddSmall[x2];
						if (BGRDst == TransparentColor)
							continue;

						COLORREF BGRSrc = AddrBig[x2];

						int RDiff = ((BGRSrc & 0x0000FF) >> 0) - ((BGRDst & 0x0000FF) >> 0);
						int GDiff = ((BGRSrc & 0x00FF00) >> 8) - ((BGRDst & 0x00FF00) >> 8);
						int BDiff = ((BGRSrc & 0xFF0000) >> 16) - ((BGRDst & 0xFF0000) >> 16);

						RDiff = abs(RDiff);
						GDiff = abs(GDiff);
						BDiff = abs(BDiff);

						RDiffSum += RDiff;
						GDiffSum += GDiff;
						BDiffSum += BDiff;
					}
				}
				unsigned int sad = RDiffSum + GDiffSum + BDiffSum;
				if (BestSAD > sad)
				{
					BestSAD = sad;
					retx = x + CurScreenshot->Left;
					rety = y + CurScreenshot->Top;
					//exact match ? I doubt it will ever happen...
					if (BestSAD == 0)
						goto docleanupandreturn;
				}
			}
		}
	docleanupandreturn:
		if (MatchesFound == 0)
			FileDebug("\t Image search found no matches");
	}

	sprintf_s(ReturnBuff, DEFAULT_STR_BUFFER_SIZE * 10, "1|%d|%d|%d", retx, rety, BestSAD);
	FileDebug("\tImage search finished");
	return ReturnBuff;
}

__forceinline int GetPixelCountRegion(const int *TempBuff, const int Width, const int Color, const int PrevCount, const int AreaWidth, const int AreaHeight)
{
	int ret = 0;
	if (PrevCount != -1)
	{
		ret = PrevCount;
		//substract prev line
		TempBuff -= Width;
		for (int x2 = 0; x2 < AreaWidth; x2++)
			if (TempBuff[x2] == Color)
				ret--;
		//add new line
		TempBuff += Width + ( AreaHeight - 1 ) * Width;
		for (int x2 = 0; x2 < AreaWidth; x2++)
			if (TempBuff[x2] == Color)
				ret++;
	}
	else
	{
		//count all
		for (int y2 = 0; y2 < AreaHeight; y2++)
		{
			for (int x2 = 0; x2 < AreaWidth; x2++)
				if (TempBuff[x2] == Color)
					ret++;
			TempBuff += Width;
		}
	}
	return ret;
}

char* WINAPI ImageSearch_Multiple_PixelCount(int Color, int Percent, int AreaWidth, int AreaHeight)
{
	char ReturnBuff2[DEFAULT_STR_BUFFER_SIZE * 50];
	int MatchesFound = 0;
	ReturnBuff[0] = 0;
	ReturnBuff2[0] = 0;
	FileDebug("Started Image search");
	if (CurScreenshot->Pixels == NULL)
	{
		FileDebug("Skipping Image search no screenshot is available");
		return "";
	}

	int Width = CurScreenshot->GetWidth();
	int Height = CurScreenshot->GetHeight();
	int AreaSize = AreaWidth * AreaHeight;
	float AreaSizeP = AreaSize / 100.0f;
	int *TempBuff = (int*)malloc(Width * Height * sizeof(int));
	memcpy(TempBuff, CurScreenshot->Pixels, Width * Height * sizeof(int));

	for (int x = 0; x < Width - AreaWidth; x += 1)
	{
		int PrevBlockCount = -1;
		for (int y = 0; y < Height - AreaHeight; y += 1)
		{
			// this will only help if we plan to find multiple areas with considerable size. ( we will find it at least once right ? )
			if (TempBuff[y * Width + x] == 66)
			{
				y += AreaHeight;
				PrevBlockCount = -1;
				continue;
			}/**/
			// count Colors
			int Counter = GetPixelCountRegion( &TempBuff[y * Width + x], Width, Color, PrevBlockCount, AreaWidth, AreaHeight);
			float Ratio = Counter / AreaSizeP;
			PrevBlockCount = Counter;
			if (Ratio > Percent)
			{
				// If we got here, there is a match
				MatchesFound++;
				int retx = x + CurScreenshot->Left;
				int rety = y + CurScreenshot->Top;
				sprintf_s(ReturnBuff2, DEFAULT_STR_BUFFER_SIZE * 50, "%s|%d|%d", ReturnBuff2, retx, rety);
				//mark this zone so we do not find it again
				for (int y2 = 0; y2 < AreaHeight; y2++)
				{
					memset(&TempBuff[(y + y2) * Width + x], TRANSPARENT_COLOR, AreaWidth*sizeof(int));
					//mark this zone that we can skip processing
					TempBuff[(y + y2) * Width + x] = 66;
				}
				//jump forward with X, there is no need to search this area
				y += AreaHeight;
				PrevBlockCount = -1;
			}
		}
	}
	if (MatchesFound == 0)
		FileDebug("\t Image search found no matches");

	sprintf_s(ReturnBuff, DEFAULT_STR_BUFFER_SIZE * 10, "%d%s", MatchesFound, ReturnBuff2);
	free(TempBuff);
	FileDebug("\tImage search finished");
	return ReturnBuff;
}

// this is 8 times slower than doing it in 2 steps !!
char* WINAPI ImageSearch_Multiple_Gradient(int Color, int GradientMatchPercent, int CountInAreaPercent, int AreaWidth, int AreaHeight)
{
	char ReturnBuff2[DEFAULT_STR_BUFFER_SIZE * 10];
	int MatchesFound = 0;
	ReturnBuff[0] = 0;
	ReturnBuff2[0] = 0;
	FileDebug("Started Image search");
	if (CurScreenshot->Pixels == NULL)
	{
		FileDebug("Skipping Image search no screenshot is available");
		return "";
	}
	int R1 = GetRValue(Color);
	int G1 = GetGValue(Color);
	int B1 = GetBValue(Color);
	float RG1 = (float)R1 / (float)G1;
	float GB1 = (float)G1 / (float)B1;
	float GradientMatchPercent2 = ( 100.0f - GradientMatchPercent ) / 100.0f;

	int Width = CurScreenshot->GetWidth();
	int Height = CurScreenshot->GetHeight();
	float AreaSize = AreaWidth * AreaHeight / 100.0f;
	int *TempBuff = (int*)malloc(Width * Height * sizeof(int));
	memcpy(TempBuff, CurScreenshot->Pixels, Width * Height * sizeof(int));

	for (int y = 0; y < Height - AreaHeight; y += 1)
	{
		for (int x = 0; x < Width - AreaWidth; x += 1)
		{
			// count Colors
			int Counter = 0;
			for (int y2 = 0; y2<AreaHeight; y2++)
			{
				int *AddrBig = &TempBuff[(y + y2) * Width + x];
				for (int x2 = 0; x2 < AreaWidth; x2++)
				{
					int Color2 = AddrBig[x2];
					int B2 = GetRValue(Color2);
					int G2 = GetGValue(Color2);
					int R2 = GetBValue(Color2);
					float RG2 = (float)R2 / (float)G2;
					if (abs(RG2 - RG1) > GradientMatchPercent2)
						continue;
					float GB2 = (float)G2 / (float)B2;
					if (abs(GB2 - GB1) <= GradientMatchPercent2)
						Counter++;
				}
			}
			float Ratio = Counter / AreaSize;
			if (Ratio > CountInAreaPercent)
			{
				// If we got here, there is a match
				MatchesFound++;
				int retx = x + CurScreenshot->Left;
				int rety = y + CurScreenshot->Top;
				sprintf_s(ReturnBuff2, DEFAULT_STR_BUFFER_SIZE * 10, "%s|%d|%d", ReturnBuff2, retx, rety);
				//mark this zone so we do not find it again
				for (int y2 = 0; y2 < AreaHeight; y2++)
					memset(&TempBuff[(y + y2) * Width + x], TRANSPARENT_COLOR, AreaWidth*sizeof(int));
				//jump forward with X, there is no need to search this area
				x += AreaWidth - 1;
			}
		}
	}
	if (MatchesFound == 0)
		FileDebug("\t Image search found no matches");

	sprintf_s(ReturnBuff, DEFAULT_STR_BUFFER_SIZE * 10, "%d|%s", MatchesFound, ReturnBuff2);
	free(TempBuff);
	FileDebug("\tImage search finished");
	return ReturnBuff;
}