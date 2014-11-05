#include "StdAfx.h"

void FileDebug( char *what )
{
#ifdef _DEBUG
	FILE *f = fopen( "debug.txt", "at" );
	fprintf( f, "%s\n", what );
	fclose( f );
#endif
}