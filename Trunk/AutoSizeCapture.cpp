#include "stdafx.h"

struct SearchedRegionRequestStore {
	int aLeft, aTop, aRight, aBottom;
	size_t LastImageId = 0;
};

#define MAX_SEARCH_HISTORY_SIZE 20		// hmm. is this overkill ? 
#define MAX_SEARCH_FRAMES_CONSIDERED 4 // if history value is older than this .. ignore it

static SearchedRegionRequestStore g_PastRequests[MAX_SEARCH_HISTORY_SIZE];

void AddSearchedRegion(size_t ImageId, int aLeft, int aTop, int aRight, int aBottom)
{
	// do we have this exact search region ?
	// we took a new screen capture, but it's only a screen refresh, not like we actually need different region
	for (size_t i = 0; i < MAX_SEARCH_HISTORY_SIZE; i++) {
		if (g_PastRequests[i].aLeft == aLeft && g_PastRequests[i].aTop == aTop &&
			g_PastRequests[i].aRight == aRight && g_PastRequests[i].aBottom == aBottom) {
			g_PastRequests[i].LastImageId = ImageId;
			return;
		}
	}
#ifdef _DEBUG
	{
		char dbgmsg[DEFAULT_STR_BUFFER_SIZE];
		sprintf_s(dbgmsg, sizeof(dbgmsg), "AddSearchedRegion:Adding new area left=%d top=%d right=%d, bottom=%d", aLeft, aTop, aRight, aBottom);
		FileDebug(dbgmsg);
	}
#endif
	// memorize this area. Use the memorized areas to know when to flush / redo the searched area min/max
	size_t nBestIndexToStore = 0;
	int64_t SmallestFrameId = MAX_INT;
	for (size_t i = 0; i < MAX_SEARCH_HISTORY_SIZE; i++) {
		if (g_PastRequests[i].LastImageId < SmallestFrameId) {
			nBestIndexToStore = i;
			SmallestFrameId = g_PastRequests[i].LastImageId;
		}
	}
	g_PastRequests[nBestIndexToStore].aLeft = aLeft;
	g_PastRequests[nBestIndexToStore].aTop = aTop;
	g_PastRequests[nBestIndexToStore].aRight = aRight;
	g_PastRequests[nBestIndexToStore].aBottom = aBottom;
	g_PastRequests[nBestIndexToStore].LastImageId = ImageId;
}

void GetAdvisedNewCaptureSize(int& aLeft, int& aTop, int& aRight, int& aBottom)
{
	// remember last known position even if we took screenshots without performing searches
	static int prev_aLeft = -1, prev_aTop = -1, prev_aRight = -1, prev_aBottom = -1;
	aLeft = MAX_INT;
	aTop = MAX_INT;
	aRight = 0;
	aBottom = 0;
	for (size_t i = 0; i < MAX_SEARCH_HISTORY_SIZE; i++) {
		if (g_PastRequests[i].LastImageId + MAX_SEARCH_FRAMES_CONSIDERED > g_UnqiueFrameCounter) {
			if (g_PastRequests[i].aLeft < aLeft) {
				aLeft = g_PastRequests[i].aLeft;
			}
			if (g_PastRequests[i].aTop < aTop) {
				aTop = g_PastRequests[i].aTop;
			}
			if (g_PastRequests[i].aRight > aRight) {
				aRight = g_PastRequests[i].aRight;
			}
			if (g_PastRequests[i].aBottom > aBottom) {
				aBottom = g_PastRequests[i].aBottom;
			}
		}
	}
	if (aLeft == MAX_INT) {
		aLeft = prev_aLeft;
		aTop = prev_aTop;
		aRight = prev_aRight;
		aBottom = prev_aBottom;
	}
	else {
		prev_aLeft = aLeft;
		prev_aTop = aTop;
		prev_aRight = aRight;
		prev_aBottom = aBottom;

	}
}
