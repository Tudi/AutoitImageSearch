#include "StdAfx.h"

//disabled as i never finished / tested it
#if 0
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
			free( ImageLayers[0][i] );
			free( ImageLayers[1][i] );
			free( ImageLayers[2][i] );
		}
	memset( ImageLayers, NULL, sizeof( ImageLayers ) );
}


void PiramidImage::InitBuffersToNewSize(int Width, int Height)
{
	bool Reinit = ( ( ImageLayersX[0] != Width ) || ( ImageLayersY[0] != Height ) );
	if( Reinit == true )
	{
		for( int i=0;i<MAX_IMAGE_LAYERS;i++ )
			if( ImageLayers[0][i] != NULL )
			{
				free( ImageLayers[0][i] );
				ImageLayers[0][i] = NULL;
#if !defined( GENERATE_ONLY_R ) && !defined( MERGE_RGB_INTO_R )
				free( ImageLayers[1][i] );
				ImageLayers[1][i] = NULL;
				free( ImageLayers[2][i] );
				ImageLayers[2][i] = NULL;
#endif
			}
	}

	ImageLayersX[0] = Width;
	ImageLayersY[0] = Height;

	LayersAvailable = 0;
	for( int i=0;i<MAX_IMAGE_LAYERS;i++ )
		if( ImageLayers[0][i] == NULL )
		{
			if( i > 0 )
			{
				ImageLayersX[ i ] = ( ImageLayersX[ i - 1 ] ) / PIXEL_STEPDOWN_LAYER;	//round down
				ImageLayersY[ i ] = ( ImageLayersY[ i - 1 ] ) / PIXEL_STEPDOWN_LAYER;	//round down
			}
			if( ImageLayersX[ i ] <= MIN_SIZE_FOR_SEARCH || ImageLayersY[ i ] <= MIN_SIZE_FOR_SEARCH )
				break;

			ImageLayers[0][i] = (int*)malloc( ImageLayersX[i] * ImageLayersY[i] * sizeof( signed int ) + SSE_PADDING );
#if !defined( GENERATE_ONLY_R ) && !defined( MERGE_RGB_INTO_R )
			ImageLayers[1][i] = (int*)malloc( ImageLayersX[i] * ImageLayersY[i] * sizeof( signed int ) + SSE_PADDING );
			ImageLayers[2][i] = (int*)malloc( ImageLayersX[i] * ImageLayersY[i] * sizeof( signed int ) + SSE_PADDING );
#endif
			LayersAvailable++;
		}
}

void PiramidImage::BuildPiramid( LPCOLORREF BGRPixels, int Width, int Height, int Stride )
{
	if( BGRPixels == NULL )
	{
		FileDebug( "Exiting PiramidBuild due to missing input buffer" );
		return;
	}

	//alloc layers
	InitBuffersToNewSize( Width, Height );

	//copy the bitmap to our buffer
	BYTE* byteptr = (unsigned char*)BGRPixels;
	for( int y = 0; y < ImageLayersY[ 0 ]; y += 1 )
	{
//		int MirrorY = ImageLayersY[ 0 ] - 1 - y;
		int MirrorY = y;
	    int rowBase = y * Stride;
		for( int x = 0; x < ImageLayersX[ 0 ]; x += 1 )
		{
			//pointer arithmetics to find (i,j) pixel colors:
			int R = *( byteptr + rowBase + x * 4 + 2 );
			ImageLayers[0][0][ MirrorY * ImageLayersX[ 0 ] + x ] = R;
#ifdef MERGE_RGB_INTO_R
			int G = *( byteptr + rowBase + x * 4 + 1 );
			int B = *( byteptr + rowBase + x * 4 + 0 ); 
			ImageLayers[0][0][ MirrorY * ImageLayersX[ 0 ] + x ] += G;
			ImageLayers[0][0][ MirrorY * ImageLayersX[ 0 ] + x ] += B;
#endif
#ifndef GENERATE_ONLY_R
			int G = *( byteptr + rowBase + x * 4 + 1 );
			int B = *( byteptr + rowBase + x * 4 + 0 ); 
			ImageLayers[1][0][ MirrorY * ImageLayersX[ 0 ] + x ] = G;
			ImageLayers[2][0][ MirrorY * ImageLayersX[ 0 ] + x ] = B;
#endif
#ifdef DEBUG_WRITEBACK_WHAT_PIXELS_WE_READ
			Src->SetPixel( x, MirrorY, RGB( R, G, B ) );
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
//assert( y / PIXEL_STEPDOWN_LAYER < ImageLayersY[ Layer ] );
//assert( x / PIXEL_STEPDOWN_LAYER < ImageLayersX[ Layer ] );
					ImageLayers[ RGB ][ Layer ][ y * ImageLayersX[ Layer ] + x ] = Sum;
				}
		}
	}
}

void PiramidImage::SaveLayersToFile( char *BaseName, int x, int y, int w, int h )
{
	for( int Layer=0;Layer<MAX_IMAGE_LAYERS;Layer++)
	{
		if( ImageLayersY[Layer] <= MIN_SIZE_FOR_SEARCH || ImageLayersX[Layer] <= MIN_SIZE_FOR_SEARCH )
			continue;

		int PixelsInPixel = 1;
		int PixelsInPixel1D = 1;
		for( int i = 0; i < Layer; i++ )
		{
			PixelsInPixel *= PIXEL_STEPDOWN_LAYER * PIXEL_STEPDOWN_LAYER;
			PixelsInPixel1D *= PIXEL_STEPDOWN_LAYER;
		}

		int width = ImageLayersX[Layer];
		int height = ImageLayersY[Layer];

		int StartX = x / PixelsInPixel1D;
		int StartY = y / PixelsInPixel1D;
		if( w != -1 )
		{
			width = min( w / PixelsInPixel1D, width );
			height = min( h / PixelsInPixel1D, height );
		}

		CImage img;
		img.Create( width + 1, height + 1, 24 /* bpp */, 0 /* No alpha channel */);

		for(int row = StartY; row < StartY + height; row++)
		{
			int nPixel = row * ImageLayersX[Layer] + StartX;
			for(int col = StartX; col < StartX + width; col++)
			{
				BYTE r = ImageLayers[0][Layer][nPixel] / PixelsInPixel;
#ifdef MERGE_RGB_INTO_R
				r = r / 3;
#endif
#ifndef GENERATE_ONLY_R
				BYTE g = ImageLayers[1][Layer][nPixel] / PixelsInPixel;
				BYTE b = ImageLayers[2][Layer][nPixel] / PixelsInPixel;
				img.SetPixel( col - StartX, row - StartY, RGB( r, g, b ) );
#else
				img.SetPixel( col - StartX, row - StartY, RGB( r, r, r ) );
#endif
				nPixel++;
			}
		}
		char FileName[500];
		sprintf_s( FileName, 500, "%s%d.bmp", BaseName, Layer );
		img.Save( FileName );
	}
}


int GetLocalSad( int *a, int *b, int stride1, int stride2, int w, int h )
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

//only search one layer and see if it can improve the search on next layer
int PiramidSearchCanRefine( int Layer, PiramidImage *Big, PiramidImage *Small, int atX, int atY, int RSadPrev, int GSadPrev, int BSadPrev, int &RetX, int &RetY, int &RetRSad, int &RetGSad, int &RetBSad  )
{
	if( Layer < 0 )
	{
		RetRSad = RSadPrev;
#ifndef GENERATE_ONLY_R
		RetGSad = GSadPrev;
		RetBSad = BSadPrev;
#endif
		RetX = atX;
		RetY = atY;
		return 0;
	}

	//too small to sample this layer ?
	if( Layer >= Small->LayersAvailable || Layer >= Big->LayersAvailable )
		return 0;

	int BestRSad = MAX_INT;
	int BestGSad = MAX_INT;
	int BestBSad = MAX_INT;

	int PixelsInPixel = 1;
	for( int i = 0; i < Layer; i++ )
		PixelsInPixel *= PIXEL_STEPDOWN_LAYER;

	int StartX = atX / PixelsInPixel - PIXEL_STEPDOWN_LAYER;
	int EndX = atX / PixelsInPixel + PIXEL_STEPDOWN_LAYER;
	int StartY = atY / PixelsInPixel - PIXEL_STEPDOWN_LAYER;
	int EndY = atY / PixelsInPixel + PIXEL_STEPDOWN_LAYER;

	if( StartX < 0 )
		StartX = 0;
	if( EndX > Big->ImageLayersX[Layer] - Small->ImageLayersX[Layer] - 1 )
		EndX = Big->ImageLayersX[Layer] - Small->ImageLayersX[Layer] - 1;

	if( StartY < 0 )
		StartY = 0;
	if( EndY > Big->ImageLayersY[Layer] - Small->ImageLayersY[Layer] - 1 )
		EndY = Big->ImageLayersY[Layer] - Small->ImageLayersY[Layer] - 1;

	for( int y = StartY; y <= EndY; y++ )
		for( int x = StartX; x < EndX; x++ )
		{
			int RSadNow = GetLocalSad( &Big->ImageLayers[0][Layer][ ( y + 0 ) * Big->ImageLayersX[Layer] + ( x + 0 ) ], &Small->ImageLayers[0][Layer][ ( 0 + 0 ) * Small->ImageLayersX[Layer] + ( 0 + 0 ) ], Big->ImageLayersX[Layer], Small->ImageLayersX[Layer], Small->ImageLayersX[Layer], Small->ImageLayersY[Layer] );
			if( RSadNow < BestRSad )
			{
#ifndef GENERATE_ONLY_R
				int GSadNow = GetLocalSad( &Big->ImageLayers[1][Layer][ ( y + 0 ) * Big->ImageLayersX[Layer] + ( x + 0 ) ], &Small->ImageLayers[1][Layer][ ( 0 + 0 ) * Small->ImageLayersX[Layer] + ( 0 + 0 ) ], Big->ImageLayersX[Layer], Small->ImageLayersX[Layer], Small->ImageLayersX[Layer], Small->ImageLayersY[Layer] );
				int BSadNow = GetLocalSad( &Big->ImageLayers[2][Layer][ ( y + 0 ) * Big->ImageLayersX[Layer] + ( x + 0 ) ], &Small->ImageLayers[2][Layer][ ( 0 + 0 ) * Small->ImageLayersX[Layer] + ( 0 + 0 ) ], Big->ImageLayersX[Layer], Small->ImageLayersX[Layer], Small->ImageLayersX[Layer], Small->ImageLayersY[Layer] );
				// (r1-r2) + (g1-g2) + (b1-b2) close to (r1+g1+b1) - ( r2+g2+b2) => Not good it is equal as we would merge all 3 into 1, like a luminosity search
				// an improvement could be to check if values are above or below our target ( RSADNew < RSADOld && abs( TextureNew(R+G+B) ) > abs( TextureOld(R+G+B) )
				if( RSadNow + GSadNow + BSadNow < BestRSad + BestGSad + BestBSad )
#endif
				{
					BestRSad = RSadNow;
#ifndef GENERATE_ONLY_R
					BestGSad = GSadNow;
					BestBSad = BSadNow;
#endif
					RetX = x * PixelsInPixel;
					RetY = y * PixelsInPixel;
				}
			}
		}
#ifndef GENERATE_ONLY_R
	RetRSad = BestRSad;
	RetGSad = BestGSad;
	RetBSad = BestBSad;
	return ( ( BestRSad - RSadPrev ) + ( BestGSad - GSadPrev ) + ( BestBSad - BSadPrev ) );
#else
	RetRSad = BestRSad;
	return ( BestRSad - RSadPrev );
#endif
}

//search all layers
int PiramidSearch( PiramidImage *Big, PiramidImage *Small, int *RetX, int *RetY, int *RSad, int *GSad, int *BSad, int SadLimit, int pLayer, int AtX, int AtY, int RecCount )
{
	//in case we fail at the search, somehow signal back
	*RetX = 0;
	*RetY = 0;
	*RSad = *GSad = *BSad = MAX_INT;

	int BestRSad = MAX_INT;
	int BestGSad = MAX_INT;
	int BestBSad = MAX_INT;
	int BestRefine = MAX_INT;

	//search in worst version only in R
	int LayerStart = MAX_IMAGE_LAYERS-1;
	int LayerEnd = -1;
	if( AtX != -1 )
	{
		LayerStart = pLayer;
		LayerEnd = pLayer-1;
		if( LayerEnd < -1 )
			LayerEnd = -1;
	}
	if( LayerStart >= Big->LayersAvailable )
		LayerStart = Big->LayersAvailable;
	if( LayerStart >= Small->LayersAvailable )
		LayerStart = Small->LayersAvailable;
	if( LayerEnd >= Big->LayersAvailable )
		LayerEnd = Big->LayersAvailable;
	if( LayerEnd >= Small->LayersAvailable )
		LayerEnd = Small->LayersAvailable;

	int FirstSearchedLayer = 1;
	int CanSkipNextLayer = 0;

	for( int Layer=LayerStart; Layer>LayerEnd; Layer = Layer - 1 - CanSkipNextLayer )
	{
		CanSkipNextLayer = 0;

		int PixelsInPixel = 1;
		for( int i = 0; i < Layer; i++ )
			PixelsInPixel *= PIXEL_STEPDOWN_LAYER;

		int StartX, EndX, StartY, EndY;
		if( AtX != -1 )
		{
			StartX = AtX / PixelsInPixel - PIXEL_STEPDOWN_LAYER;
			EndX = AtX / PixelsInPixel + PIXEL_STEPDOWN_LAYER;
			StartY = AtY / PixelsInPixel - PIXEL_STEPDOWN_LAYER;
			EndY = AtY / PixelsInPixel + PIXEL_STEPDOWN_LAYER;
		}
		else if( FirstSearchedLayer == 0 && BestRSad != MAX_INT )
		{
			StartX = *RetX / PixelsInPixel - PIXEL_STEPDOWN_LAYER;
			EndX = *RetX / PixelsInPixel + PIXEL_STEPDOWN_LAYER;
			StartY = *RetY / PixelsInPixel - PIXEL_STEPDOWN_LAYER;
			EndY = *RetY / PixelsInPixel + PIXEL_STEPDOWN_LAYER;
		}
		else
		{
			StartX = 0;
			EndX = Big->ImageLayersX[Layer] - Small->ImageLayersX[Layer] - 1;
			StartY = 0;
			EndY = Big->ImageLayersY[Layer] - Small->ImageLayersY[Layer] - 1;
		}

		if( StartX < 0 )
			StartX = 0;
		if( EndX > Big->ImageLayersX[Layer] - Small->ImageLayersX[Layer] - 1 )
			EndX = Big->ImageLayersX[Layer] - Small->ImageLayersX[Layer] - 1;

		if( StartY < 0 )
			StartY = 0;
		if( EndY > Big->ImageLayersY[Layer] - Small->ImageLayersY[Layer] - 1 )
			EndY = Big->ImageLayersY[Layer] - Small->ImageLayersY[Layer] - 1;

		printf("Starting search layer %d from %d %d to %d %d or %d %d to %d %d. Recursion %d. Small img %d %d\n",Layer, StartX, StartY, EndX, EndY, StartX * PixelsInPixel, StartY * PixelsInPixel, EndX * PixelsInPixel, EndY * PixelsInPixel, RecCount, Small->ImageLayersX[Layer], Small->ImageLayersY[Layer] );
		for( int y = StartY; y <= EndY; y++ )
			for( int x = StartX; x <= EndX; x++ )
			{
//if( y * PixelsInPixel <= 69 && ( y + 1 ) * PixelsInPixel >= 69 && x * PixelsInPixel <= 9 && ( x + 1 ) * PixelsInPixel >= 9 )
//if( y * PixelsInPixel <= 69 && ( y + 1 ) * PixelsInPixel >= 69 && x * PixelsInPixel <= 9 && ( x + 1 ) * PixelsInPixel >= 9 && Layer == 2 )
//if( y == 69 && x == 9 && Layer == 0 )
//	printf( "1903471923");
				int RSadNow = GetLocalSad( &Big->ImageLayers[0][Layer][ ( y + 0 ) * Big->ImageLayersX[Layer] + ( x + 0 ) ], &Small->ImageLayers[0][Layer][ ( 0 + 0 ) * Small->ImageLayersX[Layer] + ( 0 + 0 ) ], Big->ImageLayersX[Layer], Small->ImageLayersX[Layer], Small->ImageLayersX[Layer], Small->ImageLayersY[Layer] );
				int GSadNow = 0;
				int BSadNow = 0;
#ifdef MERGE_RGB_INTO_R
				if( RSadNow <= BestRSad )
#endif
				{
#ifndef GENERATE_ONLY_R
					GSadNow = GetLocalSad( &Big->ImageLayers[1][Layer][ ( y + 0 ) * Big->ImageLayersX[Layer] + ( x + 0 ) ], &Small->ImageLayers[1][Layer][ ( 0 + 0 ) * Small->ImageLayersX[Layer] + ( 0 + 0 ) ], Big->ImageLayersX[Layer], Small->ImageLayersX[Layer], Small->ImageLayersX[Layer], Small->ImageLayersY[Layer] );
					BSadNow = GetLocalSad( &Big->ImageLayers[2][Layer][ ( y + 0 ) * Big->ImageLayersX[Layer] + ( x + 0 ) ], &Small->ImageLayers[2][Layer][ ( 0 + 0 ) * Small->ImageLayersX[Layer] + ( 0 + 0 ) ], Big->ImageLayersX[Layer], Small->ImageLayersX[Layer], Small->ImageLayersX[Layer], Small->ImageLayersY[Layer] );
					if( RSadNow + GSadNow + BSadNow <= BestRSad + BestGSad + BestBSad )
#endif
					{
						//check if 
						int BetterX, BetterY, BetterRSad, BetterGSad, BetterBSad;
						int RefineAmt = 0;
						if( Layer > 0 )
							RefineAmt = PiramidSearchCanRefine( Layer - 1, Big, Small, x * PixelsInPixel, y * PixelsInPixel, RSadNow, GSadNow, BSadNow, BetterX, BetterY, BetterRSad, BetterGSad, BetterBSad );
						else
						{
							BetterX = x * PixelsInPixel;
							BetterY = y * PixelsInPixel;
							BetterRSad = RSadNow;
							BetterGSad = GSadNow;
							BetterBSad = BSadNow;
						}
						if( RefineAmt <= 0 )
						{
							if( Layer > 1 )
								CanSkipNextLayer = 1;

							BestRSad = BetterRSad;
#ifndef GENERATE_ONLY_R
							BestGSad = BetterGSad;
							BestBSad = BetterBSad;
#endif

							*RetX = BetterX;
							*RetY = BetterY;

#ifndef GENERATE_ONLY_R
							if( BestRSad + BestGSad + BestBSad <= SadLimit )
#else
							if( BestRSad <= SadLimit )
#endif
							{
								printf("For Layer %d best sad %d at %d %d \n", Layer, BestRSad, *RetX, *RetY );
								printf("Perfect match found. Aborting higher layer search\n" );
								return 0;
							}
						}
					}
				}
			}
		for( int t=0;t<RecCount;t++)
			printf("\t");
//		printf("For Layer %d best sad %d at %d %d \n", Layer, BestRSad, *RetX, *RetY );
#ifdef GENERATE_ONLY_R
		BestGSad = BestBSad = 0;
#endif
		printf("For Layer %d best sad %d at %d %d L = %d x = %d y = %d\n", Layer, BestRSad + BestGSad + BestBSad, *RetX, *RetY, pLayer, AtX, AtY );
		printf("\n");

		FirstSearchedLayer = 0;
	}
	return 0;
}

#endif