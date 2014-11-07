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
