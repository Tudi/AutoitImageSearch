#include "StdAfx.h"

#define MAX_PICTURE_CACHE_COUNT 500

__declspec(dllexport) LPCOLORREF ScreenshotPixels = NULL;
__declspec(dllexport) int ScreenshotLeft, ScreenshotTop, ScreenshotRight, ScreenshotBottom;
__declspec(dllexport) CachedPicture PictureCache[MAX_PICTURE_CACHE_COUNT];
__declspec(dllexport) int NrPicturesCached = 0;

void FileDebug( char *what )
{
#ifdef _DEBUG
	FILE *f = fopen( "debug.txt", "at" );
	fprintf( f, "%s\n", what );
	fclose( f );
#endif
}

void WINAPI ReleaseScreenshot()
{
	if( ScreenshotPixels )
		free( ScreenshotPixels );
	ScreenshotPixels = NULL;
}

void TakeNewScreenshot( int aLeft, int aTop, int aRight, int aBottom )
{
	ScreenshotLeft = aLeft;
	ScreenshotTop = aTop;
	ScreenshotRight = aRight;
	ScreenshotBottom = aBottom;

	HDC sdc = NULL;
	HBITMAP hbitmap_screen = NULL;
	HGDIOBJ sdc_orig_select = NULL;
	COLORREF trans_color = CLR_NONE; // The default must be a value that can't occur naturally in an image.
    
	// Create an empty bitmap to hold all the pixels currently visible on the screen that lie within the search area:
	int search_width = aRight - aLeft;
	int search_height = aBottom - aTop;

	HDC hdc = GetDC(NULL);
	if( !hdc )
	{
		return;
	}

	if( !(sdc = CreateCompatibleDC(hdc)) || !(hbitmap_screen = CreateCompatibleBitmap(hdc, search_width, search_height)) )
		goto end;

	if( !(sdc_orig_select = SelectObject(sdc, hbitmap_screen))   )
		goto end;

	// Copy the pixels in the search-area of the screen into the DC to be searched:
	if( !(BitBlt(sdc, 0, 0, search_width, search_height, hdc, aLeft, aTop, SRCCOPY)) )
		goto end;

	LONG screen_width, screen_height;
	bool screen_is_16bit;
	if( !(ScreenshotPixels = getbits(hbitmap_screen, sdc, screen_width, screen_height, screen_is_16bit)) )
		goto end;

	LONG ScreenshotPixels_count = screen_width * screen_height;

	// If either is 16-bit, convert *both* to the 16-bit-compatible 32-bit format:
	if( screen_is_16bit )
	{
		if (trans_color != CLR_NONE)
			trans_color &= 0x00F8F8F8; // Convert indicated trans-color to be compatible with the conversion below.
		for (int i = 0; i < ScreenshotPixels_count; ++i)
			ScreenshotPixels[i] &= 0x00F8F8F8; // Highest order byte must be masked to zero for consistency with use of 0x00FFFFFF below.
	}

end:
	// If found==false when execution reaches here, ErrorLevel is already set to the right value, so just
	// clean up then return.
	ReleaseDC(NULL, hdc);
	if (sdc)
	{
		if (sdc_orig_select) // i.e. the original call to SelectObject() didn't fail.
			SelectObject(sdc, sdc_orig_select); // Probably necessary to prevent memory leak.
		DeleteDC(sdc);
	}
	if (hbitmap_screen)
		DeleteObject(hbitmap_screen);
}

void WINAPI TakeScreenshot( int aLeft, int aTop, int aRight, int aBottom )
{
	FileDebug( "Started taking the screenshot" );
	if( ScreenshotPixels != NULL )
	{
		ReleaseScreenshot();
		ScreenshotPixels = NULL;
	}
	if( ScreenshotPixels == NULL )
	{
		TakeNewScreenshot( aLeft, aTop, aRight, aBottom );
	}

	FileDebug( "Finished taking the screenshot" );
//	if( ScreenshotPixels == NULL )
//		FileDebug( "WARNING:Screenshot buffer is null when taking the screenshot!" );

	return ;
}

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

void WINAPI ResizeScreenshot( int NewWidth, int NewHeight )
{
	FileDebug( "Started resizing the screenshot" );
	if( ScreenshotPixels == NULL )
	{
		FileDebug( "WARNING:Screenshot buffer is null when trying to resize it!" );
		return;
	}
	LPCOLORREF new_ScreenshotPixels = (COLORREF*)malloc( NewWidth * NewHeight * sizeof( COLORREF ) );
	int Width = ScreenshotRight - ScreenshotLeft;
	int Height = ScreenshotBottom - ScreenshotTop;
	ResampleRGBLiniar4ByteDownsample( (unsigned char *)ScreenshotPixels, (unsigned char *)new_ScreenshotPixels, Width, Height, NewWidth, NewHeight );
	ScreenshotLeft = 0;
	ScreenshotTop = 0;
	ScreenshotRight = NewWidth;
	ScreenshotBottom = NewHeight;
	free( ScreenshotPixels );
	ScreenshotPixels = new_ScreenshotPixels;
}

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

int GetCacheIndex( char *aFilespec )
{
	for( int i=0;i<NrPicturesCached;i++)
		if( strstr( PictureCache[i].FileName, aFilespec ) )
			return i;
	return -1;
}

CachedPicture *CachePicture( char *aFilespec )
{
	FileDebug( "Started caching image" );
	int ExistingCacheIndex = GetCacheIndex( aFilespec );
	if( ExistingCacheIndex != -1 )
	{
		FileDebug( "Skipped caching image as it's already cached" );
		return &PictureCache[NrPicturesCached];
	}

	if( NrPicturesCached >= MAX_PICTURE_CACHE_COUNT )
	{
		FileDebug( "Skipped caching image as no more cache slots available" );
		return NULL; 
	}
	HDC hdc = GetDC(NULL);
	if (!hdc)
	{
		FileDebug( "Skipped caching image as failed to lock DC" );
		return NULL; 
	}

	strcpy_s( PictureCache[NrPicturesCached].FileName, DEFAULT_STR_BUFFER_SIZE, aFilespec );

	int ImgType;
	PictureCache[NrPicturesCached].LoadedPicture = LoadPicture( aFilespec, 0, 0, ImgType, 0, false );
	if( PictureCache[NrPicturesCached].LoadedPicture == NULL )
	{
		FileDebug( "Skipped caching image as it could not be loaded" );
		ReleaseDC(NULL, hdc);
		return NULL;
	}

	bool image_is_16bit;
	LONG image_width, image_height;
	PictureCache[NrPicturesCached].Pixels = getbits( PictureCache[NrPicturesCached].LoadedPicture, hdc, image_width, image_height, image_is_16bit );
	if( PictureCache[NrPicturesCached].Pixels == NULL )
	{
		FileDebug( "Skipped caching image as it could not be converted to pixel map" );
		ReleaseDC(NULL, hdc);
		return NULL;
	}
	NrPicturesCached++;
	ReleaseDC(NULL, hdc);

	return &PictureCache[NrPicturesCached-1];
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

char ReturnBuff[DEFAULT_STR_BUFFER_SIZE*10];
char* WINAPI ImageSearchOnScreenshot( char *aFilespec, int TransparentColor, int AcceptedColorDiff, int AcceptedErrorCount, int StopAfterNMatches )
{
	int MatchesFound = 0;
	ReturnBuff[0]=0;
	FileDebug( "Started Image search" );
	CachedPicture *cache = CachePicture( aFilespec );
	if( cache == NULL )
	{
		FileDebug( "Skipping Image search as image could not be loaded" );
		return "";
	}
	if( ScreenshotPixels == NULL )
	{
		FileDebug( "Skipping Image search no screenshot is available" );
		return "";
	}

	BITMAP bitmap;
	GetObject( cache->LoadedPicture, sizeof(BITMAP), &bitmap); // Realistically shouldn't fail at this stage.

	int Width = ScreenshotRight - ScreenshotLeft;
	int Height = ScreenshotBottom - ScreenshotTop;
	if( AcceptedColorDiff > 0 )
	{
		unsigned char *MinMap[3];
		unsigned char *MaxMap[3];
		for( int i = 0;i<3;i++)
		{
			MinMap[i] = (unsigned char *)malloc( bitmap.bmWidth * bitmap.bmHeight );
			MaxMap[i] = (unsigned char *)malloc( bitmap.bmWidth * bitmap.bmHeight );
		}
		for( int y = 0; y < bitmap.bmHeight; y +=1 )
			for( int x = 0; x < bitmap.bmWidth; x += 1 )
			{
				if( ( cache->Pixels[ y * bitmap.bmWidth + x ] & 0x00FFFFFF ) == TransparentColor )
				{
					for( int i=0;i<3;i++)
					{
						MinMap[i][ y * bitmap.bmWidth + x ] = 0;
						MaxMap[i][ y * bitmap.bmWidth + x ] = 255;
					}
				}
				else
				{
					for( int i=0;i<3;i++)
					{
						int col = ( ((int)cache->Pixels[ y * bitmap.bmWidth + x ]) >> ( i * 8 ) ) & 0xFF;
						int min = (int)col - AcceptedColorDiff;
						int max = (int)col + AcceptedColorDiff;
						if( min < 0 )
							min = 0;
						if( max > 255 )
							max = 255;
						MinMap[i][ y * bitmap.bmWidth + x ] = min;
						MaxMap[i][ y * bitmap.bmWidth + x ] = max;
					}
				}
			}
//DumpAsPPM( MinMap[0], MinMap[1], MinMap[2], bitmap.bmWidth, bitmap.bmHeight );
//DumpAsPPM( &ScreenshotPixels[ 40 * Width + 40 ], 40, 40, Width );
//DumpAsPPM( MaxMap[0], MaxMap[1], MaxMap[2], bitmap.bmWidth, bitmap.bmHeight );
		for( int y = 0; y < Height; y +=1 )
			for( int x = 0; x < Width; x += 1 )
			{
				int ImageMatched = 1;
				int FoundErrors = 0;
				for( int y2=0;y2<bitmap.bmHeight;y2++ )
				{
					for( int x2=0;x2<bitmap.bmWidth;x2++)
					{
						int PixelIndexDst = ( 0 + y2 ) * bitmap.bmWidth + 0 + x2;
						int PixelIndexSrc = ( y + y2 ) * Width + x + x2;
						COLORREF BGRSrc = ScreenshotPixels[ PixelIndexSrc ];
						int Cols[3];
						Cols[0] = ( (int)BGRSrc >> 0 ) & 0xFF;
						if( MinMap[0][PixelIndexDst] <= Cols[0] && Cols[0] <= MaxMap[0][PixelIndexDst] )
						{
							Cols[1] = ( (int)BGRSrc >> 8 ) & 0xFF;
							if( MinMap[1][PixelIndexDst] <= Cols[1] && Cols[1] <= MaxMap[1][PixelIndexDst] )
							{
								Cols[2] = ( (int)BGRSrc >> 16 ) & 0xFF;
								if( MinMap[2][PixelIndexDst] <= Cols[2] && Cols[2] <= MaxMap[2][PixelIndexDst] )
								{
									//yaay it matches = nothig to do here
								}
								else
									FoundErrors++;
							}
							else
								FoundErrors++;
						}
						else
							FoundErrors++;

						if( FoundErrors > AcceptedErrorCount )
						{
							ImageMatched = 0;
							goto AbandonIMageInImageSearch;
						}
					}
				}
AbandonIMageInImageSearch:
				if( ImageMatched == 1 )
				{
					FileDebug( "Image search found a match" );
					sprintf_s( ReturnBuff, DEFAULT_STR_BUFFER_SIZE*10, "%s|%d|%d", ReturnBuff, x, y );
					FileDebug( ReturnBuff );
					MatchesFound++;
					if( MatchesFound >= StopAfterNMatches )
						goto docleanupandreturn;
				}
			}
docleanupandreturn:
		sprintf_s( ReturnBuff, DEFAULT_STR_BUFFER_SIZE*10, "%d%s", MatchesFound, ReturnBuff );
		for( int i = 0; i<3;i++)
		{
			free( MinMap[i] );
			free( MaxMap[i] );
		}
	}
	else
	{
		for( int y = 0; y < Height; y +=1 )
			for( int x = 0; x < Width; x += 1 )
			{
				int ImageMatched = 1;
				int FoundErrors = 0;
				for( int y2=0;y2<bitmap.bmHeight;y2++ )
				{
					for( int x2=0;x2<bitmap.bmWidth;x2++)
					{
						int PixelIndexSrc = ( y + y2 ) * Width + x + x2;
						int PixelIndexDst = ( 0 + y2 ) * bitmap.bmWidth + 0 + x2;
						COLORREF BGRSrc = ScreenshotPixels[ PixelIndexSrc ];
						COLORREF BGRDst = cache->Pixels[ PixelIndexDst ];
						if( BGRDst == TransparentColor || BGRSrc == BGRDst )
						{
							//yaay it matches = nothig to do here
						}
						else
							FoundErrors++;

						if( FoundErrors > AcceptedErrorCount )
						{
							ImageMatched = 0;
							goto AbandonIMageInImageSearch2;
						}
					}
				}
AbandonIMageInImageSearch2:
				if( ImageMatched == 1 )
				{
					FileDebug( "Image search found a match" );
					sprintf_s( ReturnBuff, DEFAULT_STR_BUFFER_SIZE*10, "%s|%d|%d", ReturnBuff, x, y );
					FileDebug( ReturnBuff );
					MatchesFound++;
					if( MatchesFound >= StopAfterNMatches )
						goto docleanupandreturn2;
				}
			}
docleanupandreturn2:
		sprintf_s( ReturnBuff, DEFAULT_STR_BUFFER_SIZE*10, "%d%s", MatchesFound, ReturnBuff );
	}
	return ReturnBuff;
}
