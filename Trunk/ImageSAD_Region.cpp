#include "StdAfx.h"

// not sure how healthy this is. Passing this buffer cross dll. Probably should convert it to global alloc
static char ReturnBuffSadRegion[DEFAULT_STR_BUFFER_SIZE * 10];
static char *ImageSearch_SAD_Region_FormatRes(ImgSrchSADRegionRes& res)
{
	ReturnBuffSadRegion[0] = 0;
	sprintf_s(ReturnBuffSadRegion, DEFAULT_STR_BUFFER_SIZE * 10, "1|%d|%d|%llu|%llu|%llu|%llu|%llu|%d|%llu|%llu",
		res.retx, res.rety, res.BestSAD, res.SADPerPixel, res.avgColorDiff, res.colorDiffCount, res.colorDifferentPct, 
		int(res.HashSmallestDiffPCT), res.BestSATD, res.BestSADBrightnessAdjusted);
	return ReturnBuffSadRegion;
}

char* WINAPI ImageSearch_SAD_Region(const char* aFilespec, int aLeft, int aTop, int aRight, int aBottom, SADSearchRegionFlags uSearchFlags)
{
	CachedPicture* cache = CachePicture(aFilespec);
	if (cache) {
		// nothing changed, we can consider the same response
		if (cache->PrevSearchImageId == CurScreenshot->UniqueFameCounter &&
			cache->PrevSearchTop == aTop && cache->PrevSearchLeft == aLeft) {
			return cache->PrevSearchReturnVal;
		}
		else {
			ImgSrchSADRegionRes res = { 0 };

			// generate a new search result on a new screenshot
			ImageSearch_SAD_Region_(cache, aLeft, aTop, aRight, aBottom, uSearchFlags, res);

			// format the results
			char* str_res = ImageSearch_SAD_Region_FormatRes(res);

			// in case caller is spamming searches more than we are able to keep up the pace with
			strcpy_s(cache->PrevSearchReturnVal, str_res);

			return str_res;
		}
	}

	ReturnBuffSadRegion[0] = 0;
	return ReturnBuffSadRegion;
}

void ImageSearch_SAD_Region_(CachedPicture* cache, int aLeft, int aTop, int aRight, int aBottom, SADSearchRegionFlags uSearchFlags, ImgSrchSADRegionRes& res)
{
#ifdef _DEBUG
	int MatchesFound = 0;
	size_t startStamp = GetTickCount();
#endif
	FileDebug("Started Image search");

	if (cache == NULL)
	{
		FileDebug("Skipping Image search as image could not be loaded");
		return;
	}
	if (cache->Pixels == NULL)
	{
		FileDebug("Skipping Image search as image pixels are missing");
		return;
	}
	if (cache->Height <= 0 || cache->Width < 8)
	{
		FileDebug("Skipping Image search as searched image height is 0");
		return;
	}
	if (CurScreenshot == NULL || CurScreenshot->Pixels == NULL)
	{
		FileDebug("Skipping Image search no screenshot is available");
		return;
	}
#if !defined(_CONSOLE) // because we do want to have benchmarks
	if (cache->PrevSearchImageId == CurScreenshot->UniqueFameCounter &&
		cache->PrevSearchTop == aTop && cache->PrevSearchLeft == aLeft)
	{
		FileDebug("Skipping Image search as it's done on same image with same params");
		return;
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
			return;
		}
		aRight = CurScreenshot->Right - cache->Width;
	}
	if (aBottom > (CurScreenshot->Bottom - cache->Height))
	{
		FileDebug("ImageSearch_SAD_Region:Search Bottom falls outside screenshot Bottom. Adjusting it");
		if (CurScreenshot->Bottom <= cache->Height) {
			FileDebug("ImageSearch_SAD_Region:Screenshot is too small for searching. Skipping search");
			return;
		}
		aBottom = CurScreenshot->Bottom - cache->Height;
	}
	if (aRight <= aLeft)
	{
		FileDebug("ImageSearch_SAD_Region:Screenshot width is too small for searching. Skipping search");
		return;
	}
	if (aBottom <= aTop)
	{
		FileDebug("ImageSearch_SAD_Region:Screenshot height is too small for searching. Skipping search");
		return;
	}

	if ((uSearchFlags & SSRF_ST_ENFORCE_SAD_WITH_HASH)) {
		ReinitScreenshotHashCache(CurScreenshot);
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
	res.HashSmallestDiffPCT = 100;
	res.BestSATD = 0x7FFFFFFF;
	res.retx = -1;
	res.rety = -1;
	res.BestSAD = 0x7FFFFFFF;
	//DumpAsPPM( &CurScreenshot->Pixels[ 40 * Width + 40 ], 40, 40, Width );
	const size_t stride1 = CurScreenshot->Width;
	const size_t stride2 = cache->Width;
	const size_t width_SAD = (cache->Width / 8) * 8; // value is in pixels
	const size_t height_SAD = cache->Height;	// value is in pixels
	for (size_t y = search_start_y; y < search_end_y; y++)
	{
		const LPCOLORREF AddrBig = &Pixels1[y * stride1];
		const LPCOLORREF AddrSmall = &Pixels2[0];
		for (size_t x = search_start_x; x < search_end_x; x++)
		{
			uint64_t sad = ImageSad(&AddrBig[x], stride1, AddrSmall, stride2, width_SAD, height_SAD);
			if (res.BestSAD >= sad)
			{
#ifdef _DEBUG
				MatchesFound++;
#endif
				int HashingWasPossible = 0;
				// this code needs more testing. Seen it crash. Did not had the time to debug
				if ((uSearchFlags & SSRF_ST_ENFORCE_SAD_WITH_HASH))
				{
					ImgHashWholeIage* cacheHash = GetCreateCacheHash(cache);
					int GenHashErr = GenHashesOnScreenshotForCachedImage(cache, CurScreenshot, (int)x, (int)y, CurScreenshot->pSSHashCache);
					if (GenHashErr == 0)
					{
						ImgHash8x8_CompareResult compareRes;
						int CompareErr = compareHash(cacheHash, CurScreenshot->pSSHashCache, &compareRes);
						//						printf("New best sad at x=%llu y=%llu. Hash old %f, hash new %f\n", x, y, HashSmallestDiffPCT, compareRes.pctMatchAvg);
						if (CompareErr == 0 && compareRes.pctMatchAvg < res.HashSmallestDiffPCT)
						{
							res.HashSmallestDiffPCT = compareRes.pctMatchAvg;
							res.retx = (int)x;
							res.rety = (int)y;
							HashingWasPossible = 1;
							res.BestSAD = sad;
							//exact match ? I doubt it will ever happen...
							if (res.HashSmallestDiffPCT == 100)
								goto docleanupandreturn;
						}
					}
				}
				// there are cases when SAD(loc2) < SAD(loc1) BUT SATD(loc2) > SATD(loc1)
				// SATD is about 100+ times slower than SAD
				if ((uSearchFlags & SSRF_ST_ENFORCE_SAD_WITH_SATD))
				{
					size_t satd = satd_nxm(&AddrBig[x], AddrSmall, width_SAD, height_SAD, stride1, stride2);
					if (satd < res.BestSATD)
					{
						res.BestSAD = sad;
						res.BestSATD = satd;
						res.retx = (int)x;
						res.rety = (int)y;
						if (res.BestSATD == 0)
							goto docleanupandreturn;
					}
					HashingWasPossible = 1;
				}
				// if secondary enforcer is not requested, fall back to simple SAD comparison
				if (HashingWasPossible == 0) {
					res.BestSAD = sad;
					res.retx = (int)x;
					res.rety = (int)y;
					//exact match ? I doubt it will ever happen...
					if (res.BestSAD == 0)
						goto docleanupandreturn;
				}
			}
		}
	}
docleanupandreturn:
	if (res.retx == -1)
		FileDebug("\t Image search found no matches");

	res.colorDiffCount = 0;
	res.colorDifferentPct = 0;
	res.avgColorDiff = 0;
	if (res.rety != -1 && (uSearchFlags & SSRF_ST_PROCESS_INLCUDE_DIFF_INFO))
	{
		const LPCOLORREF AddrBig = &Pixels1[res.rety * stride1 + res.retx];
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
					if (AddrBig2[0] != AddrSmall2[0]) res.colorDiffCount++;
					if (AddrBig2[1] != AddrSmall2[1]) res.colorDiffCount++;
					if (AddrBig2[2] != AddrSmall2[2]) res.colorDiffCount++;
#ifdef _DEBUG
					char dbgmsg[DEFAULT_STR_BUFFER_SIZE];
					sprintf_s(dbgmsg, sizeof(dbgmsg), "\t\t diff at %lld %lld screenshot %X cache %X DiffCount %llu", col, row, AddrBig[row * stride1 + col], AddrSmall[row * stride2 + col], res.colorDiffCount);
					FileDebug(dbgmsg);
#endif
				}
			}
		}
		if (res.colorDiffCount) {
			res.avgColorDiff = res.BestSAD / res.colorDiffCount;
			res.colorDifferentPct = (size_t)((double)res.colorDiffCount * 100.0 / (double)(width_SAD * cache->Height * 3));
		}
	}

	// if one of the images is brighter than the other, than remove the brightness difference from SAD
	// lazy approach is to just take avg brightness of the area and remove the difference per pixel
	// accurate solution is to get the minimum difference only for a specific pixel
	res.BestSADBrightnessAdjusted = res.BestSAD;
	if (res.rety != -1 && (uSearchFlags & SSRF_ST_REMOVE_BRIGHTNES_FROM_SAD) && res.BestSAD != 0)
	{
		// what is average r,g,b color
		size_t min_brightness_diff = 255;
		size_t BigPicIsBrighter = 2;
		size_t SmallPicIsBrighter = 2;
		const LPCOLORREF AddrBig = &Pixels1[res.rety * stride1 + res.retx];
		const LPCOLORREF AddrSmall = &Pixels2[0];
		for (size_t row = 0; row < cache->Height; row++)
		{
			for (size_t col = 0; col < width_SAD; col++) // * 4 because of 
			{
				const unsigned char* AddrBig2 = (unsigned char*)&AddrBig[row * stride1 + col];
				const unsigned char* AddrSmall2 = (unsigned char*)&AddrSmall[row * stride2 + col];
				for (size_t col_channel = 0; col_channel < 3; ++col_channel) {
					const unsigned char valBig = AddrBig2[col_channel];
					const unsigned char valSmall = AddrSmall2[col_channel];
					if (valBig > valSmall) {
						if (SmallPicIsBrighter == 1) {
							row = cache->Height;
							min_brightness_diff = 0; // nothing to adjust
							break;
						}
						BigPicIsBrighter = 1;
						size_t diff = valBig - valSmall;
						if (diff < min_brightness_diff) {
							min_brightness_diff = diff;
						}
					}
					else if (valBig < valSmall) {
						if (BigPicIsBrighter == 1) {
							row = cache->Height;
							min_brightness_diff = 0; // nothing to adjust
							break;
						}
						SmallPicIsBrighter = 1;
						size_t diff = valSmall - valBig;
						if (diff < min_brightness_diff) {
							min_brightness_diff = diff;
						}
					}
				}
			}
		}
		if (min_brightness_diff != 0) {
			size_t overlaying_pixel_count = cache->Height * width_SAD * 3;
			size_t sad_to_adjust = min_brightness_diff * overlaying_pixel_count;
			if (sad_to_adjust < res.BestSAD) {
				res.BestSADBrightnessAdjusted = res.BestSAD - sad_to_adjust;
#ifdef _DEBUG
				char dbgmsg[DEFAULT_STR_BUFFER_SIZE];
				sprintf_s(dbgmsg, sizeof(dbgmsg), "\t\t Adjusting SAD %lld to %lld due to brightness", res.BestSAD, res.BestSADBrightnessAdjusted);
				FileDebug(dbgmsg);
#endif
			}
			else {
#ifdef _DEBUG
				char dbgmsg[DEFAULT_STR_BUFFER_SIZE];
				sprintf_s(dbgmsg, sizeof(dbgmsg), "\t\t Brightness SAD adjustment %lld is larger than SAD %lld", sad_to_adjust, res.BestSAD);
				FileDebug(dbgmsg);
#endif
			}
		}
	}

#ifdef _DEBUG
	if (((res.BestSAD == 0 && res.colorDiffCount != 0) || (res.BestSAD != 0 && res.colorDiffCount == 0)) && (uSearchFlags & SSRF_ST_PROCESS_INLCUDE_DIFF_INFO))
	{
		FileDebug("\t\t!!!Unexpected difference between SIMD SAD and manual SAD");
	}
	char dbgmsg[DEFAULT_STR_BUFFER_SIZE];
	if (res.rety == -1) { res.rety = 0; res.retx = 0; }
	sprintf_s(dbgmsg, sizeof(dbgmsg), "\t\t retxy %d %d", res.retx, res.rety);
	FileDebug(dbgmsg);
	const unsigned char* AddrBig = (unsigned char*)&Pixels1[res.rety * stride1 + res.retx];
	const unsigned char* AddrSmall = (unsigned char*)&Pixels2[0];
	sprintf_s(dbgmsg, sizeof(dbgmsg), "\t\tpixel 0 0 : %d %d %d %d - %d %d %d %d",
		(int)AddrBig[0], (int)AddrBig[1], (int)AddrBig[2], (int)AddrBig[3], (int)AddrSmall[0], (int)AddrSmall[1], (int)AddrSmall[2], (int)AddrSmall[3]);
	FileDebug(dbgmsg);
	sprintf_s(dbgmsg, sizeof(dbgmsg), "\t\tpixel 1 0 : %d %d %d %d - %d %d %d %d",
		(int)AddrBig[4], (int)AddrBig[5], (int)AddrBig[6], (int)AddrBig[7], (int)AddrSmall[4], (int)AddrSmall[5], (int)AddrSmall[6], (int)AddrSmall[7]);
	FileDebug(dbgmsg);
#endif

	size_t SADPerPixel = res.BestSAD / (width_SAD * height_SAD * 3); // this actually called MAD
	res.retx = (int)(res.retx + CurScreenshot->Left);
	res.rety = (int)(res.rety + CurScreenshot->Top);

#ifdef _DEBUG
	size_t endStamp = GetTickCount();
	sprintf_s(dbgmsg, sizeof(dbgmsg), "\tImage search finished in %d ms. Name %s. Improved match %d.", (int)(endStamp - startStamp), cache->FileName, MatchesFound);
	FileDebug(dbgmsg);
	res.duration = endStamp - startStamp;
#endif

	return;
}
