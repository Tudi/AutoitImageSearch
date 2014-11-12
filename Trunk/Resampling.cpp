#include "StdAfx.h"

#ifndef MAX
	#define  MAX(a,b)		(((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
	#define  MIN(a,b)		(((a) < (b)) ? (a) : (b))
#endif


//note that this is mostly tested when in/out size is dividable by 16. Using Odd numbers might give issues at rounding
void ResampleRGBLiniar4ByteDownsample( unsigned char *psrc, unsigned char *pdst, unsigned int SrcW, unsigned int SrcH, unsigned int DestW, unsigned int DestH, unsigned int SrcStride, unsigned int DestStride )
{
	unsigned int int_conv_y = (unsigned int)( SrcH * FLOAT_PRECISSION / DestH );
	unsigned int int_conv_x = (unsigned int)( SrcW * FLOAT_PRECISSION / DestW );
	unsigned int *src = (unsigned int *)psrc;
	unsigned int *dst = (unsigned int *)pdst;

#ifdef _DEBUG
	assert( ( ( ( int_conv_y * DestH ) >> FLOAT_PRECISSION_BITS ) ) <= SrcH );
	assert( ( ( ( int_conv_x * DestW ) >> FLOAT_PRECISSION_BITS ) ) <= SrcW );
	assert( ( ( ( int_conv_y * DestH ) >> FLOAT_PRECISSION_BITS ) * ( (int_conv_x * DestW ) >> FLOAT_PRECISSION_BITS ) * 3 / 2 ) <= SrcW * SrcH * 3 / 2 );
	assert( DestH <= SrcH );
#endif
	if( SrcStride == 0 )
		SrcStride = SrcW;
	if( DestStride == 0 )
		DestStride = DestW;

	unsigned int stacking_precission_y = 0;
	for( unsigned int y=0;y<DestH;y++)
	{
		unsigned int	stacking_precission_x = 0;

		unsigned int	converted_row_index = stacking_precission_y >> FLOAT_PRECISSION_BITS;
		unsigned int    *tsrc = src + converted_row_index * SrcW;
		stacking_precission_y += int_conv_y;

		unsigned int start = y * DestW;
		unsigned int end = start + DestW;
#ifdef _DEBUG
		assert( converted_row_index <= SrcH );
		assert( dst + end <= dst + DestW * DestH );
#endif

		for( unsigned int x=start;x<end;x++)
		{
			unsigned int converted_col_index = stacking_precission_x >> FLOAT_PRECISSION_BITS;
#ifdef _DEBUG
			assert( converted_col_index <= SrcW );
#endif
			dst[ x ] = tsrc[ converted_col_index ];
			stacking_precission_x += int_conv_x;
		}
	}
}

void ResampleRGBLiniarInbox4ByteDownSample( unsigned char *psrc, unsigned char *pdst, unsigned int SrcW, unsigned int SrcH, unsigned int SrcStride, unsigned int DestW, unsigned int DestH, unsigned int DestBuffStride, int DestBuffHeight, int DestSartX, int DestStartY )
{
	unsigned int int_conv_y = (unsigned int)( SrcH * FLOAT_PRECISSION / DestH );
	unsigned int int_conv_x = (unsigned int)( SrcW * FLOAT_PRECISSION / DestW );
	unsigned int *src = (unsigned int *)psrc;
	unsigned int *dst = (unsigned int *)pdst;

#ifdef _DEBUG
	assert( ( ( ( int_conv_y * DestH ) >> FLOAT_PRECISSION_BITS ) ) <= SrcH );
	assert( ( ( ( int_conv_x * DestW ) >> FLOAT_PRECISSION_BITS ) ) <= SrcW );
	assert( ( ( ( int_conv_y * DestH ) >> FLOAT_PRECISSION_BITS ) * ( (int_conv_x * DestW ) >> FLOAT_PRECISSION_BITS ) * 3 / 2 ) <= SrcW * SrcH * 3 / 2 );
	assert( DestH <= SrcH );
#endif

	unsigned int stacking_precission_y = 0;
	for( unsigned int y=0;y<DestH;y++)
	{
		unsigned int stacking_precission_x = 0;

		unsigned int	converted_row_index = stacking_precission_y >> FLOAT_PRECISSION_BITS;
		unsigned int    *tsrc = src + converted_row_index * SrcStride * RGB_BYTE_COUNT;
		stacking_precission_y += int_conv_y;

		unsigned int start = ( ( y + DestStartY ) * DestBuffStride + DestSartX ) * RGB_BYTE_COUNT;
		unsigned int end = start + DestW * RGB_BYTE_COUNT;
#ifdef _DEBUG
		assert( converted_row_index <= SrcH );
		assert( dst + end <= dst + DestBuffStride * DestBuffHeight * RGB_BYTE_COUNT );
#endif

		for( unsigned int x=start;x<end;x++)
		{
			unsigned int converted_col_index = stacking_precission_x >> FLOAT_PRECISSION_BITS;
#ifdef _DEBUG
			assert( converted_col_index <= SrcW );
#endif
			dst[ x ] = tsrc[ converted_col_index ];
			stacking_precission_x += int_conv_x;
		}
	}
}

void AddBlackBoxRGB( unsigned char *dst, int bufWidth, int bufHeight, int BoxStartX, int BoxStartY, int boxWidth, int boxHeight, int RGBByteCount )
{
#ifdef _DEBUG
	assert( bufWidth >= BoxStartX + boxWidth && bufHeight >= boxHeight + BoxStartY );
	assert( BoxStartX >= 0 && BoxStartY >= 0 );
	assert( bufWidth >= boxWidth && bufHeight >= boxHeight );
#endif
	//avoid out of bounds buffer processing if possible
	if( bufWidth < BoxStartX + boxWidth )
		BoxStartX = bufWidth - boxWidth;
	if( bufHeight < boxHeight + BoxStartY )
		BoxStartY = bufHeight - boxHeight;
	//get the params we need
	unsigned char *dstRGB = dst + ( bufWidth * BoxStartY + BoxStartX ) * RGBByteCount;
	//process the buffer
	if( boxWidth == bufWidth )
	{
		memset( dstRGB, 0, boxWidth * boxHeight * RGBByteCount );
	}
	else
	{
		for( int y = 0; y < boxHeight; y++ )
			memset( dstRGB + y * bufWidth * RGBByteCount, 0, boxWidth * RGBByteCount );
	}
}

void ResampleRGBLiniar( unsigned char *src, unsigned char *dst, int SrcW, int SrcH, int DestW, int DestH, bool KeepAspectRatio  )
{
	if( KeepAspectRatio == true )
	{
		// using int for the sake of rounding
		int ScaleBoth;
		int ScaleWidth = ( DestW * FLOAT_PRECISSION ) / SrcW;
		int ScaleHeigth = ( DestH * FLOAT_PRECISSION ) / SrcH;
		if( ScaleWidth < ScaleHeigth )
			ScaleBoth = ScaleWidth;
		else
			ScaleBoth = ScaleHeigth;
		// calc up / down , left / right box size
		int NewHeight = MIN( ( SrcH * ScaleBoth + FLOAT_PRECISSION / 2 ) / FLOAT_PRECISSION, DestH );
		int NewWidth = MIN( ( SrcW * ScaleBoth + FLOAT_PRECISSION / 2 ) / FLOAT_PRECISSION, DestW );
		int RemainingGapSizeHeight = DestH - NewHeight;
		int RemainingGapSizeWidth = DestW - NewWidth;
		//apply boxes if needed. Worst case we could init the whole output image. That would be bugfree and slower in some cases
		if( RemainingGapSizeHeight > 0 )
		{
			AddBlackBoxRGB( dst, DestW, DestH, 0, 0, DestW, RemainingGapSizeHeight / 2, RGB_BYTE_COUNT );
			AddBlackBoxRGB( dst, DestW, DestH, 0, RemainingGapSizeHeight / 2 + NewHeight, DestW, DestH - NewHeight - RemainingGapSizeHeight / 2, RGB_BYTE_COUNT );
		}
		else if( RemainingGapSizeWidth > 0 )
		{
			AddBlackBoxRGB( dst, DestW, DestH, 0, 0, RemainingGapSizeWidth / 2, DestH, RGB_BYTE_COUNT );
			AddBlackBoxRGB( dst, DestW, DestH, RemainingGapSizeWidth / 2 + NewWidth, 0, DestW - NewWidth - RemainingGapSizeWidth / 2, DestH, RGB_BYTE_COUNT );
		} 
		// scale the picture in the remaining box
		ResampleRGBLiniarInbox4ByteDownSample( src, dst, SrcW, SrcH, SrcW, NewWidth, NewHeight, DestW, DestH, RemainingGapSizeWidth / 2, RemainingGapSizeHeight / 2 );
	}
	else 
//		if( KeepAspectRatio == false )
	{
		ResampleRGBLiniar4ByteDownsample( src, dst, SrcW, SrcH, DestW, DestH );
	}
}

void WINAPI ResizeScreenshot( int NewWidth, int NewHeight )
{
	FileDebug( "Started resizing the screenshot" );
	if( CurScreenshot->Pixels == NULL )
	{
		FileDebug( "WARNING:Screenshot buffer is null when trying to resize it!" );
		return;
	}
	LPCOLORREF new_Pixels = (COLORREF*)_aligned_malloc( NewWidth * NewHeight * sizeof( COLORREF ) + SSE_PADDING, SSE_ALIGNMENT );
	int Width = CurScreenshot->Right - CurScreenshot->Left;
	int Height = CurScreenshot->Bottom - CurScreenshot->Top;
	ResampleRGBLiniar4ByteDownsample( (unsigned char *)CurScreenshot->Pixels, (unsigned char *)new_Pixels, Width, Height, NewWidth, NewHeight );
	CurScreenshot->Left = 0;
	CurScreenshot->Top = 0;
	CurScreenshot->Right = NewWidth;
	CurScreenshot->Bottom = NewHeight;
	_aligned_free( CurScreenshot->Pixels );
	CurScreenshot->Pixels = new_Pixels;
}