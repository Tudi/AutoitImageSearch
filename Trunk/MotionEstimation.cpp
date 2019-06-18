#include "StdAfx.h"

LIBRARY_API ScreenshotStruct MotionDiff;

__forceinline unsigned int SAD_16x4byte( unsigned char *address1, unsigned char *address2, int stride1, int stride2 )
{
	unsigned short sad_array[8];
	__m128i l0, l1, line_sad, acc_sad;

	acc_sad = _mm_setzero_si128();

	l0 = _mm_loadu_si128((__m128i*)address1);
	l1 = _mm_loadu_si128((__m128i*)address2);
	line_sad = _mm_sad_epu8(l0, l1);
	acc_sad = _mm_add_epi16(acc_sad, line_sad);

	l0 = _mm_loadu_si128((__m128i*)(address1+stride1));
	l1 = _mm_loadu_si128((__m128i*)(address2+stride2));
	line_sad = _mm_sad_epu8(l0, l1);
	acc_sad = _mm_add_epi16(acc_sad, line_sad);

	l0 = _mm_loadu_si128((__m128i*)(address1+stride1+stride1));
	l1 = _mm_loadu_si128((__m128i*)(address2+stride2+stride2));
	line_sad = _mm_sad_epu8(l0, l1);
	acc_sad = _mm_add_epi16(acc_sad, line_sad);

	l0 = _mm_loadu_si128((__m128i*)(address1+stride1+stride1+stride1));
	l1 = _mm_loadu_si128((__m128i*)(address2+stride2+stride2+stride2));
	line_sad = _mm_sad_epu8(l0, l1);
	acc_sad = _mm_add_epi16(acc_sad, line_sad);

	_mm_storeu_si128((__m128i*)(&sad_array[0]), acc_sad);

	return (sad_array[0]+sad_array[4]);
}

unsigned int SAD_16x4byteNormal( unsigned char *address1, unsigned char *address2, int stride1, int stride2 )
{
	int SAD = 0;
	for( int y=0;y<4;y++)
		for( int x=0;x<4;x++)
		{
			SAD += abs( address1[ y * stride1 + x * 4 + 0 ] - address2[ y * stride2 + x * 4 + 0 ] );
			SAD += abs( address1[ y * stride1 + x * 4 + 1 ] - address2[ y * stride2 + x * 4 + 1 ] );
			SAD += abs( address1[ y * stride1 + x * 4 + 2 ] - address2[ y * stride2 + x * 4 + 2 ] );
		}
	return SAD;
}

unsigned int RetSAD = 0;
int WINAPI GenerateDiffMap()
{
	FileDebug( "Started motion estimation" );
	if( CurScreenshot->Pixels == NULL || PrevScreenshot == NULL || PrevScreenshot->Pixels == NULL )
	{
		FileDebug( "Can not motion estimate as there are not enough screenshots" );
		return 0;
	}
/*	if( CurScreenshot->Right != PrevScreenshot->Right || CurScreenshot->Left != PrevScreenshot->Left
		|| CurScreenshot->Top != PrevScreenshot->Top || CurScreenshot->Bottom != PrevScreenshot->Bottom )
	{
		FileDebug( "Can not motion estimate as the 2 screenshots were not taken from same screen place" );
		return 0;
    }/**/

	unsigned int Width = CurScreenshot->Right - CurScreenshot->Left;
	unsigned int Height = CurScreenshot->Bottom - CurScreenshot->Top;
	if( ( MotionDiff.Right - MotionDiff.Left ) * ( MotionDiff.Bottom - MotionDiff.Top ) != Width * Height && MotionDiff.Pixels != NULL )
	{
		_aligned_free( MotionDiff.Pixels );
		MotionDiff.Pixels = NULL;
	}

	MotionDiff.Right = CurScreenshot->Right;
	MotionDiff.Left = CurScreenshot->Left;
	MotionDiff.Bottom = CurScreenshot->Bottom;
	MotionDiff.Top = CurScreenshot->Top;

	if( MotionDiff.Pixels == NULL )
		MotionDiff.Pixels = (LPCOLORREF)_aligned_malloc( Width * Height * sizeof(COLORREF) + SSE_PADDING, SSE_ALIGNMENT);

    MotionDiff.IsDiffMap = true;
//DumpAsPPMBGR( PrevScreenshot->Pixels, Width, Height );
//DumpAsPPMBGR( CurScreenshot->Pixels, Width, Height );

	unsigned char *Out = (unsigned char *)MotionDiff.Pixels;
	RetSAD = GenerateDiffMap(PrevScreenshot->Pixels, CurScreenshot->Pixels, Width, Height, Out);

//	char DebugBuff[500];
//	sprintf_s( DebugBuff, 500, "current sad now %d", RetSAD );
//	FileDebug( DebugBuff );

	FileDebug( "Finished motion estimation" );

	return RetSAD;
}

unsigned int GenerateDiffMap(LPCOLORREF Pix1, LPCOLORREF Pix2, int Width, int Height, unsigned char *DiffMapOutput)
{
	int RetSAD = 0;
	for (unsigned int Y = 0; Y < Height - 4; Y += 4)
	{
		unsigned char *Data1 = (unsigned char *)(&Pix1[Y * Width]);
		unsigned char *Data1End = Data1 + Width * 4;
		unsigned char *Data2 = (unsigned char *)(&Pix2[Y * Width]);
		unsigned char *Data3 = &DiffMapOutput[Y / 4 * Width / 4];
		for (; Data1 < Data1End; Data1 += 16, Data2 += 16)
		{
			int IntrinsicSad = SAD_16x4byte(Data1, Data2, Width * 4, Width * 4);
			RetSAD += IntrinsicSad;

			*Data3 = IntrinsicSad / (4 * 4 * 3); // 4x4 pixels, each has 3 channels
//			*Data3 = ( Data1[0] + Data1[1] + Data1[2] ) / 3;
//			*Data3 = ( Data2[0] + Data2[1] + Data2[2] ) / 3;
			Data3 += 1;
		}
	}
	return RetSAD;
}