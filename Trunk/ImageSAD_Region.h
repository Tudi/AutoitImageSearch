#pragma once

/*
* Made so many experiments that moved this SAD based search to a new file
*/

enum SADSearchRegionFlags : int
{
	SSRF_ST_NO_FLAGS = 0,
	SSRF_ST_PROCESS_INLCUDE_DIFF_INFO = 1 << 0, // for debugging
	SSRF_ST_ENFORCE_SAD_WITH_HASH = 1 << 1, // hash uses image features to compare 2 regions
	SSRF_ST_ENFORCE_SAD_WITH_SATD = 1 << 2, // better difference corelation than SAD but 10x slower
	SSRF_ST_ENFORCE_SAD_WITH_SSD = 1 << 3, // squared differences. few large changes have large impact
	SSRF_ST_ENFORCE_SAD_WITH_ZNCC = 1 << 4, // Zero-mean Normalized Cross-Correlation. Tolerance to brightness and lightning changes
	SSRF_ST_ENFORCE_SAD_WITH_EDGE_COLOR = 1 << 5, // when texture is more importnt than color
	SSRF_ST_ENFORCE_SAD_WITH_EDGE_GRAY = 1 << 6, // when texture is more importnt than color
	SSRF_ST_ENFORCE_SAD_WITH_EDGE_SSIM = 1 << 7, // Structural Similarity Index
	SSRF_ST_REMOVE_BRIGHTNES_FROM_SAD = 1 << 8, // If brightness setting changes since reference image was taken, sad would increase. Try to remove this
	SSRF_ST_MAIN_CHECK_IS_HASH = 1 << 9, // Default compare is SAD, but you can request it to be HASH
	SSRF_ST_MAIN_CHECK_IS_SATD = 1 << 10, // Default compare is SAD, but you can request it to be SATD
};

// have this return sumary to be able to wrap it in case multiple input images are given.
// should benchmark speed difference first
struct ImgSrchSADRegionRes {
	int retx, rety;
	size_t BestSAD, SADPerPixel, avgColorDiff, colorDiffCount, colorDifferentPct, BestSATD, BestSADBrightnessAdjusted;
	double HashSmallestDiffPCT;
	size_t duration;
};

char* WINAPI ImageSearch_SAD_Region(const char* aFilespec, int aLeft, int aTop, int aRight, int aBottom, SADSearchRegionFlags uSearchFlags);
void ImageSearch_SAD_Region_(CachedPicture* cache, int aLeft, int aTop, int aRight, int aBottom, SADSearchRegionFlags uSearchFlags, ImgSrchSADRegionRes &res);
