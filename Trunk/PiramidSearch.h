#ifndef _PIRAMID_IMAGE_SEARCH_H_
#define _PIRAMID_IMAGE_SEARCH_H_

// ! Piramid search is unreliable !!! It find bad or no matches even if there are better versions

// The ods that numbers will overflow are :
// if screenshot size is 2024*2024 that is 4096576 pixels. Max sad / pixel is 3*255 => 2024*2024*3*255 = 3133880640
// if we want to compare texture as RS * GS * BS instead RS+GS+BS than we need 93 bits just to be safe.
//number of layers to be generated. Top Layer is 1 to 1
#define MAX_IMAGE_LAYERS			5
//number of pixels to be merged per layer. Ex : 2x2 pixel block becomes 1x1
#define PIXEL_STEPDOWN_LAYER		2
#define MIN_SIZE_FOR_SEARCH			1
#define MAX_INT						0x7FFFFFFF
#define BYTES_AVAILABLE_FOR_DATA	2	
#define MAX_PIXEL_COUNT_IN_PIXEL	( ( 1 << ( 8 * BYTES_AVAILABLE_FOR_DATA ) ) - 1 )
#define RED_LAYER_INDEX				0

// this will force luminosity based search. Search will not be able to distinguish red / green / blue texture at the same luminosity level
#define MERGE_RGB_INTO_R
// !do not panic that SAD will not be 0. Last 2 columns and rows of the small image will be 0 that is why there will always be a difference !
// data locality might improve some search results in some cases
//#define USE_DATA_LOCALITY_ON_MERGE

class PiramidImage
{
public:
	PiramidImage();
	~PiramidImage();
	void InitBuffersToNewSize( int Width, int Height );
	void BuildFromImg( LPCOLORREF Pixels, int Width, int Height, int Stride = 0 );
	void BuildFromImgOtherLevels();
//private:
	int			LayersAvailable;
	signed int	*ImageLayers[3][MAX_IMAGE_LAYERS];
	int			ImageLayersX[MAX_IMAGE_LAYERS];
	int			ImageLayersY[MAX_IMAGE_LAYERS];
};

int PiramidSearch( PiramidImage *Big, PiramidImage *Small, int *RetX, int *RetY, int *RSad, int *GSad, int *BSad, int SadLimit, int pLayer = -1, int AtX = -1, int AtY = -1, int RecCount = 0 );
char * WINAPI SearchPiramidOnScreenshot( const char *aImageFile );

#endif