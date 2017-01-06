#include "StdAfx.h"

void WINAPI ConvertToGrayScale()
{
	if (CurScreenshot == NULL)
		return;
	int Width = CurScreenshot->GetWidth();
	int Height = CurScreenshot->GetHeight();
	for (int y = 0; y < Height; y++)
	{
		int *BaseSrc = (int*)&CurScreenshot->Pixels[y*Width];
		int *BaseDst = BaseSrc;
		for (int x = 0; x < Width; x++)
		{
			unsigned char *SP = (unsigned char*)&BaseSrc[x];
			unsigned char *DP = (unsigned char*)&BaseDst[x];
			// Sum the 3 color channels
			int	Sum = 0;
			for (int i = 0; i < 3; i++)
				Sum += SP[i];
			Sum = Sum / 3; // avg the channels
			//write back
			for (int i = 0; i < 3; i++)
				DP[i] = Sum;
		}
	}
}

void WINAPI ConvertToGrayScaleMaxChannel()
{
	if (CurScreenshot == NULL)
		return;
	int Width = CurScreenshot->GetWidth();
	int Height = CurScreenshot->GetHeight();
	for (int y = 0; y < Height; y++)
	{
		int *BaseSrc = (int*)&CurScreenshot->Pixels[y*Width];
		for (int x = 0; x < Width; x++)
		{
			unsigned char *SP = (unsigned char*)&BaseSrc[x];
			unsigned char *DP = SP;
			// Which is the strongest channel ?
			if (SP[0] >= SP[1] && SP[0] >= SP[2])
			{
				DP[0] = DP[1] = DP[2] = SP[0];
			}
			else if (SP[1] >= SP[0] && SP[1] >= SP[2])
			{
				DP[0] = DP[1] = DP[2] = SP[1];
			}
			else
			{
				DP[0] = DP[1] = DP[2] = SP[2];
			}
		}
	}
}

void EdgeDetectOnePixel(unsigned char *SP, unsigned char *DP, int Width)
{
	// for the 3 color channels
	for (int i = 0; i < 3; i++)
	{
		//				int Gx = 1 * SP[i] + 0 * SP[i + 4 * Width] + 0 * SP[i + 4] - 1 * SP[i + 4 + 4 * Width];
		//				int Gy = 0 * SP[i] + 1 * SP[i + 4 * Width] - 1 * SP[i + 4] + 0 * SP[i + 4 + 4 * Width];
		//				int Res = sqrt(Gx * Gx + Gy * Gy);  // this can get to 360 !
		int Gx = (signed int)SP[i] - (signed int)SP[i + 4 + 4 * Width];
		int Gy = (signed int)SP[i + 4 * Width] - (signed int)SP[i + 4];
		int Res = (int)sqrt((Gx * Gx + Gy * Gy) / 2); //divide by 2 to make result fit into a char
		DP[i] = Res;
	}
}

void EdgeDetectOnePixelRev(unsigned char *SP, unsigned char *DP, int Width)
{
	for (int i = 0; i < 3; i++)
	{
		int Gx = (signed int)SP[i] - (signed int)SP[i - 4 - 4 * Width];
		int Gy = (signed int)SP[i - 4 * Width] - (signed int)SP[i - 4];
		int Res = (int)sqrt((Gx * Gx + Gy * Gy) / 2); //divide by 2 to make result fit into a char
		DP[i] = Res;
	}
}

void WINAPI EdgeDetectRobertCross3Channels()
{
	if (CurScreenshot == NULL)
		return;
	int Width = CurScreenshot->GetWidth();
	int Height = CurScreenshot->GetHeight();
	int *Dest = (int*)malloc(Width * Height * sizeof(int));
	for (int y = 0; y < Height - 1; y++)
	{
		int *BaseSrc = (int*)&CurScreenshot->Pixels[y*Width];
		int *BaseDst = &Dest[y*Width];
		for (int x = 0; x < Width - 1; x++)
			EdgeDetectOnePixel((unsigned char*)&BaseSrc[x], (unsigned char*)&BaseDst[x], Width);
	}
	//at the edges it's tricky
	//last row
	int y = Height - 1;
	int *BaseSrc = (int*)&CurScreenshot->Pixels[y*Width];
	int *BaseDst = &Dest[y*Width];
	for (int x = 0; x < Width; x++)
		EdgeDetectOnePixelRev((unsigned char*)&BaseSrc[x], (unsigned char*)&BaseDst[x], Width);
	//last Col
	for (int y = 0; y < Height; y++)
	{
		int *BaseSrc = (int*)&CurScreenshot->Pixels[y*Width];
		int *BaseDst = &Dest[y*Width];
		int x = Width - 1;
		EdgeDetectOnePixelRev((unsigned char*)&BaseSrc[x], (unsigned char*)&BaseDst[x], Width);
	}
	//copy back Dest
	memcpy(CurScreenshot->Pixels, Dest, Width * Height * sizeof(int));
	// free Villie
	free(Dest);
}

void WINAPI EdgeDetectRobertCross1Channel()
{
	if (CurScreenshot == NULL)
		return;
	int Width = CurScreenshot->GetWidth();
	int Height = CurScreenshot->GetHeight();
	int *Dest = (int*)malloc(Width * Height * sizeof(int));
	for (int y = 0; y < Height - 1; y++)
	{
		int *BaseSrc = (int*)&CurScreenshot->Pixels[y*Width];
		int *BaseDst = &Dest[y*Width];
		for (int x = 0; x < Width - 1; x++)
		{
			unsigned char *SP = (unsigned char*)&BaseSrc[x];
			unsigned char *DP = (unsigned char*)&BaseDst[x];
			// for the 3 color channels
			int i = 0;
			//				int Gx = 1 * SP[i] + 0 * SP[i + 4 * Width] + 0 * SP[i + 4] - 1 * SP[i + 4 + 4 * Width];
			int Gx = (signed int)SP[i] - (signed int)SP[i + 4 + 4 * Width];
			//				int Gy = 0 * SP[i] + 1 * SP[i + 4 * Width] - 1 * SP[i + 4] + 0 * SP[i + 4 + 4 * Width];
			int Gy = (signed int)SP[i + 4 * Width] - (signed int)SP[i + 4];
			//				int Res = sqrt(Gx * Gx + Gy * Gy);  // this can get to 360 !
			int Res = (int)sqrt((Gx * Gx + Gy * Gy) / 2); //divide by 2 to make result fit into a char
			DP[0] = DP[1] = DP[2] = Res;
		}
	}
	//copy back Dest
	memcpy(CurScreenshot->Pixels, Dest, Width * Height * sizeof(int));
	// free Villie
	free(Dest);
}

void WINAPI EdgeKeepColorMaximaOnly()
{

}

void WINAPI EdgeKeepUpperPercent(int Percent)
{

}

void WINAPI BleedEdges()
{

}