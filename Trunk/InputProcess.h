#ifndef _INPUT_PROCESS_H_
#define _INPUT_PROCESS_H_

void WINAPI TakeScreenshot(int aLeft, int aTop, int aRight, int aBottom);
void WINAPI ReleaseScreenshot( );
char* WINAPI GetImageSize( char *aImageFile );
char* WINAPI IsAnythingChanced( int StartX, int StartY, int EndX, int EndY );
void WINAPI CycleScreenshots();

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
	bool			NeedsSplitChannelCache;
	SimilarSearch	*SSCache;
	PiramidImage	*PSCache;
	SplitChannel	*SCCache;

	ScreenshotStruct()
	{
		Pixels = NULL;
		SSCache = NULL;
		PSCache = NULL;
		SCCache = NULL;
		Left = Top = Right = Bottom = 0;
		IsDiffMap = false;
		NeedsSSCache = NeedsPSCache = NeedsSplitChannelCache = true;
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
	__forceinline void	SetPixel(int x, int y, COLORREF NewColor)
	{
		Pixels[y * (Right - Left) + x] = NewColor;
	}
};

#define NR_SCREENSHOTS_CACHED	2

extern LIBRARY_API ScreenshotStruct ScreenshotCache[NR_SCREENSHOTS_CACHED];
extern LIBRARY_API ScreenshotStruct	*CurScreenshot, *PrevScreenshot;
extern LIBRARY_API int ScreenshotStoreIndex;
extern LIBRARY_API char ReturnBuff[DEFAULT_STR_BUFFER_SIZE * 10];

#endif