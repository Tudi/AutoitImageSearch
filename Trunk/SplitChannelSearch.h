#pragma once

//#define COMPARE_MULTI_REGION_READ_SPEED
#define COMPARE_STREAM_READ_SPEED		// 38% better than multi region according to my test

// Most used search type is transparent color search
// Try to find a way to optimize speed as much as possible
// 16 bytes transparency mask, 48 bytes RGB
class SplitChannel
{
public:
	SplitChannel();
	~SplitChannel();

#ifdef COMPARE_MULTI_REGION_READ_SPEED
	// needs to be 16 byte alligned
	unsigned char	*A;		// size if PixelCount + SSE allignement
	unsigned char	*R;		// size if PixelCount + SSE allignement
	unsigned char	*G;		// size if PixelCount + SSE allignement
	unsigned char	*B;		// size if PixelCount + SSE allignement
#endif
#ifdef COMPARE_STREAM_READ_SPEED
	//try 2. less memory regions. Larger memory requirement. Makes sense for this to be faster because of streamed memory
	unsigned char	*A2;	// size if PixelCount * 3 + SSE allignement
	unsigned char	*RGB;	// size if PixelCount * 3 + SSE allignement
#endif
};

void SplitChannelSearchGenCache(ScreenshotStruct *Img);
void SplitChannelSearchGenCache(CachedPicture *cache);

char* WINAPI ImageSearchOnScreenshotBest_Transparent_SAD(char *aFilespec);
char* WINAPI ImageSearchOnScreenshotBest_Transparent_SAD2(char *aFilespec);