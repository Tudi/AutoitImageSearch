#include "StdAfx.h"

int PrevTickCount = 0;
int StartTickCount = 0;
#ifdef _DEBUG
//#if defined( _DEBUG ) || 1
	void FileDebug( char *what )
	{
		if( StartTickCount == 0 )
		{
			PrevTickCount = StartTickCount = GetTimeTickI();
		}
		int TickNow = GetTimeTickI();
		int Diff = TickNow - PrevTickCount;
		PrevTickCount = TickNow;
		FILE *f;
		errno_t openres = fopen_s(&f, "debug.txt", "at");
		fprintf( f, "%d-%d)%s\n", TickNow - StartTickCount, Diff, what );
#ifdef _CONSOLE
		printf("%d-%d)%s\n", TickNow - StartTickCount, Diff, what);
#endif
		fclose( f );
	}
#else
	//life is a mistery. If i remove this function that "release" version crashes when used in autoit
	void FileDebug( char *what )
	{
	}
#endif
