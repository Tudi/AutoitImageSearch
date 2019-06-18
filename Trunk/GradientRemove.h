#pragma once

void WINAPI LuminosityRemove();
void WINAPI LuminosityRemoveCache(char *aFileName);
void WINAPI GradientReduceCache(char *aFileName, int GradientCount);
void GradientReduce(LPCOLORREF Pixels, int Width, int Height, int GradientCount);
