#ifndef _DEBUG_H_
#define _DEBUG_H_

#ifdef _DEBUG
	void FileDebug( char *what );
#else
//	#define FileDebug( what )
	void FileDebug( char *what );
#endif

#endif