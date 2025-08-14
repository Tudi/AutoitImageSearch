#pragma once

/*
* Some scripts will wait for certain images to appear on the screen. 
* These images might be expected to appear on specific region of the screen.
* We make statistics where these searches took place in the past and only screenshot that region next time
* In case a new searched region is requested, the next screenshot should be allowed to take place on the same location
*/
void AddSearchedRegion(size_t ImageId, int aLeft, int aTop, int aRight, int aBottom);
void GetAdvisedNewCaptureSize(int &aLeft, int &aTop, int &aRight, int &aBottom);
