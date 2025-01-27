#pragma once

// Wrapper over malloc to be able to track :
// - allocation trace
// - add fencing while debug
// - generate allocation statistics
// - use specific allignment
// - use precompiler to be able to dump __FILE__, __LINE__ to logs

#define MALLOC_BOUNDS_CHECK_SIZE_BYTES	1*8 // make it multiple of 8

#define SSE_ALIGNMENT			64
#define SSE_BYTE_COUNT			64
#define SSE_PADDING				64	//12 bytes would be enough

// This file does not intend to handle memory pools. Only simple block allocations

//#define DISABLE_ALLOCATION_TRACE
#if !defined(_DEBUG) || defined(DISABLE_ALLOCATION_TRACE)
	#define InternalMalloc(size) malloc(size)
	#define InternalFree(mypointer) { free(mypointer); mypointer = NULL; }
	#define InternalStrDup(str) _strdup(str)
	#define InternalNew(store, type, ...) { store = new type(__VA_ARGS__); }
	#define InternalDelete(store) { delete store; store = NULL; }
	#define InternalNewA(store, type, count) { store = new type[count]; }
	#define InternalDeleteA(store) { delete[] store; store = NULL; }
	#define MY_ALLOC(x)			_aligned_malloc((x)+SSE_PADDING,SSE_ALIGNMENT)
	#define MY_FREE(x)			if(x){_aligned_free(x); x = NULL;}
	#define StrDumpAllocatedRegions() NULL
#else
	//#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
	//#define FILENAME_ONLY (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
	void* TraceMalloc(const char* file, int line, int size);
	void TraceAlloc(const char* file, int line, const void* p, int size, bool isMalloc);
	char* TraceStrDup(const char* file, int line, const char *str);
	void TraceFree(const char* file, int line, void *p, bool isMalloc);
	char* StrDumpAllocatedRegions();
	void AllocatorFreeInternalMem();
	#define MY_ALLOC(size)			TraceMalloc(__FILE__,__LINE__,(int)(size + SSE_PADDING));
	#define MY_FREE(mypointer)			{ TraceFree(__FILE__,__LINE__,mypointer, true); mypointer = NULL; }
	#define InternalMalloc(size) TraceMalloc(__FILE__,__LINE__,(int)(size));
	#define InternalFree(mypointer) { TraceFree(__FILE__,__LINE__,mypointer, true); mypointer = NULL; }
	#define InternalStrDup(str) TraceStrDup(__FILE__,__LINE__,str);
	#define InternalNew(store, type, ...) { store = new type(__VA_ARGS__); TraceAlloc(__FILE__,__LINE__, store, (int)sizeof(type), false); }
	#define InternalDelete(store) { TraceFree(__FILE__,__LINE__, store, false); delete store; store = NULL; }
	#define InternalNewA(store, type, count) { store = new type[count]; TraceAlloc(__FILE__,__LINE__, store, (int)(sizeof(type) * count), false); }
	#define InternalDeleteA(store) { TraceFree(__FILE__,__LINE__, store, false); delete[] store; store = NULL; }
	// try to stack alloc or else heap. Note that stack is compile time reserved. Can't be dynamic. For deep callstacks you can't alloc a lot of times as it's limited to 2MB 
	#define StackAllocOrMalloc(var,type,size) char stackAlloc[16000]; \
		type *var; \
		if(size>=sizeof(stackAlloc)) var = (type *)InternalMalloc(size); \
		else var = (type *)stackAlloc;
	#define FreeStackAlloc(var) if(var!=stackAlloc) InternalFree(var); else var = NULL;
#endif