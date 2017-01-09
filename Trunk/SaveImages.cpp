#include "StdAfx.h"

static int ImageFileAutoIncrement = 0;
void SaveScreenshot_(ScreenshotStruct	*CurScreenshot)
{
	FileDebug( "Started saving the screenshot" );

	if( CurScreenshot->Pixels == NULL )
	{
		FileDebug( "WARNING:Screenshot buffer is null when trying to save it to file!" );
		return;
	}
	int Width = CurScreenshot->Right - CurScreenshot->Left;
	int Height = CurScreenshot->Bottom - CurScreenshot->Top;
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
	if( CurScreenshot->BytesPerPixel == 1 )
	{
		unsigned char *Pixels = (unsigned char*)CurScreenshot->Pixels;
		for( int y = 0; y < Height; y +=1 )
			for( int x = 0; x < Width; x += 1 )
				Img.SetPixel( x, y, RGB( Pixels[ y * Width + x ], Pixels[ y * Width + x ], Pixels[ y * Width + x ] ) );
	}
	else
	{
		for( int y = 0; y < Height; y +=1 )
			for( int x = 0; x < Width; x += 1 )
				Img.SetPixel( x, y, RGB( GetBValue( CurScreenshot->Pixels[ y * Width + x ] ), GetGValue( CurScreenshot->Pixels[ y * Width + x ] ), GetRValue( CurScreenshot->Pixels[ y * Width + x ] ) ) );
	}

	Img.Save( MyFileName );
}

void WINAPI SaveScreenshot()
{
	SaveScreenshot_(CurScreenshot);
}

void WINAPI SavePrevScreenshot()
{
	SaveScreenshot_(PrevScreenshot);
}

void WINAPI SaveDiffMap()
{
	FileDebug( "Started saving diffmap" );
	DumpAsPPM( (unsigned char *)MotionDiff.Pixels, (unsigned char *)MotionDiff.Pixels, (unsigned char *)MotionDiff.Pixels, MotionDiff.GetWidth(), MotionDiff.GetHeight() );
	FileDebug( "\tFinished saving diffmap" );
}

void WINAPI SaveEdgeMap()
{
	FileDebug( "Started saving diffmap" );
	DumpAsPPM( (unsigned char *)CurScreenshot->Pixels, (unsigned char *)CurScreenshot->Pixels, (unsigned char *)CurScreenshot->Pixels, CurScreenshot->GetWidth(), CurScreenshot->GetHeight() );
	FileDebug( "\tFinished saving diffmap" );
}

void WINAPI SaveScreenshotDiffMask( int LowLimit )
{

	unsigned char *Mask = (unsigned char *)MotionDiff.Pixels;
	if( Mask == NULL )
	{
		FileDebug( "Exiting SaveScreenshotDiffMask due to missing diff map" );
		return;
	}

	int Width = CurScreenshot->Right - CurScreenshot->Left;
	int Height = CurScreenshot->Bottom - CurScreenshot->Top;

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
		for( int x=0;x<Width;x++)
			if( Mask[ y / 4 * Width / 4 + x / 4 ] > LowLimit )
			{
				int R = GetRValue( CurScreenshot->Pixels[ y * Width + x ] );
				int G = GetGValue( CurScreenshot->Pixels[ y * Width + x ] );
				int B = GetBValue( CurScreenshot->Pixels[ y * Width + x ] );
				fwrite( &R, 1, 1, fp);
				fwrite( &G, 1, 1, fp);
				fwrite( &B, 1, 1, fp);
			}
			else
			{
				fwrite( &Black, 1, 1, fp);
				fwrite( &Black, 1, 1, fp);
				fwrite( &Black, 1, 1, fp);
			}
	fclose(fp);
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

//	char Black = 0;
	FILE *fp = fopen( MyFileName, "wb");
	fprintf(fp, "P6\n%d %d\n255\n", Width, Height );
	for( int y=0;y<Height;y++)
		for( int x=0;x<Width;x++)
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

void DumpAsPPMBGR( LPCOLORREF BGR, int Width, int Height )
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

//	char Black = 0;
	FILE *fp = fopen( MyFileName, "wb");
	fprintf(fp, "P6\n%d %d\n255\n", Width, Height );
	for( int y=0;y<Height;y++)
		for( int x=0;x<Height;x++)
	{
		int R = GetRValue( BGR[ y * Width + x ] );
		int G = GetGValue( BGR[ y * Width + x ] );
		int B = GetBValue( BGR[ y * Width + x ] );
		fwrite( &B, 1, 1, fp);
		fwrite( &G, 1, 1, fp);
		fwrite( &R, 1, 1, fp);
	}
	fclose(fp);
}

void WINAPI SaveScreenshotCutTransparent()
{
	FileDebug("Started saving the screenshot");

	if (CurScreenshot->Pixels == NULL)
	{
		FileDebug("WARNING:Screenshot buffer is null when trying to save it to file!");
		return;
	}
	int CountRowsStartTransparent = 0;
	int CountRowsEndTransparent = 0;
	int CountColsStartTransparent = 0;
	int CountColsEndTransparent = 0;
	COLORREF TransparentColor = TRANSPARENT_COLOR;
	//detect rows at the beginning that are transparent
	for (int i = 0; i < CurScreenshot->GetHeight(); i++)
	{
		int IsRowTransparent = 1;
		for (int j = 0; j < CurScreenshot->GetWidth(); j++)
			if (CurScreenshot->GetPixel(j, i) != TransparentColor)
			{
				IsRowTransparent = 0;
				break;
			}
		if (IsRowTransparent == 0)
			break;
		CountRowsStartTransparent++;
	}
	//detect rows at the end that are transparent
	for (int i = CurScreenshot->GetHeight() - 1; i > CountRowsStartTransparent; i--)
	{
		int IsRowTransparent = 1;
		for (int j = 0; j < CurScreenshot->GetWidth(); j++)
			if (CurScreenshot->GetPixel(j, i) != TransparentColor)
			{
				IsRowTransparent = 0;
				break;
			}
		if (IsRowTransparent == 0)
			break;
		CountRowsEndTransparent++;
	}
	//detect columns at the beginning that are transparent
	for (int i = 0; i < CurScreenshot->GetWidth(); i++)
	{
		int IsColTransparent = 1;
		for (int j = 0; j < CurScreenshot->GetHeight(); j++)
			if (CurScreenshot->GetPixel(i, j) != TransparentColor)
			{
				IsColTransparent = 0;
				break;
			}
		if (IsColTransparent == 0)
			break;
		CountColsStartTransparent++;
	}
	//detect columns at the end that are transparent
	for (int i = CurScreenshot->GetWidth() - 1; i > CountColsStartTransparent; i--)
	{
		int IsColTransparent = 1;
		for (int j = 0; j < CurScreenshot->GetHeight(); j++)
			if (CurScreenshot->GetPixel(i, j) != TransparentColor)
			{
				IsColTransparent = 0;
				break;
			}
		if (IsColTransparent == 0)
			break;
		CountColsEndTransparent++;
	}

	int Width = CurScreenshot->Right - CurScreenshot->Left - CountColsStartTransparent - CountColsEndTransparent;
	int Height = CurScreenshot->Bottom - CurScreenshot->Top - CountRowsStartTransparent - CountRowsEndTransparent;
	//find an available file name
	char MyFileName[DEFAULT_STR_BUFFER_SIZE];
	BOOL FileExists;
	do {
		sprintf_s(MyFileName, DEFAULT_STR_BUFFER_SIZE, "Screenshot_%04d_%04d_%04d.bmp", ImageFileAutoIncrement, Width, Height);
		FileExists = (_access(MyFileName, 0) == 0);
		ImageFileAutoIncrement++;
	} while (FileExists == TRUE);

	FileDebug("chosen filename is :");
	FileDebug(MyFileName);

	//create a bitmap and populate pixels on it
	CImage Img;
	Img.Create(Width, Height, 32);

	unsigned char *Pixels = (unsigned char*)CurScreenshot->Pixels;
	for (int y = 0; y < Height; y += 1)
		for (int x = 0; x < Width; x += 1)
		{
			COLORREF Pixel = CurScreenshot->GetPixel(CountColsStartTransparent + x, CountRowsStartTransparent + y);
			Img.SetPixel(x, y, RGB(GetBValue(Pixel), GetGValue(Pixel), GetRValue(Pixel)));
		}

	Img.Save(MyFileName);
}