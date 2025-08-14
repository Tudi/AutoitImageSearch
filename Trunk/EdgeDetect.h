#pragma once

/**********************************************************************
Mostly experimental image functions for 3D games where objects rotate
The idea was to be able to detect images that have slight rotation or translation compared to the searched image
Comparing edge similarities can give you a Percent based match. X image is more similar than Y
To keep as much as possible of the original image. You should : Edge detect + Color maximize + Binarize
**********************************************************************/

// good old canny edge detection based on a 3x3 matrix
void WINAPI EdgeDetectRobertCross3Channels();
void WINAPI EdgeDetectRobertCrossGrayScale();
// Experimental ! Try to guess if that 1 pixel is missing
void WINAPI EdgeSmoothing();
// Remove a pixel if : alone / not part of a straight line
// this should be run multiple times probably ( removes 1 single pixel / pass )
void WINAPI EdgeThinning();
// Might lead to disconnected edges. You might want to combine this with an EdgeConnect
// Radius should be something small. 1 or 2 pixels
void WINAPI EdgeKeepLocalMaximaMaximaOnly( int Radius );
// This is distorting the original image. Adding extra info based on guessing
void WINAPI EdgeConnectToBestMatch( int Radius );
//Only keep strong edges. Ditch the rest. This is an image binarization. You should have a grayscale image at this point
void WINAPI EdgeKeepUpperPercent(int Percent);
// for a better approximation, we could experiment with bleeding edges. 
// Might be slow to generate the bleed map on current image, but the searched image might give a better accuracy sometimes
void WINAPI EdgeBleed();
// You might want to start with this
void WINAPI ConvertToGrayScale();
// You might want to start with this
void WINAPI ConvertToGrayScaleMaxChannel();
// larger convolution matrix
void WINAPI EdgeDetectSobel3Channels();
// copy from previous screenshot the data only from edges
void WINAPI EdgeCopyOriginalForEdges();

void ConvertToGrayscale_v3(const LPCOLORREF src, uint8_t* dst, size_t count);