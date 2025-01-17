#include "stdafx.h"

void TestTextureLearnOnPictures()
{
	return;
/*	TakeScreenshot(110, 60, 450, 350);
	SaveScreenshot();
	return;*/

/*	TakeScreenshot(110, 60, 450, 350);
	DrawLine2(0, 0, 100, 100, BGR(255, 0, 0));
	SaveScreenshot();
	return;/**/

	TakeScreenshot(110, 60, 450, 350);
	ResetDistanceMapScreenshot();
	for (int i = 0; i < 200; i++)
	{
		//avoid 100% CPU throtle in case we process a small area
		unsigned int Start = GetTimeTickI();
		
		//monitor the camera input
		TakeScreenshot(110,60,450,350);

		//check if the image has some overlay movement
		ParseImageDistanceMapScreenshot();

		//visual inspectation of the movement
		char FileName[500];
		sprintf_s(FileName, sizeof(FileName), "diff%d.bmp", i);
		SaveDistMapAsImage(FileName);

		//how much time did this procedure take ?
		unsigned int End = GetTimeTickI();

		//maybe bmp save takes longer than we should have a sleep at all
		if (End - Start < 300)
			Sleep(End - Start);
	}
//	CachedPicture *cache;
	//cache = CachePicturePrintErrors("1.bmp", __FUNCTION__);

//	LPCOLORREF new_Pixels = BlurrImage(1, 7, cache->Pixels, cache->Width, cache->Height);
//	MY_FREE(cache->Pixels);
//	cache->Pixels = new_Pixels;
//	SaveImage(cache->Pixels, cache->Width, cache->Height, "1_blured.bmp");

//	ApplyColorBitmask(cache->Pixels, cache->Width, cache->Height, 0x00F0F0F0);
//	SaveImage(cache->Pixels, cache->Width, cache->Height, "1_bitmasked.bmp");

//	ColorReduceCache("1.bmp", 10);
//	SaveImage(cache->Pixels, cache->Width, cache->Height, "1_RedColor.bmp");

	//remove gradient from the image
//	GradientReduceCache("1.bmp",2);
//	SaveImage(cache->Pixels, cache->Width, cache->Height, "1_RedGrad3_noBlur.bmp");
    /*
	LineFilter_AddImage(0, "1.bmp");
	cache = CachePicturePrintErrors("1.bmp", __FUNCTION__);
	SaveImage(cache->Pixels, cache->Width, cache->Height, "1_color_reduced32.bmp");
	LineFilter_AddImageEliminateNonCommon(0, "2.bmp");
	LineFilter_AddImageEliminateNonCommon(0, "3.bmp");
	LineFilter_AddImageEliminateNonCommon(0, "4.bmp");
	LineFilter_AddImageEliminateNonCommon(0, "5.bmp");
	LineFilter_AddImageEliminateNonCommon(0, "6.bmp");

	LineFilter_MarkObjectProbability(0, "1.bmp");
	cache = CachePicturePrintErrors("1.bmp", __FUNCTION__);
	SaveImage(cache->Pixels, cache->Width, cache->Height, "1_common_3_32.bmp");
	LineFilter_MarkObjectProbability(0, "2.bmp");
	cache = CachePicturePrintErrors("2.bmp", __FUNCTION__);
	SaveImage(cache->Pixels, cache->Width, cache->Height, "2_common_3_32.bmp");
    */
//	LineFilter_MarkObjectProbability(0, "3.bmp");
//	cache = CachePicturePrintErrors("3.bmp", __FUNCTION__);
//	SaveImage(cache->Pixels, cache->Width, cache->Height, "3_probable_4_8.bmp");

/*	TakeScreenshot(0, 0, 1000, 1000);
	SaveScreenshot

	LoadCacheOverScreenshot("1.bmp", 0, 0);
	/*
	//feed it to the learning process
	EdgeFilter_LearnRefine("Learn1.bmp");

	//just to create a memory we can work on with BMPs
	TakeScreenshot(0, 0, 1000, 1000);
	//load an image of the object over the screeenshot
	LoadCacheOverScreenshot("pic1.bmp", 0, 0);*/
}
