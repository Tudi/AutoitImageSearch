#include "StdAfx.h"

void WINAPI BlurrImage( int HalfKernelSize )
{
	FileDebug( "Started bluring screenshot" );
	if( ScreenshotPixels == NULL )
	{
		FileDebug( "WARNING:Screenshot buffer is null when trying to blur it!" );
		return;
	}
	int Width = ScreenshotRight - ScreenshotLeft;
	int Height = ScreenshotBottom - ScreenshotTop;
	LPCOLORREF new_ScreenshotPixels = (COLORREF*)malloc( Width * Height * sizeof( COLORREF ) );
	if( new_ScreenshotPixels == NULL )
	{
		FileDebug( "Error:Could not allocate buffer for blur!" );
		return;
	}
	int KernelPixelCount = ( HalfKernelSize * 2 + 1 ) * ( HalfKernelSize * 2 + 1 );

	for( int y = HalfKernelSize; y < Height-HalfKernelSize; y +=1 )
		for( int x = HalfKernelSize; x < Width-HalfKernelSize; x += 1 )
		{
			int SumOfValuesR = 0;
			int SumOfValuesG = 0;
			int SumOfValuesB = 0;
			for(int ky=-HalfKernelSize;ky<=HalfKernelSize;ky++)
				for(int kx=-HalfKernelSize;kx<=HalfKernelSize;kx++)
				{
					SumOfValuesR += GetRValue( ScreenshotPixels[ ( y + ky ) * Width + x + kx ] );
					SumOfValuesG += GetGValue( ScreenshotPixels[ ( y + ky ) * Width + x + kx ] );
					SumOfValuesB += GetBValue( ScreenshotPixels[ ( y + ky ) * Width + x + kx ] );
				}
			new_ScreenshotPixels[ y * Width + x ] = RGB( SumOfValuesR / KernelPixelCount, SumOfValuesG / KernelPixelCount, SumOfValuesB / KernelPixelCount );
		}

	free( ScreenshotPixels );
	ScreenshotPixels = new_ScreenshotPixels;
	FileDebug( "Finished bluring screenshot" );
}
