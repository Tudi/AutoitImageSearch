#ifndef _LINEAR_RESAMPLER_H_
#define _LINEAR_RESAMPLER_H_

//this was required for old hardware where float to int conversion is slow. In the future you might be able to use float indexing directly
#define FLOAT_PRECISSION_BITS	15		// values are on 16 bits, using more for float precission does not count. Make sure to not have this number too big !
#define FLOAT_PRECISSION		( 1 << FLOAT_PRECISSION_BITS )
#define FLOAT_PRECISSION_ROUNUPER	( FLOAT_PRECISSION - 1 ) //add this before division for roundup effect
#define RGB_BYTE_COUNT 4

void ResampleRGBLiniar( unsigned char *src, unsigned char *dst, int SrcW, int SrcH, int DestW, int DestH, bool KeepAspectRatio  );
void ResampleRGBLiniar4ByteDownsample( unsigned char *psrc, unsigned char *pdst, int SrcW, int SrcH, int DestW, int DestH, int SrcStride = 0, int DestStride = 0 );

#endif

