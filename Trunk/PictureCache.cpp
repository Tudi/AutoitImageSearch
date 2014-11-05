#include "StdAfx.h"

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