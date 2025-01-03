#pragma once

typedef struct SADSumStoreScreenshot
{
	LPCOLORREF SADSum16x16[3]; // only generated on request
	LPCOLORREF SADSum32x32[3]; // only generated on request
	LPCOLORREF SADSum64x64[3]; // only generated on request
	LPCOLORREF SADSum128x128[3]; // only generated on request
}SADSumStoreScreenshot;

// one of the 16x16 or 32x32 or 64x64
typedef struct SADSumStoreCached
{
	COLORREF SADSum16x16[3][8][8]; // always available
	COLORREF SADSum[3];
	int width_height;
}SADSumStoreCached;

typedef enum SADSumAvailableSizes
{
	SSAS_16x16 = 16,
	SSAS_32x32 = 32,
	SSAS_64x64 = 64,
	SSAS_128x128 = 128,
}SADSumAvailableSizes;

void InitSADSUMScreenshot(SADSumStoreScreenshot* sss);
void FreeSADSUMScreenshot(SADSumStoreScreenshot* sss);
void ComputeSADSumScreenshot(LPCOLORREF pixels, int in_width, int in_height, SADSumStoreScreenshot *out_SADSums, SADSumAvailableSizes requiredSize);

// sum up R,G,B values in up to a 128x128 square. Used to make a quick prematch before doing a full SAD check
// functions used for cached images
void InitSADSUMCached(SADSumStoreCached* sss);
void FreeSADSUMCached(SADSumStoreCached* sss);
void ComputeSADSumCached(LPCOLORREF pixels, int in_width, int in_height, SADSumStoreCached* out_SADSums);

/// <summary> Search for a small image in the screenshot. Matching algorithm is mostly SAD based, but some differences have exponential impact instead liniar </summary>
/// <param name="aImageFile"></param>
/// <param name="differencePCT">0 means the image needs to be a perfect match. 100% means it will always find a match even if it's 100% different</param>
/// <returns></returns>
char* WINAPI ImageSearch_Similar(char* aImageFile, float differencePCT);