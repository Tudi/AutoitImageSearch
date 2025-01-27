#include "Allocator.h"
#include "stdafx.h"
#if defined(_DEBUG) && !defined(DISABLE_ALLOCATION_TRACE)
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <string>
#include <Windows.h>
#include "DbgHelp.h"
#include <WinBase.h>
#include <mutex>		
#include <stdlib.h>
#include <assert.h>
#pragma comment(lib, "Dbghelp.lib")

#define MAX_CALL_STACK_SIZE	28000

typedef struct AllocationTraceStore
{
	const char* file;
	int line;
	int size;
	char* block;
	bool isMalloc;
	ULONGLONG stamp; // "precise time"
	time_t unixstamp; // printable time
	const char* file_free;
	int line_free;
	char callStack[MAX_CALL_STACK_SIZE]; // maybe make it dynamic next time ? 
}AllocationTraceStore;

class AllocatorTraceStorage {
public:
	std::unordered_map<const void*, AllocationTraceStore*> g_AllocatedBlocks;
	// if you are tracking allocations you do not care about speed
	// this might help identify allocation hotspots and move them to memory pools
	std::unordered_map<const void*, AllocationTraceStore*> g_FreedBlocks;
	// count allocations to pinpoint hotsports
	std::unordered_map<std::string, __int64> g_HotSpots;
	std::unordered_map<ULONG, std::string> g_HashedTraces;
	// avoid multi threaded list corruption
	std::mutex g_AllocListMutex;
	int g_MagicNumberCheckInitialized = 0xFEEDFACE;
};

static AllocatorTraceStorage *g_AllocatorOnHeapData = new AllocatorTraceStorage();

void sprintf_CallStack(char* Output, int MaxSize, int skipStart = 1, int skipEnd = 0)
{
	Output[0] = 0; // make sure we do not return jibberish
	void* stack[100];
	const int maxSymbolNameLen = 256;
	int frames;
	SYMBOL_INFO* symbol;
	static HANDLE         process = 0;
	static int needSymbolInit = 1;

	if (process == 0)
	{
		process = GetCurrentProcess();
	}
	if (needSymbolInit)
	{
		if (!SymInitialize(process, NULL, TRUE))
		{
			return;
		}
		needSymbolInit = 0;
	}
	
	ULONG BackTraceHash;
	frames = CaptureStackBackTrace(skipStart, _countof(stack), stack, &BackTraceHash);
	auto itr = g_AllocatorOnHeapData->g_HashedTraces.find(BackTraceHash);
	if (itr != g_AllocatorOnHeapData->g_HashedTraces.end())
	{
		sprintf_s(Output, MaxSize, "%s", itr->second.c_str());
		return;
	}

	symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + maxSymbolNameLen * sizeof(char), 1);
	if (symbol == NULL)
	{
		return;
	}
	symbol->MaxNameLen = maxSymbolNameLen;
	symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

	int bytesWritten = 0;
	frames -= skipEnd;
	for (int i = 0; i < frames; i++)
	{
		if (!SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol))
		{
			continue;
		}

		IMAGEHLP_LINE64 lineInfo;
		DWORD dDisplacement;
		if (!SymGetLineFromAddr64(process, (DWORD64)stack[i], &dDisplacement, &lineInfo))
		{
			lineInfo.LineNumber = 0;
		}

		int bytesWrittenNow = sprintf_s(&Output[bytesWritten], MaxSize - bytesWritten, "\t%d: %s:%d\n", frames - i - 1, symbol->Name, lineInfo.LineNumber);
		if (bytesWrittenNow == 0)
		{
			break;
		}
		bytesWritten += bytesWrittenNow;
	}

	g_AllocatorOnHeapData->g_HashedTraces[BackTraceHash] = Output;

	free(symbol);
}

void TraceAlloc(const char* file, int line, const void *p, int size, bool isMalloc)
{
	if (p == NULL || size == 0)
	{
		return;
	}
	// !!! conflict of static variable initialization !! Try to not use so many static variables !
	if (g_AllocatorOnHeapData == NULL || g_AllocatorOnHeapData->g_MagicNumberCheckInitialized != 0xFEEDFACE)
	{
		return;
	}
	AllocationTraceStore* ns = (AllocationTraceStore*)_aligned_malloc(sizeof(AllocationTraceStore), SSE_ALIGNMENT);
	if (ns)
	{
		memset(ns, 0, sizeof(AllocationTraceStore));
		ns->file = file;
		ns->line = line;
		ns->size = size;
		ns->block = (char*)p;
		ns->stamp = GetTickCount();
		ns->unixstamp = time(NULL);
		ns->isMalloc = isMalloc;
		sprintf_CallStack(ns->callStack, (int)sizeof(ns->callStack), 2 + isMalloc, 6); // should make these magical numbers dynamic 
//		sprintf_CallStack(ns->callStack, (int)sizeof(ns->callStack), 1, 0);

		std::lock_guard<std::mutex> lock(g_AllocatorOnHeapData->g_AllocListMutex);
		auto itr = g_AllocatorOnHeapData->g_FreedBlocks.find(p);
		if (itr != g_AllocatorOnHeapData->g_FreedBlocks.end()) // because some block get reallocated
		{
			_aligned_free(itr->second);
			g_AllocatorOnHeapData->g_FreedBlocks.erase(itr);
		}

		g_AllocatorOnHeapData->g_AllocatedBlocks[p] = ns;

		g_AllocatorOnHeapData->g_HotSpots[std::string(ns->callStack)]++;
	}
}

void *TraceMalloc(const char* file, int line, int size)
{
	if (size == 0)
	{
		return NULL;
	}

	void* ret = _aligned_malloc(size + MALLOC_BOUNDS_CHECK_SIZE_BYTES, SSE_ALIGNMENT);

#if MALLOC_BOUNDS_CHECK_SIZE_BYTES > 0
	if (ret)
	{
		memset((char*)ret + size, 0, MALLOC_BOUNDS_CHECK_SIZE_BYTES);
	}
#endif

	TraceAlloc(file, line, ret, size, true);

	return ret;
}

char* TraceStrDup(const char* file, int line, const char* str)
{
	if (str == NULL)
	{
		return NULL;
	}
	size_t len = strlen(str);
	assert(len < 0xFFFF); // random value, increase it if you hit this assert

	size_t size = len + 1;
	char* ret = (char*)_aligned_malloc(size + MALLOC_BOUNDS_CHECK_SIZE_BYTES, SSE_ALIGNMENT);
	if (ret)
	{
		memcpy(ret, str, size);
	}

#if MALLOC_BOUNDS_CHECK_SIZE_BYTES > 0
	if (ret)
	{
		memset((char*)ret + size, 0, MALLOC_BOUNDS_CHECK_SIZE_BYTES);
	}
#endif

	TraceAlloc(file, line, ret, (int)(size), true);

	return ret;
}

void TraceFree(const char* file, int line, void* p, bool isMalloc)
{
	if (p == NULL)
	{
		return;
	}
	std::lock_guard<std::mutex> lock(g_AllocatorOnHeapData->g_AllocListMutex);
	auto itr1 = g_AllocatorOnHeapData->g_FreedBlocks.find(p);
	if (itr1 != g_AllocatorOnHeapData->g_FreedBlocks.end()) // because some block get reallocated
	{
		// you managed to double delete an allocated zone
		assert(false);
	}
	auto itr2 = g_AllocatorOnHeapData->g_AllocatedBlocks.find(p);
	if (itr2 != g_AllocatorOnHeapData->g_AllocatedBlocks.end()) // because some block get reallocated
	{
		AllocationTraceStore* ns = itr2->second;
		g_AllocatorOnHeapData->g_AllocatedBlocks.erase(itr2);
		ns->file_free = file;
		ns->line_free = line;
		// maybe we should check if this is a duplicate event. Not like we will have millions of locations
		g_AllocatorOnHeapData->g_FreedBlocks[p] = ns;
		if (isMalloc != ns->isMalloc)
		{
			// was allocated with new, got freed with "free" ?
			// was allocated with malloc and got freed with "delete" ?
			assert(false);
		}			
#if MALLOC_BOUNDS_CHECK_SIZE_BYTES > 0
		if (isMalloc)
		{
			__int64* boundsLocation = (__int64*)(((char*)p) + ns->size);
			for (size_t i = 0; i < MALLOC_BOUNDS_CHECK_SIZE_BYTES / 8; i++)
			{
				if (boundsLocation[i] != 0)
				{
					assert(false);
				}
			}
		}
#endif
	}
	if (isMalloc == true)
	{
		_aligned_free(p);
	}
}

#define sprintf2(buff, bytes_alloc, bytes_used, ...) { \
	if( bytes_alloc <= bytes_used + 5 * 1024 * 1024 ) { \
		char *buff2 = (char*)malloc(bytes_alloc + 2 * 5 * 1024 * 1024); \
		if(buff2) { \
			memcpy(buff2, buff, bytes_alloc); \
			bytes_alloc += 2 * 5 * 1024 * 1024; } } \
		size_t bytes_written = sprintf_s(&buff[bytes_used], bytes_alloc - bytes_used, __VA_ARGS__); \
		bytes_used += bytes_written; }

char *StrDumpAllocatedRegions()
{
	static char* SemiStaticDumpStr = (char*)malloc(2 * 5 * 1024 * 1024);
	size_t bytes_alloc = 2 * 5 * 1024 * 1024;
	size_t bytes_used = 0;
	SemiStaticDumpStr[0] = 0;

	if (!g_AllocatorOnHeapData->g_HotSpots.empty())
	{
		sprintf2(SemiStaticDumpStr, bytes_alloc, bytes_used, "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
		sprintf2(SemiStaticDumpStr, bytes_alloc, bytes_used, "Dumping top N allocation hotspots : \n");
		sprintf2(SemiStaticDumpStr, bytes_alloc, bytes_used, "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
		std::vector<std::pair<std::string, __int64>> sortedEntries(g_AllocatorOnHeapData->g_HotSpots.begin(), g_AllocatorOnHeapData->g_HotSpots.end());
		// Sort the vector based on count
		std::sort(sortedEntries.begin(), sortedEntries.end(), [](const auto& a, const auto& b) 
			{
				return a.second > b.second;
			});
		int count = 0;
		for (const auto& entry : sortedEntries)
		{
			sprintf2(SemiStaticDumpStr, bytes_alloc, bytes_used, "Count : %lld\n", entry.second);
			sprintf2(SemiStaticDumpStr, bytes_alloc, bytes_used, "%s", entry.first.c_str());
			if (++count == 10) 
			{
				break;
			}
		}
		sprintf2(SemiStaticDumpStr, bytes_alloc, bytes_used, "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
	}

	if (!g_AllocatorOnHeapData->g_AllocatedBlocks.empty())
	{
		sprintf2(SemiStaticDumpStr, bytes_alloc, bytes_used, "|||||||||||||||||||||||||||||||||||||||||\n");
		sprintf2(SemiStaticDumpStr, bytes_alloc, bytes_used, "Dumping remaining allocated blocks : \n");
		sprintf2(SemiStaticDumpStr, bytes_alloc, bytes_used, "|||||||||||||||||||||||||||||||||||||||||\n");
		for (auto itr : g_AllocatorOnHeapData->g_AllocatedBlocks)
		{
			sprintf2(SemiStaticDumpStr, bytes_alloc, bytes_used, "\t%s:%d: Size %d\n", itr.second->file, itr.second->line, itr.second->size);
			sprintf2(SemiStaticDumpStr, bytes_alloc, bytes_used, "%s", itr.second->callStack);
		}
		sprintf2(SemiStaticDumpStr, bytes_alloc, bytes_used, "|||||||||||||||||||||||||||||||||||||||||\n");
	}
	return SemiStaticDumpStr;
}

void AllocatorFreeInternalMem()
{
	for (auto itr : g_AllocatorOnHeapData->g_AllocatedBlocks)
	{
		_aligned_free(itr.second);
	}
	g_AllocatorOnHeapData->g_AllocatedBlocks.clear();

	for (auto itr : g_AllocatorOnHeapData->g_FreedBlocks)
	{
		_aligned_free(itr.second);
	}
	g_AllocatorOnHeapData->g_FreedBlocks.clear();

	g_AllocatorOnHeapData->g_HashedTraces.clear();
}
#endif
