#include "stdafx.h"

ProjectionDistanceMap PDM;

void ProjectionDistanceMap::DestroyAllocatedMemmory()
{
	if (NonProjectedImage != NULL)
	{
		_aligned_free(NonProjectedImage);
		NonProjectedImage = NULL;
	}

	for (auto itr = DistanceMaps.begin(); itr != DistanceMaps.end(); itr++)
		_aligned_free((*itr));
	DistanceMaps.clear();

	for (auto itr = DiffMaps.begin(); itr != DiffMaps.end(); itr++)
		_aligned_free((*itr));
	DiffMaps.clear();

	if (MergedDistanceMaps != NULL)
	{
		_aligned_free(MergedDistanceMaps);
		MergedDistanceMaps = NULL;
	}
}

void ProjectionDistanceMap::Init(int pWidth, int pHeight, LPCOLORREF Pixels)
{
//	if (pWidth != Width || pHeight != Height)
		DestroyAllocatedMemmory();
	pWidth = PDM.Width;
	pHeight = PDM.Height;
	NonProjectedImage = (LPCOLORREF)MY_ALLOC((pWidth * pHeight * sizeof(COLORREF)));
	memcpy(NonProjectedImage, Pixels, pWidth * pHeight * sizeof(COLORREF));
}

void ProjectionDistanceMap::AddProjectedImage(LPCOLORREF Pixels)
{
	int DiffmapSize = Width / 4 * Height / 4 * sizeof(unsigned char);
	unsigned char *NewDiffMap = (unsigned char *)MY_ALLOC(DiffmapSize);
	GenerateDiffMap(NonProjectedImage, Pixels, Width, Height, NewDiffMap);

	//check if nothing changed on the image. there is no point to store the diffmap if it the same as the previous one
	if (DiffMaps.begin() != DiffMaps.end())
	{
		unsigned char *PrevDiffMap = *(DiffMaps.begin());
		if (memcmp(NewDiffMap, PrevDiffMap, DiffmapSize) == 0)
		{
			_aligned_free(NewDiffMap);
			return;
		}
	}
	else
	{
		DiffMaps.push_front(NewDiffMap);
	}
}

void ProjectionDistanceMap::ProcessAllDiffMaps()
{
	int DiffMapHeight = Height / 4;
	int DiffMapWidth = Width / 4;
	for (auto itr = DiffMaps.begin(); itr != DiffMaps.end(); itr++)
	{
		unsigned char *CurDiffMap = (*itr);
		int DiffmapSize = DiffMapWidth * DiffMapHeight * sizeof(unsigned char);
		unsigned char *CurDistanceMap = (unsigned char *)MY_ALLOC(DiffmapSize);
		memset(CurDistanceMap, 0, DiffmapSize);
		//go through the distance map line by line and check where the SAD starts and where it ends
		for (int y = 0; y < DiffMapHeight; y++)
			for (int x = 0; x < DiffMapWidth; x++)
			{
				if (CurDiffMap[y * DiffMapWidth + x] != 0)
				{
					int StartOfZone = x;
					int EndOfZone = x;
					while (CurDiffMap[y * DiffMapWidth + EndOfZone] != 0)
						EndOfZone++;
					int LineSize = EndOfZone - StartOfZone;
					if (LineSize > DIFF_LINE_WIDTH_TOO_SMALL)
					{
						for (int i = StartOfZone; i < EndOfZone; i++)
							CurDistanceMap[i] = LineSize;
					}
				}
			}
		DistanceMaps.push_back(CurDistanceMap);
	}
	//try to calculate for each pixel it's own depth
	MergedDistanceMaps = (LPCOLORREF)MY_ALLOC(DiffMapWidth * DiffMapHeight * sizeof(COLORREF));
	int PicturesInList = DistanceMaps.size();
	MaxLength = 0;
	MinLength = 0x0FFFFFFF;
	for (int y = 0; y < DiffMapHeight; y++)
		for (int x = 0; x < DiffMapWidth; x++)
		{
			//iterate through all length maps and get the average length at this pixel position
			int SumOfLengths = 0;
			for (auto itr = DistanceMaps.begin(); itr != DistanceMaps.end(); itr++)
				SumOfLengths += (*itr)[y * DiffMapWidth + x];
			int AvgLength = SumOfLengths / PicturesInList;
			MergedDistanceMaps[y * DiffMapWidth + x] = AvgLength;
			if (AvgLength > MaxLength)
				MaxLength = AvgLength;
			if (AvgLength < MinLength)
				MinLength = AvgLength;
		}
	//scale the depth to color values
	int tMaxLength = MaxLength + MinLength;
	for (int y = 0; y < DiffMapHeight; y++)
		for (int x = 0; x < DiffMapWidth; x++)
		{
			DWORD CurLength = MergedDistanceMaps[y * DiffMapWidth + x];
			unsigned char Color = 255 * CurLength / tMaxLength;
			MergedDistanceMaps[y * DiffMapWidth + x] = RGB(Color, Color, Color);
		}
}

void ProjectionDistanceMap::SameMergedMapToFile(char *FileName)
{
	if (MergedDistanceMaps == NULL)
		return;
	SaveImage(MergedDistanceMaps, Width / 4, Height / 4, FileName);
}

void ResetDistanceMapScreenshot()
{
	PDM.Init(CurScreenshot->GetWidth(),CurScreenshot->GetHeight(), CurScreenshot->Pixels);
}

void ParseImageDistanceMapScreenshot()
{
	PDM.AddProjectedImage(CurScreenshot->Pixels);
}

void ApplyDistanceMapOnScreenshot(char *FileName)
{
	PDM.ProcessAllDiffMaps();
}