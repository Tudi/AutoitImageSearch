#pragma once

//similar to image binarization. Will be used for OCR
void WINAPI KeepColorSetRest(int SetRest, int SetColors, int Color1);
void WINAPI KeepColor3SetBoth(int SetRest, int SetColors, int Color1, int Color2, int Color3);
void WINAPI KeepColorsMinInRegion(int StartX = -1, int StartY = -1, int EndX = -1, int EndY = -1, int RMin = 0, int GMin = 0, int BMin = 0);

void WINAPI ResetColorKeepList();
void WINAPI PushToColorKeepList(int Color);
void WINAPI ApplyColorKeepList(int SetRemainingTo, int SetEliminatedTo);
void WINAPI KeepGradient(int Color, float MaxChange = 0.3f);
int WINAPI CountPixelsInArea( int Color = -1, int StartX = -1, int StartY = -1, int EndX = -1, int EndY = -1);
float WINAPI GetPixelRatioInArea(int Color = -1, int StartX = -1, int StartY = -1, int EndX = -1, int EndY = -1);