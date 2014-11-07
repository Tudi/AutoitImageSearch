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
	if( ul_reason_for_call == DLL_PROCESS_ATTACH )
	{
		memset( PictureCache, 0, sizeof( PictureCache ) );
		memset( &MotionDiff, 0, sizeof( MotionDiff ) );
		MotionDiff.IsDiffMap = true;
		ScreenshotStoreIndex = 0;
		CurScreenshot = &ScreenshotCache[ 0 ];
		PrevScreenshot = &ScreenshotCache[ 1 ];
		NrPicturesCached = 0;
		FileDebug( "=================================================" );
	}
	else if( ul_reason_for_call == DLL_PROCESS_DETACH )
	{
	}
    return TRUE;
}

#ifdef _MANAGED
#pragma managed(pop)
#endif


void main()
{
//	RunSQRTBenchmark(); //fact : sqrt( double) rocks
	SetupSimilarSearch( 0, 1, 3 );
	TakeScreenshot( 0, 0, 800, 800 );
	SearchSimilarOnScreenshot( "tosearch10.bmp" );
}