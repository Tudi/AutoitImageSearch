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
	int B = GetRValue(Color);
	int G = GetGValue(Color);
	int R = GetBValue(Color);
	KeepColorList[KeepColorListIndex++] = RGB(R,G,B);
}

__forceinline int ShouldKeepColor(int Color)
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

void WINAPI ApplyColorEliminateListToArea(int SetEliminatedTo, int StartX, int StartY, int EndX, int EndY)
{
	if (CurScreenshot == NULL || CurScreenshot->Pixels == NULL)
		return;
	int Width = CurScreenshot->GetWidth();
	for (int y = StartY; y < EndY; y++)
		for (int x = StartX; x < EndX; x++)
		{
			int Color = CurScreenshot->Pixels[y*Width + x];
			if (ShouldKeepColor(Color))
				CurScreenshot->Pixels[y*Width + x] = SetEliminatedTo;
		}
}

void WINAPI KeepColorsMinInRegion(int StartX, int StartY, int EndX, int EndY, int ColorMin)
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
	//check for valid parameters
	if (StartX > CurScreenshot->GetWidth() || StartX < 0 || StartX >= EndX || EndX > CurScreenshot->GetWidth())
		return;
	if (StartY > CurScreenshot->GetHeight() || StartY < 0 || StartY >= EndY || EndY > CurScreenshot->GetHeight())
		return;
	int RMin = GetRValue(ColorMin);
	int GMin = GetGValue(ColorMin);
	int BMin = GetBValue(ColorMin);
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

				if (R <= RMin || G <= GMin || B <= BMin)
					CurScreenshot->Pixels[y*Width + x] = TRANSPARENT_COLOR;
				else
					CurScreenshot->Pixels[y*Width + x] = 0;
			}
		}
}

void WINAPI KeepColorsMaxInRegion(int StartX, int StartY, int EndX, int EndY, int ColorMax)
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
	//check for valid parameters
	if (StartX > CurScreenshot->GetWidth() || StartX < 0 || StartX >= EndX || EndX > CurScreenshot->GetWidth())
		return;
	if (StartY > CurScreenshot->GetHeight() || StartY < 0 || StartY >= EndY || EndY > CurScreenshot->GetHeight())
		return;
	int RMax = GetRValue(ColorMax);
	int GMax = GetGValue(ColorMax);
	int BMax = GetBValue(ColorMax);
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

				if (R > RMax || G > GMax || B > BMax)
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

void WINAPI KeepGradientRegion(int Color, float MaxChange, int StartX, int StartY, int EndX, int EndY)
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
	for (int y = StartY; y < EndY; y += 1)
		for (int x = StartX; x < EndX; x += 1)
		{
			int Color = CurScreenshot->Pixels[y * Width + x];
			float B2 = GetRValue(Color);
			float G2 = GetGValue(Color);
			float R2 = GetBValue(Color);
			float RG2 = (float)R2 / (float)G2;
			float GB2 = (float)G2 / (float)B2;
			if (abs(RG2 - RG1) > MaxChange || abs(GB2 - GB1) > MaxChange)
				CurScreenshot->Pixels[y * Width + x] = TRANSPARENT_COLOR;
			else
				CurScreenshot->Pixels[y * Width + x] = 0;
		}
	FileDebug("Finished KeepGradient");
}

void WINAPI KeepGradient(int Color, float MaxChange)
{
	FileDebug("Started KeepGradient");
	KeepGradientRegion(Color, MaxChange, 0, 0, CurScreenshot->GetWidth(), CurScreenshot->GetHeight());
	FileDebug("Finished KeepGradient");
}

void WINAPI KeepGradientRegionMinValue(int Color, float MaxChange, int StartX, int StartY, int EndX, int EndY)
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
	for (int y = StartY; y < EndY; y += 1)
		for (int x = StartX; x < EndX; x += 1)
		{
			int Color = CurScreenshot->Pixels[y * Width + x];
			int B2 = GetRValue(Color);
			if (B1 > B2)
			{
				CurScreenshot->Pixels[y * Width + x] = TRANSPARENT_COLOR;
				continue;
			}
			int G2 = GetGValue(Color);
			if (G1 > G2)
			{
				CurScreenshot->Pixels[y * Width + x] = TRANSPARENT_COLOR;
				continue;
			}
			int R2 = GetBValue(Color);
			if (R1 > R2)
			{
				CurScreenshot->Pixels[y * Width + x] = TRANSPARENT_COLOR;
				continue;
			}
			float RG2 = (float)R2 / (float)G2;
			float GB2 = (float)G2 / (float)B2;
			if (abs(RG2 - RG1) > MaxChange || abs(GB2 - GB1) > MaxChange)
				CurScreenshot->Pixels[y * Width + x] = TRANSPARENT_COLOR;
			else
				CurScreenshot->Pixels[y * Width + x] = 0;
		}
	FileDebug("Finished KeepGradient");
}
void WINAPI SetGradientToColorRegion(int Color, float MaxChange, int NewColor, int StartX, int StartY, int EndX, int EndY)
{
	FileDebug("Started SetGradientToColorRegion");
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
	for (int y = StartY; y < EndY; y += 1)
		for (int x = StartX; x < EndX; x += 1)
		{
			int Color = CurScreenshot->Pixels[y * Width + x];
			float B2 = GetRValue(Color);
			float G2 = GetGValue(Color);
			float R2 = GetBValue(Color);
			float RG2 = (float)R2 / (float)G2;
			float GB2 = (float)G2 / (float)B2;
			if (abs(RG2 - RG1) <= MaxChange && abs(GB2 - GB1) <= MaxChange)
				CurScreenshot->Pixels[y * Width + x] = NewColor;
		}
	FileDebug("Finished SetGradientToColorRegion");
}

void WINAPI SetGradientToColor(int Color, float MaxChange, int NewColor)
{
	FileDebug("Started SetGradientToColor");
	SetGradientToColorRegion(Color, MaxChange, NewColor, 0, 0, CurScreenshot->GetWidth(), CurScreenshot->GetHeight());
	FileDebug("Finished SetGradientToColor");
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

void WINAPI RemoveIfHasStrongerNeighbours(int MinNeighbours, int StartX, int StartY, int EndX, int EndY)
{
	FileDebug("Started RemoveIfHasStrongerNeighbours");
	// human eye can distinguesh about 3db noise ( signal / noise ). We define a gradient to be the same if the expected value / real value is within this margin
	if (CurScreenshot->Pixels == NULL)
	{
		FileDebug("WARNING:Screenshot buffer is null when trying to extract gradient!");
		return;
	}
	int Width = CurScreenshot->Right - CurScreenshot->Left;
	for (int y = StartY; y < EndY; y += 1)
		for (int x = StartX; x < EndX; x += 1)
		{
			int Color = CurScreenshot->Pixels[y * Width + x];
			int B = GetRValue(Color);
			int G = GetGValue(Color);
			int R = GetBValue(Color);
			//int Sum = R + G + B;
			//check if it has stronger neighbours
			int StrongNeighbours = 0;
			for (int y2 = y - 1; y2 <= y + 1; y2++)
				for (int x2 = x - 1; x2 <= x + 1; x2++)
				{
					int Color2 = CurScreenshot->Pixels[y2 * Width + x2];
					int B2 = GetRValue(Color2);
					int G2 = GetGValue(Color2);
					int R2 = GetBValue(Color2);
					//int Sum2 = R2 + G2 + B2;
					//if (Sum2 > Sum)
					if (R2 > R || G2 > G || B2 > B)
					{
						StrongNeighbours++;
						if (StrongNeighbours >= MinNeighbours)
						{
							CurScreenshot->Pixels[y * Width + x] = 0;
							goto ELIMINATED_WEAK_PIXEL;
						}
					}
				}
		ELIMINATED_WEAK_PIXEL:;
		}
	FileDebug("Finished RemoveIfHasStrongerNeighbours");
}

__forceinline int IsInLine(int C1, int C2, int C3)
{
	int B1 = GetRValue(C1);
	int G1 = GetGValue(C1);
	int R1 = GetBValue(C1);

	int B2 = GetRValue(C2);
	int G2 = GetGValue(C2);
	int R2 = GetBValue(C2);

	int B3 = GetRValue(C3);
	int G3 = GetGValue(C3);
	int R3 = GetBValue(C3);

	if (R1 != R2 || R2 != R3)
		return 0;
	if (G1 != G2 || G2 != G3)
		return 0;
	if (B1 != B2 || B2 != B3)
		return 0;
	return 1;
}

void WINAPI ErodeNotInLine(int StartX, int StartY, int EndX, int EndY)
{
	FileDebug("Started ErodeNotInLine");
	// human eye can distinguesh about 3db noise ( signal / noise ). We define a gradient to be the same if the expected value / real value is within this margin
	if (CurScreenshot->Pixels == NULL)
	{
		FileDebug("WARNING:Screenshot buffer is null when trying to extract gradient!");
		return;
	}
	int Width = CurScreenshot->Right - CurScreenshot->Left;
	int *TempBuff = (int*)malloc(Width * CurScreenshot->GetHeight() * sizeof(int));
	memcpy(TempBuff, CurScreenshot->Pixels, Width * CurScreenshot->GetHeight() * sizeof(int));
	for (int y = StartY; y < EndY; y += 1)
		for (int x = StartX; x < EndX; x += 1)
		{
			int MiddlePixel = TempBuff[(y - 0) * Width + x - 0];
			//vert
			if (IsInLine(TempBuff[(y - 1) * Width + x - 0], MiddlePixel, TempBuff[(y + 1) * Width + x - 0]))
				continue;
			//hor
			if (IsInLine(TempBuff[(y - 0) * Width + x - 1], MiddlePixel, TempBuff[(y + 1) * Width + x + 1]))
				continue;
			if (IsInLine(TempBuff[(y - 1) * Width + x - 1], MiddlePixel, TempBuff[(y + 1) * Width + x + 1]))
				continue;
			if (IsInLine(TempBuff[(y - 1) * Width + x + 1], MiddlePixel, TempBuff[(y + 1) * Width + x - 1]))
				continue;
			CurScreenshot->Pixels[(y - 0) * Width + x - 0] = 0;
		}
	free(TempBuff);
	FileDebug("Finished ErodeNotInLine");
}

void WINAPI ErodeOnEdgeNeighbours(int EdgeStrength, int StartX, int StartY, int EndX, int EndY)
{
	FileDebug("Started ErodeOnEdgeNeighbours");
	// human eye can distinguesh about 3db noise ( signal / noise ). We define a gradient to be the same if the expected value / real value is within this margin
	if (CurScreenshot->Pixels == NULL)
	{
		FileDebug("WARNING:Screenshot buffer is null when trying to extract gradient!");
		return;
	}
	int Width = CurScreenshot->Right - CurScreenshot->Left;
	int *TempBuff = (int*)malloc(Width * CurScreenshot->GetHeight() * sizeof(int));
	memcpy(TempBuff, CurScreenshot->Pixels, Width * CurScreenshot->GetHeight() * sizeof(int));
	for (int y = StartY; y < EndY; y += 1)
		for (int x = StartX; x < EndX; x += 1)
		{
			int Color = TempBuff[y * Width + x];
			int B = GetRValue(Color);
			int G = GetGValue(Color);
			int R = GetBValue(Color);
			int Sum = R + G + B;
			//check if it has stronger neighbours
			int StrongNeighbours = 0;
			for (int y2 = y - 1; y2 <= y + 1; y2++)
				for (int x2 = x - 1; x2 <= x + 1; x2++)
				{
					int Color2 = TempBuff[y2 * Width + x2];
					int B2 = GetRValue(Color2);
					int G2 = GetGValue(Color2);
					int R2 = GetBValue(Color2);
					int Sum2 = R2 + G2 + B2;
					if (abs(Sum2 - Sum) > EdgeStrength )
					{
						CurScreenshot->Pixels[y * Width + x] = 0;
						goto ELIMINATED_WEAK_PIXEL;

					}
				}
		ELIMINATED_WEAK_PIXEL:;
		}
	free(TempBuff);
	FileDebug("Finished ErodeOnEdgeNeighbours");
}
