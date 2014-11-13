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
	int LoopCount = 500;
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
#ifdef _DEBUG
	int LoopCount = 1;
#else
	int LoopCount = 1000;
#endif
	int StartX = 0;
	int StartY = 0;
	int EndX = 800;
	int EndY = 800;
	int SearchWidth = 50;
	int SearchHeight = 50;
	int ExpectedResultX = EndX - 10 - SearchWidth + SearchWidth / 2;
	int ExpectedResultY = EndY - 10 - SearchHeight + SearchHeight / 2;

	printf("Make that huge console window smaller or else search will exit very quickly making benchmark irelevant...Press any key to start\n");
	_getch();

	TakeScreenshot( EndX - 10 - SearchWidth, EndY - 10 - SearchHeight, EndX - 10 , EndY - 10 );	//for me this is black box on black screen search ... worst case
	MoveScreenshotToCache( "ToSearch" );
	TakeScreenshot( StartX, StartY, EndX, EndY );
	if( CurScreenshot->SSCache == NULL )
		CurScreenshot->SSCache = new SimilarSearch;

	Start = GetTickCount();
	AntiOptimizer = 0;
	for( int i = 0; i < LoopCount; i++ )
	{
		CurScreenshot->NeedsSSCache = true;
		CurScreenshot->SSCache->BuildFromImg( CurScreenshot->Pixels, CurScreenshot->Right - CurScreenshot->Left, CurScreenshot->Bottom - CurScreenshot->Top, CurScreenshot->Right - CurScreenshot->Left );
		AntiOptimizer += i;
	}
	End = GetTickCount();
	printf( "Ignoreme : %d \n", AntiOptimizer );
	printf(" Benchmarking SetupSimilarSearch initialize : %d frames processed in %d ms. Number of pixels stored %d\n", LoopCount, ( End - Start ), ( EndX - StartX ) * ( EndY - StartY ) );
	printf(" Benchmarking SetupSimilarSearch initialize : %d FPS \n", LoopCount * 1000 / ( End - Start + 1 ) );
	printf(" Pixels Processed Per Second: %d pps \n", ( LoopCount * ( EndX - StartX ) * ( EndY - StartY ) ) / ( End - Start + 1 ) );

	Start = GetTickCount();
	AntiOptimizer = 0;
	char *res;
	for( int i = 0; i < LoopCount; i++ )
	{
		res = SearchSimilarOnScreenshot( "ToSearch" );
		if( res[0] == '1' )
			AntiOptimizer += i;
	}
	End = GetTickCount();
	printf( "Ignoreme : %d \n", AntiOptimizer );
	printf(" Benchmarking SetupSimilarSearch : %d frames processed in %d ms. Number of pixels stored %d\n", LoopCount, ( End - Start ), ( EndX - StartX ) * ( EndY - StartY ) );
	printf(" Benchmarking SetupSimilarSearch : %d FPS \n", LoopCount * 1000 / ( End - Start + 1 ) );
	printf(" Pixels Processed Per Second: %d pps \n", ( LoopCount * ( EndX - StartX ) * ( EndY - StartY ) ) / ( End - Start + 1) );
	printf("Lasts reported search result was %s was expecting %d %d \n", res, ExpectedResultX, ExpectedResultY );
}

void RunPiramidSearchBenchmark()
{
	int Start,End,AntiOptimizer;
#ifdef _DEBUG
	int LoopCount = 1;
#else
	int LoopCount = 1000;
#endif
	int StartX = 0;
	int StartY = 0;
	int EndX = 800;
	int EndY = 800;
	int SearchWidth = 50;
	int SearchHeight = 50;

	printf("Make that huge console window smaller or else search will exit very quickly making benchmark irelevant...Press any key to start\n");
	_getch();

	TakeScreenshot( EndX / 2 - 1 - SearchWidth, EndY / 2 - 1 - SearchHeight, EndX / 2 - 1 , EndY / 2 - 1 );	//for me this is black box on black screen search ... worst case
//	TakeScreenshot( 0, 0, SearchWidth , SearchHeight );	//for me this is black box on black screen search ... worst case
//	TakeScreenshot( 1, 1, SearchWidth + 1 , SearchHeight + 1 );	//for me this is black box on black screen search ... worst case
//	SaveScreenshot();
	MoveScreenshotToCache( "ToSearch" );
	TakeScreenshot( StartX, StartY, EndX, EndY );
	if( CurScreenshot->PSCache == NULL )
		CurScreenshot->PSCache = new PiramidImage;

	Start = GetTickCount();
	AntiOptimizer = 0;
	for( int i = 0; i < LoopCount; i++ )
	{
		CurScreenshot->NeedsPSCache = true;
		CurScreenshot->PSCache->BuildFromImg( CurScreenshot->Pixels, CurScreenshot->Right - CurScreenshot->Left, CurScreenshot->Bottom - CurScreenshot->Top, CurScreenshot->Right - CurScreenshot->Left );
		AntiOptimizer += i;
	}
	End = GetTickCount();
	printf( "Ignoreme : %d \n", AntiOptimizer );
	printf(" Benchmarking SetupPiramidSearch initialize : %d frames processed in %d ms. Number of pixels stored %d\n", LoopCount, ( End - Start ), ( EndX - StartX ) * ( EndY - StartY ) );
	printf(" Benchmarking SetupPiramidSearch initialize : %d FPS \n", LoopCount * 1000 / ( End - Start + 1 ) );
	printf(" Pixels Processed Per Second: %d pps \n", ( LoopCount * ( EndX - StartX ) * ( EndY - StartY ) ) / ( End - Start + 1 ) );

	Start = GetTickCount();
	AntiOptimizer = 0;
	char *res;
	for( int i = 0; i < LoopCount; i++ )
	{
		res = SearchPiramidOnScreenshot( "ToSearch" );
		if( res[0] == '1' )
			AntiOptimizer += i;
	}
	End = GetTickCount();
	printf( "Ignoreme : %d \n", AntiOptimizer );
	printf(" Benchmarking SearchPiramidOnScreenshot : %d frames processed in %d ms. Number of pixels stored %d\n", LoopCount, ( End - Start ), ( EndX - StartX ) * ( EndY - StartY ) );
	printf(" Benchmarking SearchPiramidOnScreenshot : %d FPS \n", LoopCount * 1000 / ( End - Start + 1 ) );
	printf(" Pixels Processed Per Second: %d pps \n", ( LoopCount * ( EndX - StartX ) * ( EndY - StartY ) ) / ( End - Start + 1 ) );
	printf("Lasts reported search result was %s\n",res);
	/**/
}

void RunEdgeDetectBenchmark()
{
	int Start,End,AntiOptimizer;
#ifdef _DEBUG
	int LoopCount = 1;
#else
	int LoopCount = 1000;
#endif
	int StartX = 0;
	int StartY = 0;
	int EndX = 800;
	int EndY = 800;

	TakeScreenshot( StartX, StartY, EndX, EndY );

	Start = GetTickCount();
	AntiOptimizer = 0;
	for( int i = 0; i < LoopCount; i++ )
	{
		EdgeDetect( 1 );
		AntiOptimizer += i;
	}
	End = GetTickCount();
	printf( "Ignoreme : %d \n", AntiOptimizer );
	printf(" Benchmarking EdgeDetect( 1 ) : %d frames processed in %d ms. Number of pixels stored %d\n", LoopCount, ( End - Start ), ( EndX - StartX ) * ( EndY - StartY ) );
	printf(" Benchmarking EdgeDetect( 1 ) : %d FPS \n", LoopCount * 1000 / ( End - Start + 1 ) );
	printf(" Pixels Processed Per Second: %d pps \n", ( LoopCount * ( EndX - StartX ) * ( EndY - StartY ) ) / ( End - Start + 1 ) );

	Start = GetTickCount();
	AntiOptimizer = 0;
	for( int i = 0; i < LoopCount; i++ )
	{
		EdgeDetect( 2 );
		AntiOptimizer += i;
	}
	End = GetTickCount();
	printf( "Ignoreme : %d \n", AntiOptimizer );
	printf(" Benchmarking EdgeDetect( 2 ) : %d frames processed in %d ms. Number of pixels stored %d\n", LoopCount, ( End - Start ), ( EndX - StartX ) * ( EndY - StartY ) );
	printf(" Benchmarking EdgeDetect( 2 ) : %d FPS \n", LoopCount * 1000 / ( End - Start + 1 ) );
	printf(" Pixels Processed Per Second: %d pps \n", ( LoopCount * ( EndX - StartX ) * ( EndY - StartY ) ) / ( End - Start + 1 ) );

	/**/
}

void RunOCRBenchmark()
{
}