#include "stdAfx.h"

void GradientRemove(LPCOLORREF Pixels, int Width, int Height)
{
	std::map<DWORD, DWORD> ColorMap;
	for (int y = 0; y < Height; y++)
	{
        DWORD *BaseSrc = (DWORD*)&Pixels[y*Width];
		for (int x = 0; x < Width; x++)
		{
			DWORD Pixel = BaseSrc[x];
			unsigned char *SP = (unsigned char*)&Pixel;
			unsigned char MinVal = 255;
			if (SP[0] < MinVal)
				MinVal = SP[0];
			if (SP[1] < MinVal)
				MinVal = SP[1];
			if (SP[2] < MinVal)
				MinVal = SP[2];
			//reduce the values of each channel to base luminosity. One channel is always 0
			SP[0] = SP[0] - MinVal;
			SP[1] = SP[1] - MinVal;
			SP[2] = SP[2] - MinVal;

			auto itr = ColorMap.find(Pixel);
			if (itr != ColorMap.end())
			{
				DWORD MinNow = itr->second;
				if (BaseSrc[x] < MinNow)
					itr->second = BaseSrc[x];
			}
			else
				ColorMap[Pixel] = BaseSrc[x];
		}
	}
	for (int y = 0; y < Height; y++)
	{
		int *BaseSrc = (int*)&Pixels[y*Width];
		for (int x = 0; x < Width; x++)
		{
			DWORD Pixel = BaseSrc[x];
			unsigned char *SP = (unsigned char*)&Pixel;
			unsigned char MinVal = 255;
			if (SP[0] < MinVal)
				MinVal = SP[0];
			if (SP[1] < MinVal)
				MinVal = SP[1];
			if (SP[2] < MinVal)
				MinVal = SP[2];
			//reduce the values of each channel to base luminosity. One channel is always 0
			SP[0] = SP[0] - MinVal;
			SP[1] = SP[1] - MinVal;
			SP[2] = SP[2] - MinVal;
			BaseSrc[x] = ColorMap[Pixel];
		}
	}
}

void WINAPI GradientRemove()
{
	if (CurScreenshot == NULL)
		return;
	GradientRemove(CurScreenshot->Pixels, CurScreenshot->GetWidth(), CurScreenshot->GetHeight());
}

void WINAPI GradientRemoveCache(char *aFileName)
{
	CachedPicture *cache = CachePicturePrintErrors("1.bmp", __FUNCTION__);
	if (cache == NULL)
		return;
	GradientRemove(cache->Pixels, cache->Width, cache->Height);
}

void WINAPI GradientReduceCache(char *aFileName, int GradientCount)
{
    CachedPicture *cache = CachePicturePrintErrors(aFileName, __FUNCTION__);
    if (cache == NULL)
        return;
    GradientReduce(cache->Pixels, cache->Width, cache->Height, GradientCount);
}

void GradientReduce(LPCOLORREF Pixels, int Width, int Height, int GradientCount)
{
	int GradientStep = 255 / GradientCount;
	for (int y = 0; y < Height; y++)
	{
		int *BaseSrc = (int*)&Pixels[y*Width];
		for (int x = 0; x < Width; x++)
		{
			unsigned char *SP = (unsigned char*)&BaseSrc[x];
			int Avg = (SP[0] + SP[1] + SP[2]) / 3;
			int SnapToGrid = (Avg + GradientStep) / GradientStep * GradientStep;
			int Dif = SnapToGrid - Avg;
			for (int i = 0; i < 3; i++)
			{
				int NewVal = SP[i] + Dif;
//				if (NewVal < 0)
//					NewVal = 0;
				if (NewVal > 255)
					NewVal = 255;
				SP[i] = NewVal;
			}
		}
	}
}