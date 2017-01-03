#pragma once

/**********************************************************************
These function were written for a 2D game
The game used no translations, no scaling...
This library aims to remove animation and detect transparent color in images
**********************************************************************/

// For 2D games that do not use translation for images ( static images) :
// - ver 1 : make a small image ( if possible ) that contains unique features not found elsewhere. Example your nickname
// - ver 2 : make an image that does not contain blinking for example
// take a subreagion picture
// take additional region pictures, that do not include the initial region, and check if we can find pixel pairs that make the initial subregion unique
void WINAPI TakeScreenshotDetectUniquePixelsInitial(int aLeft, int aTop, int aRight, int aBottom);
//void WINAPI LoadImageDetectUniquePixelsInitial(char *aImageFile);
// unique pixels are colors ( or color pairs ) that can not be found in other places. Example a bright red pixel. Since one single pixel is hard to be unique, you can define a block of pixel combination that is unique
// you might need to experiment what is the smalest block that makes a pixel block unique without including dynamic background
void WINAPI TakeScreenshotKeepUniquePixels(int aLeft, int aTop, int aRight, int aBottom, int BlockSize);
// dynamic pixels are colors that change in the same image
void WINAPI TakeScreenshotRemoveDynamicPixels(int aLeft, int aTop, int aRight, int aBottom);
//presumes part of the image is present in the current screen. Search for best match. Remove pixels that changed since previous search
void WINAPI TakeScreenshotRemoveDynamicPixelsAutoPosition(int aLeft, int aTop, int aRight, int aBottom);
void WINAPI SaveScreenshotUniqueFeatures();
char* WINAPI ImageSearchOnScreenshotUniqueFeatures(char *aImageFile, int AcceptedErrorCount, int StopAfterNFullMatches);
