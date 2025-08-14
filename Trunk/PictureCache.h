#ifndef _PICTURE_CACHE_H_
#define _PICTURE_CACHE_H_

#define MAX_PICTURE_CACHE_COUNT 3500

#include "ImageSearchPreSAD.h"
#include "ImageHash.h"

class SimilarSearch;
class PiramidImage;
struct OCRStore;
class SplitChannel;

struct CachedPicture
{
	char			FileName[ DEFAULT_STR_BUFFER_SIZE ];
	int				NameHash;
	HBITMAP			LoadedPicture;	//valid if loaded from bitmap file
	LPCOLORREF		Pixels;
	int				Width,Height;
	//there is a large chance that we will do the same searches over and over for these images. Maybe we can afford to prepare a cache
	int				MinMaxMapTolerance,TransparentColor;
	unsigned char	*MinMap[3];
	unsigned char	*MaxMap[3];
	SimilarSearch	*SSCache;
	PiramidImage	*PSCache;
	SplitChannel	*SCCache;
	SADSumStoreCached	SADSums;
	bool			NeedsSSCache;
	bool			NeedsPSCache;
	bool			NeedsSCCache;
	OCRStore		*OCRCache;
	// avoid searching too fast that would increase CPU usage
	size_t			PrevSearchImageId;
	int				PrevSearchTop, PrevSearchLeft, PrevSearchFlags;
	char			PrevSearchReturnVal[512];
	ImgHashWholeIage m_Hash;
	uint8_t			*pGrayscalePixels; // 1 byte / pixel. Stride is same as width
};

int GetCacheIndex( const char *aFilespec );
CachedPicture *CachePicture( const char *aFilespec );
CachedPicture *CachePicturePrintErrors(const char *aFilespec, const char *CallerFunctionName);
void CheckPrepareToleranceMaps( CachedPicture *cache, int NewTolerance, int TransparentColor );
void WINAPI MoveScreenshotToCache( const char *Name );
void RemoveCacheAlphaChannel( CachedPicture *cache );
void WINAPI LoadCacheOverScreenshot(const char *aFilename, int Atx, int Aty);
void UnloadLastCache(); //right now only used when training OCR with huge amount of files
void UnloadCache(char *aFilespec);
ImgHashWholeIage* GetCreateCacheHash(CachedPicture* cache);
void EnsureCacheHasGrayscale(CachedPicture* cache);

extern LIBRARY_API CachedPicture PictureCache[MAX_PICTURE_CACHE_COUNT];
extern LIBRARY_API int NrPicturesCached;

#endif