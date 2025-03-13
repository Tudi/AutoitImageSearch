#ifndef _DEBUG_H_
#define _DEBUG_H_

#ifdef _DEBUG
	void FileDebug( const char *what );
	void WINAPI DumpAllocationsToLogger();
#else
	//	#define FileDebug( what )
	inline void FileDebug(const char* what) { (void)what; }
	inline void WINAPI DumpAllocationsToLogger() {}
#endif

#endif