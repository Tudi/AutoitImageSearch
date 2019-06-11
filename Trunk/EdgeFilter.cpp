#include "stdafx.h"
#include "EdgeFilter.h"
#include <list>

std::list<EdgeFilterPixels*> LO_Stores;
EdgeFilterPixels *LO_Active = NULL;

void EdgeFilter_Init(int EdgeFilterPixelsIndex)
{
	LO_Active = new EdgeFilterPixels();
	LO_Stores.push_front(LO_Active);
	LO_Active->MyIndex = EdgeFilterPixelsIndex;
}

void EdgeFilter_LearnPixel(EdgeFilterPixels *LO_Active, LPCOLORREF p1, LPCOLORREF p2)
{
	//add this to the statistics
	unsigned _int64 Key1 = ((unsigned _int64)p1 << 32) | ((unsigned _int64)p2 << 0);
	//increase normal oriented version
	auto itr = LO_Active->EdgePixelStatistics.find(Key1);
	if (itr != LO_Active->EdgePixelStatistics.end())
	{
		LO_Active->EdgePixelStatistics[Key1]++;
		return;
	}
	//increase flipped version
	unsigned _int64 Key2 = ((unsigned _int64)p2 << 32) | ((unsigned _int64)p1 << 0);
	itr = LO_Active->EdgePixelStatistics.find(Key2);
	if (itr != LO_Active->EdgePixelStatistics.end())
	{
		LO_Active->EdgePixelStatistics[Key2]++;
		return;
	}
	//create new
	LO_Active->EdgePixelStatistics[Key1]++;
}

//from temporary to active
void MergeStatistics(EdgeFilterPixels *Ori, EdgeFilterPixels *Temp)
{
	//only keep the ones that are common. number of matches should be the avg of the 2 values
	for (auto itr = Ori->EdgePixelStatistics.begin(); itr != Ori->EdgePixelStatistics.end(); )
	{
		auto itrRemove = itr;
		itr++;
		unsigned _int64 Key = itr->first;
		unsigned int p1 = Key & 0xFFFFFFFF;
		unsigned int p2 = Key >> 32;
		//do not remove common values
		unsigned _int64 Key1 = ((unsigned _int64)p1 << 32) | ((unsigned _int64)p2 << 0);
		auto itr2 = Temp->EdgePixelStatistics.find(Key1);
		if (itr2 != Temp->EdgePixelStatistics.end())
			continue;
		//do not remove common values
		unsigned _int64 Key2 = ((unsigned _int64)p2 << 32) | ((unsigned _int64)p1 << 0);
		itr = Temp->EdgePixelStatistics.find(Key2);
		if (itr2 != Temp->EdgePixelStatistics.end())
			continue;
		//remove total pixels stored for this edge filter
		Ori->PixelsAdded -= itrRemove->second;
		//remove unique values from original picture. We are looking for common values
		Ori->EdgePixelStatistics.erase(itrRemove);
	}
}

void EdgeFilter_LearnRefine(char *aFileName)
{
	if (LO_Active == NULL)
	{
		FileDebug("EdgeFilter_Learn : Not initialized");
	}

	CachedPicture *cache = CachePicture(aFileName);
	if (cache == NULL)
	{
		FileDebug("EdgeFilter_Learn : image could not be loaded");
		return;
	}
	if (cache->Pixels == NULL)
	{
		FileDebug("EdgeFilter_Learn : pixels are missing");
		return;
	}
	if (cache->LoadedPicture == NULL)
	{
		FileDebug("EdgeFilter_Learn : image is missing");
		return;
	}

	//reduce color count to half
	DWORD Mask = 0x00FEFEFE;
	int PixelCount = cache->Width * cache->Height;
	for (int i = 0; i < PixelCount; i++)
		cache->Pixels[i] = cache->Pixels[i] & Mask;

	//remove gradient from the image
	GradientRemoveCache(aFileName);

	EdgeFilterPixels *LO_Used = NULL;
	if (LO_Active->NumberOfImagesLoaded == 0)
		LO_Used = LO_Active;
	else
		LO_Used = new EdgeFilterPixels();

	//detect all edge ( and non edge ) pixels and create a statistics for their count
	for (int y = 1; y < cache->Height; y++)
		for (int x = 1; x < cache->Width; x++)
		{
			//for object to be rotation invariant, we will consider all statistics
			EdgeFilter_LearnPixel(LO_Used, &cache->Pixels[y * cache->Width + x], &cache->Pixels[(y - 1) * cache->Width + (x - 1)]);
			EdgeFilter_LearnPixel(LO_Used, &cache->Pixels[y * cache->Width + x], &cache->Pixels[(y - 1) * cache->Width + (x - 0)]);
			EdgeFilter_LearnPixel(LO_Used, &cache->Pixels[y * cache->Width + x], &cache->Pixels[(y - 1) * cache->Width + (x + 1)]);
			EdgeFilter_LearnPixel(LO_Used, &cache->Pixels[y * cache->Width + x], &cache->Pixels[(y - 0) * cache->Width + (x - 1)]);
		}

	//number of pixels we added to statistcs
	int ValuesAdded = (cache->Height - 1) * (cache->Width - 1);
	LO_Active->PixelsAdded += ValuesAdded * 4;

	if (LO_Active->NumberOfImagesLoaded != 0)
	{
		MergeStatistics(LO_Active, LO_Used);
		delete LO_Used;
	}

	LO_Active->NumberOfImagesLoaded++;
}

unsigned int EdgeFilter_GetSeenCount(EdgeFilterPixels *LO_Active, LPCOLORREF p1, LPCOLORREF p2)
{
	//ever seen this pixel combo ?
	unsigned _int64 Key1 = ((unsigned _int64)p1 << 32) | ((unsigned _int64)p2 << 0);
	auto itr = LO_Active->EdgePixelStatistics.find(Key1);
	if (itr != LO_Active->EdgePixelStatistics.end())
		return LO_Active->EdgePixelStatistics[Key1];
	//ever seen the flipped version of it ?
	unsigned _int64 Key2 = ((unsigned _int64)p2 << 32) | ((unsigned _int64)p1 << 0);
	itr = LO_Active->EdgePixelStatistics.find(Key2);
	if (itr != LO_Active->EdgePixelStatistics.end())
		return LO_Active->EdgePixelStatistics[Key2];
	//have not seen this pixel combo until now
	return 0;
}

void EdgeFilter_MarkKnown(char *aFileName)
{
	if (LO_Active == NULL)
	{
		FileDebug("EdgeFilter_Learn : Not initialized");
	}

	CachedPicture *cache = CachePicture(aFileName);
	if (cache == NULL)
	{
		FileDebug("EdgeFilter_Learn : image could not be loaded");
		return;
	}
	if (cache->Pixels == NULL)
	{
		FileDebug("EdgeFilter_Learn : pixels are missing");
		return;
	}
	if (cache->LoadedPicture == NULL)
	{
		FileDebug("EdgeFilter_Learn : image is missing");
		return;
	}
	//detect all edge ( and non edge ) pixels and create a statistics for their count
	for (int y = 1; y < cache->Height; y++)
		for (int x = 1; x < cache->Width; x++)
		{
			int SeenCount, MaxSeenCount = 0;
			//for object to be rotation invariant, we will consider all statistics
			SeenCount = EdgeFilter_GetSeenCount(LO_Active, &cache->Pixels[y * cache->Width + x], &cache->Pixels[(y - 1) * cache->Width + (x - 1)]);
			if (SeenCount > MaxSeenCount)
				MaxSeenCount = SeenCount;
			SeenCount = EdgeFilter_GetSeenCount(LO_Active, &cache->Pixels[y * cache->Width + x], &cache->Pixels[(y - 1) * cache->Width + (x - 0)]);
			if (SeenCount > MaxSeenCount)
				MaxSeenCount = SeenCount;
			SeenCount = EdgeFilter_GetSeenCount(LO_Active, &cache->Pixels[y * cache->Width + x], &cache->Pixels[(y - 1) * cache->Width + (x + 1)]);
			if (SeenCount > MaxSeenCount)
				MaxSeenCount = SeenCount;
			SeenCount = EdgeFilter_GetSeenCount(LO_Active, &cache->Pixels[y * cache->Width + x], &cache->Pixels[(y - 0) * cache->Width + (x - 1)]);
			if (SeenCount > MaxSeenCount)
				MaxSeenCount = SeenCount;
			//unique features
			float UniquePCT = 1 - MaxSeenCount * 4 / LO_Active->PixelsAdded;
		}
}