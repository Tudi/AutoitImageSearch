#include "StdAfx.h"
#include <immintrin.h>

//lagest number so that it will be smaller than our cached image width / height
LIBRARY_API int SimilarSearchGroupingSizeX = 8;
LIBRARY_API int SimilarSearchGroupingSizeY = 8;

void WINAPI SetupSimilarSearch( int MaxImageSize, int DownScale, int SearchType, int OnlyAtDiffMask )
{
	if( MaxImageSize > 1 && SimilarSearchGroupingSizeX > MaxImageSize )
		SimilarSearchGroupingSizeX = MaxImageSize;
	if( MaxImageSize > 1 && SimilarSearchGroupingSizeY > MaxImageSize )
		SimilarSearchGroupingSizeY = MaxImageSize;
}

SimilarSearch::SimilarSearch()
{
	Width = Height = BlockWidth = BlockHeight = SearchType = SearchDownScale = 0;
	R = G = B = NULL;
}

SimilarSearch::~SimilarSearch()
{
	if( R != NULL )
	{
		MY_FREE( R );
		MY_FREE( G );
		MY_FREE( B );
		R = G = B = NULL;
	}
}

static inline void SumRGB_mxn_AVX2(	const COLORREF* __restrict base,
	uint64_t width, uint64_t height, uint64_t stride,           // in pixels
	int* __restrict R, int* __restrict G, int* __restrict B)
{
	const __m256i vZero = _mm256_setzero_si256();
	const __m256i maskFF = _mm256_set1_epi32(0xFF);

	__m256i rAcc = _mm256_setzero_si256();
	__m256i gAcc = _mm256_setzero_si256();
	__m256i bAcc = _mm256_setzero_si256();

	// One load per row: 8 pixels (8*4=32 bytes)
	for (int y = 0; y < height; ++y) {
		const COLORREF* row = base + y * stride;
		for (int x = 0; x < width; x += 8) {
			__m256i px = _mm256_loadu_si256((const __m256i*)(row+x));

			// Extract channels: 0x00BBGGRR
			__m256i r = _mm256_and_si256(px, maskFF);
			__m256i g = _mm256_and_si256(_mm256_srli_epi32(px, 8), maskFF);
			__m256i b = _mm256_and_si256(_mm256_srli_epi32(px, 16), maskFF);

			// Sum bytes via SAD (per 128-bit half -> two 64-bit partials)
			rAcc = _mm256_add_epi64(rAcc, _mm256_sad_epu8(r, vZero));
			gAcc = _mm256_add_epi64(gAcc, _mm256_sad_epu8(g, vZero));
			bAcc = _mm256_add_epi64(bAcc, _mm256_sad_epu8(b, vZero));
		}
	}

	// Horizontal reduce 4x 64-bit lanes per accumulator
	uint64_t r64, g64, b64;
	{
		__m128i lo = _mm256_castsi256_si128(rAcc);
		__m128i hi = _mm256_extracti128_si256(rAcc, 1);
		__m128i s = _mm_add_epi64(lo, hi);
		uint64_t s0 = (uint64_t)_mm_cvtsi128_si64(s);
		uint64_t s1 = (uint64_t)_mm_cvtsi128_si64(_mm_unpackhi_epi64(s, s));
		r64 = s0 + s1;
	}
	{
		__m128i lo = _mm256_castsi256_si128(gAcc);
		__m128i hi = _mm256_extracti128_si256(gAcc, 1);
		__m128i s = _mm_add_epi64(lo, hi);
		uint64_t s0 = (uint64_t)_mm_cvtsi128_si64(s);
		uint64_t s1 = (uint64_t)_mm_cvtsi128_si64(_mm_unpackhi_epi64(s, s));
		g64 = s0 + s1;
	}
	{
		__m128i lo = _mm256_castsi256_si128(bAcc);
		__m128i hi = _mm256_extracti128_si256(bAcc, 1);
		__m128i s = _mm_add_epi64(lo, hi);
		uint64_t s0 = (uint64_t)_mm_cvtsi128_si64(s);
		uint64_t s1 = (uint64_t)_mm_cvtsi128_si64(_mm_unpackhi_epi64(s, s));
		b64 = s0 + s1;
	}

	*R += (int)r64;
	*G += (int)g64;
	*B += (int)b64;
}

void GetPictureSumAtLoc( LPCOLORREF Pixels, int Width, int Height, int Stride, int *R, int *G, int *B )
{
	*R = *G = *B = 0;

	if( Width * Height >= ( 1 << 16 ) )
	{
		FileDebug( "!Warning : values might overflow !" );
	}

	if ((Width % 8) == 0) {
		SumRGB_mxn_AVX2(Pixels, Width, Height, Stride, R, G, B);
	}
	else {
		assert(G != NULL && B != NULL);
		uint64_t ShiftedRSum = 0;
		uint64_t ShiftedGSum = 0;
		uint64_t ShiftedBSum = 0;
		for (uint64_t y = 0; y < Height; y += 1)
		{
			LPCOLORREF PixelsRow1 = &Pixels[y * Stride];
			for (uint64_t x = 0; x < Width; x += 1)
			{
				uint64_t Pixel1 = PixelsRow1[x];
				ShiftedRSum += (Pixel1 & 0x000000FF);
				ShiftedGSum += (Pixel1 & 0x0000FF00);
				ShiftedBSum += (Pixel1 & 0x00FF0000);
			}
		}/**/
		ShiftedRSum = ShiftedRSum;
		ShiftedGSum = ShiftedGSum >> 8;
		ShiftedBSum = ShiftedBSum >> 16;
		R[0] += (int)(ShiftedRSum);
		G[0] += (int)(ShiftedGSum);
		B[0] += (int)(ShiftedBSum);
	}
}

void SimilarSearch::BuildFromImg( LPCOLORREF Pixels, int pWidth, int pHeight, int pStride, bool bIsCache)
{
	if( R != NULL && ( pWidth != Width || pHeight != Height || SimilarSearchGroupingSizeX != BlockWidth || BlockHeight != SimilarSearchGroupingSizeY ) )
	{
		MY_FREE( R );
		MY_FREE( G );
		MY_FREE( B );
		R = G = B = NULL;
	}
	if( R == NULL )
	{
		FileDebug( "Started building Similar search cache" );

		Width = pWidth;
		Height = pHeight;
		Stride = pStride;

		BlockWidth = SimilarSearchGroupingSizeX;
		BlockHeight = SimilarSearchGroupingSizeY;
		R = (int*)MY_ALLOC( Width * Height * sizeof( int ) );
		G = (int*)MY_ALLOC( Width * Height * sizeof( int ) );
		B = (int*)MY_ALLOC( Width * Height * sizeof( int ) );

		// screenshot will have a 8x8 sum at every pixel location
		if (bIsCache == false) {
			for (int y = 0; y <= Height - BlockHeight; y++) {
				for (int x = 0; x <= Width - BlockWidth; x++) {
					GetPictureSumAtLoc(&Pixels[y * pStride + x], BlockWidth, BlockHeight, pStride, &R[y * Width + x], &G[y * Width + x], &B[y * Width + x]);
				}
			}
		}
		// cached images only need to have 8x8 sums at every block location
		else {
			for (int y = 0; y <= Height - BlockHeight; y+= BlockHeight) {
				for (int x = 0; x <= Width - BlockWidth; x+= BlockWidth) {
					GetPictureSumAtLoc(&Pixels[y * pStride + x], BlockWidth, BlockHeight, pStride, &R[y * Width + x], &G[y * Width + x], &B[y * Width + x]);
				}
			}
		}
		FileDebug( "\t Finished building Similar search cache" );
	}
}

uint64_t GetImageScoreAtLoc( SimilarSearch *SearchIn, SimilarSearch *SearchFor, size_t x0, size_t y0 )
{
	uint64_t diff = 0;
	for (size_t by = 0; by < SearchFor->Height; by += SearchFor->BlockHeight) {
		for (size_t bx = 0; bx < SearchFor->Width; bx += SearchFor->BlockWidth) {
			const size_t in_idx = (y0 + by) * (size_t)SearchIn->Width + (x0 + bx);
			const size_t for_idx = (by) * (size_t)SearchFor->Width + (bx);
			int64_t RDiff = SearchIn->R[in_idx] - SearchFor->R[for_idx];
			int64_t GDiff = SearchIn->G[in_idx] - SearchFor->G[for_idx];
			int64_t BDiff = SearchIn->B[in_idx] - SearchFor->B[for_idx];
//			diff += RDiff * RDiff + GDiff * GDiff + BDiff * BDiff; // SSD
			diff += abs(RDiff) + abs(GDiff) + abs(BDiff); // SAD non local
		}
	}
	return diff;
}

//this function could use multhi threaded search. Not sure if it would speed it up as bottleneck is probably Data starvation and not CPU
uint64_t GetNextBestMatch(ScreenshotStruct *ss , CachedPicture *pc, int &retx, int &rety, uint64_t &retSAD )
{
	SimilarSearch* SearchIn = ss->SSCache;
	SimilarSearch* SearchFor = pc->SSCache;
	uint64_t BestScore = MAXUINT64;
	uint64_t BestSAD = MAXUINT64;
	const uint64_t width_SAD = ( pc->Width / 8 ) * 8;
	const uint64_t height_SAD = pc->Height;
	retx = rety = -1;
	int SmallImg = SearchFor->R[0];
	for (size_t y = 0; y <= SearchIn->Height - SearchFor->Height; y++)
	{
		int* Row = &SearchIn->R[y * SearchIn->Width];
		for (size_t x = 0; x <= SearchIn->Width - SearchFor->Width; x++)
		{
			uint64_t ScoreHere = GetImageScoreAtLoc(SearchIn, SearchFor, x, y);
//			if (ScoreHere < BestScore)
			if (ScoreHere * 80 < BestScore * 100) // SS cache looses pixel locality. we will sad even values that might not be great
			{
				uint64_t sad = ImageSad(&ss->Pixels[y * ss->Width + x], ss->Width, pc->Pixels, pc->Width, width_SAD, height_SAD);
				if (sad < BestSAD) {
					BestSAD = sad;
					BestScore = ScoreHere;
					retx = (int)x;
					rety = (int)y;
				}
			}
		}
	}
	retSAD = BestSAD;
	return ( BestScore / SearchFor->Height / SearchFor->Width );
}

static char SSReturnBuff[DEFAULT_STR_BUFFER_SIZE*10];
char * WINAPI SearchSimilarOnScreenshot(const char *aImageFile )
{
//	int MatchesFound = 0;
	SSReturnBuff[0]=0;
	FileDebug( "Started Similar Image search" );

	CachedPicture *cache = CachePicture( aImageFile );
	if( cache == NULL )
	{
		FileDebug( "Skipping Image search as image could not be loaded" );
		return SSReturnBuff;
	}
	if( cache->Pixels == NULL )
	{
		FileDebug( "Skipping Image search as image pixels are missing" );
		return SSReturnBuff;
	}

	if(CurScreenshot == NULL || CurScreenshot->Pixels == NULL )
	{
		FileDebug( "Skipping Image search no screenshot is available" );
		return SSReturnBuff;
	}

	if( cache->SSCache == NULL )
		cache->SSCache = new SimilarSearch;
	if( CurScreenshot->SSCache == NULL )
		CurScreenshot->SSCache = new SimilarSearch;

	// get the smallest sized searched image and use it as "BlockSize"
	int minWidth, minHeight;
	GetMinWHCachedImages(minWidth, minHeight);
	minWidth = (minWidth / 8) * 8; // let's round it to be able to use SIMD
	bool bBlockSizeChanged = false;
	if (SimilarSearchGroupingSizeX != minWidth || SimilarSearchGroupingSizeY != minHeight) {
		bBlockSizeChanged = true;
		char dbgmsg[DEFAULT_STR_BUFFER_SIZE];
		sprintf_s(dbgmsg, sizeof(dbgmsg), "Block size changed from %d %d to %d %d", SimilarSearchGroupingSizeX, SimilarSearchGroupingSizeY, minWidth, minHeight);
		FileDebug(dbgmsg);
		SimilarSearchGroupingSizeX = (minWidth / 8) * 8;
		SimilarSearchGroupingSizeY = minHeight;
	}
	if( cache->NeedsSSCache == true || bBlockSizeChanged)
	{
		cache->SSCache->BlockWidth = 0;
		cache->SSCache->BuildFromImg( cache->Pixels, cache->Width, cache->Height, cache->Width, true );
		cache->NeedsSSCache = false;
	}
	if( CurScreenshot->NeedsSSCache == true || bBlockSizeChanged)
	{
		CurScreenshot->SSCache->BlockWidth = 0;
		CurScreenshot->SSCache->BuildFromImg( CurScreenshot->Pixels, CurScreenshot->Right - CurScreenshot->Left, CurScreenshot->Bottom - CurScreenshot->Top, CurScreenshot->Right - CurScreenshot->Left, false );
		CurScreenshot->NeedsSSCache = false;
	}

	if( cache->SSCache->BlockHeight != CurScreenshot->SSCache->BlockHeight || cache->SSCache->BlockWidth != CurScreenshot->SSCache->BlockWidth )
	{
		FileDebug( "Skipping Image search as block size does not match in cache. This is a bug" );
		return SSReturnBuff;
	}

	int tretx,trety;
	uint64_t pixelscore;
	uint64_t bestSAD;
	FileDebug( "\t Similar Image search is searching" );
	pixelscore = GetNextBestMatch( CurScreenshot, cache, tretx, trety, bestSAD);
	FileDebug( "\t Similar Image search done searching" );
	
	char debugbuff[500];
	sprintf_s( debugbuff, 500, "best pixel score for image %s is %llu at loc %d %d", cache->FileName, pixelscore, tretx, trety );
	FileDebug( debugbuff );

	//calculate absolute positioning
	tretx += CurScreenshot->Left;
	trety += CurScreenshot->Top;

	sprintf_s( SSReturnBuff, DEFAULT_STR_BUFFER_SIZE*10, "1|%d|%d|%llu|%llu",tretx,trety, bestSAD,pixelscore);
	FileDebug( "\tFinished Similar Image search" );
	return SSReturnBuff;
}
