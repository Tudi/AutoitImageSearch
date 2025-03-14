#include "StdAfx.h"

int SearchResultCount;
int SearchResultXYSAD[500][3];

char ReturnBuff[DEFAULT_STR_BUFFER_SIZE * 10];
char* WINAPI ImageSearchOnScreenshot(const char *aFilespec, int TransparentColor, int AcceptedColorDiff, int AcceptedErrorCount, int StopAfterNFullMatches)
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
		return ReturnBuff;
	}
	if (cache->Pixels == NULL)
	{
		FileDebug("Skipping Image search as image pixels are missing");
		return ReturnBuff;
	}
	if (CurScreenshot->Pixels == NULL)
	{
		FileDebug("Skipping Image search no screenshot is available");
		return ReturnBuff;
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

char* WINAPI ImageSearchOnScreenshotMasked(const char *aFilespec, char *MaskFile, int TransparentColor, int AcceptedColorDiff, int AcceptedErrorCount, int StopAfterNFullMatches)
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
		return ReturnBuff;
	}
	if (cache->Pixels == NULL)
	{
		FileDebug("Skipping Image search as image pixels are missing");
		return ReturnBuff;
	}
	if (cache->LoadedPicture == NULL)
	{
		FileDebug("Skipping Image search as image is missing");
		return ReturnBuff;
	}

	CachedPicture *mask = CachePicture(MaskFile);
	if (mask == NULL)
	{
		FileDebug("Skipping Image search as mask could not be loaded");
		return ReturnBuff;
	}
	if (mask->Pixels == NULL)
	{
		FileDebug("Skipping Image search as mask pixels are missing");
		return ReturnBuff;
	}
	if (mask->LoadedPicture == NULL)
	{
		FileDebug("Skipping Image search as mask is missing");
		return ReturnBuff;
	}

	if (CurScreenshot->Pixels == NULL)
	{
		FileDebug("Skipping Image search no screenshot is available");
		return ReturnBuff;
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

char* WINAPI ImageSearchOnScreenshotBest_SAD(const char *aFilespec)
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
		return ReturnBuff;
	}
	if (cache->Pixels == NULL)
	{
		FileDebug("Skipping Image search as image pixels are missing");
		return ReturnBuff;
	}
	if (CurScreenshot->Pixels == NULL)
	{
		FileDebug("Skipping Image search no screenshot is available");
		return ReturnBuff;
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
					LPCOLORREF AddrSmall = &cache->Pixels[(0 + y2) * cache->Width + 0];
					size_t x2Limit = (cache->Width - 4) & ~0x03;
					for (size_t x2 = 0; x2< x2Limit; x2 += 4)
						//					for( int x2=0;x2<4;x2+=4)
					{
						//assert( ( AddrBig[0] & 0xFF000000 ) == 0 && ( AddrBig[1] & 0xFF000000 ) == 0 && ( AddrBig[2] & 0xFF000000 ) == 0 && ( AddrBig[3] & 0xFF000000 ) == 0 );
						//assert( ( AddrSmall[0] & 0xFF000000 ) == 0 && ( AddrSmall[1] & 0xFF000000 ) == 0 && ( AddrSmall[2] & 0xFF000000 ) == 0 && ( AddrSmall[3] & 0xFF000000 ) == 0 );
						l0 = _mm_loadu_si128((__m128i*)AddrBig);
						l0 = _mm_and_si128(l0, alpha_mask);
						l1 = _mm_loadu_si128((__m128i*)AddrSmall);
						line_sad = _mm_sad_epu8(l0, l1);
						acc_sad = _mm_add_epi32(acc_sad, line_sad);

						AddrBig += 4;
						AddrSmall += 4;
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

// input images are A8R8G8B8 encoded -> 4 bytes / pixel = 32 bpp
char* WINAPI ImageSearch_SAD(const char* aFilespec)
{
	return ImageSearch_SAD_Region(aFilespec, CurScreenshot->Left, CurScreenshot->Top, CurScreenshot->Right, CurScreenshot->Bottom, SADSearchRegionFlags::SSRF_ST_NO_FLAGS);
}

char* WINAPI ImageSearch_SAD_Region(const char* aFilespec, int aLeft, int aTop, int aRight, int aBottom, SADSearchRegionFlags uSearchFlags)
{
	char ReturnBuff2[DEFAULT_STR_BUFFER_SIZE * 10];
	ReturnBuff[0] = 0;
	ReturnBuff2[0] = 0;
#ifdef _DEBUG
	int MatchesFound = 0;
	size_t startStamp = GetTickCount();
#endif
	FileDebug("Started Image search");
	CachedPicture* cache = CachePicture(aFilespec);

	if (cache == NULL)
	{
		FileDebug("Skipping Image search as image could not be loaded");
		return ReturnBuff;
	}
	if (cache->Pixels == NULL)
	{
		FileDebug("Skipping Image search as image pixels are missing");
		return ReturnBuff;
	}
	if (cache->Height <= 0 || cache->Width < 8)
	{
		FileDebug("Skipping Image search as searched image height is 0");
		return ReturnBuff;
	}
	if (CurScreenshot == NULL || CurScreenshot->Pixels == NULL)
	{
		FileDebug("Skipping Image search no screenshot is available");
		return ReturnBuff;
	}
#if !defined(_CONSOLE) // because we do want to have benchmarks
	if (cache->PrevSearchImageId == CurScreenshot->UniqueFameCounter &&
		cache->PrevSearchTop == aTop && cache->PrevSearchLeft == aLeft)
	{
		FileDebug("Skipping Image search as it's done on same image with same params");
		return cache->PrevSearchReturnVal;
	}
#endif 

	cache->PrevSearchImageId = CurScreenshot->UniqueFameCounter;
	cache->PrevSearchTop = aTop;
	cache->PrevSearchLeft = aLeft;

	// record searched regions so auto screenshot might know next time
	const int dEstimatedSearchRadius = 3;
	g_SearchedRegions.aLeft = min(g_SearchedRegions.aLeft, aLeft - dEstimatedSearchRadius);
	g_SearchedRegions.aRight = max(g_SearchedRegions.aRight, aRight + cache->Width + dEstimatedSearchRadius);
	g_SearchedRegions.aTop = min(g_SearchedRegions.aTop, aTop - dEstimatedSearchRadius);
	g_SearchedRegions.aBottom = max(g_SearchedRegions.aBottom, aBottom + cache->Height + dEstimatedSearchRadius);

	if (aLeft < CurScreenshot->Left)
	{
		aLeft = CurScreenshot->Left;
		FileDebug("ImageSearch_SAD_Region:aLeft smaller than screenshot Left. Adjusting it");
	}
	if (aTop < CurScreenshot->Top)
	{
		aTop = CurScreenshot->Top;
		FileDebug("ImageSearch_SAD_Region:aTop smaller than screenshot Top. Adjusting it");
	}
	if (aRight > (CurScreenshot->Right - cache->Width))
	{
		FileDebug("ImageSearch_SAD_Region:Search Right falls outside screenshot Right. Adjusting it");
		if (CurScreenshot->Right <= cache->Width) {
			FileDebug("ImageSearch_SAD_Region:Screenshot is too small for searching. Skipping search");
			return ReturnBuff;
		}
		aRight = CurScreenshot->Right - cache->Width;
	}
	if (aBottom > (CurScreenshot->Bottom - cache->Height))
	{
		FileDebug("ImageSearch_SAD_Region:Search Bottom falls outside screenshot Bottom. Adjusting it");
		if (CurScreenshot->Bottom <= cache->Height) {
			FileDebug("ImageSearch_SAD_Region:Screenshot is too small for searching. Skipping search");
			return ReturnBuff;
		}
		aBottom = CurScreenshot->Bottom - cache->Height;
	}
	if (aRight <= aLeft)
	{
		FileDebug("ImageSearch_SAD_Region:Screenshot width is too small for searching. Skipping search");
		return ReturnBuff;
	}
	if (aBottom <= aTop)
	{
		FileDebug("ImageSearch_SAD_Region:Screenshot height is too small for searching. Skipping search");
		return ReturnBuff;
	}

	const LPCOLORREF Pixels1 = CurScreenshot->Pixels;
	const LPCOLORREF Pixels2 = cache->Pixels;

	// check if there is an alpha channel mismatch
#ifdef _DEBUG
	if ((Pixels1[0] & 0xFF000000) != (Pixels2[0] & 0xFF000000))
	{
		FileDebug("ImageSearch_SAD_Region: !!!! Alpha channel mismatch between screenshot and loaded image");
	}
#endif

	size_t search_start_x = aLeft - CurScreenshot->Left;
	size_t search_end_x = search_start_x + (aRight - aLeft);
	size_t search_start_y = aTop - CurScreenshot->Top;
	size_t search_end_y = search_start_y + (aBottom - aTop);

#ifdef _DEBUG
	{
		char dbgmsg[DEFAULT_STR_BUFFER_SIZE];
		sprintf_s(dbgmsg, sizeof(dbgmsg), "\t\t search left=%d top=%d right=%d, bottom=%d", aLeft, aTop, aRight, aBottom);
		FileDebug(dbgmsg);
		sprintf_s(dbgmsg, sizeof(dbgmsg), "\t\t search startx=%llu endX=%llu starty=%llu, endy=%llu", search_start_x, search_end_x, search_start_y, search_end_y);
		FileDebug(dbgmsg);
	}
#endif
	double HashSmallestDiffPCT = 100;
	size_t BestSATD = 0x7FFFFFFF;
	ImgHashWholeIage imgHash = { 0 };
	int retx = -1;
	int rety = -1;
	size_t BestSAD = 0x7FFFFFFF;
	//DumpAsPPM( &CurScreenshot->Pixels[ 40 * Width + 40 ], 40, 40, Width );
	const size_t stride1 = CurScreenshot->Width;
	const size_t stride2 = cache->Width;
	const size_t width_SAD = (cache->Width/8)*8; // value is in pixels
	const size_t height_SAD = cache->Height;	// value is in pixels
	for (size_t y = search_start_y; y < search_end_y; y++)
	{
		const LPCOLORREF AddrBig = &Pixels1[y * stride1];
		const LPCOLORREF AddrSmall = &Pixels2[0];
		for (size_t x = search_start_x; x < search_end_x; x++)
		{
			uint64_t sad = ImageSad(&AddrBig[x], stride1, AddrSmall, stride2, width_SAD, height_SAD);
			if (BestSAD >= sad)
			{
#ifdef _DEBUG
				MatchesFound++;
#endif
				int HashingWasPossible = 0;
				// this code needs more testing. Seen it crash. Did not had the time to debug
				if ((uSearchFlags & SSRF_ST_ENFORCE_SAD_WITH_HASH))
				{
					ImgHashWholeIage *cacheHash = GetCreateCacheHash(cache);
					int GenHashErr = GenHashesOnScreenshotForCachedImage(cache, CurScreenshot, (int)x, (int)y, &imgHash);
					if (GenHashErr == 0)
					{
						ImgHash8x8_CompareResult compareRes;
						int CompareErr = compareHash(cacheHash, &imgHash, &compareRes);
//						printf("New best sad at x=%llu y=%llu. Hash old %f, hash new %f\n", x, y, HashSmallestDiffPCT, compareRes.pctMatchAvg);
						if (CompareErr == 0 && compareRes.pctMatchAvg < HashSmallestDiffPCT)
						{
							HashSmallestDiffPCT = compareRes.pctMatchAvg;
							retx = (int)x;
							rety = (int)y;
							HashingWasPossible = 1;
							BestSAD = sad;
							//exact match ? I doubt it will ever happen...
							if (HashSmallestDiffPCT == 100)
								goto docleanupandreturn;
						}
					}
				}
				// there are cases when SAD(loc2) < SAD(loc1) BUT SATD(loc2) > SATD(loc1)
				// SATD is about 10 times slower than SAD
				if ((uSearchFlags & SSRF_ST_ENFORCE_SAD_WITH_SATD))
				{
					size_t satd = satd_nxm(&AddrBig[x], AddrSmall, width_SAD, height_SAD, stride1, stride2);
					if (satd < BestSATD)
					{
						BestSAD = sad;
						BestSATD = satd;
						retx = (int)x;
						rety = (int)y;
						if (BestSATD == 0)
							goto docleanupandreturn;
					}
					HashingWasPossible = 1;
				}
				if(HashingWasPossible == 0) {
					BestSAD = sad;
					retx = (int)x;
					rety = (int)y;
					//exact match ? I doubt it will ever happen...
					if (BestSAD == 0)
						goto docleanupandreturn;
				}
			}
		}
	}
docleanupandreturn:
	if (retx == -1)
		FileDebug("\t Image search found no matches");

	// in theory we reused the allocated data multiple times
	FreeHashAllocatedData(&imgHash);

	size_t colorDiffCount = 0;
	size_t colorDifferentPct = 0;
	size_t avgColorDiff = 0;
	if (rety != -1 && (uSearchFlags & SSRF_ST_PROCESS_INLCUDE_DIFF_INFO))
	{
		const LPCOLORREF AddrBig = &Pixels1[rety * stride1 + retx];
		const LPCOLORREF AddrSmall = &Pixels2[0];
//		uint64_t sad = ImageSad(AddrBig, stride1, AddrSmall, stride2, width_SAD, height_SAD);
		for (size_t row = 0; row < cache->Height; row++)
		{
			for (size_t col = 0; col < width_SAD; col++)
			{
				if (AddrBig[row * stride1 + col] != AddrSmall[row * stride2 + col])
				{
					const unsigned char* AddrBig2 = (unsigned char*)&AddrBig[row * stride1 + col];
					const unsigned char* AddrSmall2 = (unsigned char*)&AddrSmall[row * stride2 + col];
					if (AddrBig2[0] != AddrSmall2[0]) colorDiffCount++;
					if (AddrBig2[1] != AddrSmall2[1]) colorDiffCount++;
					if (AddrBig2[2] != AddrSmall2[2]) colorDiffCount++;
#ifdef _DEBUG
					char dbgmsg[DEFAULT_STR_BUFFER_SIZE];
					sprintf_s(dbgmsg, sizeof(dbgmsg), "\t\t diff at %lld %lld screenshot %X cache %X DiffCount %llu", col, row, AddrBig[row * stride1 + col], AddrSmall[row * stride2 + col], colorDiffCount);
					FileDebug(dbgmsg);
#endif
				}
			}
		}
		if (colorDiffCount)
		{
			avgColorDiff = BestSAD / colorDiffCount;
			colorDifferentPct = (size_t)((double)colorDiffCount * 100.0 / (double)(width_SAD * cache->Height * 3));
		}
	}

#ifdef _DEBUG
	if (((BestSAD == 0 && colorDiffCount != 0) || (BestSAD != 0 && colorDiffCount == 0)) && (uSearchFlags & SSRF_ST_PROCESS_INLCUDE_DIFF_INFO))
	{
		FileDebug("\t\t!!!Unexpected difference between SIMD SAD and manual SAD");
	}
	char dbgmsg[DEFAULT_STR_BUFFER_SIZE];
	if (rety == -1) { rety = 0; retx = 0; }
	sprintf_s(dbgmsg, sizeof(dbgmsg), "\t\t retxy %d %d", retx, rety);
	FileDebug(dbgmsg);
	const unsigned char* AddrBig = (unsigned char*)&Pixels1[rety * stride1 + retx];
	const unsigned char* AddrSmall = (unsigned char*)&Pixels2[0];
	sprintf_s(dbgmsg, sizeof(dbgmsg), "\t\tpixel 0 0 : %d %d %d %d - %d %d %d %d",
		(int)AddrBig[0], (int)AddrBig[1], (int)AddrBig[2], (int)AddrBig[3], (int)AddrSmall[0], (int)AddrSmall[1], (int)AddrSmall[2], (int)AddrSmall[3]);
	FileDebug(dbgmsg);
	sprintf_s(dbgmsg, sizeof(dbgmsg), "\t\tpixel 1 0 : %d %d %d %d - %d %d %d %d",
		(int)AddrBig[4], (int)AddrBig[5], (int)AddrBig[6], (int)AddrBig[7], (int)AddrSmall[4], (int)AddrSmall[5], (int)AddrSmall[6], (int)AddrSmall[7]);
	FileDebug(dbgmsg);
#endif

	size_t SADPerPixel = BestSAD / (width_SAD * height_SAD * 3); // this actually called MAD
	retx = (int)(retx + CurScreenshot->Left);
	rety = (int)(rety + CurScreenshot->Top);
	sprintf_s(ReturnBuff, DEFAULT_STR_BUFFER_SIZE * 10, "1|%d|%d|%llu|%llu|%llu|%llu|%llu|%d|%llu", 
		retx, rety, BestSAD, SADPerPixel, avgColorDiff, colorDiffCount, colorDifferentPct, int(HashSmallestDiffPCT), BestSATD);

	// in case caller is spamming searches more than we are able to keep up the pace with
	strcpy_s(cache->PrevSearchReturnVal, ReturnBuff);

#ifdef _DEBUG
	size_t endStamp = GetTickCount();
	sprintf_s(dbgmsg, sizeof(dbgmsg), "\tImage search finished in %d ms. Name %s. Improved match %d. Returning %s. ", (int)(endStamp - startStamp), cache->FileName, MatchesFound, ReturnBuff);
	FileDebug(dbgmsg);
#endif
	if (imgHash.hashes != NULL)
	{
		MY_FREE(imgHash.hashes);
	}

	return ReturnBuff;
}

char* WINAPI ImageSearch_Multiple_ExactMatch(const char *aFilespec)
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
		return ReturnBuff;
	}
	if (cache->Pixels == NULL)
	{
		FileDebug("Skipping Image search as image pixels are missing");
		return ReturnBuff;
	}
	if (CurScreenshot->Pixels == NULL)
	{
		FileDebug("Skipping Image search no screenshot is available");
		return ReturnBuff;
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
				LPCOLORREF AddrSmall = &cache->Pixels[(0 + y2) * cache->Width + 0];
				for (int x2 = 0; x2<CacheWidthRoundDown; x2 += 4)
				{
					__m128i l0 = _mm_loadu_si128((__m128i*)(&AddrBig[x2]));
					__m128i l1 = _mm_load_si128((__m128i*)(&AddrSmall[x2]));
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
				LPCOLORREF AddrSmall = &cache->Pixels[(0 + y2) * cache->Width + 0];
				for (int x2 = 0; x2<CacheWidthRoundDown; x2 += 4)
				{
					__m128i l0 = _mm_loadu_si128((__m128i*)(&AddrBig[x2]));
					__m128i l1 = _mm_load_si128((__m128i*)(&AddrSmall[x2]));
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
				LPCOLORREF AddrSmall = &cache->Pixels[(0 + y2) * cache->Width + 0];
				for (int x2 = 0; x2<CacheWidthRoundDown; x2++)
					if (AddrBig[x2] != AddrSmall[x2])
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

char* WINAPI ImageSearch_Multiple_Transparent(const char *aFilespec)
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
		return ReturnBuff;
	}
	if (cache->Pixels == NULL)
	{
		FileDebug("Skipping Image search as image pixels are missing");
		return ReturnBuff;
	}
	if (CurScreenshot->Pixels == NULL)
	{
		FileDebug("Skipping Image search no screenshot is available");
		return ReturnBuff;
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
				LPCOLORREF AddrSmall = &cache->Pixels[(0 + y2) * cache->Width + 0];
				for (int x2 = 0; x2<CacheWidthRoundDown; x2++)
					if (AddrSmall[x2] != TRANSPARENT_COLOR && AddrBig[x2] != AddrSmall[x2])
					{
						// due to early exit, this should be fast when only a few examples are present on the screen
						// worst case scenario, this is like 5 times slower than with SSE
						goto NO_MATCH_NO_MORE_SEARCH2;
					}
			}
			// If we got here, there is a match
			MatchesFound++;
			{
				int retx = x + CurScreenshot->Left;
				int rety = y + CurScreenshot->Top;
				sprintf_s(ReturnBuff2, DEFAULT_STR_BUFFER_SIZE * 10, "%s|%d|%d", ReturnBuff2, retx, rety);
			}
NO_MATCH_NO_MORE_SEARCH2:
			(void)0;
		}
	}
	if (MatchesFound == 0)
		FileDebug("\t Image search found no matches");

	sprintf_s(ReturnBuff, DEFAULT_STR_BUFFER_SIZE * 10, "%d|%s", MatchesFound, ReturnBuff2);
	FileDebug("\tImage search finished");
	return ReturnBuff;
}

char* WINAPI ImageSearchOnScreenshotBestTransparent(const char *aFilespec)
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
		return ReturnBuff;
	}
	if (cache->Pixels == NULL)
	{
		FileDebug("Skipping Image search as image pixels are missing");
		return ReturnBuff;
	}
	if (CurScreenshot->Pixels == NULL)
	{
		FileDebug("Skipping Image search no screenshot is available");
		return ReturnBuff;
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
					LPCOLORREF AddrSmall = &cache->Pixels[(0 + y2) * cache->Width + 0];
					for (int x2 = 0; x2<cache->Width; x2++)
					{
						COLORREF BGRDst = AddrSmall[x2];
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
	char ReturnBuff2[DEFAULT_STR_BUFFER_SIZE * 16];
	int MatchesFound = 0;
	ReturnBuff[0] = 0;
	ReturnBuff2[0] = 0;
	FileDebug("Started Image search");
	if (CurScreenshot->Pixels == NULL)
	{
		FileDebug("Skipping Image search no screenshot is available");
		return ReturnBuff;
	}

	int Width = CurScreenshot->GetWidth();
	int Height = CurScreenshot->GetHeight();
	int AreaSize = AreaWidth * AreaHeight;
	float AreaSizeP = AreaSize / 100.0f;
	int *TempBuff = (int*)MY_ALLOC(Width * Height * sizeof(int));
	if (TempBuff == NULL)
	{
		return ReturnBuff;
	}
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
//memset(&CurScreenshot->Pixels[(y + y2) * Width + x], 127, AreaWidth*sizeof(int));
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
	MY_FREE(TempBuff);
	FileDebug("\tImage search finished");
	return ReturnBuff;
}

char* WINAPI ImageSearch_Multipass_PixelCount(int Color, int PercentMax, int PercentMin, int PercentStep, int AreaWidth, int AreaHeight)
{
	char ReturnBuff2[DEFAULT_STR_BUFFER_SIZE * 50];
	int MatchesFound = 0;
	ReturnBuff[0] = 0;
	ReturnBuff2[0] = 0;
	FileDebug("Started Image search");
	if (CurScreenshot->Pixels == NULL)
	{
		FileDebug("Skipping Image search no screenshot is available");
		return ReturnBuff;
	}

	int Width = CurScreenshot->GetWidth();
	int Height = CurScreenshot->GetHeight();
	int AreaSize = AreaWidth * AreaHeight;
	float AreaSizeP = AreaSize / 100.0f;
	int *TempBuff = (int*)MY_ALLOC(Width * Height * sizeof(int));
	memcpy(TempBuff, CurScreenshot->Pixels, Width * Height * sizeof(int));

	for (int Percent = PercentMax; Percent >= PercentMin; Percent -= PercentStep)
	{
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
				int Counter = GetPixelCountRegion(&TempBuff[y * Width + x], Width, Color, PrevBlockCount, AreaWidth, AreaHeight);
				float Ratio = Counter / AreaSizeP;
				PrevBlockCount = Counter;
				if (Ratio >= Percent)
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
#ifdef _CONSOLE
memset(&CurScreenshot->Pixels[(y + y2) * Width + x], Percent, AreaWidth*sizeof(int));
#endif
						//mark this zone that we can skip processing
						TempBuff[(y + y2) * Width + x] = 66;
					}
					//jump forward with X, there is no need to search this area
					y += AreaHeight;
					PrevBlockCount = -1;
				}
			}
		}
	}
	if (MatchesFound == 0)
		FileDebug("\t Image search found no matches");

	sprintf_s(ReturnBuff, DEFAULT_STR_BUFFER_SIZE * 10, "%d%s", MatchesFound, ReturnBuff2);
	MY_FREE(TempBuff);
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
		return ReturnBuff;
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
	int *TempBuff = (int*)MY_ALLOC(Width * Height * sizeof(int));
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
	MY_FREE(TempBuff);
	FileDebug("\tImage search finished");
	return ReturnBuff;
}

void ImageSearch_Multipass_PixelCount2(int Color, int PercentMax, int PercentMin, int PercentStep, int AreaWidth, int AreaHeight, int OneSearchInRadius)
{
	int MatchesFound = 0;
	SearchResultCount = 0;
	FileDebug("Started Image search");
	if (CurScreenshot->Pixels == NULL)
	{
		FileDebug("Skipping Image search no screenshot is available");
		return;
	}

	int Width = CurScreenshot->GetWidth();
	int Height = CurScreenshot->GetHeight();
	int AreaSize = AreaWidth * AreaHeight;
	int *TempBuff = (int*)MY_ALLOC(Width * Height * sizeof(int));
	memcpy(TempBuff, CurScreenshot->Pixels, Width * Height * sizeof(int));

	for (int Percent = PercentMax; Percent >= PercentMin; Percent -= PercentStep)
	{
		int tPercent = Percent * AreaSize;
		for (int x = 0; x < Width - AreaWidth; x += 1)
		{
			int LastJumpY = -1;
			int PrevBlockCount = -1;
			for (int y = 0; y < Height - AreaHeight; y += 1)
			{
				// this will only help if we plan to find multiple areas with considerable size. ( we will find it at least once right ? )
				if (TempBuff[y * Width + x] > 0x00FFFFFF)
				{
					int newY = (TempBuff[y * Width + x] & 0x00FFFFFF); // skip search to this row
					if (newY > y)
						y = newY;
					PrevBlockCount = -1;
					LastJumpY = y + 1; // if jump over overlapped regions, we will find them more than once
					continue;
				}/**/
				// count Colors
				int Counter = GetPixelCountRegion(&TempBuff[y * Width + x], Width, Color, PrevBlockCount, AreaWidth, AreaHeight);
				int Ratio = Counter * 100;
				PrevBlockCount = Counter;
				if (Ratio >= tPercent)
				{
					// If we got here, there is a match
					MatchesFound++;
					int retx = x + CurScreenshot->Left;
					int rety = y + CurScreenshot->Top;
					//bugtest
					if (SearchResultCount > 0 && (abs(SearchResultXYSAD[SearchResultCount - 1][0] - retx) < OneSearchInRadius || abs(SearchResultXYSAD[SearchResultCount - 1][1] - rety) < OneSearchInRadius))
						continue;
					if (SearchResultCount < sizeof(SearchResultXYSAD) / sizeof(int))
					{
						SearchResultXYSAD[SearchResultCount][0] = retx;
						SearchResultXYSAD[SearchResultCount][1] = rety;
						SearchResultXYSAD[SearchResultCount][2] = Ratio;
						SearchResultCount++;
					}
					//mark this zone so we do not find it again
					int XStart = x - OneSearchInRadius;
					if (XStart < 0)
						XStart = 0;
					int XEnd = x + OneSearchInRadius;
					if (XEnd > Width)
						XEnd = Width - OneSearchInRadius;
					int YStart = y - OneSearchInRadius;
					if (YStart < 0)
						YStart = 0;
					int YEnd = y + OneSearchInRadius;
					if (YEnd >= Height)
						YEnd = Height - 1;
					//need to set "everything" or else the next search will reach into our zone. Since we search top-bottom, we would only need to set "AreaHeight" rows at the top
					for (int y2 = YStart; y2 < YEnd; y2++)
					{
						for (int x2 = XStart; x2 <= XEnd; x2++)
							TempBuff[y2 * Width + x2] = YEnd | 0x0F000000;
#if defined( _CONSOLE ) && defined( _DEBUG )
int tAreaWidth = (XEnd - XStart)*sizeof(int);
memset(&CurScreenshot->Pixels[y2 * Width + XStart], MatchesFound, tAreaWidth);
#endif
					}
					//jump forward with X, there is no need to search this area
					y = YEnd;
					PrevBlockCount = -1;
				}
			}
		}
	}
	if (MatchesFound == 0)
		FileDebug("\t Image search found no matches");

	MY_FREE(TempBuff);
	FileDebug("\tImage search finished");
	return;
}


void ImageSearch_Multipass_PixelCount3(int Color, int PercentMin, int AreaWidth, int AreaHeight, int OneSearchInRadius)
{
	int MatchesFound = 0;
	SearchResultCount = 0;
	FileDebug("Started Image search");
	if (CurScreenshot->Pixels == NULL)
	{
		FileDebug("Skipping Image search no screenshot is available");
		return;
	}

	int Width = CurScreenshot->GetWidth();
	int Height = CurScreenshot->GetHeight();
	int AreaSize = AreaWidth * AreaHeight;
	int *TempBuff = (int*)MY_ALLOC(Width * Height * sizeof(int));
	memcpy(TempBuff, CurScreenshot->Pixels, Width * Height * sizeof(int));

	{
		int tPercent = PercentMin * AreaSize;
		for (int x = 0; x < Width - AreaWidth; x += 1)
		{
			int PrevBlockCount = -1;
			for (int y = 0; y < Height - AreaHeight; y += 1)
			{
				// count Colors
				int Counter = GetPixelCountRegion(&TempBuff[y * Width + x], Width, Color, PrevBlockCount, AreaWidth, AreaHeight);
				int Ratio = Counter * 100;
				PrevBlockCount = Counter;
				if (Ratio >= tPercent)
				{
					int retx = x + CurScreenshot->Left;
					int rety = y + CurScreenshot->Top;
					//check if we have a nearby result
					int AlreadyExists = 0;
					for (int i = 0; i < SearchResultCount; i++)
						if (abs(retx - SearchResultXYSAD[i][0]) < OneSearchInRadius && abs(rety - SearchResultXYSAD[i][1]) < OneSearchInRadius)
						{
							AlreadyExists = 1;
							//this is better? update it
							if (SearchResultXYSAD[i][2] < Ratio)
							{
								SearchResultXYSAD[SearchResultCount][0] = retx;
								SearchResultXYSAD[SearchResultCount][1] = rety;
								SearchResultXYSAD[SearchResultCount][2] = Ratio;
							}
						}
					if (AlreadyExists == 0)
					{
						// If we got here, there is a match
						MatchesFound++;
						if (SearchResultCount < sizeof(SearchResultXYSAD) / sizeof(int))
						{
							SearchResultXYSAD[SearchResultCount][0] = retx;
							SearchResultXYSAD[SearchResultCount][1] = rety;
							SearchResultXYSAD[SearchResultCount][2] = Ratio;
							SearchResultCount++;
						}
						else
							return;
					}
				}
			}
		}
	}
	if (MatchesFound == 0)
		FileDebug("\t Image search found no matches");

	MY_FREE(TempBuff);
	FileDebug("\tImage search finished");
	return;
}