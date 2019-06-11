#pragma once
#include "stdafx.h"
#include <map>
/*
Description :
 -- Try to detect an object based on some feature that is always visible on the object --
 -- the feature will be defined by the edge pixels. These edges are inside the object and not outside -- 
 -- edges are defined by only 2 pixels ! --
 -- this is a filter and not really a search --
Steps to obtain the common feature :
- reduce luminosity of the image to base luminosity. Does not support shading and illumination. max 3 * 65535 colors
- reduce color depth to reduce the size of statistics. Maybe half color count ? : 3 * 32k pixel possibility ? This would also help subpixel acuracy on rotation
- extract pixel features ( 8 neighbour features ) and create statistics. Each neighbour should have a list of pixel statistics
- should be able to refine pixel feature count in order to have a better define an object
- should be able to create statistics on a region on an image
- to detect rotated image, the search matching should also search in all type of orientations : for(i=0;i<MaxCombinations;i++)
*/

class EdgeFilterPixels
{
public:
	EdgeFilterPixels()
	{
		NumberOfImagesLoaded = 0;
		PixelsAdded = 0;
	}
	std::map<unsigned _int64, unsigned int> EdgePixelStatistics;
	int MyIndex;				// if we have a list of objects, we can refenrece them by index
	int NumberOfImagesLoaded;	// number of images we used to refine the definition of this object
	unsigned _int64 PixelsAdded;
};

//allocate memory to store this object
void EdgeFilter_Init(int EdgeFilterPixelsIndex);
//add pictures that will be used to define the filter mask. Additional images will remove not common image features
void EdgeFilter_LearnRefine(char *aFileName);
//
void EdgeFilter_MarkKnown(char *aFileName);