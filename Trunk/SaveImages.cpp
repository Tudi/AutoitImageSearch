#include "StdAfx.h"

static int ImageFileAutoIncrement = 0;
void WINAPI SaveScreenshot()
{
	FileDebug( "Started saving the screenshot" );

	if( ScreenshotPixels == NULL )
	{
		FileDebug( "WARNING:Screenshot buffer is null when trying to save it to file!" );
		return;
	}
	int Width = ScreenshotRight - ScreenshotLeft;
	int Height = ScreenshotBottom - ScreenshotTop;
	//find an available file name
	char MyFileName[DEFAULT_STR_BUFFER_SIZE];
	BOOL FileExists;
	do {
		sprintf_s( MyFileName, DEFAULT_STR_BUFFER_SIZE, "%s_%04d_%04d_%04d.bmp", "Screenshot", ImageFileAutoIncrement, Width, Height );
		FileExists = ( _access( MyFileName, 0 ) == 0 );
		ImageFileAutoIncrement++;
	}while( FileExists == TRUE );

	FileDebug( "chosen filename is :" );
	FileDebug( MyFileName );

	//create a bitmap and populate pixels on it
	CImage Img;
	Img.Create( Width, Height, 32 );
	for( int y = 0; y < Height; y +=1 )
		for( int x = 0; x < Width; x += 1 )
//			Img.SetPixel( x, y, ScreenshotPixels[ y * Width + x ] );
			Img.SetPixel( x, y, RGB( GetBValue( ScreenshotPixels[ y * Width + x ] ), GetGValue( ScreenshotPixels[ y * Width + x ] ), GetRValue( ScreenshotPixels[ y * Width + x ] ) ) );

	Img.Save( MyFileName );
}

void DumpAsPPM( unsigned char *R,unsigned char *G,unsigned char *B, int Width, int Height )
{
	//find an available file name
	char MyFileName[DEFAULT_STR_BUFFER_SIZE];
	BOOL FileExists;
	int ImageFileAutoIncrement = 0;
	do {
		sprintf_s( MyFileName, DEFAULT_STR_BUFFER_SIZE, "%s_%04d_%04d_%04d.ppm", "dump", ImageFileAutoIncrement, Width, Height );
		FileExists = ( _access( MyFileName, 0 ) == 0 );
		ImageFileAutoIncrement++;
	}while( FileExists == TRUE );

	char Black = 0;
	FILE *fp = fopen( MyFileName, "wb");
	fprintf(fp, "P6\n%d %d\n255\n", Width, Height );
	for( int i=0;i<Width*Height;i++)
	{
		if( R )
			fwrite( &R[i], 1, 1, fp);
		else
			fwrite( &Black, 1, 1, fp);
		if( G )
			fwrite( &G[i], 1, 1, fp);
		else
			fwrite( &Black, 1, 1, fp);
		if( B )
			fwrite( &B[i], 1, 1, fp);
		else
			fwrite( &Black, 1, 1, fp);
	}
	fclose(fp);
}

void DumpAsPPM( LPCOLORREF RGB, int Width, int Height )
{
	DumpAsPPM( RGB, Width, Height, Width );
}

void DumpAsPPM( LPCOLORREF RGB, int Width, int Height, int Stride )
{
	//find an available file name
	char MyFileName[DEFAULT_STR_BUFFER_SIZE];
	BOOL FileExists;
	int ImageFileAutoIncrement = 0;
	do {
		sprintf_s( MyFileName, DEFAULT_STR_BUFFER_SIZE, "%s_%04d_%04d_%04d.ppm", "dump", ImageFileAutoIncrement, Width, Height );
		FileExists = ( _access( MyFileName, 0 ) == 0 );
		ImageFileAutoIncrement++;
	}while( FileExists == TRUE );

	char Black = 0;
	FILE *fp = fopen( MyFileName, "wb");
	fprintf(fp, "P6\n%d %d\n255\n", Width, Height );
	for( int y=0;y<Height;y++)
		for( int x=0;x<Height;x++)
	{
		int R = GetRValue( RGB[ y * Stride + x ] );
		int G = GetGValue( RGB[ y * Stride + x ] );
		int B = GetBValue( RGB[ y * Stride + x ] );
		fwrite( &R, 1, 1, fp);
		fwrite( &G, 1, 1, fp);
		fwrite( &B, 1, 1, fp);
	}
	fclose(fp);
}