#include "StdAfx.h"

int PrevTickCount = 0;
int StartTickCount = 0;
void FileDebug( char *what )
{
#ifdef _DEBUG
	if( StartTickCount == 0 )
	{
		PrevTickCount = StartTickCount = GetTickCount();
	}
	int TickNow = GetTickCount();
	int Diff = TickNow - PrevTickCount;
	PrevTickCount = TickNow;
	FILE *f = fopen( "debug.txt", "at" );
	fprintf( f, "%d-%d)%s\n", TickNow - StartTickCount, Diff, what );
	fclose( f );
#endif
}