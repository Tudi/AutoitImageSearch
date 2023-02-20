#pragma once

typedef struct SADSumStoreScreenshot
{
	LPCOLORREF sumSAD16x16[3];
	LPCOLORREF sumSAD32x32[3];
	LPCOLORREF sumSAD64x64[3];
}SADSumStoreScreenshot;

// one of the 16x16 or 32x32 or 64x64
typedef struct SADSumStoreCached
{
	COLORREF sumSAD[3];
	int widt_height;
}SADSumStoreCached;

void InitSADSUMScreenshot(SADSumStoreScreenshot* sss);
void FreeSADSUMScreenshot(SADSumStoreScreenshot* sss);
void ComputeSADSumScreenshot(LPCOLORREF pixels, int in_width, int in_height, SADSumStoreScreenshot *out_sumsads);

void InitSADSUMCached(SADSumStoreCached* sss);
void FreeSADSUMCached(SADSumStoreCached* sss);
void ComputeSADMapCached(LPCOLORREF pixels, int in_width, int in_height, SADSumStoreCached* out_sumsads);

char* WINAPI ImageSearch_SAD_Limit(char* aImageFile, int SAD_Limit);