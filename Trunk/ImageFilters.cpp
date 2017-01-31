#include "StdAfx.h"

void WINAPI BlurrImage( int HalfKernelSize )
{
	FileDebug( "Started bluring screenshot" );
	if( CurScreenshot->Pixels == NULL )
	{
		FileDebug( "WARNING:Screenshot buffer is null when trying to blur it!" );
		return;
	}
	int Width = CurScreenshot->Right - CurScreenshot->Left;
	int Height = CurScreenshot->Bottom - CurScreenshot->Top;
	LPCOLORREF new_Pixels = (COLORREF*)_aligned_malloc( Width * Height * sizeof( COLORREF ) + SSE_PADDING, SSE_ALIGNMENT );
	if( new_Pixels == NULL )
	{
		FileDebug( "Error:Could not allocate buffer for blur!" );
		return;
	}

	if( HalfKernelSize == 1 )
	{
		//37 fps
		for( int y = 1; y < Height-1; y +=1 )
			for( int x = 1; x < Width-1; x += 1 )
			{
				int SumOfValuesRB = 0;
				int SumOfValuesG8 = 0;
				LPCOLORREF BoxStart = &CurScreenshot->Pixels[ y * Width + x ];
				for(int ky=-1;ky<=1;ky++)
					for(int kx=-1;kx<=1;kx++)
					{
						SumOfValuesRB += ( BoxStart[ ky * Width + kx ] & 0x00FF00FF );	//remove G and sum RB
						SumOfValuesG8 += ( BoxStart[ ky * Width + kx ] & 0x0000FF00);
					}
				int SumOfValuesR = ( SumOfValuesRB >> 0 ) & 0x0000FFFF;
				int SumOfValuesG = ( SumOfValuesG8 >> 8 ) & 0x0000FFFF;
				int SumOfValuesB = ( SumOfValuesRB >> 16 ) & 0x0000FFFF;
				new_Pixels[ y * Width + x ] = RGB( SumOfValuesR / 9, SumOfValuesG / 9, SumOfValuesB / 9 );
			}
			/**/
		//32 fps
/*		int CharWidth = Width * 4;
		for( int y = 1; y < Height-1; y +=1 )
			for( int x = 1; x < Width-1; x += 1 )
			{
				unsigned char *RowStart = (unsigned char *)&CurScreenshot->Pixels[ ( y - 1 ) * Width + x - 1 ];
				int SumOfValuesR = 0;
				int SumOfValuesG = 0;
				int SumOfValuesB = 0;
				for(int ky=0;ky<=2;ky++)
					for(int kx=0;kx<=2*4;kx+=4)
					{
						SumOfValuesR += RowStart[ ky * CharWidth + kx + 0 ];
						SumOfValuesG += RowStart[ ky * CharWidth + kx + 1 ];
						SumOfValuesB += RowStart[ ky * CharWidth + kx + 2 ];
					}
				new_Pixels[ y * Width + x ] = RGB( SumOfValuesR / 9, SumOfValuesG / 9, SumOfValuesB / 9 );
			} */
	}
	else
	{
		int KernelPixelCount = ( HalfKernelSize * 2 + 1 ) * ( HalfKernelSize * 2 + 1 );
		for( int y = HalfKernelSize; y < Height-HalfKernelSize; y +=1 )
			for( int x = HalfKernelSize; x < Width-HalfKernelSize; x += 1 )
			{
				int SumOfValuesRB = 0;
				int SumOfValuesG = 0;
				LPCOLORREF BoxStart = &CurScreenshot->Pixels[ y * Width + x ];
				for(int ky=-HalfKernelSize;ky<=HalfKernelSize;ky++)
					for(int kx=-HalfKernelSize;kx<=HalfKernelSize;kx++)
					{
						SumOfValuesRB += ( BoxStart[ ky * Width + kx ] & 0x00FF00FF );	//remove G and sum RB
						SumOfValuesG += GetGValue( BoxStart[ ky * Width + kx ] );
					}
				int SumOfValuesR = ( SumOfValuesRB >> 0 ) & 0x0000FFFF;
				int SumOfValuesB = ( SumOfValuesRB >> 16 ) & 0x0000FFFF;
				new_Pixels[ y * Width + x ] = RGB( SumOfValuesR / KernelPixelCount, SumOfValuesG / KernelPixelCount, SumOfValuesB / KernelPixelCount );
			}
	}

	_aligned_free( CurScreenshot->Pixels );
	CurScreenshot->Pixels = new_Pixels;
	FileDebug( "Finished bluring screenshot" );
}

//forget small moving objects
void WINAPI ErrodeDiffMap( int HalfKernelSize )
{
	FileDebug( "Started eroding diffmap" );
	if( CurScreenshot->Pixels == NULL )
	{
		FileDebug( "WARNING:Screenshot buffer is null when trying to blur it!" );
		return;
	}
	int Width = MotionDiff.GetWidth();
	int Height = MotionDiff.GetHeight();
	int AllocSize = Width * Height * sizeof( COLORREF ) + SSE_PADDING;
	LPCOLORREF new_Pixels = (COLORREF*)_aligned_malloc( AllocSize, SSE_ALIGNMENT );
	if( new_Pixels == NULL )
	{
		FileDebug( "Error:Could not allocate buffer for blur!" );
		return;
	}
	memset( new_Pixels, 0, AllocSize );

	unsigned char *Mask = (unsigned char *)MotionDiff.Pixels;
	unsigned char *Dst = (unsigned char *)new_Pixels;

	int KernelPixelCount = ( HalfKernelSize * 2 + 1 ) * ( HalfKernelSize * 2 + 1 );

	for( int y = HalfKernelSize; y < Height-HalfKernelSize; y +=1 )
		for( int x = HalfKernelSize; x < Width-HalfKernelSize; x += 1 )
		{
			int NrValues = 0;
			for(int ky=-HalfKernelSize;ky<=HalfKernelSize;ky++)
				for(int kx=-HalfKernelSize;kx<=HalfKernelSize;kx++)
					if( Mask[ ( y + ky ) * Width + x + kx ] > 0 )
						NrValues++;

			if( NrValues >= KernelPixelCount / 2 )
//			if( NrValues >= 1 )	//this will make regions bigger
				Dst[ y * Width + x ] = 128;
			else
				Dst[ y * Width + x ] = 0;
		}

	_aligned_free( MotionDiff.Pixels );
	MotionDiff.Pixels = new_Pixels;
	FileDebug( "Finished eroding diffmap" );
}

void WINAPI EdgeDetect( int HalfKernelSize )
{
	FileDebug( "Started edge detecting screenshot" );
	if( CurScreenshot->Pixels == NULL )
	{
		FileDebug( "WARNING:Screenshot buffer is null when trying to blur it!" );
		return;
	}
	int Width = CurScreenshot->Right - CurScreenshot->Left;
	int Height = CurScreenshot->Bottom - CurScreenshot->Top;
	char *new_Pixels = (char*)_aligned_malloc( Width * Height * sizeof( COLORREF ) + SSE_PADDING, SSE_ALIGNMENT );
	if( new_Pixels == NULL )
	{
		FileDebug( "Error:Could not allocate buffer for blur!" );
		return;
	}

	//sobel
	if( HalfKernelSize == 2 )
	{
		for( int y = 2; y < Height - 1; y +=1 )
			for( int x = 2; x < Width - 1; x += 1 )
			{
				int p[10];
				for( int ty = -1; ty <= 1; ty++)
					for( int tx = -1; tx <= 1; tx++)
						p[ 1 + ( ty + 1) * 3 + ( tx + 1 ) ] = GetRValue( CurScreenshot->Pixels[ ( y + ty ) * Width + x + tx ] );
				int EdgeR = abs((p[1]+2*p[2]+p[3])-(p[7]+2*p[8]+p[9]))+abs((p[3]+2*p[6]+p[9])-(p[1]+2*p[4]+p[7])) / 8;

				for( int ty = -1; ty <= 1; ty++)
					for( int tx = -1; tx <= 1; tx++)
						p[ 1 + ( ty + 1) * 3 + ( tx + 1 ) ] = GetGValue( CurScreenshot->Pixels[ ( y + ty ) * Width + x + tx ] );
				int EdgeG = abs((p[1]+2*p[2]+p[3])-(p[7]+2*p[8]+p[9]))+abs((p[3]+2*p[6]+p[9])-(p[1]+2*p[4]+p[7])) / 8;

				for( int ty = -1; ty <= 1; ty++)
					for( int tx = -1; tx <= 1; tx++)
						p[ 1 + ( ty + 1) * 3 + ( tx + 1 ) ] = GetBValue( CurScreenshot->Pixels[ ( y + ty ) * Width + x + tx ] );
				int EdgeB = abs((p[1]+2*p[2]+p[3])-(p[7]+2*p[8]+p[9]))+abs((p[3]+2*p[6]+p[9])-(p[1]+2*p[4]+p[7])) / 8;

				int Edge = EdgeR;
				if( EdgeG > Edge )
					Edge = EdgeG;
				if( EdgeB > Edge )
					Edge = EdgeB;
				new_Pixels[ y * Width + x ] = Edge;
			}
	}
	else if( HalfKernelSize == 1 )
	{
		//robert cross
		for( int y = 1; y < Height; y +=1 )
			for( int x = 1; x < Width; x += 1 )
			{
				int p[4];
				p[0] = GetRValue( CurScreenshot->Pixels[ ( y - 1 ) * Width + x - 1 ] );
				p[1] = GetRValue( CurScreenshot->Pixels[ ( y - 1 ) * Width + x - 0 ] );
				p[2] = GetRValue( CurScreenshot->Pixels[ ( y - 0 ) * Width + x - 1 ] );
				p[3] = GetRValue( CurScreenshot->Pixels[ ( y - 0 ) * Width + x - 0 ] );
				int EdgeR = abs( p[0] - p[3] ) + abs( p[1] - p[2] ) / 2;
				p[0] = GetGValue( CurScreenshot->Pixels[ ( y - 1 ) * Width + x - 1 ] );
				p[1] = GetGValue( CurScreenshot->Pixels[ ( y - 1 ) * Width + x - 0 ] );
				p[2] = GetGValue( CurScreenshot->Pixels[ ( y - 0 ) * Width + x - 1 ] );
				p[3] = GetGValue( CurScreenshot->Pixels[ ( y - 0 ) * Width + x - 0 ] );
				int EdgeG = abs( p[0] - p[3] ) + abs( p[1] - p[2] ) / 2;
				p[0] = GetBValue( CurScreenshot->Pixels[ ( y - 1 ) * Width + x - 1 ] );
				p[1] = GetBValue( CurScreenshot->Pixels[ ( y - 1 ) * Width + x - 0 ] );
				p[2] = GetBValue( CurScreenshot->Pixels[ ( y - 0 ) * Width + x - 1 ] );
				p[3] = GetBValue( CurScreenshot->Pixels[ ( y - 0 ) * Width + x - 0 ] );
				int EdgeB = abs( p[0] - p[3] ) + abs( p[1] - p[2] ) / 2;
				int Edge = EdgeR;
				if( EdgeG > Edge )
					Edge = EdgeG;
				if( EdgeB > Edge )
					Edge = EdgeB;
				new_Pixels[ y * Width + x ] = Edge;
			}
	}
	_aligned_free( CurScreenshot->Pixels );
	CurScreenshot->Pixels = (LPCOLORREF)new_Pixels;
	CurScreenshot->BytesPerPixel = 1;
	FileDebug( "Finished bluring screenshot" );
}

void WINAPI KeepColorSetRest(int SetRest, int Color1)
{
	FileDebug("Started KeepColorSetRest");
	if (CurScreenshot->Pixels == NULL)
	{
		FileDebug("WARNING:Screenshot buffer is null when trying to extract color!");
		return;
	}
	int Width = CurScreenshot->Right - CurScreenshot->Left;
	int Height = CurScreenshot->Bottom - CurScreenshot->Top;
	for (int y = 1; y < Height; y += 1)
		for (int x = 1; x < Width; x += 1)
			if (CurScreenshot->Pixels[y * Width + x] != Color1)
				CurScreenshot->Pixels[y * Width + x] = SetRest;

	FileDebug("Finished KeepColorSetRest");
}

void WINAPI ApplyColorBitmask(int Mask)
{
	int PixelCount = CurScreenshot->GetWidth() * CurScreenshot->GetHeight();
	for (int i = 0; i < PixelCount; i++)
		CurScreenshot->Pixels[i] = CurScreenshot->Pixels[i] & Mask;
}

void DecreaseColorCount_(ScreenshotStruct *cache, unsigned int ColorsPerChannel)
{
	int PixelCount = cache->GetWidth() * cache->GetHeight();
	int ColorStep = 255 / ColorsPerChannel; // only valid for 8bpp. Which we intend ot use
	int ColorStepHalf = ColorStep / 2; // because we use rounding
	for (int i = 0; i < PixelCount; i++)
	{
		int Colors[3];
		Colors[0] = GetRValue(cache->Pixels[i]);
		Colors[1] = GetGValue(cache->Pixels[i]);
		Colors[2] = GetBValue(cache->Pixels[i]);
		for (int j = 0; j < 3; j++)
		{
			int NewC = (Colors[j] / ColorsPerChannel) * ColorsPerChannel;
			if (Colors[j] - NewC >= ColorStepHalf)
				Colors[j] = NewC + 1; // round up
			else
				Colors[j] = NewC;
		}
		cache->Pixels[i] = RGB(Colors[0], Colors[1], Colors[2]);
	}
}

void WINAPI DecreaseColorCount(unsigned int ColorsPerChannel)
{
	DecreaseColorCount_(CurScreenshot, ColorsPerChannel);
}

void RemoveScreenshotAlphaChannel(ScreenshotStruct *cache)
{
	if (cache->NeedsAlphaRemoved == true)
	{
		cache->NeedsAlphaRemoved = false;
		int PixelCount = cache->GetWidth() * cache->GetHeight();
		for (int i = 0; i<PixelCount; i++)
			cache->Pixels[i] = cache->Pixels[i] & REMOVE_ALPHA_CHANNEL_MASK;
	}
}

void DecreaseColorPrecision(ScreenshotStruct *cache, unsigned int Div, unsigned int And)
{
	int PixelCount = cache->GetWidth() * cache->GetHeight();
	for (int i = 0; i < PixelCount; i++)
	{
		int Colors[3];
		Colors[0] = GetRValue(cache->Pixels[i]);
		Colors[1] = GetGValue(cache->Pixels[i]);
		Colors[2] = GetBValue(cache->Pixels[i]);
		if (Div != 0)
		{
			Colors[0] = Colors[0] / Div;
			Colors[1] = Colors[1] / Div;
			Colors[2] = Colors[2] / Div;
		}
		if (And != 0)
		{
			Colors[0] = Colors[0] & And;
			Colors[1] = Colors[1] & And;
			Colors[2] = Colors[2] & And;
		}
		cache->Pixels[i] = RGB(Colors[0], Colors[1], Colors[2]);
	}
}
