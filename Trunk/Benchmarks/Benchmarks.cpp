#include "../StdAfx.h"

void RunSQRTBenchmark()
{
	int Start,End,AntiOptimizer;
	int LoopCount = 5000000;
	__int64 AntiOptimizer2;

	Start = GetTickCount();
	AntiOptimizer = 0;
	for( int i = 0; i < LoopCount; i++ )
		AntiOptimizer += sqrt1( i * i );
	End = GetTickCount();
	printf( "Ignoreme : %d \n", AntiOptimizer );
	printf("benchmarking sqrt1 : %d \n", End - Start );

	Start = GetTickCount();
	AntiOptimizer = 0;
	for( int i = 0; i < LoopCount; i++ )
		AntiOptimizer += (int)sqrt( (float)i * (float)i );
	End = GetTickCount();
	printf( "Ignoreme : %d \n", AntiOptimizer );
	printf("benchmarking sqrt(float) : %d \n", End - Start );

	Start = GetTickCount();
	AntiOptimizer = 0;
	for( int i = 0; i < LoopCount; i++ )
		AntiOptimizer += (int)sqrt( (double)i * (double)i );
	End = GetTickCount();
	printf( "Ignoreme : %d \n", AntiOptimizer );
	printf("benchmarking sqrt(double) : %d \n", End - Start );

	Start = GetTickCount();
	AntiOptimizer2 = 0;
	for( __int64 i = 0; i < LoopCount; i++ )
		AntiOptimizer2 += sqrt1( i * i );
	End = GetTickCount();
	printf( "Ignoreme : %lld \n", AntiOptimizer );
	printf("benchmarking sqrt1 : %d \n", End - Start );

	Start = GetTickCount();
	AntiOptimizer2 = 0;
	for( __int64 i = 0; i < LoopCount; i++ )
		AntiOptimizer2 += (__int64)sqrt( (double)i * (double)i );
	End = GetTickCount();
	printf( "Ignoreme : %lld \n", AntiOptimizer );
	printf("benchmarking sqrt(double) : %d \n", End - Start );
}

void RunSCreenCaptureBenchmark()
{
	int Start,End,AntiOptimizer;
	int LoopCount = 1000;
	int StartX = 0;
	int StartY = 0;
	int EndX = 800;
	int EndY = 800;

	Start = GetTickCount();
	AntiOptimizer = 0;
	for( int i = 0; i < LoopCount; i++ )
	{
		AntiOptimizer += i;
		TakeScreenshot( StartX, StartY, EndX, EndY );
	}
	End = GetTickCount();
	printf( "Ignoreme : %d \n", AntiOptimizer );
	printf(" Benchmarking screencapture : %d frames processed in %d ms. Number of pixels stored %d\n", LoopCount, ( End - Start ), ( EndX - StartX ) * ( EndY - StartY ) );
	printf(" Benchmarking screencapture : %d FPS \n", LoopCount * 1000 / ( End - Start ) );
	printf(" Pixels Processed Per Second: %d pps \n", ( LoopCount * ( EndX - StartX ) * ( EndY - StartY ) ) / ( End - Start ) );
}

void RunCheckChangeBenchmark()
{
	int Start,End,AntiOptimizer;
	int LoopCount = 1000;
	int StartX = 0;
	int StartY = 0;
	int EndX = 800;
	int EndY = 800;

	//fill up cache
	TakeScreenshot( StartX, StartY, EndX, EndY );
	TakeScreenshot( StartX, StartY, EndX, EndY );

	Start = GetTickCount();
	AntiOptimizer = 0;
	for( int i = 0; i < LoopCount; i++ )
	{
		if( IsAnythingChanced( StartX, StartY, EndX, EndY ) )
			AntiOptimizer += i;
	}
	End = GetTickCount();
	printf( "Ignoreme : %d \n", AntiOptimizer );
	printf(" Benchmarking IsAnythingChanced : %d frames processed in %d ms. Number of pixels stored %d\n", LoopCount, ( End - Start ), ( EndX - StartX ) * ( EndY - StartY ) );
	printf(" Benchmarking IsAnythingChanced : %d FPS \n", LoopCount * 1000 / ( End - Start ) );
	printf(" Pixels Processed Per Second: %d pps \n", ( LoopCount * ( EndX - StartX ) * ( EndY - StartY ) ) / ( End - Start ) );
}

void RunResizeBenchmark()
{
	int Start,End,AntiOptimizer;
	int LoopCount = 1000;
	int StartX = 0;
	int StartY = 0;
	int EndX = 800;
	int EndY = 800;

	Start = GetTickCount();
	AntiOptimizer = 0;
	for( int i = 0; i < LoopCount; i++ )
	{
		TakeScreenshot( StartX, StartY, EndX, EndY );
		ResizeScreenshot( 320, 200 );
		AntiOptimizer += i;
	}
	End = GetTickCount();
	printf( "Ignoreme : %d \n", AntiOptimizer );
	printf(" Benchmarking TakeScreenshot + ResizeScreenshot : %d frames processed in %d ms. Number of pixels stored %d\n", LoopCount, ( End - Start ), ( EndX - StartX ) * ( EndY - StartY ) );
	printf(" Benchmarking TakeScreenshot + ResizeScreenshot : %d FPS \n", LoopCount * 1000 / ( End - Start ) );
	printf(" Pixels Processed Per Second: %d pps \n", ( LoopCount * ( EndX - StartX ) * ( EndY - StartY ) ) / ( End - Start ) );
}

void RunBlurrBenchmark()
{
	int Start,End,AntiOptimizer;
	int LoopCount = 1000;
	int StartX = 0;
	int StartY = 0;
	int EndX = 800;
	int EndY = 800;

	TakeScreenshot( StartX, StartY, EndX, EndY );

	Start = GetTickCount();
	AntiOptimizer = 0;
	for( int i = 0; i < LoopCount; i++ )
	{
		BlurrImage( 1 );
		AntiOptimizer += i;
	}
	End = GetTickCount();
	printf( "Ignoreme : %d \n", AntiOptimizer );
	printf(" Benchmarking BlurrImage : %d frames processed in %d ms. Number of pixels stored %d\n", LoopCount, ( End - Start ), ( EndX - StartX ) * ( EndY - StartY ) );
	printf(" Benchmarking BlurrImage : %d FPS \n", LoopCount * 1000 / ( End - Start ) );
	printf(" Pixels Processed Per Second: %d pps \n", ( LoopCount * ( EndX - StartX ) * ( EndY - StartY ) ) / ( End - Start ) );
}

void RunDiffBenchmark()
{
	int Start,End,AntiOptimizer;
	int LoopCount = 1000;
	int StartX = 0;
	int StartY = 0;
	int EndX = 800;
	int EndY = 800;

	TakeScreenshot( StartX, StartY, EndX, EndY );
	TakeScreenshot( StartX, StartY, EndX, EndY );

	Start = GetTickCount();
	AntiOptimizer = 0;
	for( int i = 0; i < LoopCount; i++ )
	{
		GenerateDiffMap( );
		AntiOptimizer += i;
	}
	End = GetTickCount();
	printf( "Ignoreme : %d \n", AntiOptimizer );
	printf(" Benchmarking TakeScreenshot + GenerateDiffMap : %d frames processed in %d ms. Number of pixels stored %d\n", LoopCount, ( End - Start ), ( EndX - StartX ) * ( EndY - StartY ) );
	printf(" Benchmarking TakeScreenshot + GenerateDiffMap : %d FPS \n", LoopCount * 1000 / ( End - Start ) );
	printf(" Pixels Processed Per Second: %d pps \n", ( LoopCount * ( EndX - StartX ) * ( EndY - StartY ) ) / ( End - Start ) );
}

void RunImgSearchBenchmark()
{
	int Start,End,AntiOptimizer;
	int LoopCount = 500;
	int StartX = 0;
	int StartY = 0;
	int EndX = 800;
	int EndY = 800;
	int SearchWidth = 50;
	int SearchHeight = 50;

//	TakeScreenshot( EndX - SearchWidth - 1, EndY - SearchHeight - 1, EndX - 1 , EndY - 1 );	//for me this is black box on black screen search ... worst case
	TakeScreenshot( 0, 0, SearchWidth , SearchHeight ); // for me this will become a very good speed search as it has low chance to match in other places
	MoveScreenshotToCache( "ToSearch" );
	TakeScreenshot( StartX, StartY, EndX, EndY );

	Start = GetTickCount();
	AntiOptimizer = 0;
	for( int i = 0; i < LoopCount; i++ )
	{
//		char *res = ImageSearchOnScreenshot( "ToSearch", 0x0F000000, 20, SearchWidth * SearchHeight * 2 / 100, EndY * EndX );	//bad case scenario
		char *res = ImageSearchOnScreenshot( "ToSearch", 0x0F000000, 1, 1, EndY * EndX );	//good case scenario
		if( res[0] == '1' )
			AntiOptimizer += i;
	}
	End = GetTickCount();
	printf( "Ignoreme : %d \n", AntiOptimizer );
	printf(" Benchmarking ImageSearchOnScreenshot : %d frames processed in %d ms. Number of pixels stored %d\n", LoopCount, ( End - Start ), ( EndX - StartX ) * ( EndY - StartY ) );
	printf(" Benchmarking ImageSearchOnScreenshot : %d FPS \n", LoopCount * 1000 / ( End - Start ) );
	printf(" Pixels Processed Per Second: %d pps \n", ( LoopCount * ( EndX - StartX ) * ( EndY - StartY ) ) / ( End - Start ) );
}

void RunErrodeBenchmark()
{
	int Start,End,AntiOptimizer;
	int LoopCount = 1000;
	int StartX = 0;
	int StartY = 0;
	int EndX = 800;
	int EndY = 800;

	TakeScreenshot( StartX, StartY, EndX, EndY );
	TakeScreenshot( StartX, StartY, EndX, EndY );
	GenerateDiffMap( );

	Start = GetTickCount();
	AntiOptimizer = 0;
	for( int i = 0; i < LoopCount; i++ )
	{
		ErrodeDiffMap( 1 );
		AntiOptimizer += i;
	}
	End = GetTickCount();
	printf( "Ignoreme : %d \n", AntiOptimizer );
	printf(" Benchmarking ErrodeDiffMap : %d frames processed in %d ms. Number of pixels stored %d\n", LoopCount, ( End - Start ), ( EndX - StartX ) * ( EndY - StartY ) );
	printf(" Benchmarking ErrodeDiffMap : %d FPS \n", LoopCount * 1000 / ( End - Start ) );
	printf(" Pixels Processed Per Second: %d pps \n", ( LoopCount * ( EndX - StartX ) * ( EndY - StartY ) ) / ( End - Start ) );
}

void RunSimilarSearchBenchmark()
{
	int Start,End,AntiOptimizer;
	int LoopCount = 1000;
	int StartX = 0;
	int StartY = 0;
	int EndX = 800;
	int EndY = 800;
	int SearchWidth = 50;
	int SearchHeight = 50;

	TakeScreenshot( EndX - SearchWidth - 1, EndY - SearchHeight - 1, EndX - 1 , EndY - 1 );	//for me this is black box on black screen search ... worst case
	MoveScreenshotToCache( "ToSearch" );
	TakeScreenshot( StartX, StartY, EndX, EndY );
	if( CurScreenshot->SSCache == NULL )
		CurScreenshot->SSCache = new SimilarSearch;

	Start = GetTickCount();
	AntiOptimizer = 0;
	for( int i = 0; i < LoopCount; i++ )
	{
		CurScreenshot->SSCache->BuildFromImg( CurScreenshot->Pixels, CurScreenshot->Right - CurScreenshot->Left, CurScreenshot->Bottom - CurScreenshot->Top, CurScreenshot->Right - CurScreenshot->Left );
		AntiOptimizer += i;
	}
	End = GetTickCount();
	printf( "Ignoreme : %d \n", AntiOptimizer );
	printf(" Benchmarking SetupSimilarSearch initialize : %d frames processed in %d ms. Number of pixels stored %d\n", LoopCount, ( End - Start ), ( EndX - StartX ) * ( EndY - StartY ) );
	printf(" Benchmarking SetupSimilarSearch initialize : %d FPS \n", LoopCount * 1000 / ( End - Start ) );
	printf(" Pixels Processed Per Second: %d pps \n", ( LoopCount * ( EndX - StartX ) * ( EndY - StartY ) ) / ( End - Start ) );

	Start = GetTickCount();
	AntiOptimizer = 0;
	for( int i = 0; i < LoopCount; i++ )
	{
		char *res = SearchSimilarOnScreenshot( "ToSearch" );
		if( res[0] == '1' )
			AntiOptimizer += i;
	}
	End = GetTickCount();
	printf( "Ignoreme : %d \n", AntiOptimizer );
	printf(" Benchmarking SetupSimilarSearch : %d frames processed in %d ms. Number of pixels stored %d\n", LoopCount, ( End - Start ), ( EndX - StartX ) * ( EndY - StartY ) );
	printf(" Benchmarking SetupSimilarSearch : %d FPS \n", LoopCount * 1000 / ( End - Start ) );
	printf(" Pixels Processed Per Second: %d pps \n", ( LoopCount * ( EndX - StartX ) * ( EndY - StartY ) ) / ( End - Start ) );
}
