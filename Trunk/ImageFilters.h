#ifndef _IMAGE_FILTERS_H_
#define _IMAGE_FILTERS_H_

void WINAPI BlurrImage( int HalfKernelSize );
void WINAPI ErrodeDiffMap( int HalfKernelSize );
void WINAPI EdgeDetect( int HalfKernelSize );

#endif