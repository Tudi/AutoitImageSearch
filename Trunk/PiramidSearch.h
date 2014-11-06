#ifndef _PIRAMID_IMAGE_SEARCH_H_
#define _PIRAMID_IMAGE_SEARCH_H_

// The ods that numbers will overflow are :
// if screenshot size is 2024*2024 that is 4096576 pixels. Max sad / pixel is 3*255 => 2024*2024*3*255 = 3133880640
// if we want to compare texture as RS * GS * BS instead RS+GS+BS than we need 93 bits just to be safe.
//number of layers to be generated. Top Layer is 1 to 1
#define MAX_IMAGE_LAYERS		5
//number of pixels to be merged per layer. Ex : 2x2 pixel block becomes 1x1
#define PIXEL_STEPDOWN_LAYER	2
#define MIN_SIZE_FOR_SEARCH		1
#define MAX_INT					0x01FFFFFF

//#define	GENERATE_ONLY_R
//#define DEBUG_WRITEBACK_WHAT_PIXELS_WE_READ
// this will force luminosity based search. Search will not be able to distinguish red / green / blue texture at the same luminosity level
//#define MERGE_RGB_INTO_R	//you want to use this in conjunction with GENERATE_ONLY_R

class PiramidImage
{
public:
	PiramidImage();
	~PiramidImage();
	void InitBuffersToNewSize( int Width, int Height );
	void BuildPiramid( LPCOLORREF Pixels, int Width, int Height, int Stride );
	void BuildFromImgOtherLevels();
	void SaveLayersToFile( char *BaseName, int x = 0, int y = 0, int w = -1, int h = -1 );
//private:
	int			LayersAvailable;
	signed int	*ImageLayers[3][MAX_IMAGE_LAYERS];
	int			ImageLayersX[MAX_IMAGE_LAYERS];
	int			ImageLayersY[MAX_IMAGE_LAYERS];
};

int PiramidSearch( PiramidImage *Big, PiramidImage *Small, int *RetX, int *RetY, int *RSad, int *GSad, int *BSad, int SadLimit, int pLayer, int AtX, int AtY, int RecCount );

#endif