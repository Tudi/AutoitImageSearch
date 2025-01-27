#include "StdAfx.h"

//disabled as i never finished / tested it
#if 1
PiramidImage::PiramidImage()
{
	memset( ImageLayers, NULL, sizeof( ImageLayers ) );
	ImageLayersX[0] = ImageLayersY[0] = 0;
	LayersAvailable = 0;
}

PiramidImage::~PiramidImage()
{
	for( int i=0;i<MAX_IMAGE_LAYERS;i++ )
		if( ImageLayers[0][i] )
		{
			MY_FREE( ImageLayers[RED_LAYER_INDEX][i] );
#if !defined( GENERATE_ONLY_R ) && !defined( MERGE_RGB_INTO_R )
			MY_FREE( ImageLayers[1][i] );
			MY_FREE( ImageLayers[2][i] );
#endif
		}
	memset( ImageLayers, NULL, sizeof( ImageLayers ) );
}

int GetPixelsInPixel( int LayerDepth )
{
	int PixelsInPixel = 1;
	for( int i = 0; i < LayerDepth; i++ )
		PixelsInPixel *= PIXEL_STEPDOWN_LAYER * PIXEL_STEPDOWN_LAYER;
	return PixelsInPixel;
}

int GetPixelsInPixel1D( int LayerDepth )
{
	int PixelsInPixel = 1;
	for( int i = 0; i < LayerDepth; i++ )
		PixelsInPixel *= PIXEL_STEPDOWN_LAYER;
	return PixelsInPixel;
}

void PiramidImage::InitBuffersToNewSize(int Width, int Height)
{
	bool Reinit = ( ( ImageLayersX[0] != Width ) || ( ImageLayersY[0] != Height ) );
	if( Reinit == true )
	{
		for( int i=0;i<MAX_IMAGE_LAYERS;i++ )
			if( ImageLayers[0][i] != NULL )
			{
				MY_FREE( ImageLayers[0][i] );
				ImageLayers[0][i] = NULL;
#if !defined( GENERATE_ONLY_R ) && !defined( MERGE_RGB_INTO_R )
				MY_FREE( ImageLayers[1][i] );
				ImageLayers[1][i] = NULL;
				MY_FREE( ImageLayers[2][i] );
				ImageLayers[2][i] = NULL;
#endif
			}
		ImageLayersX[0] = Width;
		ImageLayersY[0] = Height;

		LayersAvailable = 0;
		for( int i=0;i<MAX_IMAGE_LAYERS;i++ )
		{
				if( i > 0 )
				{
					ImageLayersX[ i ] = ( ImageLayersX[ i - 1 ] ) / PIXEL_STEPDOWN_LAYER;	//round down
					ImageLayersY[ i ] = ( ImageLayersY[ i - 1 ] ) / PIXEL_STEPDOWN_LAYER;	//round down
				}

				if( ImageLayersX[ i ] <= MIN_SIZE_FOR_SEARCH || ImageLayersY[ i ] <= MIN_SIZE_FOR_SEARCH )
					break;

				if( GetPixelsInPixel( i ) > MAX_PIXEL_COUNT_IN_PIXEL )
					break;

				int ImagePlaneSize = ImageLayersX[i] * ImageLayersY[i] * sizeof( signed int ) + SSE_PADDING;
				ImageLayers[0][i] = (int*)MY_ALLOC( ImagePlaneSize );
				memset( ImageLayers[0][i], 0, ImagePlaneSize );
#if !defined( GENERATE_ONLY_R ) && !defined( MERGE_RGB_INTO_R )
				ImageLayers[1][i] = (int*)MY_ALLOC( ImagePlaneSize );
				ImageLayers[2][i] = (int*)MY_ALLOC( ImagePlaneSize );
#endif
				LayersAvailable++;
		}
	}
}

void PiramidImage::BuildFromImg( LPCOLORREF BGRPixels, int Width, int Height, int Stride )
{
	if( BGRPixels == NULL )
	{
		FileDebug( "Exiting PiramidBuild due to missing input buffer" );
		return;
	}

	if( Stride == 0 )
		Stride = Width;

	//alloc layers
	InitBuffersToNewSize( Width, Height );

	//copy the bitmap to our buffer
#ifdef USE_DATA_LOCALITY_ON_MERGE
	int LinkPixelStep = 1;
#else
	int LinkPixelStep = 0;
#endif
	for( int y = 0; y < ImageLayersY[ 0 ] - LinkPixelStep; y += 1 )
	{
		LPCOLORREF RGBRowStart1 = &BGRPixels[ ( y + 0 ) * Stride ];
#ifdef USE_DATA_LOCALITY_ON_MERGE
		LPCOLORREF RGBRowStart2 = &BGRPixels[ ( y + LinkPixelStep ) * Stride ];
#endif
		for( int x = 0; x < ImageLayersX[ 0 ] - LinkPixelStep; x += 1 )
		{
#ifdef USE_DATA_LOCALITY_ON_MERGE
			int Pixel, r1,g1,b1,r2,g2,b2;
			Pixel = 0x00010101 | RGBRowStart1[ ( x + 0 ) ];
			r1 = ( Pixel >> 0 ) & 0xFF;
			g1 = ( Pixel >> 8 ) & 0xFF;
			b1 = ( Pixel >> 16 ) & 0xFF;
			Pixel = 0x00010101 | RGBRowStart2[ ( x + LinkPixelStep ) ];
			r2 = ( Pixel >> 0 ) & 0xFF;
			g2 = ( Pixel >> 8 ) & 0xFF;
			b2 = ( Pixel >> 16 ) & 0xFF;
			ImageLayers[RED_LAYER_INDEX][0][ y * ImageLayersX[ 0 ] + x ] = (int)( r1 * r2 + g1 * g2 + b1 * b2 );
//			ImageLayers[RED_LAYER_INDEX][0][ y * ImageLayersX[ 0 ] + x ] = (int)sqrt((double)( r1 * r2 + g1 * g2 + b1 * b2 ) );
#elif defined( MERGE_RGB_INTO_R ) || defined( GENERATE_ONLY_R )
			int Pixel, r1,g1,b1;
			Pixel = RGBRowStart1[ x ];
			r1 = ( Pixel >> 0 ) & 0xFF;
			g1 = ( Pixel >> 8 ) & 0xFF;
			b1 = ( Pixel >> 16 ) & 0xFF;
			ImageLayers[RED_LAYER_INDEX][0][ y * ImageLayersX[ 0 ] + x ] = (int)( r1 + g1 + b1 );
//			ImageLayers[RED_LAYER_INDEX][0][ y * ImageLayersX[ 0 ] + x ] = (int)sqrt((double)( r1 * r1 + g1 * g1 + b1 * b1 ));
#else
			int Pixel, r1,g1,b1;
			Pixel = RGBRowStart1[ x ];
			r1 = ( Pixel >> 0 ) & 0xFF;
			g1 = ( Pixel >> 8 ) & 0xFF;
			b1 = ( Pixel >> 16 ) & 0xFF;
			ImageLayers[RED_LAYER_INDEX][0][ y * ImageLayersX[ 0 ] + x ] = (int)( r1 );
			ImageLayers[1][0][ y * ImageLayersX[ 0 ] + x ] = (int)( g1 );
			ImageLayers[2][0][ y * ImageLayersX[ 0 ] + x ] = (int)( b1 );
#endif
		}
	}

	BuildFromImgOtherLevels();
}

void PiramidImage::BuildFromImgOtherLevels()
{
	for( int Layer=1;Layer<LayersAvailable;Layer++)
	{
		int BigerLayer = Layer - 1;

		int RGB = 0;
#if !defined( GENERATE_ONLY_R ) && !defined( MERGE_RGB_INTO_R )
		for( ; RGB < 3 ; RGB++ )
#endif
		{
			for( int y=0;y<ImageLayersY[ Layer ];y++)
				for( int x=0;x<ImageLayersX[ Layer ];x++)
				{
					int Sum = 0;
					int *BigImg = &ImageLayers[ RGB ][ BigerLayer ][ y * PIXEL_STEPDOWN_LAYER * ImageLayersX[ BigerLayer ] + x * PIXEL_STEPDOWN_LAYER ];
					for( int y1=0;y1<PIXEL_STEPDOWN_LAYER;y1++)
						for( int x1=0;x1<PIXEL_STEPDOWN_LAYER;x1++)
						{
							int SumPrevLayer = BigImg[ y1 * ImageLayersX[ BigerLayer ] + x1 ];
							Sum += SumPrevLayer;
						}
					ImageLayers[ RGB ][ Layer ][ y * ImageLayersX[ Layer ] + x ] = Sum;
				}
		}
	}
}

__forceinline int GetLocalSad( int *a, int *b, int stride1, int stride2, int w, int h )
{
	int RetSad = 0;
	for( int y1 = 0; y1 < h; y1++ )
		for( int x1 = 0; x1 < w; x1++ )
		{
			int BigImg = a[ y1 * stride1 + x1 ];
			int SmallImg = b[ y1 * stride2 + x1 ];
			RetSad += abs( BigImg - SmallImg );
#ifdef COUNT_NUMBER_OF_SADS
			SadCounter++;
#endif
		}
	return RetSad;
}

//search all layers
int PiramidSearch( PiramidImage *Big, PiramidImage *Small, int *RetX, int *RetY, int *RSad, int *GSad, int *BSad, int SadLimit, int pLayer, int AtX, int AtY, int RecCount )
{
	//in case we fail at the search, somehow signal back
	*RetX = 0;
	*RetY = 0;
	*RSad = *GSad = *BSad = MAX_INT;

	int BestRSad = MAX_INT;
#if !defined( GENERATE_ONLY_R ) && !defined( MERGE_RGB_INTO_R )
	int BestGSad = MAX_INT;
	int BestBSad = MAX_INT;
#endif

	//search in worst version only in R
	int LayerStart = Small->LayersAvailable - 1;
	int LayerEnd = -1;
	if( pLayer != -1 )
	{
		LayerStart = pLayer;
		LayerEnd = pLayer-1;
		if( LayerEnd < -1 )
			LayerEnd = -1;
	}
	if( LayerStart >= Big->LayersAvailable )
		LayerStart = Big->LayersAvailable - 1;
	if( LayerStart >= Small->LayersAvailable )
		LayerStart = Small->LayersAvailable - 1;

	int SearchRadiusX = Big->ImageLayersX[ LayerStart ] / 2 + 1;
	int SearchRadiusY = Big->ImageLayersY[ LayerStart ] / 2 + 1;
	if( AtX == - 1 )
	{
		AtX = Big->ImageLayersX[ 0 ] / 2;
		AtY = Big->ImageLayersY[ 0 ] / 2;
	}
	else
	{
		SearchRadiusX = PIXEL_STEPDOWN_LAYER;
		SearchRadiusY = PIXEL_STEPDOWN_LAYER;
	}

	int CanSkipNextLayer = 0;

	for( int Layer=LayerStart; Layer>LayerEnd; Layer = Layer - 1 - CanSkipNextLayer )
	{
		CanSkipNextLayer = 0;

		int PixelsInPixel = GetPixelsInPixel1D( Layer );

		int StartX, EndX, StartY, EndY;
		StartX = AtX / PixelsInPixel - SearchRadiusX;
		EndX = AtX / PixelsInPixel + SearchRadiusX;
		StartY = AtY / PixelsInPixel - SearchRadiusY;
		EndY = AtY / PixelsInPixel + SearchRadiusY;
		if( StartX < 0 )
			StartX = 0;
		if( EndX > Big->ImageLayersX[Layer] - Small->ImageLayersX[Layer] )
			EndX = Big->ImageLayersX[Layer] - Small->ImageLayersX[Layer];
		if( StartY < 0 )
			StartY = 0;
		if( EndY > Big->ImageLayersY[Layer] - Small->ImageLayersY[Layer] )
			EndY = Big->ImageLayersY[Layer] - Small->ImageLayersY[Layer];

//		printf("Starting search layer %d from %d %d to %d %d or %d %d to %d %d. Recursion %d. Small img %d %d\n",Layer, StartX, StartY, EndX, EndY, StartX * PixelsInPixel, StartY * PixelsInPixel, EndX * PixelsInPixel, EndY * PixelsInPixel, RecCount, Small->ImageLayersX[Layer], Small->ImageLayersY[Layer] );
		for( int y = StartY; y < EndY; y++ )
			for( int x = StartX; x < EndX; x++ )
			{
				int RSadNow = GetLocalSad( &Big->ImageLayers[RED_LAYER_INDEX][Layer][ y * Big->ImageLayersX[Layer] + x ], &Small->ImageLayers[RED_LAYER_INDEX][Layer][ 0 ], Big->ImageLayersX[Layer], Small->ImageLayersX[Layer], Small->ImageLayersX[Layer], Small->ImageLayersY[Layer] );
#ifdef MERGE_RGB_INTO_R
				if( RSadNow <= BestRSad )
#endif
				{
#if !defined( GENERATE_ONLY_R ) && !defined( MERGE_RGB_INTO_R )
					int GSadNow = GetLocalSad( &Big->ImageLayers[1][Layer][ y * Big->ImageLayersX[Layer] + x ], &Small->ImageLayers[1][Layer][ 0 ], Big->ImageLayersX[Layer], Small->ImageLayersX[Layer], Small->ImageLayersX[Layer], Small->ImageLayersY[Layer] );
					int BSadNow = GetLocalSad( &Big->ImageLayers[2][Layer][ y * Big->ImageLayersX[Layer] + x ], &Small->ImageLayers[2][Layer][ 0 ], Big->ImageLayersX[Layer], Small->ImageLayersX[Layer], Small->ImageLayersX[Layer], Small->ImageLayersY[Layer] );
					if( RSadNow + GSadNow + BSadNow <= BestRSad + BestGSad + BestBSad )
//					if( RSadNow * RSadNow + GSadNow * GSadNow + BSadNow * BSadNow <= BestRSad * BestRSad + BestGSad * BestGSad + BestBSad * BestBSad )
#endif
					{
						//check if 
						int BetterX, BetterY, BetterRSad, BetterGSad, BetterBSad;
						int RefineAmt = 0;
						if( Layer > 0 && pLayer == -1 )
						{
							PiramidSearch( Big, Small, &BetterX, &BetterY, &BetterRSad, &BetterGSad, &BetterBSad, 0, Layer - 1, x * PixelsInPixel, y * PixelsInPixel, RecCount + 1 );
							RefineAmt = BetterRSad - RSadNow;
						}
						else
						{
							BetterX = x * PixelsInPixel;
							BetterY = y * PixelsInPixel;
							BetterRSad = RSadNow;
#if !defined( GENERATE_ONLY_R ) && !defined( MERGE_RGB_INTO_R )
							BetterGSad = GSadNow;
							BetterBSad = BSadNow;
#endif
						}
						if( RefineAmt <= 0 )
						{
							if( Layer > 1 )
								CanSkipNextLayer = 1;

							*RSad = BestRSad = BetterRSad;
#if !defined( GENERATE_ONLY_R ) && !defined( MERGE_RGB_INTO_R )
							*GSad = BestGSad = BetterGSad;
							*BSad = BestBSad = BetterBSad;
#endif

							*RetX = BetterX;
							*RetY = BetterY;

#if !defined( GENERATE_ONLY_R ) && !defined( MERGE_RGB_INTO_R )
							if( BestRSad + BestGSad + BestBSad <= SadLimit )
#else
							if( BestRSad <= SadLimit )
#endif
							{
//								printf("For Layer %d best sad %d at %d %d \n", Layer, BestRSad, *RetX, *RetY );
//								printf("Perfect match found. Aborting higher layer search\n" );
								return 0;
							}
						}
					}
				}
			}
//		for( int t=0;t<RecCount;t++)
//			printf("\t");
//		printf("For Layer %d best sad %d at %d %d \n", Layer, BestRSad, *RetX, *RetY );
#if !defined( GENERATE_ONLY_R ) && !defined( MERGE_RGB_INTO_R )
		BestGSad = BestBSad = 0;
#endif
//		printf("For Layer %d best sad %d at %d %d L = %d x = %d y = %d\n", Layer, BestRSad + BestGSad + BestBSad, *RetX, *RetY, pLayer, AtX, AtY );
//		printf("\n");
		SearchRadiusX = PIXEL_STEPDOWN_LAYER;
		SearchRadiusY = PIXEL_STEPDOWN_LAYER;
		AtX = *RetX;
		AtY = *RetY;
	}
	return 0;
}

char PSReturnBuff[DEFAULT_STR_BUFFER_SIZE*10];
char * WINAPI SearchPiramidOnScreenshot( const char *aImageFile )
{
	PSReturnBuff[0]=0;
	FileDebug( "Started Similar Image search" );

	CachedPicture *cache = CachePicture( aImageFile );
	if( cache == NULL )
	{
		FileDebug( "Skipping Image search as image could not be loaded" );
		return PSReturnBuff;
	}
	if( cache->Pixels == NULL )
	{
		FileDebug( "Skipping Image search as image pixels are missing" );
		return PSReturnBuff;
	}

	if( CurScreenshot->Pixels == NULL )
	{
		FileDebug( "Skipping Image search no screenshot is available" );
		return PSReturnBuff;
	}

	if( cache->PSCache == NULL )
		cache->PSCache = new PiramidImage;
	if( CurScreenshot->PSCache == NULL )
		CurScreenshot->PSCache = new PiramidImage;

	if( cache->NeedsPSCache == true )
	{
		cache->PSCache->BuildFromImg( cache->Pixels, cache->Width, cache->Height, cache->Width );
		cache->NeedsPSCache = false;
	}
	if( CurScreenshot->NeedsPSCache == true )
	{
		CurScreenshot->PSCache->BuildFromImg( CurScreenshot->Pixels, CurScreenshot->Right - CurScreenshot->Left, CurScreenshot->Bottom - CurScreenshot->Top, CurScreenshot->Right - CurScreenshot->Left );
		CurScreenshot->NeedsPSCache = false;
	}
	int retx, rety, retrsad, retgsad, retbsad;
	PiramidSearch( CurScreenshot->PSCache, cache->PSCache, &retx, &rety, &retrsad, &retgsad, &retbsad, 0 );
//	PiramidSearch( CurScreenshot->PSCache, cache->PSCache, &retx, &rety, &retrsad, &retgsad, &retbsad, 0, 0 );	//image to image search

	//calculate absolute positioning
	retx += CurScreenshot->Left;
	rety += CurScreenshot->Top;

	//calculate middle of the image
//	tretx += cache->Width / 2;
//	trety += cache->Height / 2;

//	TakeScreenshot( retx, rety, retx + 150 , rety + 150 );	//for me this is black box on black screen search ... worst case
//	SaveScreenshot();

	sprintf_s( PSReturnBuff, DEFAULT_STR_BUFFER_SIZE*10, "1|%d|%d|%d", retx+cache->Width / 2, rety + cache->Height / 2, retrsad );
	return PSReturnBuff;
}

#endif