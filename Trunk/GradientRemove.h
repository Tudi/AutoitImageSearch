#pragma once

void WINAPI GradientRemove();
void WINAPI GradientRemoveCache(char *aFileName);
void WINAPI GradientReduceCache(char *aFileName, int GradientCount);
void GradientReduce(LPCOLORREF Pixels, int Width, int Height, int GradientCount);
