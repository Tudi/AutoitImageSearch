#include "StdAfx.h"

//lagest number so that it will be smaller than our cached image width / height
LIBRARY_API int SimilarSearchGroupingSizeX = 1024;
LIBRARY_API int SimilarSearchGroupingSizeY = 1024;

SimilarSearch::SimilarSearch()
{
	Width = Height = BlockWidth = BlockHeight = 0;
	R = G = B = NULL;
}

SimilarSearch::~SimilarSearch()
{
	if( R != NULL )
	{
		free( R );
		free( G );
		free( B );
		R = G = B = NULL;
	}
}

int sqrt1( int N )
{
//	int a = 1;
//	int b = N;

	int b = N / 1;
	int a = ( 1 + b ) / 2;

//	int b = N / ( ( 1 + N ) / 2 );
//	int a = ( ( 1 + N ) / 2 + N / ( ( 1 + N ) / 2 ) ) / 2;

	while( b > 1 + a )
	{
		b = N / a;
		a = (a + b) / 2;
	}
	return a;
}

void GetPictureSumAtLoc( LPCOLORREF Pixels, int Width, int Height, int Stride, int *R, int *G, int *B )
{
	*R = *G = *B = 0;
#if defined( ADD_COLOR_LOCALIZATION_4x4 )

	for( int y=0;y<Height;y+=2)
		for( int x=0;x<Width;x+=2)
		{
			int t;
			t = 1;
			for( int ty=0;ty<2;ty++)
				for( int tx=0;tx<2;tx++)
					t = t * ( ( Pixels[ ( y + ty ) * Stride + x + tx ] >> 0 ) & 0xFF );
			R[0] += t;

			t = 1;
			for( int ty=0;ty<2;ty++)
				for( int tx=0;tx<2;tx++)
					t = t * ( ( Pixels[ ( y + ty ) * Stride + x + tx ] >> 8 ) & 0xFF );
			G[0] += t;

			t = 1;
			for( int ty=0;ty<2;ty++)
				for( int tx=0;tx<2;tx++)
					t = t * ( ( Pixels[ ( y + ty ) * Stride + x + tx ] >> 16 ) & 0xFF );
			B[0] += t;
		}
#elif defined( ADD_COLOR_LOCALIZATION_DIAG2 )
	for( int y=0;y<Height-2;y+=2)
		for( int x=0;x<Width-2;x+=2)
		{
			int t;
			t = (1+(( Pixels[ ( y + 0 ) * Stride + x + 0 ] >> 0 ) & 0xFF )) * (1+(( Pixels[ ( y + 1 ) * Stride + x + 1 ] >> 0 ) & 0xFF ));
			t += (1+(( Pixels[ ( y + 1 ) * Stride + x + 0 ] >> 0 ) & 0xFF )) * (1+(( Pixels[ ( y + 0 ) * Stride + x + 1 ] >> 0 ) & 0xFF ));
			R[0] += t;

			t = (1+(( Pixels[ ( y + 0 ) * Stride + x + 0 ] >> 8 ) & 0xFF )) * (1+(( Pixels[ ( y + 1 ) * Stride + x + 1 ] >> 8 ) & 0xFF ));
			t += (1+(( Pixels[ ( y + 1 ) * Stride + x + 0 ] >> 8 ) & 0xFF )) * (1+(( Pixels[ ( y + 0 ) * Stride + x + 1 ] >> 8 ) & 0xFF ));
			G[0] += t;

			t = (1+(( Pixels[ ( y + 0 ) * Stride + x + 0 ] >> 16 ) & 0xFF )) * (1+(( Pixels[ ( y + 1 ) * Stride + x + 1 ] >> 16 ) & 0xFF ));
			t += (1+(( Pixels[ ( y + 1 ) * Stride + x + 0 ] >> 16 ) & 0xFF )) * (1+(( Pixels[ ( y + 0 ) * Stride + x + 1 ] >> 16 ) & 0xFF ));
			B[0] += t;
		}
#elif defined( ADD_COLOR_LOCALIZATION_DIAG3 )
	for( int y=0;y<Height-3;y+=3)
		for( int x=0;x<Width-3;x+=3)
		{
			int tr,tg,tb;
			tr = tg = tb = 1;
			for( int i=0;i<3;i++)
			{
				int Pixel = Pixels[ ( y + i ) * Stride + x + i ];
				tr *= (1+(( Pixel >> 0 ) & 0xFF ));
				tg *= (1+(( Pixel >> 8 ) & 0xFF ));
				tb *= (1+(( Pixel >> 16 ) & 0xFF ));
			}
			R[0] += tr;
			G[0] += tg;
			B[0] += tb;
			tr = tg = tb = 1;
			for( int i=0;i<3;i++)
			{
				int Pixel = Pixels[ ( y + i ) * Stride + x + 3 - 1 - i ];
				tr *= (1+(( Pixel >> 0 ) & 0xFF ));
				tg *= (1+(( Pixel >> 8 ) & 0xFF ));
				tb *= (1+(( Pixel >> 16 ) & 0xFF ));
			}
			R[0] += tr;
			G[0] += tg;
			B[0] += tb;
			tr = tg = tb = 1;
			{
				int Pixel = Pixels[ ( y + 0 ) * Stride + x + 1 ];
				tr *= (1+(( Pixel >> 0 ) & 0xFF ));
				tg *= (1+(( Pixel >> 8 ) & 0xFF ));
				tb *= (1+(( Pixel >> 16 ) & 0xFF ));
			}
			{
				int Pixel = Pixels[ ( y + 2 ) * Stride + x + 1 ];
				tr *= (1+(( Pixel >> 0 ) & 0xFF ));
				tg *= (1+(( Pixel >> 8 ) & 0xFF ));
				tb *= (1+(( Pixel >> 16 ) & 0xFF ));
			}
			R[0] += tr;
			G[0] += tg;
			B[0] += tb;
			tr = tg = tb = 1;
			{
				int Pixel = Pixels[ ( y + 1 ) * Stride + x + 0 ];
				tr *= (1+(( Pixel >> 0 ) & 0xFF ));
				tg *= (1+(( Pixel >> 8 ) & 0xFF ));
				tb *= (1+(( Pixel >> 16 ) & 0xFF ));
			}
			{
				int Pixel = Pixels[ ( y + 1 ) * Stride + x + 2 ];
				tr *= (1+(( Pixel >> 0 ) & 0xFF ));
				tg *= (1+(( Pixel >> 8 ) & 0xFF ));
				tb *= (1+(( Pixel >> 16 ) & 0xFF ));
			}
			R[0] += tr;
			G[0] += tg;
			B[0] += tb;
		}
#elif defined( ADD_COLOR_LOCALIZATION_HOR3 )
	for( int y=0;y<Height;y++)
		for( int x=0;x<Width-3;x+=3)
		{
			int tr,tg,tb;
			tr = tg = tb = 1;
			for( int i=0;i<3;i++)
			{
				int Pixel = Pixels[ y * Stride + x + i ];
				Pixel |= 0x00010101;
				tr *= (( Pixel >> 0 ) & 0xFF );
				tg *= (( Pixel >> 8 ) & 0xFF );
				tb *= (( Pixel >> 16 ) & 0xFF );
			}
			R[0] += tr;
			G[0] += tg;
			B[0] += tb;
		}
#elif defined( ADD_COLOR_LOCALIZATION_HOR4 )
	for( int y=0;y<Height;y++)
		for( int x=0;x<Width-4;x+=4)
		{
			int tr,tg,tb;
			tr = tg = tb = 1;
			for( int i=0;i<4;i++)
			{
				int Pixel = Pixels[ y * Stride + x + i ];
				Pixel |= 0x00010101;
				tr *= (( Pixel >> 1 ) & 0xFF );
				tg *= (( Pixel >> 9 ) & 0xFF );
				tb *= (( Pixel >> 17 ) & 0xFF );
			}
			R[0] += tr;
			G[0] += tg;
			B[0] += tb;
		}
#elif defined( ADD_COLOR_LOCALIZATION_DIAG3_1 )
	for( int y=0;y<Height-3;y+=3)
		for( int x=0;x<Width-3;x+=1)
		{
			int tr,tg,tb;
			tr = tg = tb = 1;
			for( int i=0;i<3;i++)
			{
				int Pixel = Pixels[ ( y + i ) * Stride + x + i ];
				Pixel |= 0x00010101;
				tr *= (( Pixel >> 0 ) & 0xFF );
				tg *= (( Pixel >> 8 ) & 0xFF );
				tb *= (( Pixel >> 16 ) & 0xFF );
			}
			R[0] += tr;
			G[0] += tg;
			B[0] += tb;
		}
#elif defined( ADD_COLOR_LOCALIZATION_DIAG5_OVERF )
	for( int y=0;y<Height-5;y+=5)
		for( int x=0;x<Width-5;x+=1)
		{
			int tr,tg,tb;
			tr = tg = tb = 1;
			for( int i=0;i<5;i++)
			{
				int Pixel = Pixels[ ( y + i ) * Stride + x + i ];
				Pixel |= 0x00010101;
				tr *= (( Pixel >> 0 ) & 0xFF );
				tg *= (( Pixel >> 8 ) & 0xFF );
				tb *= (( Pixel >> 16 ) & 0xFF );
			}
			R[0] += tr;
			G[0] += tg;
			B[0] += tb;
		}
#elif defined( ADD_COLOR_LOCALIZATION_DIAG6_OVERF )
	for( int y=0;y<Height-6;y+=6)
		for( int x=0;x<Width-6;x+=1)
		{
			int tr,tg,tb;
			tr = tg = tb = 1;
			for( int i=0;i<6;i++)
			{
				int Pixel = Pixels[ ( y + i ) * Stride + x + i ];
				Pixel |= 0x00010101;
				tr *= (( Pixel >> 0 ) & 0xFF );
				tg *= (( Pixel >> 8 ) & 0xFF );
				tb *= (( Pixel >> 16 ) & 0xFF );
			}
			R[0] += tr;
			G[0] += tg;
			B[0] += tb;
		}
#elif defined( ADD_COLOR_LOCALIZATION_HOR4_DIV4 )
	for( int y=0;y<Height;y++)
		for( int x=0;x<Width-4;x+=4)
		{
			int tr,tg,tb;
			tr = tg = tb = 1;
			for( int i=0;i<4;i++)
			{
				int Pixel = Pixels[ y * Stride + x + i ];
				tr *= (1+(( Pixel >> ( 0 + 2 ) ) & 0xFF ));
				tg *= (1+(( Pixel >> ( 8 + 2 ) ) & 0xFF ));
				tb *= (1+(( Pixel >> ( 16 + 2 ) ) & 0xFF ));
			}
			R[0] += tr;
			G[0] += tg;
			B[0] += tb;
		}
#elif defined( ADD_COLOR_LOCALIZATION_DIAG3RGB )
	for( int y=0;y<Height-3;y+=3)
		for( int x=0;x<Width-3;x+=3)
		{
#define GetRGBSumFromPixel( Pixel ) ( 1 + ( ( Pixel >> 0 ) & 0xFF ) + ( ( Pixel >> 8 ) & 0xFF ) + ( ( Pixel >> 16 ) & 0xFF ) )
			int t;
			t = 1;
			for( int i=0;i<3;i++)
				t *= GetRGBSumFromPixel( Pixels[ ( y + i ) * Stride + x + i ] );
			R[0] += t;
			t = 1;
			for( int i=0;i<3;i++)
				t *= GetRGBSumFromPixel( Pixels[ ( y + i ) * Stride + x + 3 - 1 - i ] );
			R[0] += t;
			t = GetRGBSumFromPixel( Pixels[ ( y + 0 ) * Stride + x + 1 ] );
			t *= GetRGBSumFromPixel( Pixels[ ( y + 2 ) * Stride + x + 1 ] );
			R[0] += t;
			t = GetRGBSumFromPixel( Pixels[ ( y + 1 ) * Stride + x + 0 ] );
			t *= GetRGBSumFromPixel( Pixels[ ( y + 1 ) * Stride + x + 2 ] );
			R[0] += t;
		}
#elif defined( ADD_COLOR_LOCALIZATION_XOR_ROW )
	for( int y=0;y<Height;y+=1)
	{
		int Pixel = 0;
		for( int x=0;x<Width;x+=1)
			Pixel ^= Pixels[ y * Stride + x ];
		R[0] += (( Pixel >> 0 ) & 0xFF );
		G[0] += (( Pixel >> 8 ) & 0xFF );
		B[0] += (( Pixel >> 16 ) & 0xFF );
	}
#elif defined( ADD_COLOR_LOCALIZATION_ADDBUG )
	for( int y=0;y<Height;y+=1)
	{
		for( int x=0;x<Width-4;x+=4)
		{
			int sub1 = Pixels[ y * Stride + x + 0 ] - Pixels[ y * Stride + x + 1 ];
			int sub2 = Pixels[ y * Stride + x + 2 ] - Pixels[ y * Stride + x + 3 ];
			int Pixel = sub1 * sub2;
			R[0] += (( Pixel >> 0 ) & 0xFF );
			G[0] += (( Pixel >> 8 ) & 0xFF );
			B[0] += (( Pixel >> 16 ) & 0xFF );
		}
	}
#elif defined( ADD_COLOR_LOCALIZATION_MULBUGRGB )
	for( int y=0;y<Height;y+=1)
	{
//		for( int x=0;x<Width-2;x+=2)
		for( int x=0;x<Width-3;x+=3)
		{
//			int sub1 = sqrt( (double)( Pixels[ y * Stride + x + 0 ] * Pixels[ y * Stride + x + 1 ] ) );
//			int sub1 = sqrt1( Pixels[ y * Stride + x + 0 ] * Pixels[ y * Stride + x + 1 ] );	//release version is 2x faster than STD
			int sub1 = sqrt1( 0x00010101 | Pixels[ y * Stride + x + 0 ] * 0x00010101 | Pixels[ y * Stride + x + 1 ] );	//release version is 2x faster than STD
//			int sub1 = sqrt1( 0x00010101 | Pixels[ y * Stride + x + 0 ] * 0x00010101 | Pixels[ y * Stride + x + 1 ] * 0x00010101 | Pixels[ y * Stride + x + 3 ] );	//release version is 2x faster than STD
			R[0] += sub1;
		}
	}
#else
	for( int y=0;y<Height;y++)
		for( int x=0;x<Width;x++)
		{
			R[0] += ( ( Pixels[ y * Stride + x ] >> 0 ) & 0xFF );
			G[0] += ( ( Pixels[ y * Stride + x ] >> 8 ) & 0xFF );
			B[0] += ( ( Pixels[ y * Stride + x ] >> 16 ) & 0xFF );
		}
#endif
}

void SimilarSearch::BuildFromImg( LPCOLORREF Pixels, int pWidth, int pHeight, int pStride )
{
	if( R != NULL && ( pWidth != Width || pHeight != Height || SimilarSearchGroupingSizeX != BlockWidth || BlockHeight != SimilarSearchGroupingSizeY ) )
	{
		free( R );
		free( G );
		free( B );
		R = G = B = NULL;
	}
	if( R == NULL )
	{
		FileDebug( "Started building Similar search cache" );
		Width = pWidth;
		Height = pHeight;

#ifndef IMPLEMENTING_MULTI_BLOCKS
		if( Width < SimilarSearchGroupingSizeX )
			SimilarSearchGroupingSizeX = Width - 1;
		if( Height < SimilarSearchGroupingSizeY )
			SimilarSearchGroupingSizeY = Height - 1;
#endif

		BlockWidth = SimilarSearchGroupingSizeX;
		BlockHeight = SimilarSearchGroupingSizeY;
		R = (int*)malloc( Width * Height * sizeof( int ) );
		G = (int*)malloc( Width * Height * sizeof( int ) );
		B = (int*)malloc( Width * Height * sizeof( int ) );

#ifndef IMPLEMENTING_MULTI_BLOCKS
		for( int y=0;y<Height - BlockHeight;y++)
			for( int x=0;x<Width - BlockWidth;x++)
				GetPictureSumAtLoc( &Pixels[y*pStride+x], BlockWidth, BlockHeight, pStride, &R[ y * Width + x ], &G[ y * Width + x ], &B[ y * Width + x ] );
#endif
		FileDebug( "\t Finished building Similar search cache" );
	}
}

double GetImageScoreAtLoc( SimilarSearch *SearchIn, SimilarSearch *SearchFor, int x, int y )
{
#if defined( ADD_COLOR_LOCALIZATION_DIAG3RGB ) || defined( ADD_COLOR_LOCALIZATION_MULBUGRGB )
	int RGBDiff = SearchIn->R[ y * SearchIn->Width + x ] - SearchFor->R[ 0 ];
	if( RGBDiff < 0 )
		RGBDiff = -RGBDiff;
	return RGBDiff;
#endif
#ifndef IMPLEMENTING_MULTI_BLOCKS
	int RDiff = SearchIn->R[ y * SearchIn->Width + x ] - SearchFor->R[ 0 ];
	int GDiff = SearchIn->G[ y * SearchIn->Width + x ] - SearchFor->G[ 0 ];
	int BDiff = SearchIn->B[ y * SearchIn->Width + x ] - SearchFor->B[ 0 ];
#endif
	double RD = RDiff;
	double GD = GDiff;
	double BD = BDiff;
	double diff = RD * GD * BD;
	if( diff < 0 )
		diff = -diff;
	return diff;
}

int GetNextBestMatch( SimilarSearch *SearchIn, SimilarSearch *SearchFor, int &retx, int &rety )
{
	double BestScore = 1.e+60;
	retx = rety = -1;
	for( int y=0;y<SearchIn->Height-SearchFor->Height;y++)
	{
		for( int x=0;x<SearchIn->Width-SearchFor->Width;x++)
		{
			double ScoreHere = GetImageScoreAtLoc( SearchIn, SearchFor, x, y );
/*
//if( y % 5 == 0 && x % 5 == 0 )
if( ScoreHere == 0 )
{
char debugbuff[500];
//sprintf_s( debugbuff, 500, "SS at loc %d %d out of %d %d, score %f, sum ref %d sum img %d", x, y, SearchIn->Width-SearchFor->Width, SearchIn->Height-SearchFor->Height, (float)ScoreHere, SearchIn->R[ y * SearchIn->Width + x ], SearchFor->R[ 0 ] );
sprintf_s( debugbuff, 500, "SS at loc %d %d sums %d - %d / %d - %d / %d - %d, score %f", x, y, SearchIn->R[ y * SearchIn->Width + x ], SearchFor->R[ 0 ], SearchIn->G[ y * SearchIn->Width + x ], SearchFor->G[ 0 ], SearchIn->B[ y * SearchIn->Width + x ], SearchFor->B[ 0 ], (float)ScoreHere );
FileDebug( debugbuff );
}
/**/
			if( ScoreHere < BestScore )
			{
//FileDebug( "\tNew Best" );
				BestScore = ScoreHere;
				retx = x;
				rety = y;
			}
		}
	}
	return (int)( BestScore / (double)SearchFor->Height / (double)SearchFor->Width );
}

char SSReturnBuff[DEFAULT_STR_BUFFER_SIZE*10];
char * WINAPI SearchSimilarOnScreenshot( char *aImageFile )
{
	int MatchesFound = 0;
	SSReturnBuff[0]=0;
	FileDebug( "Started Similar Image search" );

	CachedPicture *cache = CachePicture( aImageFile );
	if( cache == NULL )
	{
		FileDebug( "Skipping Image search as image could not be loaded" );
		return "";
	}
	if( cache->Pixels == NULL )
	{
		FileDebug( "Skipping Image search as image pixels are missing" );
		return "";
	}
	if( cache->LoadedPicture == NULL )
	{
		FileDebug( "Skipping Image search as image is missing" );
		return "";
	}

	if( CurScreenshot->Pixels == NULL )
	{
		FileDebug( "Skipping Image search no screenshot is available" );
		return "";
	}

	if( cache->SSCache == NULL )
		cache->SSCache = new SimilarSearch;
	if( CurScreenshot->SSCache == NULL )
		CurScreenshot->SSCache = new SimilarSearch;

	cache->SSCache->BuildFromImg( cache->Pixels, cache->Width, cache->Height, cache->Width );
	CurScreenshot->SSCache->BuildFromImg( CurScreenshot->Pixels, CurScreenshot->Right - CurScreenshot->Left, CurScreenshot->Bottom - CurScreenshot->Top, CurScreenshot->Right - CurScreenshot->Left );

	if( cache->SSCache->BlockHeight != CurScreenshot->SSCache->BlockHeight || cache->SSCache->BlockWidth != CurScreenshot->SSCache->BlockWidth )
	{
		FileDebug( "Skipping Image search as block size does not match in cache. This is a bug" );
		return "";
	}

	int tretx,trety;
	int pixelscore;
	FileDebug( "\t Similar Image search is searching" );
	pixelscore = GetNextBestMatch( CurScreenshot->SSCache, cache->SSCache, tretx, trety );
	FileDebug( "\t Similar Image search done searching" );
	
	char debugbuff[500];
	sprintf_s( debugbuff, 500, "best pixel score for image %s is %d at loc %d %d", cache->FileName, pixelscore, tretx, trety );
	FileDebug( debugbuff );

	//calculate absolute positioning
	tretx += CurScreenshot->Left;
	trety += CurScreenshot->Top;

	//calculate middle of the image
	tretx += cache->Width / 2;
	trety += cache->Height / 2;

	sprintf_s( SSReturnBuff, DEFAULT_STR_BUFFER_SIZE*10, "1|%d|%d|%d",tretx,trety,pixelscore);
	FileDebug( "\tFinished Similar Image search" );
	return SSReturnBuff;
}
