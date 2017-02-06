#include "StdAfx.h"

int OCRAvgRForText = 0;
int OCRAvgGForText = 0;
int OCRAvgBForText = 0;
int OCRTransparentColor = TRANSPARENT_COLOR;
std::set<COLORREF> OCRTextColors;
int OCRMaxFontWidth = 8;
int OCRMaxFontHeight = 8;

//pinch off 2 bits -> reduce from 24 to 19 bpp. This reduces shade variations. Only worth using if you want to avoid storing too many pixels
//#define COLOR_REDUCE_MASK	0xFC
//pinch off 3 bits -> reduce from 24 to 15 bpp. This reduces shade variations. Only worth using if you want to avoid storing too many pixels
#define COLOR_REDUCE_MASK8	0xF8
#define COLOR_REDUCE_MASK24	0x00F8F8F8

void WINAPI OCR_SetMaxFontSize(int Width, int Height)
{
	OCRMaxFontWidth = Width;
	OCRMaxFontHeight = Height;
}

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

			cache->OCRCache->PixelCount++;
/*				
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

void WINAPI OCR_RegisterFont( char *aFilespec, int Font )
{
	CachedPicture *cache = CachePicture( aFilespec );
	if( cache != NULL && cache->Pixels != NULL )
	{
		if( cache->OCRCache == NULL )
		{
			cache->OCRCache = new OCRStore;
			memset( cache->OCRCache, 0, sizeof( OCRStore ) );
		}
		GetCacheColorStatistics( cache );

		if( cache->Width > OCRMaxFontWidth )
			OCRMaxFontWidth = cache->Width;
		if( cache->Height > OCRMaxFontHeight )
			OCRMaxFontHeight = cache->Height;

		cache->OCRCache->AssignedChar = Font;
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

char OCRReturnBuff[DEFAULT_STR_BUFFER_SIZE*10];
//return format : [TextRead]|[ReadTextUntilX]
char * WINAPI ReadTextFromScreenshot( int StartX, int StartY, int EndX, int EndY )
{
	char ReturnBuff[DEFAULT_STR_BUFFER_SIZE*10];
	int WriteIndex = 0;
	FileDebug( "Started OCR" );
	if( NrPicturesCached == 0 )
	{
		FileDebug( "\tOCR has not cached images for fonts" );
		return "|0";
	}
	if( CurScreenshot->Pixels == NULL )
	{
		FileDebug( "\tOCR has no screenshot to work on" );
		return "|0";
	}
	if( EndX < StartX || StartX > CurScreenshot->Right || EndY > CurScreenshot->Bottom )
	{
		FileDebug( "\tOCR coordinates are wrong. Can't search outside the screenshot" );
		return "|0";
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
/*
{
	char td[500];
	sprintf_s( td, 500, "Searchbox is %d %d %d %d", StartX, StartY, EndX, EndY );
	FileDebug( td );
}/**/

	//will only make statistics once per image 
	GetCharacterSetColorStatistics();
//printf( "Found %d different font colors\n", OCRTextColors.size() );

	CachedPicture *BestMatch = &PictureCache[ 0 ];
	for( int x = StartX; x < EndX; x++ )
	{
		if( IsColumnEmpty( x, StartY, EndY ) )
			continue;
		for( int y = StartY; y < EndY; y++ )
		{
			BestMatch = NULL;
			for( int c = 0; c < NrPicturesCached; c++ )
			{
				//if this font would be the best match, at what location would it be at ?
				CachedPicture *FontCache = &PictureCache[ c ];
				FontCache->OCRCache->LastSearchHitCount = 0;
				FontCache->OCRCache->LastSearchMissCount = MAX_INT;
				for( int yc = 0; yc < FontCache->Height * 3 / 2; yc++ )
					for( int xc = 0; xc < FontCache->Width * 3 / 2; xc++ )
					{
						int Hits,Misses;
						GetCacheScoreAtLoc( FontCache, x + xc, y + yc, &Hits, &Misses );
						if( Hits >= FontCache->OCRCache->LastSearchHitCount && Misses <= FontCache->OCRCache->LastSearchMissCount )
						{
							FontCache->OCRCache->LastSearchHitCount = Hits;
							FontCache->OCRCache->LastSearchMissCount = Misses;
							FontCache->OCRCache->LastSearchX = x + xc;
							FontCache->OCRCache->LastSearchY = y + yc;
						}
					}

				//this font, at it's best location, is it better than other fonts at their best location ?
				if( BestMatch == NULL || FontCache->OCRCache->LastSearchMissCount * 10000 / ( FontCache->OCRCache->LastSearchHitCount + 1 ) < BestMatch->OCRCache->LastSearchMissCount * 10000 / ( BestMatch->OCRCache->LastSearchHitCount + 1 ) )
					BestMatch = FontCache;

				//if this font matches perfectly in this spot, than stop searching for other candidates
				if( FontCache->OCRCache->LastSearchMissCount == 0 && FontCache->OCRCache->LastSearchHitCount > 0 )
				{
//					FileDebug( "\tOCR Perfect letter match found, aborting other searches" );
//printf( "Perfect match found, adjusting font start row to %d from %d\n", FontCache->OCRCache->LastSearchY + FontCache->Height - OCRMaxFontHeight, StartY );
					StartY = FontCache->OCRCache->LastSearchY + FontCache->Height - OCRMaxFontHeight;
					y = EndY;
					break;
				}
			}
		}
/*
{
	char td[500];
	sprintf_s( td, 500, "Best match is %s - %c with hitcount %d and misscount %d at %d %d", BestMatch->FileName, BestMatch->OCRCache->AssignedChar, BestMatch->OCRCache->LastSearchHitCount, BestMatch->OCRCache->LastSearchMissCount, BestMatch->OCRCache->LastSearchX, BestMatch->OCRCache->LastSearchY );
	FileDebug( td );
}/**/
		// best search result is taken even if we are wrong
		if( BestMatch->OCRCache->LastSearchMissCount * 100 / ( BestMatch->OCRCache->LastSearchHitCount + 1 ) < 50 )
		{
//printf( "Best match name is %s - %c with hitcount %d and misscount %d \n", BestMatch->FileName, BestMatch->OCRCache->AssignedChar, BestMatch->OCRCache->LastSearchHitCount, BestMatch->OCRCache->LastSearchMissCount );
			x = BestMatch->OCRCache->LastSearchX + BestMatch->Width;
//			sprintf_s( OCRReturnBuff, DEFAULT_STR_BUFFER_SIZE*10, "%s%c", OCRReturnBuff, BestMatch->OCRCache->AssignedChar );
			if( WriteIndex < DEFAULT_STR_BUFFER_SIZE*10 - 2 )
				ReturnBuff[ WriteIndex++ ] = BestMatch->OCRCache->AssignedChar;
		}
	}
	ReturnBuff[ WriteIndex++ ] = 0;
//printf( "Final return is %s as number %d \n", OCRReturnBuff, atoi( OCRReturnBuff ) );
	sprintf_s( OCRReturnBuff, DEFAULT_STR_BUFFER_SIZE*10, "%s|%d", ReturnBuff, BestMatch->OCRCache->LastSearchX + BestMatch->Width );
	//skip empty columns
//FileDebug( OCRReturnBuff );
	FileDebug( "\tFinished  OCR" );
	return OCRReturnBuff;
}

void WINAPI OCR_LoadFontsFromFile(char *aFilespec)
{
	FILE *f = fopen(aFilespec, "rt");
	if (f)
	{
		char c;
		char path[32000];
		int res = fscanf_s(f, "%c %s\n", &c, 1, path, sizeof( path ));
		while (res == 2)
		{
			OCR_RegisterFont(path, c);
			res = fscanf_s(f, "%c %s\n", &c, 1, path, sizeof(path));
		}
		fclose(f);
	}
}

void WINAPI OCR_LoadFontsFromDir(char *Path, char *SkipFileNameStart)
{
	int skiptocharpos = strlen(SkipFileNameStart);
	std::string search_path = Path;
	search_path += "/*.*";
	WIN32_FIND_DATA fd;
	HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
	if (hFind != INVALID_HANDLE_VALUE) 
	{
		do {
			// read all (real) files in current folder
			// , delete '!' read other 2 default folder . and ..
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) 
			{
				char c = fd.cFileName[skiptocharpos];
				char FullPath[2500];
				sprintf_s(FullPath, sizeof(FullPath), "%s/%s", Path, fd.cFileName);
				OCR_RegisterFont(FullPath, c);
			}
		} while (::FindNextFile(hFind, &fd));
		::FindClose(hFind);
	}
}
