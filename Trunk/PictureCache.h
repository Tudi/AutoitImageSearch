#ifndef _PICTURE_CACHE_H_
#define _PICTURE_CACHE_H_

#define MAX_PICTURE_CACHE_COUNT 500

struct CachedPicture
{
	char			FileName[ DEFAULT_STR_BUFFER_SIZE ];
	HBITMAP			LoadedPicture;
	LPCOLORREF		Pixels;
	unsigned char	MinMaxMapTolerance;
	unsigned char	*MinMap[3];
	unsigned char	*MaxMap[3];
};

int GetCacheIndex( char *aFilespec );
CachedPicture *CachePicture( char *aFilespec );

extern LIBRARY_API CachedPicture PictureCache[MAX_PICTURE_CACHE_COUNT];
extern LIBRARY_API int NrPicturesCached;

#endif