#include "stdafx.h"

//#define ENABLE_CROSS_FUNCTIONALITY_CHECK

SplitChannel::SplitChannel()
{
#ifdef COMPARE_MULTI_REGION_READ_SPEED
	A = R = G = B = NULL;
#endif
#ifdef COMPARE_STREAM_READ_SPEED
	A2 = RGB = NULL;
#endif
}

SplitChannel::~SplitChannel()
{
#ifdef COMPARE_MULTI_REGION_READ_SPEED
	if (A)
	{
		_aligned_free(A);
		_aligned_free(R);
		_aligned_free(G);
		_aligned_free(B);
		A = R = G = B = NULL;
	}
#endif
#ifdef COMPARE_STREAM_READ_SPEED
	if (A2)
	{
		_aligned_free(A2);
		_aligned_free(RGB);
		A2 = RGB = NULL;
	}
#endif
}

#ifdef COMPARE_MULTI_REGION_READ_SPEED
// simply remove alpha channel and split it into 3 different buffers
void CopyFromRGBToSplitChannelScreenshot_(unsigned char *Src, unsigned char *R, unsigned char *G, unsigned char *B, unsigned int Width, unsigned int Height)
{
	if (Src == NULL || R == NULL || G == NULL || B == NULL)
		return;
	for (unsigned int y = 0; y < Height; y++)
	{
		for (unsigned int x = 0; x < Width * 4; x += 4)
		{
				R[x / 4] = Src[x + 0]; // R
				G[x / 4] = Src[x + 1]; // G
				B[x / 4] = Src[x + 2]; // B
		}
		//src to the next row
		Src += Width * 4;
		R += Width;
		G += Width;
		B += Width;
	}
}
// split the 4 channels into 4 buffers. If color is transparent, there is no need to copy the value. Width is rounded up to a multiple of 16 to be able to use SSE
void CopyFromRGBToSplitChannelCache_(unsigned char *Src, unsigned char *A, unsigned char *R, unsigned char *G, unsigned char *B, unsigned int Width, unsigned int Height)
{
	if (Src == NULL || A == NULL || R == NULL || G == NULL || B == NULL)
		return;
	unsigned int RoundupWidth = (Width + 15) / 16 * 16;
	memset(A, 0, RoundupWidth * Height);
	for (unsigned int y = 0; y < Height; y++)
	{
		for (unsigned int x = 0; x < Width * 4; x += 4)
		{
			if (((*(int*)&Src[x]) & REMOVE_ALPHA_CHANNEL_MASK) != TRANSPARENT_COLOR)
//				A[x / 4] = 0;
//			else
			{
				A[x / 4] = 0xFF;	// mask for this pixel
				R[x / 4] = Src[x + 0]; // R
				G[x / 4] = Src[x + 1]; // G
				B[x / 4] = Src[x + 2]; // B
			}
		}
		// round up width to be multiple of 16
//		for (unsigned int x = Width; x < RoundupWidth; x++)
//			A[x] = 0;
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
// simply remove alpha channel. From 4 bytes to 3 bytes conversion
void CopyFromRGBToSplitChannelScreenshot(unsigned char *Src, unsigned char *RGB, unsigned int Width, unsigned int Height)
{
	if (Src == NULL || RGB == NULL)
		return;
	// process all pixels
	for (unsigned int i = 0; i < Height * Width * 4; i += 4)
		*(int*)(RGB + i / 4 * 3) = (*(int*)(Src + i)) & REMOVE_ALPHA_CHANNEL_MASK; // RGB
}
// split alpha and RGB channels to 2 buffers. Alpa pixels are repeated 3 times One for R, one G, one for B to load them with SSE
void CopyFromRGBToSplitChannelCache(unsigned char *Src, unsigned char *A2, unsigned char *RGB, unsigned int Width, unsigned int Height)
{
	if (Src == NULL || A2 == NULL || RGB == NULL)
		return;
	// even if we overread the image by 16 bytes, we should have 0 SAD on it ( does not matter )
	unsigned int RoundupWidth = (Width * 3 + 14) / 15 * 16;
	memset(A2, 0, (RoundupWidth * Height + SSE_PADDING));
	// process all pixels
	for (unsigned int y = 0; y < Height; y++)
	{
		// 5 pixels as RGB than 1 empty byte to leave data alligned
		for (unsigned int x = 0; x < Width * 4; x += 4) //src 5 pixels / 4 bytes each = 20 bytes
			if ((*(int*)(Src + x) & REMOVE_ALPHA_CHANNEL_MASK) != TRANSPARENT_COLOR)
			{
				*(int*)(A2 + x / 4 * 3) = REMOVE_ALPHA_CHANNEL_MASK;
				*(int*)(RGB + x / 4 * 3) = (*(int*)(Src + x)) & REMOVE_ALPHA_CHANNEL_MASK; // RGB
			}
		//src to the next row
		Src += Width * 4;
		A2 += RoundupWidth;
		RGB += RoundupWidth;
	}
}
#endif
void SplitChannelSearchGenCache(ScreenshotStruct *Img)
{
	if (Img->NeedsSplitChannelCache == false)
		return;

	if (Img->SCCache != NULL)
		delete Img->SCCache;
	Img->SCCache = new SplitChannel;
	int Width = Img->GetWidth();
	int WidthRounded = (Width + 15) / 16 * 16;
	int PixelCount = Img->GetHeight() * WidthRounded;

#ifdef COMPARE_MULTI_REGION_READ_SPEED
	Img->SCCache->A = NULL;	// only smaller image Alpha channel is used
	Img->SCCache->R = (unsigned char*)_aligned_malloc(PixelCount + SSE_PADDING, SSE_ALIGNMENT);
	Img->SCCache->G = (unsigned char*)_aligned_malloc(PixelCount + SSE_PADDING, SSE_ALIGNMENT);
	Img->SCCache->B = (unsigned char*)_aligned_malloc(PixelCount + SSE_PADDING, SSE_ALIGNMENT);
	CopyFromRGBToSplitChannelScreenshot_((unsigned char *)Img->Pixels, Img->SCCache->R, Img->SCCache->G, Img->SCCache->B, Width, Img->GetHeight());
#endif
#ifdef COMPARE_STREAM_READ_SPEED
	Img->SCCache->A2 = NULL; // NULL
	Img->SCCache->RGB = (unsigned char*)_aligned_malloc( ( WidthRounded * Img->GetHeight() + SSE_PADDING ) * 4, SSE_ALIGNMENT);
	CopyFromRGBToSplitChannelScreenshot((unsigned char *)Img->Pixels, Img->SCCache->RGB, Width, Img->GetHeight());
#endif

	Img->NeedsSplitChannelCache = false;
}

void SplitChannelSearchGenCache(CachedPicture *Img)
{
	if (Img->NeedsSCCache == false)
		return;

	if (Img->SCCache != NULL)
		delete Img->SCCache;
	Img->SCCache = new SplitChannel;
	int Width = Img->Width;
	int WidthRounded = (Width + 15) / 16 * 16;
	int PixelCount = Img->Height * WidthRounded;
#ifdef COMPARE_MULTI_REGION_READ_SPEED
	Img->SCCache->A = (unsigned char*)_aligned_malloc(PixelCount + SSE_PADDING, SSE_ALIGNMENT);
	Img->SCCache->R = (unsigned char*)_aligned_malloc(PixelCount + SSE_PADDING, SSE_ALIGNMENT);
	Img->SCCache->G = (unsigned char*)_aligned_malloc(PixelCount + SSE_PADDING, SSE_ALIGNMENT);
	Img->SCCache->B = (unsigned char*)_aligned_malloc(PixelCount + SSE_PADDING, SSE_ALIGNMENT);
	CopyFromRGBToSplitChannelCache_((unsigned char *)Img->Pixels, Img->SCCache->A, Img->SCCache->R, Img->SCCache->G, Img->SCCache->B, Width, Img->Height);
#endif
#ifdef COMPARE_STREAM_READ_SPEED
	Img->SCCache->A2 = (unsigned char*)_aligned_malloc((WidthRounded * Img->Height + SSE_PADDING) * 4, SSE_ALIGNMENT);
	Img->SCCache->RGB = (unsigned char*)_aligned_malloc((WidthRounded * Img->Height + SSE_PADDING) * 4, SSE_ALIGNMENT);
	CopyFromRGBToSplitChannelCache((unsigned char *)Img->Pixels, Img->SCCache->A2, Img->SCCache->RGB, Width, Img->Height);
#endif

	Img->NeedsSCCache = false;
}

#ifdef COMPARE_MULTI_REGION_READ_SPEED
unsigned int GetSADAtPos_ref(unsigned char *R1, unsigned char *G1, unsigned char *B1, int Width, unsigned char *A2, unsigned char *R2, unsigned char *G2, unsigned char *B2, int CacheWidthRounded, int CacheHeight)
{
	unsigned int ret = 0;
	for (int y2 = 0; y2 < CacheHeight; y2++)
	{
		for (int x2 = 0; x2 < CacheWidthRounded; x2++)
		{
			if (A2[y2*CacheWidthRounded + x2] == 0xFF)
			{
				int SadR = abs((signed int)R1[y2*Width + x2] - (signed int)R2[y2*CacheWidthRounded + x2]);
				int SadG = abs((signed int)G1[y2*Width + x2] - (signed int)G2[y2*CacheWidthRounded + x2]);
				int SadB = abs((signed int)B1[y2*Width + x2] - (signed int)B2[y2*CacheWidthRounded + x2]);
				ret += SadR;
				ret += SadG;
				ret += SadB;
			}
		}
	}
	return ret;
}
unsigned int GetSADAtPos_ref2(unsigned char *Pixels1, int Width, unsigned char *Pixels2, int CacheWidth, int CacheHeight)
{
	unsigned int ret = 0;
	for (int y2 = 0; y2 < CacheHeight; y2++)
	{
		for (int x2 = 0; x2 < CacheWidth * 4; x2+=4)
		{
			if ((*(int*)(&Pixels2[y2 * CacheWidth * 4 + x2]) & REMOVE_ALPHA_CHANNEL_MASK) != TRANSPARENT_COLOR)
			{
				int R1 = (signed int)Pixels1[y2*Width * 4 + x2 + 0];
				int R2 = (signed int)Pixels2[y2*CacheWidth * 4 + x2 + 0];
				int SadR = abs(R1 - R2);
				int SadG = abs((signed int)Pixels1[y2*Width * 4 + x2 + 1] - (signed int)Pixels2[y2*CacheWidth * 4 + x2 + 1]);
				int SadB = abs((signed int)Pixels1[y2*Width * 4 + x2 + 2] - (signed int)Pixels2[y2*CacheWidth * 4 + x2 + 2]);
				ret += SadR;
				ret += SadG;
				ret += SadB;
			}
		}
		ret = ret;
	}
	return ret;
}
unsigned int GetSADAtPos(unsigned char *R1, unsigned char *G1, unsigned char *B1, int Width, unsigned char *A2, unsigned char *R2, unsigned char *G2, unsigned char *B2, int CacheWidthRounded, int CacheHeight)
{
	unsigned int sad_array[4];
	__m128i l0, l1, line_sad, acc_sad, alpha_mask;
	acc_sad = _mm_setzero_si128();

	for (int y2 = 0; y2<CacheHeight; y2++)
	{
		for (int x2 = 0; x2<CacheWidthRounded; x2 += 16)
		{
			// load the alpha mask
			alpha_mask = _mm_load_si128((__m128i*)&A2[x2]);

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
		A2 += CacheWidthRounded;
		R2 += CacheWidthRounded;
		G2 += CacheWidthRounded;
		B2 += CacheWidthRounded;
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
#if defined( ENABLE_CROSS_FUNCTIONALITY_CHECK )
			unsigned int CurSad_Ref = GetSADAtPos_ref2((unsigned char*)&CurScreenshot->Pixels[y * Width + x], Width, (unsigned char*)cache->Pixels, cache->Width, cache->Height);
			if (CurSad != CurSad_Ref)
			{
				printf("Cross check failed !!\n");
				unsigned int CurSad_Ref2 = GetSADAtPos_ref2((unsigned char*)&CurScreenshot->Pixels[y * Width + x], Width, (unsigned char*)cache->Pixels, cache->Width, cache->Height);
				unsigned int CurSad2 = GetSADAtPos((unsigned char*)&CurScreenshot->SCCache->R[y * Width + x],
					(unsigned char*)&CurScreenshot->SCCache->G[y * Width + x],
					(unsigned char*)&CurScreenshot->SCCache->B[y * Width + x],
					Width,
					(unsigned char*)cache->SCCache->A,
					(unsigned char*)cache->SCCache->R,
					(unsigned char*)cache->SCCache->G,
					(unsigned char*)cache->SCCache->B,
					CacheWidthRounded, cache->Height);
			}
#endif

			if (CurSad < BestSAD)
			{
				BestSAD = CurSad;
				retx = x + CurScreenshot->Left;
				rety = y + CurScreenshot->Top;
				//exact match ? I doubt it will ever happen...
				if (BestSAD == 0)
				{
					MatchesFound++;
#ifndef _CONSOLE
					goto docleanupandreturn;
#endif
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
	int Widthx3 = Width * 3;
	int CacheWidthx3 = cache->Width * 3;
	int CacheWidthRoundedx3 = (cache->Width * 3 + 14) / 15 * 16;
	// at every position on the current screenshot, get the SAD of the cache
	for (int y = 0; y < Height - cache->Height; y += 1)
	{
//		for (int x = 0; x < Width - cache->Width; x += 1)
		for (signed int x = Width - cache->Width - 1; x >= 0; x--)
			{
			unsigned int CurSad = GetSADAtPos2((unsigned char*)&CurScreenshot->SCCache->RGB[(y * Width + x) * 3], Widthx3, (unsigned char*)cache->SCCache->A2, (unsigned char*)cache->SCCache->RGB, CacheWidthx3, CacheWidthRoundedx3, cache->Height);
#if defined( ENABLE_CROSS_FUNCTIONALITY_CHECK )
			unsigned int CurSad_Ref = GetSADAtPos_ref2((unsigned char*)&CurScreenshot->Pixels[y * Width + x], Width, (unsigned char*)cache->Pixels, cache->Width, cache->Height);
			if (CurSad != CurSad_Ref)
			{
				printf("Cross check failed !!\n");
				unsigned int CurSad_Ref2 = GetSADAtPos_ref2((unsigned char*)&CurScreenshot->Pixels[y * Width + x], Width, (unsigned char*)cache->Pixels, cache->Width, cache->Height);
				unsigned int CurSad2 = GetSADAtPos2((unsigned char*)&CurScreenshot->SCCache->RGB[(y * Width + x) * 3], Width, (unsigned char*)cache->SCCache->A2, (unsigned char*)cache->SCCache->RGB, CacheWidthx3, CacheWidthRoundedx3, cache->Height);
			}
#endif
			if (CurSad < BestSAD)
			{
				BestSAD = CurSad;
				retx = x + CurScreenshot->Left;
				rety = y + CurScreenshot->Top;
				//exact match ? I doubt it will ever happen...
				if (BestSAD == 0)
				{
					MatchesFound++;
#ifndef _CONSOLE
					goto docleanupandreturn;
#endif
				}
			}
		}
	}
//docleanupandreturn:
	if (MatchesFound == 0)
		FileDebug("\t Image search found no matches");

	sprintf_s(ReturnBuff, DEFAULT_STR_BUFFER_SIZE * 10, "1|%d|%d|%d", retx, rety, BestSAD);
	FileDebug("\tImage search finished");
	return ReturnBuff;
}
#endif