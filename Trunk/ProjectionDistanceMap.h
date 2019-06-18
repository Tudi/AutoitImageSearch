#pragma once

/*
you have a fixed camera and a projector that will project moving lines on the target you want to capture
as the lines move from left to right, this filter will measure the width of the projected lines
each new image is going to create a very rough distance map. At any point you can merge multiple rough distance maps to create a finetuned distance map
right now it will consider all projected colors as same color, but it could be made to support multiple colors to have a better estimation
*/

#define DIFF_LINE_WIDTH_TOO_SMALL	2 // that is actually 8 pixels in current implementation

class ProjectionDistanceMap
{
public:
	ProjectionDistanceMap()
	{
		NonProjectedImage = NULL;
		Width = 0;
		Height = 0;
		MergedDistanceMaps = NULL;
	}
	void DestroyAllocatedMemmory();
	void Init(int pWidth, int pHeight, LPCOLORREF Pixels);
	void AddProjectedImage(LPCOLORREF Pixels);
	void ProcessAllDiffMaps();
	void SameMergedMapToFile(char *FileName);
private:
	int Width, Height;
	LPCOLORREF NonProjectedImage;	// we need a screenshot without any projected lines to be able to detect where the projection start and end
	LPCOLORREF MergedDistanceMaps;
	std::list<unsigned char*> DiffMaps;
	std::list<unsigned char*> DistanceMaps;
	int MaxLength, MinLength; // to scale colors from 0 to 255
};

void ResetDistanceMapScreenshot();	// step 1 : when we have a clean image without any projected overlay
void ParseImageDistanceMapScreenshot();	// step 2-X : keep adding as many images as you like
void ApplyDistanceMapOnScreenshot(char *FileName); // step X+1 : calculate the distance for each pixel based on all the added images
