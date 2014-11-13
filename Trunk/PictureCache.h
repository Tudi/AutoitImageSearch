#ifndef _PICTURE_CACHE_H_
#define _PICTURE_CACHE_H_

#define MAX_PICTURE_CACHE_COUNT 500

class SimilarSearch;
class PiramidImage;
struct OCRStore;

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
	bool			NeedsSSCache;
	bool			NeedsPSCache;
	OCRStore		*OCRCache;
};

int GetCacheIndex( char *aFilespec );
CachedPicture *CachePicture( char *aFilespec );
void CheckPrepareToleranceMaps( CachedPicture *cache, int NewTolerance, int TransparentColor );
void WINAPI MoveScreenshotToCache( char *Name );

extern LIBRARY_API CachedPicture PictureCache[MAX_PICTURE_CACHE_COUNT];
extern LIBRARY_API int NrPicturesCached;

#endif