#ifndef _INPUT_PROCESS_H_
#define _INPUT_PROCESS_H_

#include "ImageSearchPreSAD.h"

// autoit seems to crap out for some reason. Let's try to reduce allocations to see if it helps
#define REDUCE_ALLOC_COUNT

void WINAPI TakeScreenshot(int aLeft, int aTop, int aRight, int aBottom);
void WINAPI ReleaseScreenshot( );
char* WINAPI GetImageSize( char *aImageFile );
char* WINAPI IsAnythingChanced( int StartX, int StartY, int EndX, int EndY );
void WINAPI CycleScreenshots();

class ScreenshotStruct
{
public:
	LPCOLORREF		Pixels;
	int				Left, Top, Right, Bottom, Width, Height;
	int				BytesPerPixel;	//default is 4, could be 1
	bool			IsDiffMap;	//width and height needs to be divided by 4
	bool			NeedsSSCache;
	bool			NeedsPSCache;
	bool			NeedsAlphaRemoved;
	bool			NeedsSplitChannelCache;
	int				AppliedColorMask;
	SimilarSearch	*SSCache;
	PiramidImage	*PSCache;
	SplitChannel	*SCCache;
	SADSumStoreScreenshot		SADSums;
	ImgHashWholeIage* pSSHashCache; // if multiple images are searched on the same SS, they can reuse the hash
	uint8_t			*pGrayscalePixels; // 1 byte / pixel. Stride is same as width
	size_t			TimeStampTaken;
	size_t			UniqueFameCounter;

	ScreenshotStruct()
	{
		Constuctor();
	}
	void Constuctor()
	{
		Pixels = NULL;
		SSCache = NULL;
		PSCache = NULL;
		SCCache = NULL;
		pSSHashCache = NULL;
		pGrayscalePixels = NULL;
		InitSADSUMScreenshot(&SADSums);
		Left = Top = Right = Bottom = 0;
		IsDiffMap = false;
		NeedsSSCache = NeedsPSCache = NeedsSplitChannelCache = true;
		AppliedColorMask = false;
		static size_t g_UnqiueFrameCounter = 0;
		UniqueFameCounter = (++g_UnqiueFrameCounter);
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
//	void ReplaceReadOnlyPixels();
};

#define NR_SCREENSHOTS_CACHED	2

extern LIBRARY_API ScreenshotStruct ScreenshotCache[NR_SCREENSHOTS_CACHED];
extern LIBRARY_API ScreenshotStruct	*CurScreenshot, *PrevScreenshot;
extern LIBRARY_API int ScreenshotStoreIndex;
extern LIBRARY_API char ReturnBuff[DEFAULT_STR_BUFFER_SIZE * 10];

// if no screenshot region is specified, try to guess that we wanted the minimum region as last time
struct SearchedRegionMinMax
{
	int aLeft, aTop, aRight, aBottom;
};
extern LIBRARY_API SearchedRegionMinMax g_SearchedRegions;

void EnsureScreenshotHasGrayscale();
#endif