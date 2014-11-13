#include "StdAfx.h"

int OCRAvgRForText = 0;
int OCRAvgGForText = 0;
int OCRAvgBForText = 0;
int OCRTransparentColor = 0x00FFFFFF;
std::set<COLORREF> OCRTextColors;
int OCRMaxFontWidth = 0;
int OCRMaxFontHeight = 0;

//pinch off 2 bits -> reduce from 24 to 19 bpp. This reduces shade variations. Only worth using if you want to avoid storing too many pixels
//#define COLOR_REDUCE_MASK	0xFC
//pinch off 3 bits -> reduce from 24 to 15 bpp. This reduces shade variations. Only worth using if you want to avoid storing too many pixels
#define COLOR_REDUCE_MASK8	0xF8
#define COLOR_REDUCE_MASK24	0x00F8F8F8

COLORREF ReduceColorCount( COLORREF Pixel )
{
	return Pixel & COLOR_REDUCE_MASK24;
/*	int R = GetRValue( Pixel );
	int G = GetGValue( Pixel );
	int B = GetBValue( Pixel );

	R = R & COLOR_REDUCE_MASK;
	G = G & COLOR_REDUCE_MASK;
	B = B & COLOR_REDUCE_MASK;

	return RGB( R, G, B ); */
}

void GetCacheColorStatistics( CachedPicture *cache )
{
	cache->OCRCache->PixelCount = 0;
	for( int y = 0; y < cache->Height; y++ )
		for( int x = 0; x < cache->Width; x++ )
		{
			int Pixel = cache->Pixels[ y * cache->Width + x ];

			if( Pixel == OCRTransparentColor )
				continue;

			OCRTextColors.insert( ReduceColorCount( Pixel ) );

/*				cache->OCRCache->PixelCount++;
			int R = GetRValue( Pixel );
			if( R >= 0 ) 
			{
				cache->OCRCache->SumR += R;
				cache->OCRCache->CountR += 1;
			}
			int G = GetGValue( Pixel );
			if( G >= 0 ) 
			{
				cache->OCRCache->SumG += G;
				cache->OCRCache->CountG += 1;
			}
			int B = GetBValue( Pixel );
			if( B >= 0 ) 
			{
				cache->OCRCache->SumB += B;
				cache->OCRCache->CountB += 1;
			} */
		}
}

void GetCharacterSetColorStatistics()
{
	int SumR,SumG,SumB,CountR,CountG,CountB;
	SumR = SumG = SumB = CountR = CountG = CountB = 0;
	for( int c = 0; c < NrPicturesCached; c++ )
	{
		CachedPicture *cache = &PictureCache[ c ];

		if( cache->Pixels == NULL )
			continue;

		if( cache->OCRCache == NULL )
		{
			cache->OCRCache = new OCRStore;
			memset( cache->OCRCache, 0, sizeof( OCRStore ) );
			GetCacheColorStatistics( cache );

			if( cache->Width > OCRMaxFontWidth )
				OCRMaxFontWidth = cache->Width;
			if( cache->Height > OCRMaxFontHeight )
				OCRMaxFontHeight = cache->Height;
		}
/*
		SumR += cache->OCRCache->SumR;
		SumG += cache->OCRCache->SumG;
		SumB += cache->OCRCache->SumB; */
	}
}

//binarize image. If there are strong colors
bool IsColumnEmpty( int X, int StartY, int EndY )
{
	int Width = CurScreenshot->GetWidth();
	for( int y = StartY; y < EndY; y++ )
	{
		int Pixel = CurScreenshot->Pixels[ y * Width + X ];

		if( Pixel == OCRTransparentColor )
			continue;

		if( OCRTextColors.find( ReduceColorCount( Pixel ) ) != OCRTextColors.end() )
			return false;

/*		int R = GetRValue( Pixel );
		if( R >= OCRAvgRForText ) 
			return false;
		int G = GetGValue( Pixel );
		if( G >= OCRAvgGForText ) 
			return false;
		int B = GetBValue( Pixel );
		if( B >= OCRAvgBForText ) 
			return false; */
	}
	return true;
}

void GetCacheScoreAtLoc( CachedPicture *cache, int AtX, int AtY, int *HitCount, int *MissCount )
{
	int Width = CurScreenshot->GetWidth();
	*HitCount = 0;
	*MissCount = 0;
	for( int y = 0; y < cache->Height; y++ )
		for( int x = 0; x< cache->Width; x++ )
		{
			if( cache->Pixels[ y * cache->Width + x ] == OCRTransparentColor )
				continue;
//			if( CurScreenshot->Pixels[ ( AtY + y ) * Width + ( AtX + x ) ] == cache->Pixels[ y * cache->Width + x ] )
			if( ReduceColorCount( CurScreenshot->Pixels[ ( AtY + y ) * Width + ( AtX + x ) ] ) == ReduceColorCount( cache->Pixels[ y * cache->Width + x ] ) )
				*HitCount += 1;
			else
				*MissCount += 1;
		}
}

//return format : [ResultCount][ReadTextUntilX][TextRead]
char * WINAPI ReadTextFromScreenshot( int StartX, int StartY, int EndX, int EndY )
{
	FileDebug( "Started OCR" );
	if( NrPicturesCached == 0 )
	{
		FileDebug( "\tOCR has not cached images for fonts" );
		return "0|0|";
	}
	if( CurScreenshot->Pixels == NULL )
	{
		FileDebug( "\tOCR has no screenshot to work on" );
		return "0|0|";
	}
	if( EndX < StartX || StartX > CurScreenshot->Right || EndY > CurScreenshot->Bottom )
	{
		FileDebug( "\tOCR coordinates are wrong. Can't search outside the screenshot" );
		return "0|0|";
	}

	StartX = StartX - CurScreenshot->Left;
	EndX = EndX - CurScreenshot->Left;
	StartY = StartY - CurScreenshot->Top;
	EndY = EndY - CurScreenshot->Top;
	EndX = EndX - OCRMaxFontWidth;
	EndY = EndY - OCRMaxFontHeight;
	if( StartX < 0 )
		StartX = 0;
	if( EndX > CurScreenshot->Right )
		EndX = CurScreenshot->Right;
	if( StartY < 0 )
		StartY = 0;
	if( EndY > CurScreenshot->Bottom )
		EndY = CurScreenshot->Bottom;

	//will only make statistics once per image 
	GetCharacterSetColorStatistics();
//printf( "Found %d different font colors\n", OCRTextColors.size() );

	for( int x = StartX; x < EndX; x++ )
	{
		if( IsColumnEmpty( x, StartY, EndY ) )
			continue;
		CachedPicture *BestMatch = &PictureCache[ 0 ];
		for( int y = StartY; y < EndY; y++ )
		{
			for( int c = 0; c < NrPicturesCached; c++ )
			{
				CachedPicture *cache = &PictureCache[ c ];
				cache->OCRCache->LastSearchHitCount = 0;
				cache->OCRCache->LastSearchMissCount = MAX_INT;
				for( int yc = 0; yc < cache->Height * 3 / 2; yc++ )
					for( int xc = 0; xc < cache->Width * 3 / 2; xc++ )
					{
						int Hits,Misses;
						GetCacheScoreAtLoc( cache, x + xc, y + yc, &Hits, &Misses );
						if( Hits >= cache->OCRCache->LastSearchHitCount && Misses <= cache->OCRCache->LastSearchMissCount )
						{
							cache->OCRCache->LastSearchHitCount = Hits;
							cache->OCRCache->LastSearchMissCount = Misses;
							cache->OCRCache->LastSearchX = x + xc;
							cache->OCRCache->LastSearchY = y + yc;
						}
					}

				if( cache->OCRCache->LastSearchMissCount * 10000 / ( cache->OCRCache->LastSearchHitCount + 1 ) < BestMatch->OCRCache->LastSearchMissCount * 10000 / ( BestMatch->OCRCache->LastSearchHitCount + 1 ) )
					BestMatch = cache;

				if( cache->OCRCache->LastSearchMissCount == 0 && cache->OCRCache->LastSearchHitCount > 0 )
				{
//					FileDebug( "\tOCR Perfect letter match found, aborting other searches" );
//printf( "Perfect match found, adjusting font start row to %d from %d\n", cache->OCRCache->LastSearchY + cache->Height - OCRMaxFontHeight, StartY );
					StartY = cache->OCRCache->LastSearchY + cache->Height - OCRMaxFontHeight;
					y = EndY;
					break;
				}
			}
		}
printf( "Best match name is %s with hitcount %d and misscount %d \n", BestMatch->FileName, BestMatch->OCRCache->LastSearchHitCount, BestMatch->OCRCache->LastSearchMissCount );
		// best search result is taken even if we are wrong
		x = BestMatch->OCRCache->LastSearchX + BestMatch->Width;
	}

	//skip empty columns
	FileDebug( "\tFinished  OCR" );
	return "0|0|";
}