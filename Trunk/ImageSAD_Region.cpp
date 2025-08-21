#include "StdAfx.h"

// not sure how healthy this is. Passing this buffer cross dll. Probably should convert it to global alloc
static char ReturnBuffSadRegion[DEFAULT_STR_BUFFER_SIZE * 10];

template <const bool secondary_cmp_hash, const bool secondary_cmp_SATD, const bool primary_cmp_hash, const bool primary_cmp_SATD, const int MultiStageSadVer>
void ImageSearch_SAD_Region_(CachedPicture* cache, int aLeft, int aTop, int aRight, int aBottom, SADSearchRegionFlags uSearchFlags, ImgSrchSADRegionRes& res)
{
#ifdef _DEBUG
	int MatchesFound = 0;
	size_t startStamp = GetTickCount();
	FileDebug("Started Image search");
#endif

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
		cache->PrevSearchTop == aTop && cache->PrevSearchLeft == aLeft && cache->PrevSearchFlags == uSearchFlags)
	{
		FileDebug("Skipping Image search as it's done on same image with same params");
		return;
	}
#endif 

	cache->PrevSearchImageId = CurScreenshot->UniqueFameCounter;
	cache->PrevSearchTop = aTop;
	cache->PrevSearchLeft = aLeft;
	cache->PrevSearchFlags = uSearchFlags;

	// record searched regions so auto screenshot might know next time
	const int dEstimatedSearchRadius = 1;
	AddSearchedRegion(CurScreenshot->UniqueFameCounter, 
		aLeft - dEstimatedSearchRadius, aTop - dEstimatedSearchRadius, 
		aRight + cache->Width + dEstimatedSearchRadius,	aBottom + cache->Height + dEstimatedSearchRadius);

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
	// this happens when searched image is larger than screenshot
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

	ImgHashCache* cacheHash = NULL;
	if constexpr (primary_cmp_hash == true || secondary_cmp_hash == true ) {
		ReinitScreenshotHashCache(CurScreenshot);
		cacheHash = GetCreateCacheHash(cache);
	} else if ((uSearchFlags & SSRF_ST_INLCUDE_HASH_INFO)) {
		ReinitScreenshotHashCache(CurScreenshot);
		cacheHash = GetCreateCacheHash(cache);
	}

#ifdef _DEBUG
	{
		char dbgmsg[DEFAULT_STR_BUFFER_SIZE];
		sprintf_s(dbgmsg, sizeof(dbgmsg), "\t\t search flags=%d, primary_hash=%d, primary_satd=%d, sec_hash=%d, sec_satd=%d", uSearchFlags, primary_cmp_hash, primary_cmp_SATD, secondary_cmp_hash, secondary_cmp_SATD);
		FileDebug(dbgmsg);
		if ((uSearchFlags & SSRF_ST_PROCESS_INLCUDE_DIFF_INFO))
			FileDebug("Result should include diff info");
		if ((uSearchFlags & SSRF_ST_REMOVE_BRIGHTNES_FROM_SAD))
			FileDebug("Result should include brightness corrected SAD");
	}
#endif

	const LPCOLORREF Pixels1 = CurScreenshot->Pixels;
	const LPCOLORREF Pixels2 = cache->Pixels;

	// check if there is an alpha channel mismatch
#ifdef _DEBUG
	if ((Pixels1[0] & 0xFF000000) != (Pixels2[0] & 0xFF000000))
	{
		char dbgmsg[DEFAULT_STR_BUFFER_SIZE];
		sprintf_s(dbgmsg, sizeof(dbgmsg), "ImageSearch_SAD_Region: !!!! Alpha channel mismatch between screenshot and loaded image. SS %d, C %d", 
			((Pixels1[0] >> 24 ) & 0xFF), ((Pixels2[0] >> 24) & 0xFF));
		FileDebug(dbgmsg);
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
	const uint64_t max_sad_value = ~(uint64_t)0;
	const uint64_t max_smallestDiffPCT = 10000;
	uint64_t multiStageSads[9];
	if constexpr (MultiStageSadVer != 0) {
		memset(multiStageSads, (uint8_t)0xFF, sizeof(multiStageSads));
	}
	res.HashSmallestDiffPCT = max_smallestDiffPCT;
	res.BestSATD = max_sad_value;
	res.retx = -1;
	res.rety = -1;
	res.BestSAD = max_sad_value;
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
			{
				// because you can't do double checks using template programing :S 
#define INLINE_HASH_CHECK_CODE \
				int64_t GenHashErr = GenHashesOnScreenshotForCachedImage<primary_cmp_hash>(cache, CurScreenshot, x, y); \
				if (GenHashErr == 0) \
				{ \
					ImgHash8x8_CompareResult compareRes; \
					compareHash(CurScreenshot->pSSHashCache, cacheHash, x, y, &compareRes); \
					if (compareRes.PctDifferAvg < res.HashSmallestDiffPCT) { \
						res.BestSAD = sad; \
						res.HashSmallestDiffPCT = compareRes.PctDifferAvg; \
						res.retx = (int)x; \
						res.rety = (int)y; \
						if (res.HashSmallestDiffPCT == 0.0) \
							goto docleanupandreturn; \
					} \
					HashingWasPossible = 1; \
				}
#define INLINE_SATD_CHECK_CODE \
				size_t satd = satd_nxm(&AddrBig[x], AddrSmall, width_SAD, height_SAD, stride1, stride2, res.BestSATD); \
				if (satd < res.BestSATD) \
				{ \
					res.BestSAD = sad; \
					res.BestSATD = satd; \
					res.retx = (int)x; \
					res.rety = (int)y; \
					if (res.BestSATD == 0) \
						goto docleanupandreturn; \
				}

				size_t HashingWasPossible = 0;

				if constexpr (primary_cmp_hash == true) {
					uint64_t sad = max_sad_value;
					INLINE_HASH_CHECK_CODE;
				}
				else if constexpr (secondary_cmp_hash == true) {
					HashingWasPossible = 1;
					uint64_t sad;
					if constexpr (MultiStageSadVer == 1) {
						sad = ImageSad2Stage(&AddrBig[x], stride1, AddrSmall, stride2, width_SAD, height_SAD, multiStageSads);
					}
					else if constexpr (MultiStageSadVer == 2) {
						sad = ImageSad4Stage(&AddrBig[x], stride1, AddrSmall, stride2, width_SAD, height_SAD, multiStageSads);
					}
					else if constexpr (MultiStageSadVer == 3) {
						sad = ImageSad9Stage(&AddrBig[x], stride1, AddrSmall, stride2, width_SAD, height_SAD, multiStageSads);
					}
					else if constexpr (MultiStageSadVer == 4) {
						sad = ImageSadGrayScale_s(CurScreenshot, cache, x, y, width_SAD, height_SAD, multiStageSads);
					}
					else if constexpr (MultiStageSadVer == 5) {
						sad = ImageSad2StageGrayScale_s(CurScreenshot, cache, x, y, width_SAD, height_SAD, multiStageSads);
					}
					else {
						sad = ImageSad(&AddrBig[x], stride1, AddrSmall, stride2, width_SAD, height_SAD);
					}
					if (sad < res.BestSAD) {
						INLINE_HASH_CHECK_CODE;
					}
				}
				// there are cases when SAD(loc2) < SAD(loc1) BUT SATD(loc2) > SATD(loc1)
				// SATD is about 100+ times slower than SAD
				else if constexpr (primary_cmp_SATD == true) {
					HashingWasPossible = 1;
					uint64_t sad = max_sad_value;
					INLINE_SATD_CHECK_CODE;
				}
				else if constexpr (secondary_cmp_SATD == true) {
					HashingWasPossible = 1;
					uint64_t sad;
					if constexpr (MultiStageSadVer == 1) {
						sad = ImageSad2Stage(&AddrBig[x], stride1, AddrSmall, stride2, width_SAD, height_SAD, multiStageSads);
					}
					else if constexpr (MultiStageSadVer == 2) {
						sad = ImageSad4Stage(&AddrBig[x], stride1, AddrSmall, stride2, width_SAD, height_SAD, multiStageSads);
					}
					else if constexpr (MultiStageSadVer == 3) {
						sad = ImageSad9Stage(&AddrBig[x], stride1, AddrSmall, stride2, width_SAD, height_SAD, multiStageSads);
					}
					else if constexpr (MultiStageSadVer == 4) {
						sad = ImageSadGrayScale_s(CurScreenshot, cache, x, y, width_SAD, height_SAD, multiStageSads);
					}
					else if constexpr (MultiStageSadVer == 5) {
						sad = ImageSad2StageGrayScale_s(CurScreenshot, cache, x, y, width_SAD, height_SAD, multiStageSads);
					}
					else {
						sad = ImageSad(&AddrBig[x], stride1, AddrSmall, stride2, width_SAD, height_SAD);
					}
					if (sad < res.BestSAD) {
						INLINE_SATD_CHECK_CODE;
					}
				}

				// if secondary enforcer is not requested, fall back to simple SAD comparison
				if (HashingWasPossible == 0) {
					uint64_t sad;
					if constexpr (MultiStageSadVer == 1) {
						sad = ImageSad2Stage(&AddrBig[x], stride1, AddrSmall, stride2, width_SAD, height_SAD, multiStageSads);
					}
					else if constexpr (MultiStageSadVer == 2) {
						sad = ImageSad4Stage(&AddrBig[x], stride1, AddrSmall, stride2, width_SAD, height_SAD, multiStageSads);
					}
					else if constexpr (MultiStageSadVer == 3) {
						sad = ImageSad9Stage(&AddrBig[x], stride1, AddrSmall, stride2, width_SAD, height_SAD, multiStageSads);
					}
					else if constexpr (MultiStageSadVer == 4) {
						sad = ImageSadGrayScale_s(CurScreenshot, cache, x, y, width_SAD, height_SAD, multiStageSads);
					}
					else if constexpr (MultiStageSadVer == 5) {
						sad = ImageSad2StageGrayScale_s(CurScreenshot, cache, x, y, width_SAD, height_SAD, multiStageSads);
					}
					else {
						sad = ImageSad(&AddrBig[x], stride1, AddrSmall, stride2, width_SAD, height_SAD);
					}
					if (sad < res.BestSAD) {
#ifdef _DEBUG
						MatchesFound++;
#endif
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
	}
docleanupandreturn:
	if (res.retx == -1)
		FileDebug("\t Image search found no matches");

	// there are chances we skipped calculating best sad value when using hashing or satd as primary compare function
	if constexpr (primary_cmp_hash == true || primary_cmp_SATD == true || secondary_cmp_hash == true && secondary_cmp_SATD == true) {
		if (res.BestSAD == max_sad_value && res.retx != -1){
			const LPCOLORREF AddrBig = &Pixels1[res.rety * stride1 + res.retx];
			const LPCOLORREF AddrSmall = &Pixels2[0];
			res.BestSAD = ImageSad(AddrBig, stride1, AddrSmall, stride2, width_SAD, height_SAD);
		}
	}

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
#if defined(_DEBUG) && 0
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
	if (res.rety != -1 && (uSearchFlags & SSRF_ST_INLCUDE_SATD_INFO) && res.BestSATD == max_sad_value)
	{
		const LPCOLORREF AddrBig = &Pixels1[res.rety * stride1 + res.retx];
		const LPCOLORREF AddrSmall = &Pixels2[0];
		res.BestSATD = satd_nxm(AddrBig, AddrSmall, width_SAD, height_SAD, stride1, stride2);
	}
	if (res.rety != -1 && (uSearchFlags & SSRF_ST_INLCUDE_HASH_INFO) && res.HashSmallestDiffPCT == max_smallestDiffPCT)
	{
		int64_t GenHashErr = GenHashesOnScreenshotForCachedImage<primary_cmp_hash>(cache, CurScreenshot, res.retx, res.rety);
		if (GenHashErr == 0)
		{
			ImgHash8x8_CompareResult compareRes;
			if (compareHash(CurScreenshot->pSSHashCache, cacheHash, res.retx, res.rety, &compareRes) == 0) {
				res.HashSmallestDiffPCT = compareRes.PctDifferAvg;
			}
			else {
				FileDebug("\t\t!!!Failed to compare final hash values");
			}
		}
		else {
			FileDebug("\t\t!!!Failed to generate final hash value");
		}
	}
#ifdef _DEBUG
	if (((res.BestSAD == 0 && res.colorDiffCount != 0) || (res.BestSAD != 0 && res.colorDiffCount == 0)) && (uSearchFlags & SSRF_ST_PROCESS_INLCUDE_DIFF_INFO))
	{
		FileDebug("\t\t!!!Unexpected difference between SIMD SAD and manual SAD");
	}
	char dbgmsg[DEFAULT_STR_BUFFER_SIZE];
	if (res.rety == -1) { res.rety = 0; res.retx = 0; }
	sprintf_s(dbgmsg, sizeof(dbgmsg), "\t\t retxy %d %d -> %d %d", res.retx, res.rety, (int)(res.retx + CurScreenshot->Left), (int)(res.rety + CurScreenshot->Top));
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

	// we are missing SATD value here
/*	if constexpr (primary_cmp_SATD == false && secondary_cmp_SATD == false) {
		if (res.BestSATD == max_sad_value) {
			res.BestSATD = 0;
		}
	}
	// we are missing hash value here
	if constexpr (primary_cmp_hash == false && secondary_cmp_hash == false) {
		if (res.HashSmallestDiffPCT == 100) {
			res.HashSmallestDiffPCT = 0;
		}
	}*/
	res.SADPerPixel = res.BestSADBrightnessAdjusted / (width_SAD * height_SAD * 3); // this actually called MAD
	if (res.BestSATD != max_sad_value) {
		res.SATDPerPixel = res.BestSATD / (width_SAD * height_SAD * 3);
	}
	else {
		res.SATDPerPixel = max_sad_value;
	}
	if (res.retx == -1) {
		res.found_res = 0;
	}
	else {
		res.found_res = 1;
	}
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

static inline char* ImageSearch_SAD_Region_FormatRes(ImgSrchSADRegionRes& res)
{
	ReturnBuffSadRegion[0] = 0;
	sprintf_s(ReturnBuffSadRegion, DEFAULT_STR_BUFFER_SIZE * 10, "%d|%d|%d|%llu|%llu|%llu|%llu|%llu|%.2f|%llu|%llu|%llu",
		res.found_res, res.retx, res.rety, res.BestSAD, res.SADPerPixel, res.avgColorDiff, res.colorDiffCount, res.colorDifferentPct,
		float(res.HashSmallestDiffPCT)/100.0f, res.BestSATD, res.SATDPerPixel, res.BestSADBrightnessAdjusted);
	return ReturnBuffSadRegion;
}

template<int MS>
static inline void ImageSearch_SAD_Region_DecideCmpFunc(CachedPicture* cache, int aLeft, int aTop, int aRight, int aBottom, SADSearchRegionFlags uSearchFlags, ImgSrchSADRegionRes& res)
{
	const bool secondary_cmp_hash = (uSearchFlags & SSRF_ST_ENFORCE_SAD_WITH_HASH) != 0;
	const bool secondary_cmp_SATD = (uSearchFlags & SSRF_ST_ENFORCE_SAD_WITH_SATD) != 0;
	const bool primary_cmp_hash = (uSearchFlags & SSRF_ST_MAIN_CHECK_IS_HASH) != 0;
	const bool primary_cmp_SATD = (uSearchFlags & SSRF_ST_MAIN_CHECK_IS_SATD) != 0;

	// Optional sanity checks if your flags must be mutually exclusive:
	// assert(!(primary_cmp_hash && primary_cmp_SATD));
	// assert(secondary_cmp_hash || secondary_cmp_SATD); // etc.

	const unsigned idx =
		(unsigned(secondary_cmp_hash) << 3) |
		(unsigned(secondary_cmp_SATD) << 2) |
		(unsigned(primary_cmp_hash) << 1) |
		(unsigned(primary_cmp_SATD) << 0);

	switch (idx) {
	case 0b0000: return ImageSearch_SAD_Region_<false, false, false, false, MS>(cache, aLeft, aTop, aRight, aBottom, uSearchFlags, res);
	case 0b0001: return ImageSearch_SAD_Region_<false, false, false, true, MS>(cache, aLeft, aTop, aRight, aBottom, uSearchFlags, res);
	case 0b0010: return ImageSearch_SAD_Region_<false, false, true, false, MS>(cache, aLeft, aTop, aRight, aBottom, uSearchFlags, res);
	case 0b0011: return ImageSearch_SAD_Region_<false, false, true, true, MS>(cache, aLeft, aTop, aRight, aBottom, uSearchFlags, res);
	case 0b0100: return ImageSearch_SAD_Region_<false, true, false, false, MS>(cache, aLeft, aTop, aRight, aBottom, uSearchFlags, res);
	case 0b0101: return ImageSearch_SAD_Region_<false, true, false, true, MS>(cache, aLeft, aTop, aRight, aBottom, uSearchFlags, res);
	case 0b0110: return ImageSearch_SAD_Region_<false, true, true, false, MS>(cache, aLeft, aTop, aRight, aBottom, uSearchFlags, res);
	case 0b0111: return ImageSearch_SAD_Region_<false, true, true, true, MS>(cache, aLeft, aTop, aRight, aBottom, uSearchFlags, res);
	case 0b1000: return ImageSearch_SAD_Region_< true, false, false, false, MS>(cache, aLeft, aTop, aRight, aBottom, uSearchFlags, res);
	case 0b1001: return ImageSearch_SAD_Region_< true, false, false, true, MS>(cache, aLeft, aTop, aRight, aBottom, uSearchFlags, res);
	case 0b1010: return ImageSearch_SAD_Region_< true, false, true, false, MS>(cache, aLeft, aTop, aRight, aBottom, uSearchFlags, res);
	case 0b1011: return ImageSearch_SAD_Region_< true, false, true, true, MS>(cache, aLeft, aTop, aRight, aBottom, uSearchFlags, res);
	case 0b1100: return ImageSearch_SAD_Region_< true, true, false, false, MS>(cache, aLeft, aTop, aRight, aBottom, uSearchFlags, res);
	case 0b1101: return ImageSearch_SAD_Region_< true, true, false, true, MS>(cache, aLeft, aTop, aRight, aBottom, uSearchFlags, res);
	case 0b1110: return ImageSearch_SAD_Region_< true, true, true, false, MS>(cache, aLeft, aTop, aRight, aBottom, uSearchFlags, res);
	case 0b1111: return ImageSearch_SAD_Region_< true, true, true, true, MS>(cache, aLeft, aTop, aRight, aBottom, uSearchFlags, res);
	default:
		assert(false);
	}
}

static inline void ImageSearch_SAD_Region_DecideSad(CachedPicture* cache, int aLeft, int aTop, int aRight, int aBottom, SADSearchRegionFlags uSearchFlags, ImgSrchSADRegionRes& res)
{
	if ((uSearchFlags & SSRF_ST_ALLOW_MULTI_STAGE_SAD2) != 0) {
		// 2 stage SAD
		if (cache->Height > 2 && cache->Width > 8) {
			ImageSearch_SAD_Region_DecideCmpFunc<1>(cache, aLeft, aTop, aRight, aBottom, uSearchFlags, res);
			return;
		}
	}
	if ((uSearchFlags & SSRF_ST_ALLOW_MULTI_STAGE_SAD4) != 0) {
		// 4 stage SAD
		if (cache->Height > 2 && cache->Width > 16) {
			ImageSearch_SAD_Region_DecideCmpFunc<2>(cache, aLeft, aTop, aRight, aBottom, uSearchFlags, res);
			return;
		}
	}
	if ((uSearchFlags & SSRF_ST_ALLOW_MULTI_STAGE_SAD9) != 0) {
		if (cache->Height > 3 && cache->Width > 24) {
			// 9 stage SAD
			ImageSearch_SAD_Region_DecideCmpFunc<3>(cache, aLeft, aTop, aRight, aBottom, uSearchFlags, res);
			return;
		}
	}
	if ((uSearchFlags & SSRF_ST_ALLOW_MULTI_STAGE_GSAD) != 0) {
		if (cache->Width > 32) {
			EnsureCacheHasGrayscale(cache);
			EnsureScreenshotHasGrayscale();
			ImageSearch_SAD_Region_DecideCmpFunc<4>(cache, aLeft, aTop, aRight, aBottom, uSearchFlags, res);
			return;
		}
	}
	if ((uSearchFlags & SSRF_ST_ALLOW_MULTI_STAGE_GSAD2) != 0) {
		if (cache->Height > 2 && cache->Width > 64) {
			EnsureCacheHasGrayscale(cache);
			EnsureScreenshotHasGrayscale();
			ImageSearch_SAD_Region_DecideCmpFunc<5>(cache, aLeft, aTop, aRight, aBottom, uSearchFlags, res);
			return;
		}
		// fall back to mini version
		else if (cache->Width > 32) {
			EnsureCacheHasGrayscale(cache);
			EnsureScreenshotHasGrayscale();
			ImageSearch_SAD_Region_DecideCmpFunc<4>(cache, aLeft, aTop, aRight, aBottom, uSearchFlags, res);
			return;
		}
	}
	ImageSearch_SAD_Region_DecideCmpFunc<0>(cache, aLeft, aTop, aRight, aBottom, uSearchFlags, res);
}

char* WINAPI ImageSearch_SAD_Region(const char* aFilespec, int aLeft, int aTop, int aRight, int aBottom, SADSearchRegionFlags uSearchFlags)
{
	CachedPicture* cache = CachePicture(aFilespec);
	if (cache) {
		// nothing changed, we can consider the same response
		if (cache->PrevSearchImageId == CurScreenshot->UniqueFameCounter &&
			cache->PrevSearchTop == aTop && cache->PrevSearchLeft == aLeft && cache->PrevSearchFlags == uSearchFlags) {
			FileDebug("Skipping Image search as previous search parameters match");
			return cache->PrevSearchReturnVal;
		}
		else {
			ImgSrchSADRegionRes res = { 0 };

			// generate a new search result on a new screenshot
			ImageSearch_SAD_Region_DecideSad(cache, aLeft, aTop, aRight, aBottom, uSearchFlags, res);

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
