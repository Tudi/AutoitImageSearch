#include "stdafx.h"

ProjectionDistanceMap PDM;

void ProjectionDistanceMap::DestroyAllocatedMemmory()
{
	if (PrevImage != NULL)
	{
		_aligned_free(PrevImage);
		PrevImage = NULL;
	}

	if (MergedDistanceMaps != NULL)
	{
		_aligned_free(MergedDistanceMaps);
		MergedDistanceMaps = NULL;
	}

	if (TempDiffBuffer != NULL)
	{
		_aligned_free(TempDiffBuffer);
		TempDiffBuffer = NULL;
	}
}

void ProjectionDistanceMap::Init(int pWidth, int pHeight, LPCOLORREF Pixels)
{
//	if (pWidth != Width || pHeight != Height)
		DestroyAllocatedMemmory();
	PDM.Width = pWidth;
	PDM.Height = pHeight;
	PrevImage = (LPCOLORREF)MY_ALLOC((pWidth * pHeight * sizeof(COLORREF)));
	memcpy(PrevImage, Pixels, pWidth * pHeight * sizeof(COLORREF));
}

void ProjectionDistanceMap::AddProjectedImage(LPCOLORREF Pixels)
{
	//if first image, allocate buffer for it and nothing else to do
	if (PrevImage == NULL)
	{
		PrevImage = (LPCOLORREF)MY_ALLOC(Width * Height * sizeof(COLORREF) );
		memcpy(PrevImage, Pixels, Width * Height * sizeof(COLORREF));
		return;
	}

	int DiffMapHeight = Height / 4;
	int DiffMapWidth = Width / 4;
	if (TempDiffBuffer == NULL)
	{
		int DiffmapSize = (DiffMapWidth + 3) * (DiffMapHeight + 3) * sizeof(unsigned char);
		TempDiffBuffer = (unsigned char *)MY_ALLOC(DiffmapSize);
	}

	unsigned int SAD;
	unsigned int SADCount;
	GenerateDiffMapAvgSAD(PrevImage, Pixels, Width, Height, TempDiffBuffer, &SAD, &SADCount);

	//no changes detected, nothing to do
	if (SADCount == 0)
	{
		return;
	}

	//check the length of each SAD column length
	unsigned int AvgSADPerPixel = SAD / SADCount / 16; // 16 because it's actually 4x4 pixel merge
//	unsigned int SADLimit = AvgSADPerPixel;
	unsigned int SADLimit = 1 * 3;
	if (MergedDistanceMaps == NULL)
	{
		MergedDistanceMaps = (LPCOLORREF)MY_ALLOC(DiffMapWidth * DiffMapHeight * sizeof(DWORD));
		memset(MergedDistanceMaps, 0, DiffMapWidth * DiffMapHeight * sizeof(DWORD));
		MapsMerged = 0;
	}
/*
#ifdef _DEBUG
	static int fileIndex = 0; fileIndex++;
	char FileName[500];
	sprintf_s(FileName, sizeof(FileName), "diffm%d.bmp", fileIndex);
	SaveImage((LPCOLORREF)TempDiffBuffer, DiffMapWidth, DiffMapHeight, FileName,1);
#endif/**/

	//avoid overflowing values
	if ((MapsMerged + 1) * Width / 4 >= 0x0FFFFFFF)
		MergeDistances();

	//go through the distance map line by line and check where the SAD starts and where it ends
	for (int y = 0; y < DiffMapHeight; y++)
		for (int x = 0; x < DiffMapWidth; x++)
		{
			if (TempDiffBuffer[y * DiffMapWidth + x] >= SADLimit)
			{
				int StartOfZone = x;
				int EndOfZone = x + 1;
				while (TempDiffBuffer[y * DiffMapWidth + EndOfZone] >= SADLimit && EndOfZone < DiffMapWidth)
					EndOfZone++;
				int LineSize = EndOfZone - StartOfZone;
				if (LineSize > DIFF_LINE_WIDTH_TOO_SMALL)
				{
					for (int i = StartOfZone; i < EndOfZone; i++)
						MergedDistanceMaps[y * DiffMapWidth + i] += LineSize;
				}
			}
		}
	//we added a new distance map
	MapsMerged++;
/*
#ifdef _DEBUG
	memset(MergedDistanceMaps, 0, DiffMapWidth * DiffMapHeight * sizeof(DWORD));
	for (int y = 0; y < DiffMapHeight; y++)
		for (int x = 0; x < DiffMapWidth; x++)
		{
			if (TempDiffBuffer[y * DiffMapWidth + x] >= SADLimit)
			{
				int StartOfZone = x;
				int EndOfZone = x + 1;
				while (TempDiffBuffer[y * DiffMapWidth + EndOfZone] >= SADLimit && EndOfZone < DiffMapWidth)
					EndOfZone++;
				int LineSize = EndOfZone - StartOfZone;
				if (LineSize > DIFF_LINE_WIDTH_TOO_SMALL)
				{
					for (int i = StartOfZone; i < EndOfZone; i++)
						MergedDistanceMaps[y * DiffMapWidth + i] = RGB(255,255,255);
				}
			}
		}
	static int fileIndex = 0; fileIndex++;
	char FileName[500];
	sprintf_s(FileName, sizeof(FileName), "diffd%d.bmp", fileIndex);
	SaveImage((LPCOLORREF)MergedDistanceMaps, DiffMapWidth, DiffMapHeight, FileName);
#endif*/

	//make current image as current image :P
	memcpy(PrevImage, Pixels, Width * Height * sizeof(COLORREF));
}

//Called when adding more lengths would overflow a DWORD
void ProjectionDistanceMap::MergeDistances()
{
	int DiffMapHeight = Height / 4;
	int DiffMapWidth = Width / 4;
	for (int y = 0; y < DiffMapHeight; y++)
		for (int x = 0; x < DiffMapWidth; x++)
		{
			//iterate through all length maps and get the average length at this pixel position
			int AvgLength = MergedDistanceMaps[y * DiffMapWidth + x] / MapsMerged;
			MergedDistanceMaps[y * DiffMapWidth + x] = AvgLength;
		}
	MapsMerged = 1;
}

void ProjectionDistanceMap::SaveMergedMapToFile(char *FileName)
{
	if (MergedDistanceMaps == NULL)
		return;
	if (MapsMerged == 0)
		return;

	int DiffMapHeight = Height / 4;
	int DiffMapWidth = Width / 4;

	//try to calculate for each pixel it's own depth
	LPCOLORREF DistanceMapColors = (LPCOLORREF)MY_ALLOC(DiffMapWidth * DiffMapHeight * sizeof(COLORREF));
	int MaxLength, MinLength; // to scale colors from 0 to 255
	MaxLength = 0;
	MinLength = 0x0FFFFFFF;
	for (int y = 0; y < DiffMapHeight; y++)
		for (int x = 0; x < DiffMapWidth; x++)
		{
			//iterate through all length maps and get the average length at this pixel position
			int AvgLength = MergedDistanceMaps[y * DiffMapWidth + x] / MapsMerged;
			if (AvgLength > MaxLength)
				MaxLength = AvgLength;
			if (AvgLength < MinLength)
				MinLength = AvgLength;
		}

	//scale the depth to color values
	int tMaxLength = MaxLength + MinLength;
	if (tMaxLength == 0)
	{
		memset(DistanceMapColors, 0, DiffMapWidth * DiffMapHeight * sizeof(COLORREF));
	}
	else
	{
		for (int y = 0; y < DiffMapHeight; y++)
			for (int x = 0; x < DiffMapWidth; x++)
			{
				DWORD CurLength = MergedDistanceMaps[y * DiffMapWidth + x] / MapsMerged;
				unsigned char Color = (unsigned char)(255 * CurLength / tMaxLength);
				DistanceMapColors[y * DiffMapWidth + x] = RGB(Color, Color, Color);
			}
	}

	//save the diff map as a grayscale image
	SaveImage(DistanceMapColors, DiffMapWidth, DiffMapHeight, FileName);

	//dealloc temp memory
	_aligned_free(DistanceMapColors);
}

void ResetDistanceMapScreenshot()
{
	PDM.Init(CurScreenshot->GetWidth(),CurScreenshot->GetHeight(), CurScreenshot->Pixels);
}

void ParseImageDistanceMapScreenshot()
{
	PDM.AddProjectedImage(CurScreenshot->Pixels);
}

void SaveDistMapAsImage(char *FileName)
{
	PDM.SaveMergedMapToFile(FileName);
}

/*
void ApplyDistanceMapOnScreenshot(char *FileName)
{
	PDM.ProcessAllDiffMaps();
}*/