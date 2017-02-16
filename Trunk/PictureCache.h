#ifndef _PICTURE_CACHE_H_
#define _PICTURE_CACHE_H_

#define MAX_PICTURE_CACHE_COUNT 15000

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
	bool			NeedsSSCache;
	bool			NeedsPSCache;
	bool			NeedsSCCache;
	OCRStore		*OCRCache;
};

int GetCacheIndex( char *aFilespec );
CachedPicture *CachePicture( char *aFilespec );
void CheckPrepareToleranceMaps( CachedPicture *cache, int NewTolerance, int TransparentColor );
void WINAPI MoveScreenshotToCache( char *Name );
void RemoveCacheAlphaChannel( CachedPicture *cache );
void WINAPI LoadCacheOverScreenshot(char *aFilename, int Atx, int Aty);
void UnloadLastCache(); //right now only used when training OCR with huge amount of files

extern LIBRARY_API CachedPicture PictureCache[MAX_PICTURE_CACHE_COUNT];
extern LIBRARY_API int NrPicturesCached;

#endif