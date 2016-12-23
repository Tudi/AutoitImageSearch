#pragma once

// For 2D games that do not use translation for images ( static images) :
// - ver 1 : make a small image ( if possible ) that contains unique features not found elsewhere. Example your nickname
// - ver 2 : make an image that does not contain blinking for example
// take a subreagion picture
// take additional region pictures, that do not include the initial region, and check if we can find pixel pairs that make the initial subregion unique
void WINAPI TakeScreenshotDetectUniquePixelsInitial(int aLeft, int aTop, int aRight, int aBottom);
//void WINAPI LoadImageDetectUniquePixelsInitial(char *aImageFile);
void WINAPI TakeScreenshotKeepUniquePixels(int aLeft, int aTop, int aRight, int aBottom);
void WINAPI TakeScreenshotRemoveDynamicPixels(int aLeft, int aTop, int aRight, int aBottom);
void WINAPI SaveScreenshotUniqueFeatures();
char* WINAPI ImageSearchOnScreenshotUniqueFeatures(char *aImageFile, int TransparentColor, int AcceptedColorDiff, int AcceptedErrorCount, int StopAfterNMatches);
