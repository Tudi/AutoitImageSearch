#include "stdafx.h"

int KeepColorList[255];
int KeepColorListIndex = 0;

void WINAPI ResetColorKeepList()
{
	KeepColorListIndex = 0;
}

void WINAPI PushToColorKeepList(int Color)
{
	if (KeepColorListIndex >= sizeof(KeepColorList) / sizeof(int))
		return;
	KeepColorList[KeepColorListIndex++] = Color;
}

int ShouldKeepColor(int Color)
{
	for (int i = 0; i < KeepColorListIndex; i++)
		if (KeepColorList[i] == Color)
			return 1;
	return 0;
}

void WINAPI ApplyColorKeepList(int SetRemainingTo, int SetEliminatedTo)
{
	if (CurScreenshot == NULL || CurScreenshot->Pixels == NULL)
		return;
	int	StartX = 0;
	int	StartY = 0;
	int	EndX = CurScreenshot->GetWidth();
	int	EndY = CurScreenshot->GetHeight();
	int Width = CurScreenshot->GetWidth();
	for (int y = StartY; y < EndY; y++)
		for (int x = StartX; x < EndX; x++)
		{
			int Color = CurScreenshot->Pixels[y*Width + x];

			if (ShouldKeepColor(Color))
				CurScreenshot->Pixels[y*Width + x] = SetRemainingTo;
			else
				CurScreenshot->Pixels[y*Width + x] = SetEliminatedTo;
		}
}

void WINAPI KeepColorsMinInRegion(int StartX, int StartY, int EndX, int EndY, int RMin, int GMin, int BMin)
{
	if (CurScreenshot == NULL || CurScreenshot->Pixels == NULL)
		return;
	if (StartX == -1)
	{
		StartX = 0;
		StartY = 0;
		EndX = CurScreenshot->GetWidth();
		EndY = CurScreenshot->GetHeight();
	}
	int Width = CurScreenshot->GetWidth();
	for (int y = StartY; y < EndY; y++)
		for (int x = StartX; x < EndX; x++)
		{
			int Color = CurScreenshot->Pixels[y*Width + x];
			//			if (Color != TRANSPARENT_COLOR)
			{
				int B = GetRValue(Color);
				int G = GetGValue(Color);
				int R = GetBValue(Color);

				if (R < RMin || G < GMin || B < BMin)
					CurScreenshot->Pixels[y*Width + x] = TRANSPARENT_COLOR;
				else
					CurScreenshot->Pixels[y*Width + x] = 0;
			}
		}
}

void WINAPI KeepColorSetRest(int SetRest, int SetColors, int Color1)
{
	FileDebug("Started KeepColorSetRest");
	if (CurScreenshot->Pixels == NULL)
	{
		FileDebug("WARNING:Screenshot buffer is null when trying to extract color!");
		return;
	}
	int Width = CurScreenshot->Right - CurScreenshot->Left;
	int Height = CurScreenshot->Bottom - CurScreenshot->Top;
	for (int y = 0; y < Height; y += 1)
		for (int x = 0; x < Width; x += 1)
			if (CurScreenshot->Pixels[y * Width + x] != Color1)
				CurScreenshot->Pixels[y * Width + x] = SetRest;
			else
				CurScreenshot->Pixels[y * Width + x] = SetColors;

	FileDebug("Finished KeepColorSetRest");
}

void WINAPI KeepColor3SetBoth(int SetRest, int SetColors, int Color1, int Color2, int Color3)
{
	FileDebug("Started KeepColor3SetBoth");
	if (CurScreenshot->Pixels == NULL)
	{
		FileDebug("WARNING:Screenshot buffer is null when trying to extract color!");
		return;
	}
	int Width = CurScreenshot->Right - CurScreenshot->Left;
	int Height = CurScreenshot->Bottom - CurScreenshot->Top;
	for (int y = 0; y < Height; y += 1)
		for (int x = 0; x < Width; x += 1)
			if (CurScreenshot->Pixels[y * Width + x] == Color1 || CurScreenshot->Pixels[y * Width + x] == Color2 || CurScreenshot->Pixels[y * Width + x] == Color3)
				CurScreenshot->Pixels[y * Width + x] = SetColors;
			else
				CurScreenshot->Pixels[y * Width + x] = SetRest;

	FileDebug("Finished KeepColor3SetBoth");
}

void WINAPI KeepGradient(int Color, float MaxChange)
{
	FileDebug("Started KeepGradient");
	int R1 = GetRValue(Color);
	int G1 = GetGValue(Color);
	int B1 = GetBValue(Color);
	float RG1 = (float)R1 / (float)G1;
	float GB1 = (float)G1 / (float)B1;
	// human eye can distinguesh about 3db noise ( signal / noise ). We define a gradient to be the same if the expected value / real value is within this margin
	if (CurScreenshot->Pixels == NULL)
	{
		FileDebug("WARNING:Screenshot buffer is null when trying to extract gradient!");
		return;
	}
	int Width = CurScreenshot->Right - CurScreenshot->Left;
	int Height = CurScreenshot->Bottom - CurScreenshot->Top;
	for (int y = 0; y < Height; y += 1)
		for (int x = 0; x < Width; x += 1)
		{
			int Color = CurScreenshot->Pixels[y * Width + x];
			int B2 = GetRValue(Color);
			int G2 = GetGValue(Color);
			int R2 = GetBValue(Color);
			float RG2 = (float)R2 / (float)G2;
			float GB2 = (float)G2 / (float)B2;
			if (abs(RG2 - RG1) > MaxChange || abs(GB2 - GB1) > MaxChange)
				CurScreenshot->Pixels[y * Width + x] = TRANSPARENT_COLOR;
			else
				CurScreenshot->Pixels[y * Width + x] = 0;
		}

	FileDebug("Finished KeepGradient");
}

int WINAPI CountPixelsInArea(int Color, int StartX, int StartY, int EndX, int EndY)
{
	FileDebug("Started CountPixelsInArea");
	if (CurScreenshot->Pixels == NULL)
	{
		FileDebug("WARNING:Screenshot buffer is null when trying to extract gradient!");
		return -1;
	}
	if (StartX == -1)
	{
		StartX = 0;
		StartY = 0;
		EndX = CurScreenshot->GetWidth();
		EndY = CurScreenshot->GetHeight();
	}
	int Width = CurScreenshot->Right - CurScreenshot->Left;
	int ret = 0;
	for (int y = StartY; y < EndY; y += 1)
		for (int x = StartX; x < EndX; x += 1)
			if (CurScreenshot->Pixels[y * Width + x] == Color)
				ret++;
	FileDebug("Finished CountPixelsInArea");
	return ret;
}

float WINAPI GetPixelRatioInArea(int Color, int StartX, int StartY, int EndX, int EndY)
{
	FileDebug("Started CountPixelsInArea");
	if (CurScreenshot->Pixels == NULL)
	{
		FileDebug("WARNING:Screenshot buffer is null when trying to extract gradient!");
		return -1;
	}
	if (StartX == -1)
	{
		StartX = 0;
		StartY = 0;
		EndX = CurScreenshot->GetWidth();
		EndY = CurScreenshot->GetHeight();
	}
	int Width = CurScreenshot->Right - CurScreenshot->Left;
	int ret = 0;
	for (int y = StartY; y < EndY; y += 1)
		for (int x = StartX; x < EndX; x += 1)
			if (CurScreenshot->Pixels[y * Width + x] == Color)
				ret++;
	float AreaSize = (EndX - StartX)*(EndY - StartY);
	FileDebug("Finished CountPixelsInArea");
	return ret * 100.0f / AreaSize;
}