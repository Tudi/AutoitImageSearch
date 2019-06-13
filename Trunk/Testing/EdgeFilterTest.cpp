#include "stdafx.h"

void TestTextureLearnOnPictures()
{
	CachedPicture *cache;
	//cache = CachePicturePrintErrors("1.bmp", __FUNCTION__);

//	LPCOLORREF new_Pixels = BlurrImage(1, 7, cache->Pixels, cache->Width, cache->Height);
//	_aligned_free(cache->Pixels);
//	cache->Pixels = new_Pixels;
//	SaveImage(cache->Pixels, cache->Width, cache->Height, "1_blured.bmp");

//	ApplyColorBitmask(cache->Pixels, cache->Width, cache->Height, 0x00F0F0F0);
//	SaveImage(cache->Pixels, cache->Width, cache->Height, "1_bitmasked.bmp");

//	ColorReduceCache("1.bmp", 10);
//	SaveImage(cache->Pixels, cache->Width, cache->Height, "1_RedColor.bmp");

	//remove gradient from the image
//	GradientReduceCache("1.bmp",2);
//	SaveImage(cache->Pixels, cache->Width, cache->Height, "1_RedGrad3_noBlur.bmp");

	LineFilter_AddImage(0, "1.bmp");
	LineFilter_AddImage(0, "2.bmp");
	LineFilter_AddImage(0, "3.bmp");
	LineFilter_AddImage(0, "4.bmp");
	LineFilter_AddImage(0, "5.bmp");
	LineFilter_AddImage(0, "6.bmp");
	LineFilter_MarkObjectProbability(0, "1.bmp");
	cache = CachePicturePrintErrors("1.bmp", __FUNCTION__);
	SaveImage(cache->Pixels, cache->Width, cache->Height, "1_probable_5_6.bmp");
	LineFilter_MarkObjectProbability(0, "2.bmp");
	cache = CachePicturePrintErrors("2.bmp", __FUNCTION__);
	SaveImage(cache->Pixels, cache->Width, cache->Height, "2_probable_5_6.bmp");
	LineFilter_MarkObjectProbability(0, "3.bmp");
	cache = CachePicturePrintErrors("3.bmp", __FUNCTION__);
	SaveImage(cache->Pixels, cache->Width, cache->Height, "3_probable_5_6.bmp");

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
