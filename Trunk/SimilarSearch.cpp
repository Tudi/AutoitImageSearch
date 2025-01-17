#include "StdAfx.h"

//lagest number so that it will be smaller than our cached image width / height
LIBRARY_API int SimilarSearchGroupingSizeX = 1024;
LIBRARY_API int SimilarSearchGroupingSizeY = 1024;
LIBRARY_API int SimilarSearchResizeStep = 1;
LIBRARY_API int SimilarSearchLinkSize = 0;
LIBRARY_API int SimilarSearchSearchType = SS_SEARCH_TYPE_SUMMED_PIXELS;
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
		MY_FREE( R );
#if !( defined( SS_SUM_RGB ) || defined( ADD_COLOR_LOCALIZATION_DIAG3RGB ) || defined( ADD_COLOR_LOCALIZATION_MULBUGRGB ) || defined( ADD_COLOR_LOCALIZATION_2MULBUGRGB ) )
		MY_FREE( G );
		MY_FREE( B );
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

	if( Width * Height >= ( 1 << 16 ) )
	{
		FileDebug( "!Warning : values might overflow !" );
	}

	if( SimilarSearchSearchType == SS_SEARCH_TYPE_SUMMED_PIXELS )
	{
		assert( G != NULL && B != NULL );
		int ShiftedRSum = 0;
		int ShiftedGSum = 0;
		int ShiftedBSum = 0;
		for( int y=0;y<Height;y+=1)
		{
			LPCOLORREF PixelsRow1 = &Pixels[ y * Stride ];
			for( int x=0;x<Width;x+=1)
			{
				int Pixel1 = PixelsRow1[ x ];
				ShiftedRSum += ( Pixel1 & 0x000000FF );
				ShiftedGSum += ( Pixel1 & 0x0000FF00 );
				ShiftedBSum += ( ( Pixel1 & 0x00FF0000 ) >> 16 );	
			}
		}/**/
		ShiftedRSum = ShiftedRSum;
		ShiftedGSum = ShiftedGSum >> 8;
		ShiftedBSum = ShiftedBSum >> 0;
		R[0] += (int)( ShiftedRSum );
		G[0] += (int)( ShiftedGSum );
		B[0] += (int)( ShiftedBSum );
	}
	else if( SimilarSearchSearchType == SS_SEARCH_TYPE_BUGGED_LINKED_PIXELS )
	{
		//this makes no sense for similarity test. Links 2 pixels to be 1 pixel but you can not use SAD to guess if it is better or not
		for( int y=0;y<Height;y+=SimilarSearchResizeStep)
			for( int x=0;x<Width;x+=SimilarSearchResizeStep)
			{
				double MyMul = (double)( 0x00010101 | Pixels[ (y + 0 ) * Stride + ( x + 0 ) ] );
				MyMul *= (double)( 0x00010101 | Pixels[ ( y + SimilarSearchLinkSize ) * Stride + ( x + SimilarSearchLinkSize ) ] );
				R[0] += (int)sqrt( MyMul );
			}/**/
	}
	else if( SimilarSearchSearchType == SS_SEARCH_TYPE_LINKED_SUMMED_PIXELS2 )
	{
		// can not differentiate textures, only luminosity of a pixel
		for( int y=0;y<Height;y+=SimilarSearchResizeStep)
			for( int x=0;x<Width;x+=SimilarSearchResizeStep)
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
		int ShiftedRSum = 0;
		int ShiftedGSum = 0;
		int ShiftedBSum = 0;
		for( int y=0;y<Height;y+=SimilarSearchResizeStep)
		{
			LPCOLORREF PixelsRow1 = &Pixels[ ( y + 0 ) * Stride ];
			LPCOLORREF PixelsRow2 = &Pixels[ ( y + SimilarSearchLinkSize ) * Stride ];
			for( int x=0;x<Width;x+=SimilarSearchResizeStep)
			{
/*				int Pixel, r1,g1,b1,r2,g2,b2;
				Pixel = 0x00010101 | Pixels[ ( y + 0 ) * Stride + ( x + 0 ) ];
				r1 = ( Pixel >> 0 ) & 0xFF;
				g1 = ( Pixel >> 8 ) & 0xFF;
				b1 = ( Pixel >> 16 ) & 0xFF;
				Pixel = 0x00010101 | Pixels[ ( y + SimilarSearchLinkSize ) * Stride + ( x + SimilarSearchLinkSize ) ];
				r2 = ( Pixel >> 0 ) & 0xFF;
				g2 = ( Pixel >> 8 ) & 0xFF;
				b2 = ( Pixel >> 16 ) & 0xFF;
				R[0] += (int)( r1 * r2 + g1 * g2 + b1 * b2 ); */
				int Pixel1, Pixel2;
				Pixel1 = 0x00010101 | PixelsRow1[ x ];
				Pixel2 = 0x00010101 | PixelsRow2[ x + SimilarSearchLinkSize ];
				ShiftedRSum += ( Pixel1 & 0x000000FF ) * ( Pixel2 & 0x000000FF );
				ShiftedGSum += ( Pixel1 & 0x0000FF00 ) * ( Pixel2 & 0x0000FF00 );
				ShiftedBSum += ( ( Pixel1 & 0x00FF0000 ) >> 16 ) * ( ( Pixel2 & 0x00FF0000 ) >> 16 );	
			}
		}/**/
		ShiftedRSum = ShiftedRSum;
		ShiftedGSum = ShiftedGSum >> 8;
//		ShiftedBSum = ShiftedBSum >> 16;
		R[0] += (int)( ShiftedRSum + ShiftedGSum + ShiftedBSum );
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
		MY_FREE( R );
#if !( defined( SS_SUM_RGB ) || defined( ADD_COLOR_LOCALIZATION_DIAG3RGB ) || defined( ADD_COLOR_LOCALIZATION_MULBUGRGB ) || defined( ADD_COLOR_LOCALIZATION_2MULBUGRGB ) )
		MY_FREE( G );
		MY_FREE( B );
#endif
		R = G = B = NULL;
	}
	if( R == NULL )
	{
		FileDebug( "Started building Similar search cache" );

#ifndef IMPLEMENTING_MULTI_BLOCKS
		if( SimilarSearchSearchType == SS_SEARCH_TYPE_BUGGED_LINKED_PIXELS || SimilarSearchSearchType == SS_SEARCH_TYPE_LINKED_SUMMED_PIXELS )
			SimilarSearchLinkSize = 4;
		else
			SimilarSearchLinkSize = 3;

		if( pHeight < SimilarSearchLinkSize )
			SimilarSearchLinkSize = pHeight / 3;
		if( pWidth < SimilarSearchLinkSize )
			SimilarSearchLinkSize = pWidth / 3;

		if( pWidth - SimilarSearchLinkSize <= SimilarSearchGroupingSizeX )
			SimilarSearchGroupingSizeX = pWidth - SimilarSearchLinkSize;
		if( pHeight - SimilarSearchLinkSize <= SimilarSearchGroupingSizeY )
			SimilarSearchGroupingSizeY = pHeight - SimilarSearchLinkSize;
#endif

		Width = pWidth - SimilarSearchLinkSize;
		Height = pHeight - SimilarSearchLinkSize;
		Stride = pStride;

		BlockWidth = SimilarSearchGroupingSizeX;
		BlockHeight = SimilarSearchGroupingSizeY;
		R = (int*)MY_ALLOC( Width * Height * sizeof( int ) + SSE_PADDING );
#if !( defined( SS_SUM_RGB ) || defined( ADD_COLOR_LOCALIZATION_DIAG3RGB ) || defined( ADD_COLOR_LOCALIZATION_MULBUGRGB ) || defined( ADD_COLOR_LOCALIZATION_2MULBUGRGB ) )
		G = (int*)MY_ALLOC( Width * Height * sizeof( int ) + SSE_PADDING );
		B = (int*)MY_ALLOC( Width * Height * sizeof( int ) + SSE_PADDING );
#endif

#ifndef IMPLEMENTING_MULTI_BLOCKS
		if( SimilarSearchOnlySearchOnDiffMask == 0 || Pixels != CurScreenshot->Pixels )
		{
			for( int y=0;y<=Height - BlockHeight;y++)
				for( int x=0;x<=Width - BlockWidth;x++)
#if !( defined( SS_SUM_RGB ) || defined( ADD_COLOR_LOCALIZATION_DIAG3RGB ) || defined( ADD_COLOR_LOCALIZATION_MULBUGRGB ) || defined( ADD_COLOR_LOCALIZATION_2MULBUGRGB ) )
					GetPictureSumAtLoc( &Pixels[y*pStride+x], BlockWidth, BlockHeight, pStride, &R[ y * Width + x ], &G[ y * Width + x ], &B[ y * Width + x ] );
#else
					GetPictureSumAtLoc( &Pixels[y*pStride+x], BlockWidth, BlockHeight, pStride, &R[ y * Width + x ], NULL, NULL );
#endif
		}
		else
		{
			if( MotionDiff.GetWidth() != CurScreenshot->GetWidth() || MotionDiff.GetHeight() != CurScreenshot->GetHeight() )
			{
				FileDebug( "\t WARNING : Diff map seems to be outdated compared to screenshot" );
			}
			int MotionDiffStride = MotionDiff.GetWidth();
			unsigned char *MDMask = (unsigned char *)MotionDiff.Pixels;
			for( int y=0;y<=Height - BlockHeight;y++)
				for( int x=0;x<=Width - BlockWidth;x++)
					if( MDMask[ y / 4 * MotionDiffStride + x / 4 ] )
#if !( defined( SS_SUM_RGB ) || defined( ADD_COLOR_LOCALIZATION_DIAG3RGB ) || defined( ADD_COLOR_LOCALIZATION_MULBUGRGB ) || defined( ADD_COLOR_LOCALIZATION_2MULBUGRGB ) )
						GetPictureSumAtLoc( &Pixels[y*pStride+x], BlockWidth, BlockHeight, pStride, &R[ y * Width + x ], &G[ y * Width + x ], &B[ y * Width + x ] );
#else
						GetPictureSumAtLoc( &Pixels[y*pStride+x], BlockWidth, BlockHeight, pStride, &R[ y * Width + x ], NULL, NULL );
#endif
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
	int index = y * SearchIn->Width + x;
	int RDiff = SearchIn->R[ index ] - SearchFor->R[ 0 ];
	int GDiff = SearchIn->G[ index ] - SearchFor->G[ 0 ];
	int BDiff = SearchIn->B[ index ] - SearchFor->B[ 0 ];
	RDiff |= 1;
	GDiff |= 1;
	BDiff |= 1;
#endif
	double RD = RDiff;
	double GD = GDiff;
	double BD = BDiff;
//	double diff = RD * GD * BD;
	double diff = RD * RD + GD * GD + BD * BD;
	if( diff < 0 )
		diff = -diff;
	return diff;
}
#endif

int GetNextBestMatchIntrinsic32Bit( SimilarSearch *SearchIn, SimilarSearch *SearchFor, int &retx, int &rety )
{
	retx = rety = -1;
	int SmallImg = SearchFor->R[0];
	int EndYIntrinsic = SearchIn->Height-SearchIn->BlockHeight;
#define INT_COUNT_PROCESS	4
	int EndXIntrinsic = ( ( SearchIn->Width-SearchIn->BlockWidth ) / INT_COUNT_PROCESS ) * INT_COUNT_PROCESS;

	int BestScore = 0x07FFFFFF;
	int SmallImgVect[4] = { SmallImg, SmallImg, SmallImg, SmallImg };
	__m128i l0, l1, subres1, absres1;
//	__m128i l2, l3, l4, subres, subres2, subres3, subres4, absres2, absres3, absres4;

	l0 = _mm_loadu_si128((__m128i*)SmallImgVect);
	for( int y = 0; y < EndYIntrinsic; y++ )
	{
		int *Addr1 = &SearchIn->R[ y * SearchIn->Width ];
		for( int x=0;x<EndXIntrinsic;x+=INT_COUNT_PROCESS)
		{
			l1 = _mm_loadu_si128((__m128i*)Addr1);
//			l2 = _mm_loadu_si128((__m128i*)(Addr1+4));
//			l3 = _mm_loadu_si128((__m128i*)(Addr1+8));
//			l4 = _mm_loadu_si128((__m128i*)(Addr1+12));
			Addr1 += INT_COUNT_PROCESS;

			subres1 = _mm_sub_epi32( l0, l1 );
//			subres2 = _mm_sub_epi32( l0, l2 );
//			subres3 = _mm_sub_epi32( l0, l3 );
//			subres4 = _mm_sub_epi32( l0, l3 );
			absres1 = _mm_abs_epi32( subres1 );
//			absres2 = _mm_abs_epi32( subres2 );
//			absres3 = _mm_abs_epi32( subres3 );
//			absres4 = _mm_abs_epi32( subres4 );

			int TempRes[INT_COUNT_PROCESS];
			_mm_storeu_si128((__m128i*)TempRes, absres1);
//			_mm_storeu_si128((__m128i*)(TempRes+4), absres2);
//			_mm_storeu_si128((__m128i*)(TempRes+8), absres3);
//			_mm_storeu_si128((__m128i*)(TempRes+12), absres4);
			for( int t=0;t<INT_COUNT_PROCESS;t++)
			{
				if( TempRes[t] < BestScore )
				{
					BestScore = TempRes[t];
					retx = x+t;
					rety = y;
#ifndef _CONSOLE
					if( BestScore == 0 )
					{
						y = EndYIntrinsic;
						break;
					}
#endif
				}
			}
		}
	}

#ifndef _CONSOLE
	if( BestScore != 0 )
#endif
	{
		for( int y=0;y<SearchIn->Height-SearchIn->BlockHeight;y++)
		{
			int *Row = &SearchIn->R[ y * SearchIn->Width ];
			for( int x=EndXIntrinsic;x<SearchIn->Width-SearchIn->BlockWidth;x++)
			{
				int ScoreHere = abs( Row[ x ] - SmallImg );
				if( ScoreHere < BestScore )
				{
					BestScore = ScoreHere;
					retx = x;
					rety = y;
#ifndef _CONSOLE
					if( ScoreHere == 0 )
					{
						y = SearchIn->Height;
						break;
					}
#endif
				}
			}
		}
	}
	return (int)( BestScore / (double)SearchFor->Height / (double)SearchFor->Width );
}

//this function could use multhi threaded search. Not sure if it would speed it up as bottleneck is probably Data starvation and not CPU
int GetNextBestMatch( SimilarSearch *SearchIn, SimilarSearch *SearchFor, int &retx, int &rety )
{
#if defined( SS_SUM_RGB ) || defined( ADD_COLOR_LOCALIZATION_DIAG3RGB ) || defined( ADD_COLOR_LOCALIZATION_MULBUGRGB ) || defined( ADD_COLOR_LOCALIZATION_2MULBUGRGB )
//	if( SimilarSearchOnlySearchOnDiffMask == 0 )
//		return GetNextBestMatchIntrinsic32Bit( SearchIn, SearchFor, retx, rety );
#endif
	if( SimilarSearchOnlySearchOnDiffMask == 0 )
	{
#if defined( SS_SUM_RGB ) || defined( ADD_COLOR_LOCALIZATION_DIAG3RGB ) || defined( ADD_COLOR_LOCALIZATION_MULBUGRGB ) || defined( ADD_COLOR_LOCALIZATION_2MULBUGRGB )
		int BestScore = 0x7FFFFFFF;
#else
		double BestScore = 1.e+60;
#endif
		retx = rety = -1;
		int SmallImg = SearchFor->R[0];
		for( int y=0;y<SearchIn->Height-SearchIn->BlockHeight;y++)
		{
			int *Row = &SearchIn->R[ y * SearchIn->Width ];
			for( int x=0;x<SearchIn->Width-SearchIn->BlockWidth;x++)
			{
#if defined( SS_SUM_RGB ) || defined( ADD_COLOR_LOCALIZATION_DIAG3RGB ) || defined( ADD_COLOR_LOCALIZATION_MULBUGRGB ) || defined( ADD_COLOR_LOCALIZATION_2MULBUGRGB )
				int ScoreHere = abs( Row[ x ] - SmallImg );
#else
				double ScoreHere = GetImageScoreAtLoc( SearchIn, SearchFor, x, y );
#endif
				if( ScoreHere < BestScore )
				{
					BestScore = ScoreHere;
					retx = x;
					rety = y;
#ifndef _CONSOLE
					if( ScoreHere == 0 )
					{
						y = SearchIn->Height;
						break;
					}
#endif
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
//	int MatchesFound = 0;
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

	if( cache->NeedsSSCache == true )
	{
		cache->SSCache->BlockWidth = 0;
		cache->SSCache->BuildFromImg( cache->Pixels, cache->Width, cache->Height, cache->Width );
		cache->NeedsSSCache = false;
	}
	if( CurScreenshot->NeedsSSCache == true )
	{
		CurScreenshot->SSCache->BlockWidth = 0;
		CurScreenshot->SSCache->BuildFromImg( CurScreenshot->Pixels, CurScreenshot->Right - CurScreenshot->Left, CurScreenshot->Bottom - CurScreenshot->Top, CurScreenshot->Right - CurScreenshot->Left );
		CurScreenshot->NeedsSSCache = false;
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
//	tretx += cache->Width / 2;
//	trety += cache->Height / 2;

	sprintf_s( SSReturnBuff, DEFAULT_STR_BUFFER_SIZE*10, "1|%d|%d|%d",tretx,trety,pixelscore);
	FileDebug( "\tFinished Similar Image search" );
	return SSReturnBuff;
}
