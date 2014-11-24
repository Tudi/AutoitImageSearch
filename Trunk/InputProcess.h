#ifndef _INPUT_PROCESS_H_
#define _INPUT_PROCESS_H_

void WINAPI TakeScreenshot(int aLeft, int aTop, int aRight, int aBottom);
void WINAPI ReleaseScreenshot( );
char* WINAPI ImageSearchOnScreenshot( char *aImageFile, int TransparentColor, int AcceptedColorDiff, int AcceptedErrorCount, int StopAfterNMatches );
// mask value 0 = screenshot pixel is hidden and should not be searched
// mask value 1 = screenshot pixel is visible and should be searched
char* WINAPI ImageSearchOnScreenshotMasked( char *ImageFile, char *MaskFile, int TransparentColor, int AcceptedColorDiff, int AcceptedErrorCount, int StopAfterNMatches );
char* WINAPI GetImageSize( char *aImageFile );
char* WINAPI IsAnythingChanced( int StartX, int StartY, int EndX, int EndY );

void CycleScreenshots();

class ScreenshotStruct
{
public:
	LPCOLORREF		Pixels;
	int				Left, Top, Right, Bottom;
	int				BytesPerPixel;	//default is 4, could be 1
	bool			IsDiffMap;	//width and height needs to be divided by 4
	bool			NeedsSSCache;
	bool			NeedsPSCache;
	bool			NeedsAlphaRemoved;
	SimilarSearch	*SSCache;
	PiramidImage	*PSCache;

	ScreenshotStruct()
	{
		Pixels = NULL;
		SSCache = NULL;
		PSCache = NULL;
		Left = Top = Right = Bottom = 0;
		IsDiffMap = false;
		NeedsSSCache = NeedsPSCache = true;
	}
	int				GetWidth()
	{
		if( IsDiffMap == true )
			return ( Right - Left ) / 4;
		return Right - Left;
	}

	int				GetHeight()
	{
		if( IsDiffMap == true )
			return ( Bottom - Top ) / 4;
		return Bottom - Top;
	}

	//probably much better if it is handled on site :P
	__forceinline COLORREF	GetPixel( int x, int y )
	{
		if( IsDiffMap == true )
		{
			unsigned char *MDMask = (unsigned char *)Pixels;
			return MDMask[ y / 4 * ( Right - Left ) / 4 + x / 4 ];
		}
		return Pixels[ y * ( Right - Left ) + x ];
	}
};

#define NR_SCREENSHOTS_CACHED	2

extern LIBRARY_API ScreenshotStruct ScreenshotCache[NR_SCREENSHOTS_CACHED];
extern LIBRARY_API ScreenshotStruct	*CurScreenshot, *PrevScreenshot;
extern LIBRARY_API int ScreenshotStoreIndex;

void RemoveScreenshotAlphaChannel( ScreenshotStruct *cache );

#endif