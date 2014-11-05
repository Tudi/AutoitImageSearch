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
	LPCOLORREF new_Pixels = (COLORREF*)malloc( Width * Height * sizeof( COLORREF ) + SSE_PADDING );
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

//forget small moving objects
void WINAPI ErrodeDiffMap( int HalfKernelSize )
{
	FileDebug( "Started eroding diffmap" );
	if( CurScreenshot->Pixels == NULL )
	{
		FileDebug( "WARNING:Screenshot buffer is null when trying to blur it!" );
		return;
	}
	int Width = ( MotionDiff.Right - MotionDiff.Left );
	int Height = ( MotionDiff.Bottom - MotionDiff.Top );
	int AllocSize = Width * Height * sizeof( COLORREF ) + SSE_PADDING;
	LPCOLORREF new_Pixels = (COLORREF*)malloc( AllocSize );
	if( new_Pixels == NULL )
	{
		FileDebug( "Error:Could not allocate buffer for blur!" );
		return;
	}
	memset( new_Pixels, 0, AllocSize );

	unsigned char *Mask = (unsigned char *)MotionDiff.Pixels;
	unsigned char *Dst = (unsigned char *)new_Pixels;
	Width = Width / 4;
	Height = Height / 4;
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

	free( MotionDiff.Pixels );
	MotionDiff.Pixels = new_Pixels;
	FileDebug( "Finished eroding diffmap" );
}