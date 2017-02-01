#pragma once

// it's a lucky case because all characters are separated + they do not use blending / transparency / antialias. Or at least we apply some filter to the image that makes them be separated ( some fonts have a fade out effect that can get neglected )
// this lucky case will try to aim to find new fonts and save them as images. You can than later define a character for them and load them next time

//start X and Y should be the upper left part of the text
// !! you should have a binary image at this point !!
// characters should not touch each other. If they do, use some other OCR !
char * WINAPI OCR_ReadTextLeftToRightSaveUnknownChars(int StartX, int StartY, int EndX, int EndY);