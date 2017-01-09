#include "stdafx.h"

SplitChannel::SplitChannel()
{
	A = R = G = B = NULL;
#ifdef COMPARE_STREAM_READ_SPEED
	A2 = RGB = NULL;
#endif
}

SplitChannel::~SplitChannel()
{
	if (A)
	{
		_aligned_free(A);
		_aligned_free(R);
		_aligned_free(G);
		_aligned_free(B);
		A = R = G = B = NULL;
	}
	if (A2)
	{
		_aligned_free(A2);
		_aligned_free(RGB);
		A2 = RGB = NULL;
	}
}

#ifdef COMPARE_MULTI_REGION_READ_SPEED
void CopyFromRGBToSplitChannel(unsigned char *Src, unsigned char *A, unsigned char *R, unsigned char *G, unsigned char *B, unsigned int Width, unsigned int Height)
{
	if (Src == NULL || A == NULL || R == NULL || G == NULL || B == NULL)
		return;
	unsigned int RoundupWidth = (Width + 15) / 16 * 16;
	for (unsigned int y = 0; y < Height; y++)
	{
		for (unsigned int x = 0; x < Width; x += 4)
		{
			if (((*(int*)&Src[x]) & REMOVE_ALPHA_CHANNEL_MASK) == TRANSPARENT_COLOR)
				A[x / 4] = 0;
			else
			{
				A[x / 4] = 0xFF;	// mask for this pixel
				R[x / 4] = Src[x + 1]; // R
				G[x / 4] = Src[x + 2]; // G
				B[x / 4] = Src[x + 3]; // B
			}
		}
		// round up width to be multiple of 16
		for (unsigned int x = Width; x < RoundupWidth; x++)
			A[x] = 0;
		//src to the next row
		Src += Width * 4;
		A += RoundupWidth;
		R += RoundupWidth;
		G += RoundupWidth;
		B += RoundupWidth;
	}
}
#endif
#ifdef COMPARE_STREAM_READ_SPEED
void CopyFromRGBToSplitChannel2(unsigned char *Src, unsigned char *A2, unsigned char *RGB, unsigned int Width, unsigned int Height)
{
	if (Src == NULL || A2 == NULL || RGB == NULL)
		return;
	// even if we overread the image by 16 bytes, we should have 0 SAD on it ( does not matter )
	unsigned int RoundupWidth = (Width + 15) / 16 * 16;
	memset(A2, 0, ( RoundupWidth * Height + SSE_PADDING ) * 3 );
	// process all pixels
	for (unsigned int y = 0; y < Height; y++)
	{
		// 5 pixels as RGB than 1 empty byte to leave data alligned
		for (unsigned int x = 0; x < Width; x += 4)
			if (((*(int*)&Src[x]) & REMOVE_ALPHA_CHANNEL_MASK) != TRANSPARENT_COLOR)
			{
				*(int*)&A2[x / 4 * 3] = 0x00FFFFFF;
				*(int*)&RGB[x / 4 * 3] = (*(int*)&Src[x]) & REMOVE_ALPHA_CHANNEL_MASK; // RGB
			}
		//src to the next row
		Src += Width * 4;
		A2 += RoundupWidth * 3;
		RGB += RoundupWidth * 3;
	}
}
#endif
void SplitChannelSearchGenCache(ScreenshotStruct *Img)
{
	if (Img->NeedsSplitChannelCache == false)
		return;

	if (Img->SCCache == NULL)
		Img->SCCache = new SplitChannel;
	int Width = Img->GetWidth();
	int WidthRounded = (Width + 15) / 16 * 16;
	int PixelCount = Img->GetHeight() * WidthRounded;

#ifdef COMPARE_MULTI_REGION_READ_SPEED
	Img->SCCache->A = NULL;	// only smaller image Alpha channel is used
	Img->SCCache->R = (unsigned char*)_aligned_malloc(PixelCount + SSE_PADDING, SSE_ALIGNMENT);
	Img->SCCache->G = (unsigned char*)_aligned_malloc(PixelCount + SSE_PADDING, SSE_ALIGNMENT);
	Img->SCCache->B = (unsigned char*)_aligned_malloc(PixelCount + SSE_PADDING, SSE_ALIGNMENT);
	CopyFromRGBToSplitChannel((unsigned char *)Img->Pixels, Img->SCCache->A, Img->SCCache->R, Img->SCCache->G, Img->SCCache->B, Width, Img->GetHeight());
#endif
#ifdef COMPARE_STREAM_READ_SPEED
	Img->SCCache->A2 = NULL; // NULL
	Img->SCCache->RGB = (unsigned char*)_aligned_malloc( ( WidthRounded * Img->GetHeight() + SSE_PADDING ) * 3, SSE_ALIGNMENT);
	CopyFromRGBToSplitChannel2((unsigned char *)Img->Pixels, Img->SCCache->A2, Img->SCCache->RGB, Width, Img->GetHeight());
#endif

	Img->NeedsSplitChannelCache = false;
}

void SplitChannelSearchGenCache(CachedPicture *Img)
{
	if (Img->NeedsSCCache == false)
		return;

	if (Img->SCCache == NULL)
		Img->SCCache = new SplitChannel;
	int Width = Img->Width;
	int WidthRounded = (Width + 15) / 16 * 16;
	int PixelCount = Img->Height * WidthRounded;
#ifdef COMPARE_MULTI_REGION_READ_SPEED
	Img->SCCache->A = (unsigned char*)_aligned_malloc(PixelCount + SSE_PADDING, SSE_ALIGNMENT);
	Img->SCCache->R = (unsigned char*)_aligned_malloc(PixelCount + SSE_PADDING, SSE_ALIGNMENT);
	Img->SCCache->G = (unsigned char*)_aligned_malloc(PixelCount + SSE_PADDING, SSE_ALIGNMENT);
	Img->SCCache->B = (unsigned char*)_aligned_malloc(PixelCount + SSE_PADDING, SSE_ALIGNMENT);
	CopyFromRGBToSplitChannel((unsigned char *)Img->Pixels, Img->SCCache->A, Img->SCCache->R, Img->SCCache->G, Img->SCCache->B, Width, Img->Height);
#endif
#ifdef COMPARE_STREAM_READ_SPEED
	Img->SCCache->A2 = (unsigned char*)_aligned_malloc((WidthRounded * Img->Height + SSE_PADDING) * 3, SSE_ALIGNMENT);
	Img->SCCache->RGB = (unsigned char*)_aligned_malloc((WidthRounded * Img->Height + SSE_PADDING) * 3, SSE_ALIGNMENT);
	CopyFromRGBToSplitChannel2((unsigned char *)Img->Pixels, Img->SCCache->A2, Img->SCCache->RGB, Width, Img->Height);
#endif

	Img->NeedsSCCache = false;
}

#ifdef COMPARE_MULTI_REGION_READ_SPEED
unsigned int GetSADAtPos(unsigned char *R1, unsigned char *G1, unsigned char *B1, int Width, unsigned char *A2, unsigned char *R2, unsigned char *G2, unsigned char *B2, int CacheWidth, int CacheHeight)
{
	unsigned int sad_array[4];
	__m128i l0, l1, line_sad, acc_sad, alpha_mask;
	acc_sad = _mm_setzero_si128();

	for (int y2 = 0; y2<CacheHeight; y2++)
	{
		for (int x2 = 0; x2<CacheWidth; x2 += 16)
		{
			// load the alpha mask
			alpha_mask = _mm_loadu_si128((__m128i*)&A2[x2]);

			// load 16 bytes of R values
			l0 = _mm_loadu_si128((__m128i*)&R1[x2]);
			l1 = _mm_load_si128((__m128i*)&R2[x2]);	// small image is always SSE aligned
			// apply mask to not count transparent values
			l0 = _mm_and_si128(l0, alpha_mask);
			l1 = _mm_and_si128(l1, alpha_mask);
			// sad R values
			line_sad = _mm_sad_epu8(l0, l1);
			// add to acc
			acc_sad = _mm_add_epi32(acc_sad, line_sad);

			// load 16 bytes of G values
			l0 = _mm_loadu_si128((__m128i*)&G1[x2]);
			l1 = _mm_load_si128((__m128i*)&G2[x2]); // small image is always SSE aligned
			// apply mask to not count transparent values
			l0 = _mm_and_si128(l0, alpha_mask);
			l1 = _mm_and_si128(l1, alpha_mask);
			// sad R values
			line_sad = _mm_sad_epu8(l0, l1);
			// add to acc
			acc_sad = _mm_add_epi32(acc_sad, line_sad);

			// load 16 bytes of B values
			l0 = _mm_loadu_si128((__m128i*)&B1[x2]);
			l1 = _mm_load_si128((__m128i*)&B2[x2]); // small image is always SSE aligned
			// apply mask to not count transparent values
			l0 = _mm_and_si128(l0, alpha_mask);
			l1 = _mm_and_si128(l1, alpha_mask);
			// sad R values
			line_sad = _mm_sad_epu8(l0, l1);
			// add to acc
			acc_sad = _mm_add_epi32(acc_sad, line_sad);
		}

		R1 += Width;
		G1 += Width;
		B1 += Width;
		A2 += CacheWidth;
		R2 += CacheWidth;
		G2 += CacheWidth;
		B2 += CacheWidth;
	}

	_mm_storeu_si128((__m128i*)(&sad_array[0]), acc_sad);

	unsigned int sad = sad_array[0] + sad_array[2];
	return sad;
}

char* WINAPI ImageSearchOnScreenshotBest_Transparent_SAD(char *aFilespec)
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

	//make sure we have the split image
	SplitChannelSearchGenCache(cache);
	SplitChannelSearchGenCache(CurScreenshot);

	int Width = CurScreenshot->Right - CurScreenshot->Left;
	int Height = CurScreenshot->Bottom - CurScreenshot->Top;

	int retx = -1;
	int rety = -1;
	unsigned int BestSAD = 0x7FFFFFFF;
	int CacheWidthRounded = ( cache->Width + 15 ) / 16 * 16;
	// at every position on the current screenshot, get the SAD of the cache
	for (int y = 0; y < Height - cache->Height; y += 1)
	{
		for (int x = 0; x < Width - cache->Width; x += 1)
		{
			unsigned int CurSad = GetSADAtPos((unsigned char*)&CurScreenshot->SCCache->R[y * Width + x], 
				(unsigned char*)&CurScreenshot->SCCache->G[y * Width + x],
				(unsigned char*)&CurScreenshot->SCCache->B[y * Width + x],
				Width, 
				(unsigned char*)cache->SCCache->A,
				(unsigned char*)cache->SCCache->R,
				(unsigned char*)cache->SCCache->G,
				(unsigned char*)cache->SCCache->B,
				CacheWidthRounded, cache->Height);

			if (CurSad < BestSAD)
			{
				BestSAD = CurSad;
				retx = x + CurScreenshot->Left;
				rety = y + CurScreenshot->Top;
				//exact match ? I doubt it will ever happen...
				if (BestSAD == 0)
				{
					MatchesFound++;
					goto docleanupandreturn;
				}
			}
		}
	}
docleanupandreturn:
	if (MatchesFound == 0)
		FileDebug("\t Image search found no matches");

	sprintf_s(ReturnBuff, DEFAULT_STR_BUFFER_SIZE * 10, "1|%d|%d|%d", retx, rety, BestSAD);
	FileDebug("\tImage search finished");
	return ReturnBuff;
}
#endif
#ifdef COMPARE_STREAM_READ_SPEED
unsigned int GetSADAtPos2(unsigned char *RGB1, int Width, unsigned char *A2, unsigned char *RGB2, int CacheWidthRoundedx3, int CacheHeight)
{
	unsigned int sad_array[4];
	__m128i l0, l1, line_sad, acc_sad, alpha_mask;
	acc_sad = _mm_setzero_si128();

	for (int y2 = 0; y2<CacheHeight; y2++)
	{
		for (int x2 = 0; x2<CacheWidthRoundedx3; x2 += 15) // 5 pixels at a time
		{
			// load the alpha mask
			alpha_mask = _mm_loadu_si128((__m128i*)&A2[x2]);

			// load 16 bytes of RGB values
			l0 = _mm_loadu_si128((__m128i*)&RGB1[x2]);
			l1 = _mm_loadu_si128((__m128i*)&RGB2[x2]); // we could skip 1 byte to make an alligned read ?
			// apply mask to not count transparent values
			l0 = _mm_and_si128(l0, alpha_mask);
			l1 = _mm_and_si128(l1, alpha_mask);
			// sad RGB values
			line_sad = _mm_sad_epu8(l0, l1);
			// add to acc
			acc_sad = _mm_add_epi32(acc_sad, line_sad);
		}

		RGB1 += Width * 3;
		A2 += CacheWidthRoundedx3;
		RGB2 += CacheWidthRoundedx3;
	}

	_mm_storeu_si128((__m128i*)(&sad_array[0]), acc_sad);

	unsigned int sad = sad_array[0] + sad_array[2];
	return sad;
}

char* WINAPI ImageSearchOnScreenshotBest_Transparent_SAD2(char *aFilespec)
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
	//make sure we have the split image
	SplitChannelSearchGenCache(cache);
	SplitChannelSearchGenCache(CurScreenshot);

	int Width = CurScreenshot->Right - CurScreenshot->Left;
	int Height = CurScreenshot->Bottom - CurScreenshot->Top;

	int retx = -1;
	int rety = -1;
	unsigned int BestSAD = 0x7FFFFFFF;
	int CacheWidthRoundedx3 = (cache->Width + 15) / 16 * 16 * 3;
	// at every position on the current screenshot, get the SAD of the cache
	for (int y = 0; y < Height - cache->Height; y += 1)
	{
		for (int x = 0; x < Width - cache->Width; x += 1)
		{
			unsigned int CurSad = GetSADAtPos2((unsigned char*)&CurScreenshot->SCCache->RGB[(y * Width + x) * 3], Width, (unsigned char*)cache->SCCache->A2, (unsigned char*)cache->SCCache->RGB, CacheWidthRoundedx3, cache->Height);

			if (CurSad < BestSAD)
			{
				BestSAD = CurSad;
				retx = x + CurScreenshot->Left;
				rety = y + CurScreenshot->Top;
				//exact match ? I doubt it will ever happen...
				if (BestSAD == 0)
				{
					MatchesFound++;
					goto docleanupandreturn;
				}
			}
		}
	}
docleanupandreturn:
	if (MatchesFound == 0)
		FileDebug("\t Image search found no matches");

	sprintf_s(ReturnBuff, DEFAULT_STR_BUFFER_SIZE * 10, "1|%d|%d|%d", retx, rety, BestSAD);
	FileDebug("\tImage search finished");
	return ReturnBuff;
}
#endif