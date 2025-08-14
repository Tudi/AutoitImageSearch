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


int main(int argc, char **arg)
{
	StartCounter();

#ifdef _CONSOLE
	ParseArgStrings(argc, arg);
#endif

	//RunSQRTBenchmark(); //fact : sqrt( double) rocks

	//RunLordsTesting();
	//RunLordsMobileTests();
	//RunLordsMobileTestsNoOCR();

//	TestTextureLearnOnPictures();

#if defined( _CONSOLE ) && !defined( _DEBUG )
//	RunDrawLineBenchmark();
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
//	RunImgSearchBenchmark2();
//	RunImgSearchGradientBenchmark();

//	_getch();
//	return;
#endif

#if defined(_DEBUG) || defined(_CONSOLE)
	{
		// take a screenshot
		TakeScreenshot(0, 0, 1920, 1080);
		ApplyColorBitmask(0x00F0F0F0);
		// load cache and test grayscale prefilter
		const char* searchedPicFilename = "visual_studio_text.bmp";
		const char* ret = ImageSearch_SAD_Region(searchedPicFilename, 0, 0, 1900, 1000, SADSearchRegionFlags::SSRF_ST_ALLOW_MULTI_STAGE_GSAD);
		CachedPicture* cache = CachePicture(searchedPicFilename);
		SaveImage((LPCOLORREF)cache->pGrayscalePixels, cache->Width, cache->Height, "ciGS.bmp", 1);
		SaveImage((LPCOLORREF)CurScreenshot->pGrayscalePixels, CurScreenshot->Width, CurScreenshot->Height, "ssGS.bmp", 1);
		ret = ImageSearch_SAD_Region(searchedPicFilename, 0, 0, 1900, 1000, SADSearchRegionFlags::SSRF_ST_ALLOW_MULTI_STAGE_GSAD2);
	}
//	TestSATDCorrectness(); return 0;
/* {
		for (size_t i = 0; i < 4; i++) { // testing leak detector
			TakeScreenshot(0, 0, 1920, 1080);
		}
		ApplyColorBitmask(0x00F0F0F0);
		ApplyColorBitmaskCache("visual_studio_text.bmp", 0x00F0F0F0);
//		SaveScreenshot();

		int Start,End;
#ifdef _DEBUG
	#define IMG_HASH_REPEAT_TEST_COUNT 1
#else
	#define IMG_HASH_REPEAT_TEST_COUNT 10
#endif
		// ignore first run from benchmark
		const char* ret = ImageSearch_SAD_Region("visual_studio_text.bmp", 0, 0, 1920, 1080, SADSearchRegionFlags::SSRF_ST_ENFORCE_SAD_WITH_HASH);
		printf("New method Returned : %s\n", ret);
		ret = ImageSearch_SAD_Region("visual_studio_text.bmp", 0, 0, 1920, 1080, SADSearchRegionFlags::SSRF_ST_PROCESS_INLCUDE_DIFF_INFO);
		printf("Old method Returned : %s\n", ret);

		// test how much time it takes to prepare the whole screenshot
		Start = GetTimeTickI();
		for (size_t i = 0; i < IMG_HASH_REPEAT_TEST_COUNT; i++)
		{
			ret = ImageSearch_SAD_Region("visual_studio_text.bmp", 0, 0, 1920, 1080, (SADSearchRegionFlags)(SADSearchRegionFlags::SSRF_ST_ENFORCE_SAD_WITH_HASH | SADSearchRegionFlags::SSRF_ST_PROCESS_INLCUDE_DIFF_INFO));
		}
		End = GetTimeTickI();
		printf("result of HASH enforced SAD prepare : %d ms duration, %f FPS\n", (End - Start)/ IMG_HASH_REPEAT_TEST_COUNT, 1000.0 / (float)(End - Start + 1) * IMG_HASH_REPEAT_TEST_COUNT);

		Start = GetTimeTickI();
		for (size_t i = 0; i < IMG_HASH_REPEAT_TEST_COUNT; i++)
		{
			ret = ImageSearch_SAD_Region("visual_studio_text.bmp", 0, 0, 1920, 1080, SADSearchRegionFlags::SSRF_ST_NO_FLAGS);
		}
		End = GetTimeTickI();
		printf("result of SAD search : %d ms duration, %f FPS\n", (End - Start) / IMG_HASH_REPEAT_TEST_COUNT, 1000.0 / (float)(End - Start + 1) * IMG_HASH_REPEAT_TEST_COUNT);

		DumpAllocationsToLogger();
	}/**/
/* {
		char* res;
		TakeScreenshot(0, 0, 1920, 1080);
//		SaveScreenshot();
		
		int Start,End;
		
#define SUMSAD_REPEAT_TEST_COUNT 10
		// ignore first run from benchmark
		char* ret = ImageSearch_Similar("visual_studio_text.bmp", 40);
		printf("New method Returned : %s\n", ret);

		// test how much time it takes to prepare the whole screenshot
		Start = GetTimeTickI();
		for (size_t i = 0; i < SUMSAD_REPEAT_TEST_COUNT; i++)
		{
			FreeSADSUMScreenshot(&CurScreenshot->SADSums);
			ComputeSADSumScreenshot(CurScreenshot->Pixels, 1920, 1080, &CurScreenshot->SADSums, SSAS_128x128);
		}
		End = GetTimeTickI();
		printf("result of SUM SAD prepare : %d ms duration, %f FPS\n", (End - Start)/ SUMSAD_REPEAT_TEST_COUNT, 1000.0 / (float)(End - Start + 1) * SUMSAD_REPEAT_TEST_COUNT);

		Start = GetTimeTickI();
		for (size_t i = 0; i < SUMSAD_REPEAT_TEST_COUNT; i++)
		{
			char *ret = ImageSearch_Similar("visual_studio_text.bmp", 40);
		}
		End = GetTimeTickI();
		printf("result of SUM SAD search : %d ms duration, %f FPS\n", (End - Start) / SUMSAD_REPEAT_TEST_COUNT, 1000.0 / (float)(End - Start + 1) * SUMSAD_REPEAT_TEST_COUNT);

		Start = GetTimeTickI();
		res = ImageSearch_SAD("visual_studio_text.bmp");
		End = GetTimeTickI();
		printf("result of SAD search benchmarking : %d ms duration, %f FPS\n", End - Start, 1000.0 / (float)(End - Start + 1));
		printf("Old method returned : %s\n", ret);
	}/**/
/*	{
		OCR_RegisterFont( "OCR_1_green.bmp", '1' );
		OCR_RegisterFont( "OCR_2_green.bmp", '2' );
		OCR_RegisterFont( "OCR_3_green.bmp", '3' );

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
 /*{
		_getch();
		const char *res;
		TakeScreenshot(0, 0, 1920, 1080);
		int Start = GetTimeTickI();
		res = ImageSearch_SAD("bobber_try2.bmp");
		int End = GetTimeTickI();
		printf("result of  SAD search benchmarking : %d ms duration, %f FPS\n", End - Start, 1000.0 / (float)(End - Start + 1));

		Start = GetTimeTickI();
		for( int i = 0; i < 1; i++ )
		{
			TakeScreenshot(0, 0, 1920, 1080);
			res = IsAnythingChanced(0, 0, 7, 23);
			if (res[0] != '0')
			{
//				res[0] = '1';
			}
		}
		End = GetTimeTickI();
		printf("result of changedetect benchmarking : %d ms duration, %f FPS\n", End - Start, 1000.0 / (float)( End - Start + 1 ) );
		_getch();
		}/**/
/*	{
		//		_getch();
		TakeScreenshot(0, 0, 1900, 960);
		char *res;
		res = ImageSearch_SAD("16x16.bmp");
		int i, Start = GetTimeTickI();
		for (i = 0; i < 10; i++)
		{
		TakeScreenshot(0, 0, 1900, 960);
		res = ImageSearch_SAD("16x16.bmp");
		if (res[0] != '0')
		{
		res[0] = '1';
		}
		}
		int End = GetTimeTickI();
		printf("result of search benchmarking : %d ms %d FPS\n", End - Start, i * 1000 * 1000 / (End - Start + 1));
		//		_getch();
		}/**/
/*	{
		//		_getch();
		TakeScreenshot(0, 0, 1900, 960);
		char *res;
		res = ImageSearch_Multiple_ExactMatch("16x16.bmp");
		int i, Start = GetTimeTickI();
		for (i = 0; i < 10; i++)
		{
		TakeScreenshot(0, 0, 1900, 960);
		res = ImageSearch_Multiple_ExactMatch("16x16.bmp");
		if (res[0] != '0')
		{
		res[0] = '1';
		}
		}
		int End = GetTimeTickI();
		printf("result of search benchmarking : %d ms %d FPS\n", End - Start, i * 1000 * 1000 / (End - Start + 1));
		//		_getch();
		}/**/
#endif
	return 0;
}