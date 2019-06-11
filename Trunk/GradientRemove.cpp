#include "stdAfx.h"


void GradientRemove(LPCOLORREF Pixels, int Width, int Height)
{
	for (int y = 0; y < Height; y++)
	{
		int *BaseSrc = (int*)&Pixels[y*Width];
		for (int x = 0; x < Width; x++)
		{
			unsigned char *SP = (unsigned char*)&BaseSrc[x];
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
	CachedPicture *cache = CachePicture(aFileName);
	if (cache == NULL)
	{
		FileDebug("GradientRemoveCache : image could not be loaded");
		return;
	}
	if (cache->Pixels == NULL)
	{
		FileDebug("GradientRemoveCache : pixels are missing");
		return;
	}
	if (cache->LoadedPicture == NULL)
	{
		FileDebug("GradientRemoveCache : image is missing");
		return;
	}
	GradientRemove(cache->Pixels, cache->Width, cache->Height);
}