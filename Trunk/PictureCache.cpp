#include "StdAfx.h"

LIBRARY_API CachedPicture PictureCache[MAX_PICTURE_CACHE_COUNT];
LIBRARY_API int NrPicturesCached = 0;

int GetCacheIndex( char *aFilespec )
{
//	for( int i=0;i<NrPicturesCached;i++)
//		if( strstr( PictureCache[i].FileName, aFilespec ) )
//			return i;
	int ThisHash = GetStrHash( aFilespec );
	for( int i=0;i<NrPicturesCached;i++)
		if( ThisHash == PictureCache[i].NameHash )
			return i;
	return -1;
}

void FixAlphaChannelZero()
{
	if (PictureCache[NrPicturesCached].Pixels == NULL)
	{
		return;
	}
	if ((PictureCache[NrPicturesCached].Pixels[0] >> 24) != 0)
	{
		return;
	}
	FileDebug("Started FixAlphaChannelZero");

	LPCOLORREF		Pixels = PictureCache[NrPicturesCached].Pixels;
	for (size_t y = 0; y < (size_t)PictureCache[NrPicturesCached].Height; y++)
	{
		for (size_t x = 0; x < (size_t)PictureCache[NrPicturesCached].Width; x++)
		{
			Pixels[x] |= 0xFF000000;
		}
		Pixels += PictureCache[NrPicturesCached].Width;
	}

	FileDebug("\tFinished FixAlphaChannelZero");

}

CachedPicture *CachePicture( char *aFilespec )
{
	FileDebug( "Started caching image" );
	int ExistingCacheIndex = GetCacheIndex( aFilespec );
	if( ExistingCacheIndex != -1 )
	{
		FileDebug( "Skipped caching image as it's already cached" );
		return &PictureCache[ExistingCacheIndex];
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
	PictureCache[NrPicturesCached].NameHash = GetStrHash( aFilespec );

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

	PictureCache[NrPicturesCached].Width = image_width;
	PictureCache[NrPicturesCached].Height = image_height;

	PictureCache[NrPicturesCached].NeedsPSCache = true;
	PictureCache[NrPicturesCached].NeedsSSCache = true;
	PictureCache[NrPicturesCached].NeedsSCCache = true;

	PictureCache[NrPicturesCached].OCRCache = NULL;

	FixAlphaChannelZero();

	NrPicturesCached++;
	ReleaseDC(NULL, hdc);

	FileDebug( "\tFinished caching image" );

	return &PictureCache[NrPicturesCached-1];
}

CachedPicture *CachePicturePrintErrors(char *aFilespec, char *CallerFunctionName)
{
	char TempBuf[250];
	CachedPicture *cache = CachePicture(aFilespec);
	if (CallerFunctionName == NULL)
		return cache;
	if (cache == NULL)
	{
		sprintf_s(TempBuf, sizeof(TempBuf), "%s : image could not be loaded", CallerFunctionName);
		FileDebug(TempBuf);
		return cache;
	}
	if (cache->Pixels == NULL)
	{
		sprintf_s(TempBuf, sizeof(TempBuf), "%s : pixels are missing", CallerFunctionName);
		FileDebug(TempBuf);
		return cache;
	}
	if (cache->LoadedPicture == NULL)
	{
		sprintf_s(TempBuf, sizeof(TempBuf), "%s : image is missing", CallerFunctionName);
		FileDebug(TempBuf);
		return cache;
	}
	return cache;
}

void CheckPrepareToleranceMaps( CachedPicture *cache, int NewTolerance, int TransparentColor )
{
	if( cache == NULL )
		return;

	if( NewTolerance == cache->MinMaxMapTolerance && TransparentColor == cache->TransparentColor )
		return;

	FileDebug( "Generating tolerance map" );

	cache->MinMaxMapTolerance = NewTolerance;
	cache->TransparentColor = TransparentColor;

	if( cache->MinMap[0] == NULL )
	{
		for( int i = 0;i<3;i++)
		{
			cache->MinMap[i] = (unsigned char *)_aligned_malloc( cache->Width * cache->Height + SSE_PADDING, SSE_ALIGNMENT );
			cache->MaxMap[i] = (unsigned char *)_aligned_malloc( cache->Width * cache->Height + SSE_PADDING, SSE_ALIGNMENT );
		}
	}

	for( int y = 0; y < cache->Height; y +=1 )
		for( int x = 0; x < cache->Width; x += 1 )
		{
			if ((cache->Pixels[y * cache->Width + x] & REMOVE_ALPHA_CHANNEL_MASK) == TransparentColor)
			{
				for( int i=0;i<3;i++)
				{
					cache->MinMap[i][ y * cache->Width + x ] = 0;
					cache->MaxMap[i][ y * cache->Width + x ] = 255;
				}
			}
			else
			{
				for( int i=0;i<3;i++)
				{
					int col = ( ((int)cache->Pixels[ y * cache->Width + x ]) >> ( i * 8 ) ) & 0xFF;
					int min = (int)col - NewTolerance;
					int max = (int)col + NewTolerance;
					if( min < 0 )
						min = 0;
					if( max > 255 )
						max = 255;
					cache->MinMap[i][ y * cache->Width + x ] = min;
					cache->MaxMap[i][ y * cache->Width + x ] = max;
				}
			}
		}
}

void WINAPI MoveScreenshotToCache( char *Name )
{
	if( CurScreenshot->Pixels == NULL )
	{
		FileDebug( "Can not move screenshot to cache because there are no screenshots" );
		return;
	}


	if( NrPicturesCached >= MAX_PICTURE_CACHE_COUNT )
	{
		FileDebug( "Skipped caching image as no more cache slots available" );
		return; 
	}

	FileDebug( "Started caching screenshot" );

	strcpy_s( PictureCache[NrPicturesCached].FileName, DEFAULT_STR_BUFFER_SIZE, Name );
	PictureCache[NrPicturesCached].NameHash = GetStrHash( Name );

	PictureCache[ NrPicturesCached ].LoadedPicture = NULL;

	int PixelsByteSize = CurScreenshot->GetWidth() * CurScreenshot->GetHeight() * sizeof( COLORREF );
	PictureCache[ NrPicturesCached ].Pixels = (LPCOLORREF) _aligned_malloc( PixelsByteSize, SSE_ALIGNMENT );
	memcpy(	PictureCache[ NrPicturesCached ].Pixels, CurScreenshot->Pixels, PixelsByteSize );

	PictureCache[ NrPicturesCached ].Width = CurScreenshot->GetWidth();
	PictureCache[ NrPicturesCached ].Height = CurScreenshot->GetHeight();

	PictureCache[NrPicturesCached].NeedsPSCache = true;
	PictureCache[NrPicturesCached].NeedsSSCache = true;
	PictureCache[NrPicturesCached].NeedsSCCache = true;

	NrPicturesCached++;

	FileDebug( "\tFinished caching screenshot" );
}

void RemoveCacheAlphaChannel( CachedPicture *cache )
{
	if( ( cache->Pixels[ 0 ] & 0xFF000000 ) != 0 )
	{
		int PixelCount = cache->Width * cache->Height;
		for( int i=0;i<PixelCount;i++)
			cache->Pixels[i] = cache->Pixels[i] & REMOVE_ALPHA_CHANNEL_MASK;
	}
}

void WINAPI LoadCacheOverScreenshot(char *aFilename, int Atx, int Aty)
{
	FileDebug("LoadCacheOverScreenshot : Start");
	CachedPicture *Cache = CachePicture(aFilename);
	if (Cache == NULL)
	{
		FileDebug("LoadCacheOverScreenshot : Could not find cache");
		return;
	}
	if (CurScreenshot == NULL || CurScreenshot->Pixels == NULL)
	{
		FileDebug("LoadCacheOverScreenshot : No screensshot to copy over to");
		return;
	}
	if (Atx > CurScreenshot->GetWidth())
	{
		FileDebug("LoadCacheOverScreenshot : target X is arger than Screensot width");
		return;
	}
	if (Aty > CurScreenshot->GetHeight())
	{
		FileDebug("LoadCacheOverScreenshot : target y is larger than screenshot height");
		return;
	}
	int MaxCopyX = min(Cache->Width, CurScreenshot->GetWidth() - Atx);
	int MaxCopyY = min(Cache->Height, CurScreenshot->GetHeight() - Aty);
	for (int y = 0; y < MaxCopyY; y++)
		for (int x = 0; x < MaxCopyX; x++)
			CurScreenshot->SetPixel(x + Atx, y + Aty, Cache->Pixels[y*Cache->Width + x]);
	FileDebug("LoadCacheOverScreenshot : End");
}

void UnloadLastCache()
{
	if (NrPicturesCached == 0)
		return;
	// PictureCache[NrPicturesCached - 1].LoadedPicture; // leaking this :(
	_aligned_free(PictureCache[NrPicturesCached - 1].Pixels);
	PictureCache[NrPicturesCached - 1].Pixels = NULL;
	NrPicturesCached--;
}

void UnloadCache(char *aFilespec)
{
	int ExistingCacheIndex = GetCacheIndex(aFilespec);
	if (ExistingCacheIndex == -1)
	{
		FileDebug("No cache to unload");
	}
	_aligned_free(PictureCache[ExistingCacheIndex].Pixels);
	PictureCache[ExistingCacheIndex].Pixels = NULL;
	PictureCache[ExistingCacheIndex].NameHash = 0;
}