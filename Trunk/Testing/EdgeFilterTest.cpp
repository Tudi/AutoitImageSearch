#include "stdafx.h"

void TestLearnObjectFromScreenshots()
{
	//use index 0
	EdgeFilter_Init(0);

	//feed it to the learning process
	EdgeFilter_LearnRefine("Learn1.bmp");

	//just to create a memory we can work on with BMPs
	TakeScreenshot(0, 0, 1000, 1000);
	//load an image of the object over the screeenshot
	LoadCacheOverScreenshot("pic1.bmp", 0, 0);
}
