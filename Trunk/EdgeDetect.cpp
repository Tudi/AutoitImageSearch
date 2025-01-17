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
		int Res = (int)sqrt((Gx * Gx + Gy * Gy) / 2); //max 255^2 + 255^2 -> divide by 2 to make result fit into a char
		DP[i] = Res;
	}
}

void WINAPI EdgeDetectRobertCross3Channels()
{
	if (CurScreenshot == NULL)
		return;
	int Width = CurScreenshot->GetWidth();
	int Height = CurScreenshot->GetHeight();
	int *Dest = (int*)MY_ALLOC(Width * Height * sizeof(int));
	memset(Dest, 0, Width * Height * sizeof(int));	//leaves 1 pixel edge untouched
	for (int y = 0; y < Height - 1; y++)
	{
		int *BaseSrc = (int*)&CurScreenshot->Pixels[y*Width];
		int *BaseDst = &Dest[y*Width];
		for (int x = 0; x < Width - 1; x++)
			EdgeDetectOnePixel((unsigned char*)&BaseSrc[x], (unsigned char*)&BaseDst[x], Width);
	}
	//special function for special pixels ?
	Dest[Width - 1] = 0;
	Dest[(Height - 1) * Width] = 0;
	//copy back Dest
	memcpy(CurScreenshot->Pixels, Dest, Width * Height * sizeof(int));
	// free Villie
	MY_FREE(Dest);
}

void WINAPI EdgeDetectRobertCross1Channel()
{
	if (CurScreenshot == NULL)
		return;
	int Width = CurScreenshot->GetWidth();
	int Height = CurScreenshot->GetHeight();
	int *Dest = (int*)MY_ALLOC(Width * Height * sizeof(int));
	memset(Dest, 0, Width * Height * sizeof(int));	//leaves 1 pixel edge untouched
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
	MY_FREE(Dest);
}

void WINAPI EdgeKeepUpperPercent(int Percent)
{

}

// how fast the bleeding will stop
#define BleedSpeed 2

void WINAPI EdgeBleed()
{
	if (CurScreenshot == NULL)
		return;
	int Width = CurScreenshot->GetWidth();
	int Height = CurScreenshot->GetHeight();
	int CanBleed = 1;
	while (CanBleed == 1)
	{
		CanBleed = 0;
		for (int y = 1; y < Height - 1; y++)
		{
			int *BaseSrc = (int*)&CurScreenshot->Pixels[y*Width];
			for (int x = 1; x < Width - 1; x++)
			{
				unsigned char *SP = (unsigned char*)&BaseSrc[x];

				//get region maxima and bleed it into us
				int CenterVal = SP[0] / BleedSpeed;
				for (int y1 = -Width * 4; y1 <= Width * 4; y1 += Width * 4)
					for (int x1 = -4; x1 <= 4; x1 += 4)
						if (CenterVal > SP[y1 + x1])
						{
							SP[y1 + x1 + 0] = CenterVal;
							SP[y1 + x1 + 1] = CenterVal;
							SP[y1 + x1 + 2] = CenterVal;
							CanBleed = 1;
						}
			}
		}
	}
}

void EdgeDetectOnePixelSobel(unsigned char *SP, unsigned char *DP, int Width)
{
	// for the 3 color channels
	for (int i = 0; i < 3; i++)
	{
		int Gx = 0;
		int Gy = 0;
		Gx += +1 * SP[i - 4 - 4 * Width] + 0 * SP[i + 0 - 4 * Width] - 1 * SP[i + 4 - 4 * Width];
		Gx += +2 * SP[i - 4 - 0 * Width] + 0 * SP[i + 0 - 0 * Width] - 2 * SP[i + 4 - 0 * Width];
		Gx += +1 * SP[i - 4 + 4 * Width] + 0 * SP[i + 0 + 4 * Width] - 1 * SP[i + 4 + 4 * Width];

		Gy += -1 * SP[i - 4 - 4 * Width] - 2 * SP[i + 0 - 4 * Width] - 1 * SP[i + 4 - 4 * Width];
		Gy += -0 * SP[i - 4 - 0 * Width] + 0 * SP[i + 0 - 0 * Width] + 0 * SP[i + 4 - 0 * Width];
		Gy += +1 * SP[i - 4 + 4 * Width] + 2 * SP[i + 0 + 4 * Width] + 1 * SP[i + 4 + 4 * Width];

		int Res = (int)sqrt((Gx * Gx + Gy * Gy) / 32); //4^2*255^2+4^2*255^2 = 2*4^2*255^2 => divide by 32 to make result fit into a char
		DP[i] = Res;
	}
}

void WINAPI EdgeDetectSobel3Channels()
{
	if (CurScreenshot == NULL)
		return;
	int Width = CurScreenshot->GetWidth();
	int Height = CurScreenshot->GetHeight();
	int *Dest = (int*)MY_ALLOC(Width * Height * sizeof(int));
	memset(Dest, 0, Width * Height * sizeof(int));	//Sobel leaves 1 pixel edge untouched
	for (int y = 1; y < Height - 1; y++)
	{
		int *BaseSrc = (int*)&CurScreenshot->Pixels[y*Width];
		int *BaseDst = &Dest[y*Width];
		for (int x = 1; x < Width - 1; x++)
			EdgeDetectOnePixelSobel((unsigned char*)&BaseSrc[x], (unsigned char*)&BaseDst[x], Width);
	}
	//copy back Dest
	memcpy(CurScreenshot->Pixels, Dest, Width * Height * sizeof(int));
	// free Villie
	MY_FREE(Dest);
}

int IsLocalMaxima(unsigned char *SP, int Width, int Radius)
{
	int CenterVal = SP[0];
	for (signed int y = -Radius * Width; y <= Radius * Width; y += Width)
		for (signed int x = -Radius * 4; x <= Radius * 4; x += 4)
			if (SP[y + x]>CenterVal)
				return 0;
	return 1;
}

int IsValidPixelPos(int x, int y, int Width, int Height)
{
	if (x < 0 || x >= Width)
		return 0;
	if (y< 0 || y >= Height)
		return 0;
	return 1;
}

int IsLocalMaximaSafe(unsigned int *SP, int x1, int y1, int Width, int Height, int Radius)
{
	int CenterVal = SP[0];
	for (signed int y = -Radius; y <= Radius; y++)
		for (signed int x = -Radius * 4; x <= Radius * 4; x += 4)
			if (IsValidPixelPos(x + x1, y + y1, Width, Height) && (int)SP[(y + y1)*Width + x + x1]>CenterVal)
				return 0;
	return 1;
}

void WINAPI EdgeKeepLocalMaximaMaximaOnly(int Radius)
{
	if (CurScreenshot == NULL)
		return;
	if (Radius <= 0 || Radius > CurScreenshot->GetWidth() / 4 || Radius > CurScreenshot->GetHeight() / 4)
		Radius = 1;
	int Width = CurScreenshot->GetWidth();
	int Height = CurScreenshot->GetHeight();
	int *Dest = (int*)MY_ALLOC(Width * Height * sizeof(int));
	memcpy(Dest, CurScreenshot->Pixels, Width * Height * sizeof(int));
	//first rows
	for (int y = 0; y < Radius; y++)
	{
		int *BaseSrc = (int*)&CurScreenshot->Pixels[y*Width];
		int *BaseDest = &Dest[y*Width];
		for (int x = 0; x < Width; x++)
			if (IsLocalMaximaSafe((unsigned int*)CurScreenshot->Pixels, x, y, Width, Height, Radius) == 0)
				BaseDest[x] = 0;
	}
	//bottom rows
	for (int y = Height - Radius; y < Height; y++)
	{
		int *BaseSrc = (int*)&CurScreenshot->Pixels[y*Width];
		int *BaseDest = &Dest[y*Width];
		for (int x = 0; x < Width; x++)
			if (IsLocalMaximaSafe((unsigned int*)CurScreenshot->Pixels, x, y, Width, Height, Radius) == 0)
				BaseDest[x] = 0;
	}
	//first and last columns 
	for (int y = Radius; y < Height - Radius; y++)
	{
		int *BaseSrc = (int*)&CurScreenshot->Pixels[y*Width];
		int *BaseDest = &Dest[y*Width];
		for (int x = 0; x < Radius; x++)
			if (IsLocalMaximaSafe((unsigned int*)CurScreenshot->Pixels, x, y, Width, Height, Radius) == 0)
				BaseDest[x] = 0;
		for (int x = Width - Radius; x < Width; x++)
			if (IsLocalMaximaSafe((unsigned int*)CurScreenshot->Pixels, x, y, Width, Height, Radius) == 0)
				BaseDest[x] = 0;
	}
	//handle the rest
	for (int y = Radius; y < Height - Radius; y++)
		{
			int *BaseSrc = (int*)&CurScreenshot->Pixels[y*Width];
			int *BaseDest = &Dest[y*Width];
			for (int x = Radius; x < Width - Radius; x++)
				if (IsLocalMaxima((unsigned char*)&BaseSrc[x], Width, Radius) == 0)
					BaseDest[x] = 0;
		}
	//copy back Dest
	memcpy(CurScreenshot->Pixels, Dest, Width * Height * sizeof(int));
	// free Villie
	MY_FREE(Dest);
}

// Have to check why this does not produce as expected !
void WINAPI EdgeCopyOriginalForEdges()
{
	if (CurScreenshot == NULL)
		return;
	if (PrevScreenshot == NULL)
		return;
	if (PrevScreenshot->GetWidth() != CurScreenshot->GetWidth() || PrevScreenshot->GetHeight() != CurScreenshot->GetHeight())
		return;

	int Width = CurScreenshot->GetWidth();
	int Height = CurScreenshot->GetHeight();
	int PixelCount = Width * Height;
	for (int i = 0; i < PixelCount; i++)
		if (CurScreenshot->Pixels[i] != 0)
			CurScreenshot->Pixels[i] = PrevScreenshot->Pixels[i];
		else
			CurScreenshot->Pixels[i] = TRANSPARENT_COLOR;
}