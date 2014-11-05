// ImageSearchDLL.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"

#ifdef _MANAGED
#pragma managed(push, off)
#endif

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	memset( PictureCache, 0, sizeof( PictureCache ) );
	memset( &MotionDiff, 0, sizeof( MotionDiff ) );
	ScreenshotStoreIndex = 0;
	CurScreenshot = &ScreenshotCache[ 0 ];
	PrevScreenshot = &ScreenshotCache[ 1 ];
	NrPicturesCached = 0;
    return TRUE;
}

#ifdef _MANAGED
#pragma managed(pop)
#endif


void _tmain()
{
}