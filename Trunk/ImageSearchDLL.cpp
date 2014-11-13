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

#ifdef _CONSOLE
//	RunSCreenCaptureBenchmark(); // 350 fps on my pc
//	RunCheckChangeBenchmark(); // 587 fps on my pc
//	RunResizeBenchmark(); // 346 fps on my pc
//	RunBlurrBenchmark(); // 37 fps on my pc
//	RunDiffBenchmark(); // 704 fps on my pc
//	RunImgSearchBenchmark(); // 0.05 - 109 fps on my pc
//	RunErrodeBenchmark(); // 66 fps on my pc
//	RunSimilarSearchBenchmark(); // 302 fps is the setup and 131 fps is the search on my pc
//	RunPiramidSearchBenchmark(); // 151 fps is the setup and 600 fps is the search on my pc
//	RunEdgeDetectBenchmark(); // 62 fps / 15 fps 
#endif

	_getch();
	TakeScreenshot( 0, 0, 800, 800 );
//	TakeScreenshot( 63, 100, 103, 108 );
//	SaveScreenshot();
	CachePicture( "OCR_1.bmp" );
	CachePicture( "OCR_2.bmp" );
	CachePicture( "OCR_3.bmp" );

	ReadTextFromScreenshot( 53, 98, 107, 117 );
//	ReadTextFromScreenshot( 63, 100, 103, 108 );
//	SetupSimilarSearch( 0, 1, 3, 0 );
//	SearchSimilarOnScreenshot( "tosearch10.bmp" );
}