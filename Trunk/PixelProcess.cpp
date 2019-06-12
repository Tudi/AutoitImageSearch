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
	int rgb = RGB(R, G, B);
	//check if we already have it
	for (int i = 0; i < KeepColorListIndex; i++)
		if (KeepColorList[i] == rgb)
			return;
	KeepColorList[KeepColorListIndex++] = rgb;
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

				if (R < RMin || G < GMin || B < BMin)
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
	float AreaSize = (float)((EndX - StartX)*(EndY - StartY));
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

void HistorygramInArea(int StartX, int StartY, int EndX, int EndY)
{
	FileDebug("Started HistorygramInArea");
	if (CurScreenshot->Pixels == NULL)
	{
		FileDebug("WARNING:Screenshot buffer is null when trying to extract gradient!");
		return;
	}
	if (StartX == -1)
	{
		StartX = 0;
		StartY = 0;
		EndX = CurScreenshot->GetWidth();
		EndY = CurScreenshot->GetHeight();
	}
	//create historygram
	std::map<int, int> Colors;
	int Width = CurScreenshot->Right - CurScreenshot->Left;
	for (int y = StartY; y < EndY; y += 1)
		for (int x = StartX; x < EndX; x += 1)
			Colors[CurScreenshot->Pixels[y * Width + x]]++;

	std::map<int, int> Colors2;
	for (std::map<int, int>::iterator itr = Colors.begin(); itr != Colors.end(); itr++)
	{
		//get a unique key
		int key = itr->second * 1000;
		while (Colors2.find(key) != Colors2.end())
			key++;
		Colors2[key] = itr->first;
	}

	//print hystorygram
	for (std::map<int, int>::iterator itr = Colors2.begin(); itr != Colors2.end(); itr++)
	{
		int Color = itr->second;
		int Count = itr->first/1000;
		int B = GetRValue(Color);
		int G = GetGValue(Color);
		int R = GetBValue(Color);
		float RG = (float)(R) / (float)(G);
		float GB = (float)(G) / (float)(B);
		float RB = (float)(R) / (float)(B);
		printf("0x%X 0x%X %d %f %f %f %d %d %d\n", Color, STATIC_BGR_RGB(Color), Count, RG, GB, RB, R, G, B);
	}
	FileDebug("Finished HistorygramInArea");
	return;
}

void KeepColorRange(int RMin, int RMax, int GMin, int GMax, int BMin, int BMax)
{
	FileDebug("Started KeepColorRange");
	if (CurScreenshot->Pixels == NULL)
	{
		FileDebug("WARNING:Screenshot buffer is null when trying to extract gradient!");
		return;
	}
	int Width = CurScreenshot->Right - CurScreenshot->Left;
	for (int y = 0; y < CurScreenshot->GetHeight(); y += 1)
		for (int x = 0; x < CurScreenshot->GetWidth(); x += 1)
		{
			int Color = CurScreenshot->Pixels[y * Width + x];
			float B = GetRValue(Color);
			float G = GetGValue(Color);
			float R = GetBValue(Color);
			if (R >= RMin && R <= RMax && G >= GMin && G <= GMax && B >= BMin && B <= BMax)
				CurScreenshot->Pixels[y * Width + x] = 0;
			else
				CurScreenshot->Pixels[y * Width + x] = TRANSPARENT_COLOR;
		}
	FileDebug("Finished KeepColorRange");
}

void KeepColorRangeAndGradient(int Color, int MaxChange, int RMin, int RMax, int GMin, int GMax, int BMin, int BMax)
{
	FileDebug("Started KeepColorRange");
	if (CurScreenshot->Pixels == NULL)
	{
		FileDebug("WARNING:Screenshot buffer is null when trying to extract gradient!");
		return;
	}
	int R1 = GetRValue(Color);
	int G1 = GetGValue(Color);
	int B1 = GetBValue(Color);
	float RG1 = (float)R1 / (float)G1;
	float GB1 = (float)G1 / (float)B1;
	int Width = CurScreenshot->Right - CurScreenshot->Left;
	for (int y = 0; y < CurScreenshot->GetHeight(); y += 1)
		for (int x = 0; x < CurScreenshot->GetWidth(); x += 1)
		{
			int Color = CurScreenshot->Pixels[y * Width + x];
			float B = GetRValue(Color);
			float G = GetGValue(Color);
			float R = GetBValue(Color);
			if (R >= RMin && R <= RMax && G >= GMin && G <= GMax && B >= BMin && B <= BMax)
			{
				float RG2 = (float)R / (float)G;
				float GB2 = (float)G / (float)B;
				if (abs(RG2 - RG1) > MaxChange || abs(GB2 - GB1) > MaxChange)
					CurScreenshot->Pixels[y * Width + x] = TRANSPARENT_COLOR;
//				else
//					CurScreenshot->Pixels[y * Width + x] = 0;
			}
			else
				CurScreenshot->Pixels[y * Width + x] = TRANSPARENT_COLOR;
		}
	FileDebug("Finished KeepColorRange");
}
void WINAPI KeepGradient3(int Color1, float MaxChange1, int Color2, float MaxChange2, int Color3, float MaxChange3)
{
	FileDebug("Started KeepGradient2");
	int R1 = GetRValue(Color1);
	int G1 = GetGValue(Color1);
	int B1 = GetBValue(Color1);
	float RG1 = (float)R1 / (float)G1;
	float GB1 = (float)G1 / (float)B1;
	int R2 = GetRValue(Color2);
	int G2 = GetGValue(Color2);
	int B2 = GetBValue(Color2);
	float RG2 = (float)R2 / (float)G2;
	float GB2 = (float)G2 / (float)B2;
	int R3 = GetRValue(Color3);
	int G3 = GetGValue(Color3);
	int B3 = GetBValue(Color3);
	float RG3 = (float)R3 / (float)G3;
	float GB3 = (float)G3 / (float)B3;
	// human eye can distinguesh about 3db noise ( signal / noise ). We define a gradient to be the same if the expected value / real value is within this margin
	if (CurScreenshot->Pixels == NULL)
	{
		FileDebug("WARNING:Screenshot buffer is null when trying to extract gradient!");
		return;
	}
	int StartX = 0;
	int StartY = 0;
	int EndX = CurScreenshot->GetWidth();
	int EndY = CurScreenshot->GetHeight();
	int Width = CurScreenshot->Right - CurScreenshot->Left;
	for (int y = StartY; y < EndY; y += 1)
		for (int x = StartX; x < EndX; x += 1)
		{
			int Color = CurScreenshot->Pixels[y * Width + x];
			float B2 = GetRValue(Color);
			float G2 = GetGValue(Color);
			float R2 = GetBValue(Color);
			float RG = (float)R2 / (float)G2;
			float GB = (float)G2 / (float)B2;
			if (!(abs(RG2 - RG) > MaxChange2 || abs(GB2 - GB) > MaxChange2) && !(abs(RG - RG1) > MaxChange1 || abs(GB - GB1) > MaxChange1) && !(abs(RG - RG3) > MaxChange3 || abs(GB - GB3) > MaxChange3))
				CurScreenshot->Pixels[y * Width + x] = 0;
			else
				CurScreenshot->Pixels[y * Width + x] = TRANSPARENT_COLOR;
		}
	FileDebug("Finished KeepGradient2");

}

void ColorReduceCache(char *aFileName, int ChannelColorCount)
{
	CachedPicture *cache = CachePicturePrintErrors(aFileName, __FUNCTION__);
	if (cache == NULL)
		return;
	int ColorStep = 255 / ChannelColorCount;
	LPCOLORREF Pixels = cache->Pixels;
	int Width = cache->Width;
	int Height = cache->Height;
	for (int y = 0; y < Height; y++)
	{
		int *BaseSrc = (int*)&Pixels[y*Width];
		for (int x = 0; x < Width; x++)
		{
			unsigned char *SP = (unsigned char*)&BaseSrc[x];
			for (int i = 0; i < 3; i++)
			{
				int NewVal = (SP[i] + ColorStep) / ColorStep * ColorStep;
				SP[i] = NewVal & 0xFF;
			}
		}
	}
}