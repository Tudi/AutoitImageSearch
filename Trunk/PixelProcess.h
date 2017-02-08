#pragma once

//similar to image binarization. Will be used for OCR
void WINAPI KeepColorSetRest(int SetRest, int SetColors, int Color1);
void WINAPI KeepColor3SetBoth(int SetRest, int SetColors, int Color1, int Color2, int Color3);
void WINAPI KeepColorsMinInRegion(int StartX = -1, int StartY = -1, int EndX = -1, int EndY = -1, int ColorMin = 0);
void WINAPI KeepColorsMaxInRegion(int StartX = -1, int StartY = -1, int EndX = -1, int EndY = -1, int ColorMax = 0);

void WINAPI ResetColorKeepList();
void WINAPI PushToColorKeepList(int Color);
void WINAPI ApplyColorKeepList(int SetRemainingTo, int SetEliminatedTo);
void WINAPI KeepGradient(int Color, float MaxChange = 0.3f);
void WINAPI KeepGradientRegionMinValue(int Color, float MaxChange, int StartX, int StartY, int EndX, int EndY);
void WINAPI SetGradientToColor(int Color, float MaxChange = 0.3f, int NewColor = TRANSPARENT_COLOR);
void WINAPI KeepGradientRegion(int Color, float MaxChange = 0.3f, int StartX = -1, int StartY = -1, int EndX = -1, int EndY = -1);
int WINAPI CountPixelsInArea( int Color = -1, int StartX = -1, int StartY = -1, int EndX = -1, int EndY = -1);
float WINAPI GetPixelRatioInArea(int Color = -1, int StartX = -1, int StartY = -1, int EndX = -1, int EndY = -1);
void WINAPI RemoveIfHasStrongerNeighbours(int MinNeighbours, int StartX = -1, int StartY = -1, int EndX = -1, int EndY = -1);
void WINAPI ErodeNotInLine(int StartX = -1, int StartY = -1, int EndX = -1, int EndY = -1);
void WINAPI ErodeOnEdgeNeighbours(int EdgeStrength, int StartX = -1, int StartY = -1, int EndX = -1, int EndY = -1);