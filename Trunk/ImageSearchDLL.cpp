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
		//start precise counter
		StartCounter();
		FileDebug("=================================================");
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
	StartCounter();
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
//	_getch();
#endif

/*	{
		RegisterOCRFont( "OCR_1_green.bmp", '1' );
		RegisterOCRFont( "OCR_2_green.bmp", '2' );
		RegisterOCRFont( "OCR_3_green.bmp", '3' );

		_getch();
	//	TakeScreenshot( 0, 0, 800, 800 );
	//	TakeScreenshot( 63, 100, 103, 108 );
		TakeScreenshot( 218, 95, 271, 116 );
	//	SaveScreenshot();

	//	ReadTextFromScreenshot( 53, 98, 127, 117 );
	//	ReadTextFromScreenshot( 63, 100, 103, 108 );
		ReadTextFromScreenshot( 218, 95, 271, 116 );
	}/**/
//	SetupSimilarSearch( 0, 1, 3, 0 );
//	SearchSimilarOnScreenshot( "tosearch10.bmp" );
/*	{
		_getch();
		TakeScreenshot( 0, 0, 2000, 2000 );
		//SaveScreenshot();
		char *res;
		res = ImageSearchOnScreenshot( "Resync.bmp", 0x01000000, 0, 0, 1 );
		TakeScreenshot( 1709, 1139, 1709 + 7, 1139 + 23 );
		//SaveScreenshot();
	}/**/
/*	{
		_getch();
		TakeScreenshot( 0, 0, 2000, 2000 );
		char *res;
		res = ImageSearchOnScreenshot( "Resync.bmp", 0x01000000, 0, 0, 1 );
		int Start = GetTimeTickI();
		for( int i = 0; i < 1000; i++ )
		{
			TakeScreenshot( 1709, 1139, 1709 + 7, 1139 + 23 );
			res = IsAnythingChanced( 0, 0, 7, 23 );
			if( res[0] != '0' )
			{
				res[0] = '1';
			}
		}
		int End = GetTimeTickI();
		printf("result of search benchmarking : %d %d FPS\n", End - Start, 1000 * 1000 / ( End - Start + 1 ) );
		_getch();
	}/**/
/*	{
		_getch();
		TakeScreenshot( 100, 100, 1500, 800 );
		char *res;
		res = ImageSearchOnScreenshotBest_SAD("bobber_try2.bmp");
		int Start = GetTimeTickI();
		for( int i = 0; i < 1; i++ )
		{
			TakeScreenshot( 100,100, 1500, 800 );
			res = IsAnythingChanced( 0, 0, 7, 23 );
			if( res[0] != '0' )
			{
				res[0] = '1';
			}
		}
		int End = GetTimeTickI();
		printf("result of search benchmarking : %d %d FPS\n", End - Start, 1000 * 1000 / ( End - Start + 1 ) );
		_getch();
	}/**/
	{
//		_getch();
		TakeScreenshot(0, 0, 1600, 960);
		char *res;
		res = ImageSearchOnScreenshotBest_Transparent_SAD("16x16.bmp");
		res = ImageSearchOnScreenshotBest_Transparent_SAD2("16x16.bmp");
		int i, Start = GetTimeTickI();
		for (i = 0; i < 100; i++)
		{
			TakeScreenshot(0, 0, 1600, 960);
			res = ImageSearchOnScreenshotBest_Transparent_SAD("16x16.bmp");
			if (res[0] != '0')
			{
				res[0] = '1';
			}
		}
		int End = GetTimeTickI();
		printf("result of search benchmarking : %d ms %d FPS\n", End - Start, i * 1000 * 1000 / (End - Start + 1));
//		_getch();
	}/**/
	{
//		_getch();
		TakeScreenshot(0, 0, 1600, 960);
		char *res;
		res = ImageSearchOnScreenshotBest_Transparent_SAD2("16x16.bmp");
		int i, Start = GetTimeTickI();
		for (i = 0; i < 100; i++)
		{
			TakeScreenshot(0, 0, 1600, 960);
			res = ImageSearchOnScreenshotBest_Transparent_SAD2("16x16.bmp");
			if (res[0] != '0')
			{
				res[0] = '1';
			}
		}
		int End = GetTimeTickI();
		printf("result of search benchmarking : %d ms %d FPS\n", End - Start, i * 1000 * 1000 / (End - Start + 1));
//		_getch();
	}/**/
	{
		//		_getch();
		TakeScreenshot(0, 0, 1600, 960);
		char *res;
		res = ImageSearchOnScreenshotBestTransparent("16x16.bmp");
		int i, Start = GetTimeTickI();
		for (i = 0; i < 10; i++)
		{
			TakeScreenshot(0, 0, 1600, 960);
			res = ImageSearchOnScreenshotBestTransparent("16x16.bmp");
			if (res[0] != '0')
			{
				res[0] = '1';
			}
		}
		int End = GetTimeTickI();
		printf("result of search benchmarking : %d ms %d FPS\n", End - Start, i * 1000 * 1000 / (End - Start + 1));
		//		_getch();
	}/**/
}