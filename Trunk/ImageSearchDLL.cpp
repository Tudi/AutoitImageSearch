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

#if defined( _CONSOLE ) && !defined( _DEBUG )
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
	RunImgSearchGradientBenchmark();
//	_getch();
#endif

#ifdef _DEBUG
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
/*	{
		char *res;
		//OCR_LoadFontsFromFile("K_C_M_FontMap.txt");
		OCR_LoadFontsFromDir("K_C_M", "KCM_");
		TakeScreenshot(0, 0, 1025, 599);
		OCR_SetMaxFontSize(20, 20);
		//		LoadCacheOverScreenshot("Screenshot_0021_1025_0599.bmp", 0, 0);
		//		LoadCacheOverScreenshot("Screenshot_0024_1025_0599.bmp", 0, 0);
		//		LoadCacheOverScreenshot("Screenshot_0042_1025_0599.bmp", 0, 0);
		//		LoadCacheOverScreenshot("Screenshot_0045_1025_0599.bmp", 0, 0);
		//		LoadCacheOverScreenshot("Screenshot_0048_1025_0599.bmp", 0, 0);
		LoadCacheOverScreenshot("Screenshot_0051_1025_0599.bmp", 0, 0);
		//SaveScreenshot();
		//		KeepColorsMinInRegion(446, 182, 680, 205, 144, 133, 41);
		KeepColorsMinInRegion(446, 182, 680, 205, 171, 160, 49);
		//SaveScreenshot();
		//KeepColorsMinInRegion(405, 223, 680, 482, 154, 158, 159);
		KeepColorsMinInRegion(502, 223, 680, 240, 154, 158, 159);
		KeepColorsMinInRegion(554, 249, 680, 265, 154, 158, 159);
		KeepColorsMinInRegion(405, 276, 680, 295, 154, 158, 159);
		KeepColorsMinInRegion(502, 469, 529, 482, 154, 158, 159);
		KeepColorsMinInRegion(543, 469, 570, 482, 154, 158, 159);
		//SaveScreenshot();
		//ConvertToGrayScale();
		//DecreaseColorCount(1);
		//SaveScreenshot();
		//SaveScreenshot();
		//GetUniqueColorsInRegion(502, 223, 580, 237);
		//KeepColorSetRest(0x00000000, 0x00FFFFFF, 0x00000000);
		//SaveScreenshot();
		//ErrodeRegionToTransparent();
		//SaveScreenshot();
		//read name
		res = OCR_ReadTextLeftToRightSaveUnknownChars(446, 182, 680, 205);
		if (res != NULL)
		{
		printf("Name : %s\n", res);
		}
		//read might
		res = OCR_ReadTextLeftToRightSaveUnknownChars(502, 223, 680, 240);
		if (res != NULL)
		{
		printf("might : %s\n", res);
		RemoveCharFromNumberString(res, ',');
		printf("might : %s\n", res);
		}
		//read troops killed
		res = OCR_ReadTextLeftToRightSaveUnknownChars(554, 249, 680, 265);
		if (res != NULL)
		{
		printf("kills : %s\n", res);
		RemoveCharFromNumberString(res, ',');
		printf("kills : %s\n", res);
		}
		//read guild
		res = OCR_ReadTextLeftToRightSaveUnknownChars(405, 276, 680, 295);
		if (res != NULL)
		{
		printf("guild : %s\n", res);
		}
		//read location X
		res = OCR_ReadTextLeftToRightSaveUnknownChars(502, 469, 529, 482);
		if (res != NULL)
		{
		printf("x : %s\n", res);
		}
		//read location y
		res = OCR_ReadTextLeftToRightSaveUnknownChars(543, 469, 570, 482);
		if (res != NULL)
		{
		printf("y : %s\n", res);
		}
		}/**/
	{
		TakeScreenshot(0, 0, 1025, 599);

		LoadCacheOverScreenshot("Screenshot_0015_1025_0599.bmp", 0, 0);
//		char * ttt = ImageSearch_Multiple_Gradient(RGB(33, 106, 148), 60, 75, 181 - 151, 235 - 220);
		KeepGradient(RGB(33, 106, 148), 0.4f);
		//SaveScreenshot();
		float t = GetPixelRatioInArea(0, 151, 220, 181, 235);
		printf("t = %f\n", t);
		char * tt = ImageSearch_Multiple_PixelCount(0, 75, 181 - 151, 235 - 220);
		return;

		PushToColorKeepList(0x406060);
		PushToColorKeepList(0x406080);
		PushToColorKeepList(0x206080);
		PushToColorKeepList(0x204060);
		PushToColorKeepList(0x204040);
		PushToColorKeepList(0x204080);
		PushToColorKeepList(0x206060);
		PushToColorKeepList(0x2060A0);
		PushToColorKeepList(0x004060);
		PushToColorKeepList(0x004080);
		PushToColorKeepList(0x006060);
		PushToColorKeepList(0x006080);
		LoadCacheOverScreenshot("Screenshot_0015_1025_0599.bmp", 0, 0);
		ApplyColorBitmask(0x00E0E0E0);
		//SaveScreenshot();
		//KeepColor3SetBoth(0, 0x00FFFFFF, 0x206080, 0x4080, 0x4060);
		ApplyColorKeepList(0x00FFFFFF, 0);
		SaveScreenshot();

		LoadCacheOverScreenshot("Screenshot_0015_1025_0599.bmp", 0, 0);
		ApplyColorBitmask(0x00C0C0C0);
		//SaveScreenshot();
		KeepColor3SetBoth(0, 0x00FFFFFF, 0x00004080, 0x00004040, 0x00408080);
		//SaveScreenshot();

		LoadCacheOverScreenshot("Screenshot_0015_1025_0599.bmp", 0, 0);
		ApplyColorBitmask(0x00808080);
		//SaveScreenshot();
		KeepColor3SetBoth(0, 0x00FFFFFF, 0x00000080, 0x00008080, 0x00008080);
		//SaveScreenshot();
	}/**/
#endif
}