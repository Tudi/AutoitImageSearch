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
	LPCOLORREF new_Pixels = (COLORREF*)malloc( Width * Height * sizeof( COLORREF ) );
	if( new_Pixels == NULL )
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
					SumOfValuesR += GetRValue( CurScreenshot->Pixels[ ( y + ky ) * Width + x + kx ] );
					SumOfValuesG += GetGValue( CurScreenshot->Pixels[ ( y + ky ) * Width + x + kx ] );
					SumOfValuesB += GetBValue( CurScreenshot->Pixels[ ( y + ky ) * Width + x + kx ] );
				}
			new_Pixels[ y * Width + x ] = RGB( SumOfValuesR / KernelPixelCount, SumOfValuesG / KernelPixelCount, SumOfValuesB / KernelPixelCount );
		}

	free( CurScreenshot->Pixels );
	CurScreenshot->Pixels = new_Pixels;
	FileDebug( "Finished bluring screenshot" );
}
