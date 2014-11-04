#include "StdAfx.h"

#define MAX_PICTURE_CACHE_COUNT 500

__declspec(dllexport) LPCOLORREF screen_pixel = NULL;
__declspec(dllexport) int ScreenshotLeft, ScreenshotTop, ScreenshotRight, ScreenshotBottom;
__declspec(dllexport) CachedPicture PictureCache[MAX_PICTURE_CACHE_COUNT];
__declspec(dllexport) int NrPicturesCached = 0;

void FileDebug( char *what )
{
	FILE *f = fopen( "debug.txt", "at" );
	fprintf( f, "%s\n", what );
	fclose( f );
}

void WINAPI ReleaseScreenshot()
{
	if( screen_pixel )
		free( screen_pixel );
	screen_pixel = NULL;
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
	if( !(screen_pixel = getbits(hbitmap_screen, sdc, screen_width, screen_height, screen_is_16bit)) )
		goto end;

	LONG screen_pixel_count = screen_width * screen_height;

	// If either is 16-bit, convert *both* to the 16-bit-compatible 32-bit format:
	if( screen_is_16bit )
	{
		if (trans_color != CLR_NONE)
			trans_color &= 0x00F8F8F8; // Convert indicated trans-color to be compatible with the conversion below.
		for (int i = 0; i < screen_pixel_count; ++i)
			screen_pixel[i] &= 0x00F8F8F8; // Highest order byte must be masked to zero for consistency with use of 0x00FFFFFF below.
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
	if( screen_pixel != NULL )
	{
		ReleaseScreenshot();
		screen_pixel = NULL;
	}
	if( screen_pixel == NULL )
	{
		TakeNewScreenshot( aLeft, aTop, aRight, aBottom );
	}

	FileDebug( "Finished taking the screenshot" );
//	if( screen_pixel == NULL )
//		FileDebug( "WARNING:Screenshot buffer is null when taking the screenshot!" );

	return ;
}

static int ImageFileAutoIncrement = 0;
void WINAPI SaveScreenshot()
{
	FileDebug( "Started saving the screenshot" );

	if( screen_pixel == NULL )
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
//			Img.SetPixel( x, y, screen_pixel[ y * Width + x ] );
			Img.SetPixel( x, y, RGB( GetBValue( screen_pixel[ y * Width + x ] ), GetGValue( screen_pixel[ y * Width + x ] ), GetRValue( screen_pixel[ y * Width + x ] ) ) );

	Img.Save( MyFileName );
}

void WINAPI ResizeScreenshot( int NewWidth, int NewHeight )
{
	FileDebug( "Started resizing the screenshot" );
	if( screen_pixel == NULL )
	{
		FileDebug( "WARNING:Screenshot buffer is null when trying to resize it!" );
		return;
	}
	LPCOLORREF new_screen_pixel = (COLORREF*)malloc( NewWidth * NewHeight * sizeof( COLORREF ) );
	int Width = ScreenshotRight - ScreenshotLeft;
	int Height = ScreenshotBottom - ScreenshotTop;
	ResampleRGBLiniar4ByteDownsample( (unsigned char *)screen_pixel, (unsigned char *)new_screen_pixel, Width, Height, NewWidth, NewHeight );
	ScreenshotLeft = 0;
	ScreenshotTop = 0;
	ScreenshotRight = NewWidth;
	ScreenshotBottom = NewHeight;
	free( screen_pixel );
	screen_pixel = new_screen_pixel;
}

void WINAPI BlurrImage( int HalfKernelSize )
{
	FileDebug( "Started bluring screenshot" );
	if( screen_pixel == NULL )
	{
		FileDebug( "WARNING:Screenshot buffer is null when trying to blur it!" );
		return;
	}
	int Width = ScreenshotRight - ScreenshotLeft;
	int Height = ScreenshotBottom - ScreenshotTop;
	LPCOLORREF new_screen_pixel = (COLORREF*)malloc( Width * Height * sizeof( COLORREF ) );
	if( new_screen_pixel == NULL )
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
					SumOfValuesR += GetRValue( screen_pixel[ ( y + ky ) * Width + x + kx ] );
					SumOfValuesG += GetGValue( screen_pixel[ ( y + ky ) * Width + x + kx ] );
					SumOfValuesB += GetBValue( screen_pixel[ ( y + ky ) * Width + x + kx ] );
				}
			new_screen_pixel[ y * Width + x ] = RGB( SumOfValuesR / KernelPixelCount, SumOfValuesG / KernelPixelCount, SumOfValuesB / KernelPixelCount );
		}

	free( screen_pixel );
	screen_pixel = new_screen_pixel;
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

char* WINAPI ImageSearchOnScreenshot( char *aFilespec )
{
	FileDebug( "Started Image search" );
	CachedPicture *cache = CachePicture( aFilespec );
	if( cache == NULL )
	{
		FileDebug( "Skipping Image search as image could not be loaded" );
		return "";
	}
}
