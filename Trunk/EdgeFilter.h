#pragma once
#include "stdafx.h"
/*
Description :
 -- try to construct objects out of lines with length of 2 pixels ( next step is longer lines ) --
 -- feeding more than 1 picture should strengthen some features while weaken onther features -- 
 -- when 2 objects are compared, a similarity index should be returned representing how many lines match and in what ratio --
Steps to obtain the common feature :
- blur the image to reduce scale related issues
- reduce luminosity of the image to base luminosity. Does not support shading and illumination. max 3 * 65535 colors
- reduce color depth to reduce the size of statistics. Maybe half color count ? : 3 * 32k pixel possibility ? This would also help subpixel acuracy on rotation
- extract pixel features ( 8 neighbour features ) and create statistics. Each neighbour should have a list of pixel statistics
- should be able to refine pixel feature count in order to have a better define an object
- should be able to create statistics on a region on an image
- to detect rotated image, the search matching should also search in all type of orientations : for(i=0;i<MaxCombinations;i++)
*/

#define MIN_LINE_LENGTH_PIXELS 3
#define MAX_LINE_LENGTH_PIXELS 4 //not inclusive

#define PIXEL_BYTE_COUNT 3

class LineStore
{
public:
	LineStore(int Length)
	{
		Line = (char *)_aligned_malloc(Length + SSE_PADDING, SSE_ALIGNMENT);
	}
	~LineStore()
	{
		_aligned_free(Line);
		Line = NULL;
	}
	char *Line; // does not support Alpha channel
	int TotalFoundCount; // in all pictures, how many times did we find this line
	int TotalPictureFoundCount; // number of pictures where we found this line
};

class LineFilter
{
public:
	LineFilter()
	{
		NumberOfImagesLoaded = 0;
		LinesAdded = 0;
//		for (int i = 0; i < MAX_LINE_LENGTH_PIXELS - MIN_LINE_LENGTH_PIXELS; i++)
//			Lines[i] = new RedBlackTree((i + MIN_LINE_LENGTH_PIXELS) * PIXEL_BYTE_COUNT);
        Lines[0] = NULL;
    }
    LineFilter(int LineLength)
    {
        NumberOfImagesLoaded = 0;
        LinesAdded = 0;
        Lines[0] = new RedBlackTree(LineLength * PIXEL_BYTE_COUNT);
    }
	~LineFilter()
	{
//		for (int i = 0; i < MAX_LINE_LENGTH_PIXELS - MIN_LINE_LENGTH_PIXELS; i++)
//		{
//			delete Lines[i];
//			Lines[i] = NULL;
//		}
        delete Lines[0];
        Lines[0] = NULL;
    }
//	RedBlackTree *Lines[MAX_LINE_LENGTH_PIXELS - MIN_LINE_LENGTH_PIXELS]; //0 index is for line length 2
    RedBlackTree *Lines[1];

	int MyIndex;				// if we have a list of objects, we can refenrece them by index
	int NumberOfImagesLoaded;	// number of images we used to refine the definition of this object
	unsigned _int64 LinesAdded;
};

//add pictures that will be used to define the filter mask. Additional images will remove not common image features
void LineFilter_AddImage(int ObjectIndex, int LineLength, char *aFileName);
void LineFilter_AddImageEliminateNonCommon(int ObjectIndex, int LineLength, char *aFileName);
// iterate theough our list of objects and remove lines that are not marked to be part of our objects
void LineFilter_MarkObjectProbability(int ObjectIndex, char *aFileName);