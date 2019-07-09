#include "stdafx.h"

void DrawLine(int x1, int y1, int x2, int y2, COLORREF color)
{
	if (CurScreenshot == NULL)
		return;
	int Width = CurScreenshot->GetWidth();
	int Height = CurScreenshot->GetHeight();
	if (x1 < 0)
		x1 = 0;
	if (x1 > Width)
		x1 = Width;
	if (y1 < 0)
		y1 = 0;
	if (y1 > Height)
		y1 = Height;
	int XDist = x2 - x1;
	int YDist = y2 - y1;
	int MaxSteps = abs(XDist);
	if (abs(YDist) > MaxSteps)
		MaxSteps = abs(YDist);
	for (int i = 0; i < MaxSteps; i++)
	{
		int tx = x1 + i * XDist / MaxSteps;
		int ty = y1 + i * YDist / MaxSteps;
		CurScreenshot->Pixels[ty * Width + tx] = color;
	}
}

void DrawLine2(int x1, int y1, int x2, int y2, COLORREF color)
{
	if (CurScreenshot == NULL)
		return;
	int Width = CurScreenshot->GetWidth();
	int Height = CurScreenshot->GetHeight();
	if (x1 < 0)
		x1 = 0;
	if (x1 > Width)
		x1 = Width;
	if (y1 < 0)
		y1 = 0;
	if (y1 > Height)
		y1 = Height;
	int XDist = x2 - x1;
	int YDist = y2 - y1;
	int MaxSteps = abs(XDist);
	if (abs(YDist) > MaxSteps)
		MaxSteps = abs(YDist);
	float XIncr = (float)XDist / MaxSteps;
	float YIncr = (float)YDist / MaxSteps;
	float tx = x1;
	float ty = y1;
	for (int i = 0; i < MaxSteps; i++)
	{
		CurScreenshot->Pixels[(int)(ty * Width) + (int)tx] = color;
		tx += XIncr;
		ty += YIncr;
	}
}