/*
AutoHotkey

Copyright 2003-2007 Chris Mallett (support@autohotkey.com)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/


#ifndef util_h
#define util_h

#include "stdafx.h" // pre-compiled headers
//#include "defines.h"

#define CLR_DEFAULT 0x808080
#define ToWideChar(source, dest, dest_size_in_wchars) MultiByteToWideChar(CP_ACP, 0, source, -1, dest, dest_size_in_wchars)

#define CLR_NONE 0xFFFFFFFF
#define IS_SPACE_OR_TAB(c) (c == ' ' || c == '\t')

#define DEFAULT_STR_BUFFER_SIZE 1024

struct CachedPicture
{
	char		FileName[ DEFAULT_STR_BUFFER_SIZE ];
	HBITMAP		LoadedPicture;
	LPCOLORREF	Pixels;
};

HBITMAP LoadPicture(char *aFilespec, int aWidth, int aHeight, int &aImageType, int aIconNumber
	, bool aUseGDIPlusIfAvailable);

LPCOLORREF getbits(HBITMAP ahImage, HDC hdc, LONG &aWidth, LONG &aHeight, bool &aIs16Bit, int aMinColorDepth = 8);

char* WINAPI ImageSearch(int aLeft, int aTop, int aRight, int aBottom, char *aImageFile);

void WINAPI TakeScreenshot(int aLeft, int aTop, int aRight, int aBottom);
void WINAPI SaveScreenshot();
void WINAPI ResizeScreenshot( int NewWidth, int NewHeight );
void WINAPI ReleaseScreenshot( );
void WINAPI BlurrImage( int HalfKernelSize );
char* WINAPI ImageSearchOnScreenshot( char *aImageFile, int TransparentColor, int AcceptedColorDiff, int AcceptedErrorCount, int StopAfterNMatches );

void DumpAsPPM( unsigned char *R,unsigned char *G,unsigned char *B, int Width, int Height );
void DumpAsPPM( LPCOLORREF RGB, int Width, int Height );
void DumpAsPPM( LPCOLORREF RGB, int Width, int Height, int Stride );

#endif
