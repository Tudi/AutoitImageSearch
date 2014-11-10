#include "StdAfx.h"

//lagest number so that it will be smaller than our cached image width / height
LIBRARY_API int SimilarSearchGroupingSizeX = 1024;
LIBRARY_API int SimilarSearchGroupingSizeY = 1024;
LIBRARY_API int SimilarSearchResizeStep = 1;
LIBRARY_API int SimilarSearchSearchType = SS_SEARCH_TYPE_LINKED_SUMMED_PIXELS;
LIBRARY_API int SimilarSearchOnlySearchOnDiffMask = 0;

void WINAPI SetupSimilarSearch( int MaxImageSize, int DownScale, int SearchType, int OnlyAtDiffMask )
{
	if( MaxImageSize > 1 && SimilarSearchGroupingSizeX > MaxImageSize )
		SimilarSearchGroupingSizeX = MaxImageSize;
	if( MaxImageSize > 1 && SimilarSearchGroupingSizeY > MaxImageSize )
		SimilarSearchGroupingSizeY = MaxImageSize;

	if( DownScale > 0 )
		SimilarSearchResizeStep = DownScale;

	if( SearchType > SS_SEARCH_TYPE_START && SearchType < SS_SEARCH_TYPE_END )
		SimilarSearchSearchType = SearchType;

	SimilarSearchOnlySearchOnDiffMask = OnlyAtDiffMask;
}

SimilarSearch::SimilarSearch()
{
	Width = Height = BlockWidth = BlockHeight = SearchType = SearchDownScale = 0;
	R = G = B = NULL;
}

SimilarSearch::~SimilarSearch()
{
	if( R != NULL )
	{
		free( R );
#if !( defined( SS_SUM_RGB ) || defined( ADD_COLOR_LOCALIZATION_DIAG3RGB ) || defined( ADD_COLOR_LOCALIZATION_MULBUGRGB ) || defined( ADD_COLOR_LOCALIZATION_2MULBUGRGB ) )
		free( G );
		free( B );
#endif
		R = G = B = NULL;
	}
}

void GetPictureSumAtLoc( LPCOLORREF Pixels, int Width, int Height, int Stride, int *R, int *G, int *B )
{
	*R = 0;
#if !( defined( SS_SUM_RGB ) || defined( ADD_COLOR_LOCALIZATION_DIAG3RGB ) || defined( ADD_COLOR_LOCALIZATION_MULBUGRGB ) || defined( ADD_COLOR_LOCALIZATION_2MULBUGRGB ) )
	*G = *B = 0;
#endif

	int Step = Height;
	if( Width < Step )
		Step = Width;
	Step = Step / 3;

	if( SimilarSearchSearchType == SS_SEARCH_TYPE_BUGGED_LINKED_PIXELS )
	{
		//this makes no sense for similarity test. Links 2 pixels to be 1 pixel but you can not use SAD to guess if it is better or not
		for( int y=0;y<Height-Step;y+=SimilarSearchResizeStep)
			for( int x=0;x<Width-Step;x+=SimilarSearchResizeStep)
			{
				double MyMul = (double)( 0x00010101 | Pixels[ (y + 0 ) * Stride + ( x + 0 ) ] );
				MyMul *= (double)( 0x00010101 | Pixels[ ( y + Step ) * Stride + ( x + Step ) ] );
				R[0] += (int)sqrt( MyMul );
			}/**/
	}
	else if( SimilarSearchSearchType == SS_SEARCH_TYPE_SUMMED_PIXELS )
	{
		// can not differentiate textures, only luminosity of a pixel
		for( int y=0;y<Height-3;y+=SimilarSearchResizeStep)
			for( int x=0;x<Width-3;x+=SimilarSearchResizeStep)
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
	}
	else if( SimilarSearchSearchType == SS_SEARCH_TYPE_LINKED_SUMMED_PIXELS )
	{
		//this makes no sense for similarity test. Links 2 pixels to be 1 pixel but you can not use SAD to guess if it is better or not
		for( int y=0;y<Height-Step;y+=SimilarSearchResizeStep)
			for( int x=0;x<Width-Step;x+=SimilarSearchResizeStep)
			{
				int Pixel, r1,g1,b1,r2,g2,b2;
				Pixel = 0x00010101 | Pixels[ (y + 0 ) * Stride + ( x + 0 ) ];
				r1 = ( Pixel >> 0 ) & 0xFF;
				g1 = ( Pixel >> 8 ) & 0xFF;
				b1 = ( Pixel >> 16 ) & 0xFF;
				Pixel = 0x00010101 | Pixels[ ( y + Step ) * Stride + ( x + Step ) ];
				r2 = ( Pixel >> 0 ) & 0xFF;
				g2 = ( Pixel >> 8 ) & 0xFF;
				b2 = ( Pixel >> 16 ) & 0xFF;
				R[0] += (int)( r1 * r2 + g1 * g2 + b1 * b2 );
			}/**/
	}

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
#elif defined( ADD_COLOR_LOCALIZATION_2MULBUGRGB )
	//this makes no sense for similarity test
	int MinSize = Height;
	if( Width < MinSize )
		MinSize = Width;
	for( int k = 0; k < MinSize; k++ )
	{
		double MyMul1 = 1;
		double MyMul2 = 1;
		for( int i=0;i<MinSize-k;i++)
		{
			MyMul1 *= (double)( 0x00010101 | Pixels[ ( k + i ) * Stride + 0 + i ] );
			MyMul2 *= (double)( 0x00010101 | Pixels[ ( 0 + i ) * Stride + k + i ] );
		}
		R[0] += (int)sqrt( MyMul1 );
		R[0] += (int)sqrt( MyMul2 );
	}
	for( int y=0;y<Height;y+=3)
	{
		for( int x=0;x<Width;x+=1)
		{
			double MyMul = 1;
			for( int i=0;i<3;i++)
				MyMul *= (double)( 0x00010101 | Pixels[ ( y + i ) * Stride + x + i ] );
			R[0] += (int)sqrt( MyMul );
		}
	}
#elif defined( ADD_COLOR_LOCALIZATION_MULBUGRGB )
	//this makes no sense for similarity test
	int MinSize = Height;
	if( Width < MinSize )
		MinSize = Width;
	MinSize = MinSize / 3;
	int StepY = MinSize,StepX = MinSize;
	for( int y=0;y<Height-StepY;y+=SimilarSearchResizeStep)
		for( int x=0;x<Width-StepX;x+=SimilarSearchResizeStep)
		{
			double MyMul = (double)( 0x00010101 | Pixels[ (y + 0 ) * Stride + ( x + 0 ) ] );
			MyMul *= (double)( 0x00010101 | Pixels[ ( y + StepY ) * Stride + ( x + StepX ) ] );
			R[0] += (int)sqrt( MyMul );
		}/**/
#elif defined( ADD_COLOR_LOCALIZATION_SIMPLESUM )
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
	if( R != NULL && ( pWidth != Width || pHeight != Height || SimilarSearchGroupingSizeX != BlockWidth || BlockHeight != SimilarSearchGroupingSizeY || SearchType != SimilarSearchSearchType || SearchDownScale != SimilarSearchResizeStep ) )
	{
		free( R );
#if !( defined( SS_SUM_RGB ) || defined( ADD_COLOR_LOCALIZATION_DIAG3RGB ) || defined( ADD_COLOR_LOCALIZATION_MULBUGRGB ) || defined( ADD_COLOR_LOCALIZATION_2MULBUGRGB ) )
		free( G );
		free( B );
#endif
		R = G = B = NULL;
	}
	if( R == NULL )
	{
		FileDebug( "Started building Similar search cache" );
		Width = pWidth;
		Height = pHeight;

#ifndef IMPLEMENTING_MULTI_BLOCKS
		if( Width <= SimilarSearchGroupingSizeX )
			SimilarSearchGroupingSizeX = Width - 1;
		if( Height <= SimilarSearchGroupingSizeY )
			SimilarSearchGroupingSizeY = Height - 1;
#endif

		BlockWidth = SimilarSearchGroupingSizeX;
		BlockHeight = SimilarSearchGroupingSizeY;
		R = (int*)malloc( Width * Height * sizeof( int ) );
#if !( defined( SS_SUM_RGB ) || defined( ADD_COLOR_LOCALIZATION_DIAG3RGB ) || defined( ADD_COLOR_LOCALIZATION_MULBUGRGB ) || defined( ADD_COLOR_LOCALIZATION_2MULBUGRGB ) )
		G = (int*)malloc( Width * Height * sizeof( int ) );
		B = (int*)malloc( Width * Height * sizeof( int ) );
#endif

#ifndef IMPLEMENTING_MULTI_BLOCKS
		if( SimilarSearchOnlySearchOnDiffMask == 0 || Pixels != CurScreenshot->Pixels )
		{
			for( int y=0;y<Height - BlockHeight;y++)
				for( int x=0;x<Width - BlockWidth;x++)
					GetPictureSumAtLoc( &Pixels[y*pStride+x], BlockWidth, BlockHeight, pStride, &R[ y * Width + x ], &G[ y * Width + x ], &B[ y * Width + x ] );
		}
		else
		{
			if( MotionDiff.GetWidth() != CurScreenshot->GetWidth() || MotionDiff.GetHeight() != CurScreenshot->GetHeight() )
			{
				FileDebug( "\t WARNING : Diff map seems to be outdated compared to screenshot" );
			}
			int MotionDiffStride = MotionDiff.GetWidth();
			unsigned char *MDMask = (unsigned char *)MotionDiff.Pixels;
			for( int y=0;y<Height - BlockHeight;y++)
				for( int x=0;x<Width - BlockWidth;x++)
					if( MDMask[ y / 4 * MotionDiffStride + x / 4 ] )
						GetPictureSumAtLoc( &Pixels[y*pStride+x], BlockWidth, BlockHeight, pStride, &R[ y * Width + x ], &G[ y * Width + x ], &B[ y * Width + x ] );
		}
#endif
		FileDebug( "\t Finished building Similar search cache" );
	}
}

#if defined( SS_SUM_RGB ) || defined( ADD_COLOR_LOCALIZATION_DIAG3RGB ) || defined( ADD_COLOR_LOCALIZATION_MULBUGRGB ) || defined( ADD_COLOR_LOCALIZATION_2MULBUGRGB )
__forceinline int GetImageScoreAtLoc( SimilarSearch *SearchIn, SimilarSearch *SearchFor, int x, int y )
{
	int RGBDiff = SearchIn->R[ y * SearchIn->Width + x ] - SearchFor->R[ 0 ];
	if( RGBDiff < 0 )
		RGBDiff = -RGBDiff;
	return RGBDiff;
}
#else
double GetImageScoreAtLoc( SimilarSearch *SearchIn, SimilarSearch *SearchFor, int x, int y )
{
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
#endif

int GetNextBestMatch( SimilarSearch *SearchIn, SimilarSearch *SearchFor, int &retx, int &rety )
{
	if( SimilarSearchOnlySearchOnDiffMask == 0 )
	{
#if defined( SS_SUM_RGB ) || defined( ADD_COLOR_LOCALIZATION_DIAG3RGB ) || defined( ADD_COLOR_LOCALIZATION_MULBUGRGB ) || defined( ADD_COLOR_LOCALIZATION_2MULBUGRGB )
		int BestScore = 0x7FFFFFFF;
#else
		double BestScore = 1.e+60;
#endif
		retx = rety = -1;
		for( int y=0;y<SearchIn->Height-SearchFor->Height;y++)
		{
			for( int x=0;x<SearchIn->Width-SearchFor->Width;x++)
			{
#if defined( SS_SUM_RGB ) || defined( ADD_COLOR_LOCALIZATION_DIAG3RGB ) || defined( ADD_COLOR_LOCALIZATION_MULBUGRGB ) || defined( ADD_COLOR_LOCALIZATION_2MULBUGRGB )
				int ScoreHere = GetImageScoreAtLoc( SearchIn, SearchFor, x, y );
#else
				double ScoreHere = GetImageScoreAtLoc( SearchIn, SearchFor, x, y );
#endif
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
					if( ScoreHere == 0 )
					{
						y = SearchIn->Height;
						break;
					}
				}
			}
		}
		return (int)( BestScore / (double)SearchFor->Height / (double)SearchFor->Width );
	}
	else if( SimilarSearchOnlySearchOnDiffMask == 0 )
	{
		double BestScore = 1.e+60;
		retx = rety = -1;
		int MotionDiffStride = MotionDiff.GetWidth();
		unsigned char *MDMask = (unsigned char *)MotionDiff.Pixels;
		for( int y=0;y<SearchIn->Height-SearchFor->Height;y++)
			for( int x=0;x<SearchIn->Width-SearchFor->Width;x++)
				if( MDMask[ y / 4 * MotionDiffStride + x / 4 ] )
				{
					double ScoreHere = GetImageScoreAtLoc( SearchIn, SearchFor, x, y );
					if( ScoreHere < BestScore )
					{
						BestScore = ScoreHere;
						retx = x;
						rety = y;
						if( ScoreHere == 0 )
						{
							y = SearchIn->Height;
							break;
						}
					}
				}
		return (int)( BestScore / (double)SearchFor->Height / (double)SearchFor->Width );
	}
	return 0;
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
	if( CurScreenshot->NeedsSScache == true )
	{
		CurScreenshot->SSCache->BlockWidth = 0;
		CurScreenshot->SSCache->BuildFromImg( CurScreenshot->Pixels, CurScreenshot->Right - CurScreenshot->Left, CurScreenshot->Bottom - CurScreenshot->Top, CurScreenshot->Right - CurScreenshot->Left );
		CurScreenshot->NeedsSScache = false;
	}

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
