#include "stdafx.h"

struct SearchedRegionRequestStore {
	int aLeft, aTop, aRight, aBottom;
	size_t LastImageId = 0;
};

#define MAX_SEARCH_HISTORY_SIZE 10
#define MAX_SEARCH_FRAMES_CONSIDERED 2 // if history value is older than this .. ignore it

static SearchedRegionRequestStore g_PastRequests[MAX_SEARCH_HISTORY_SIZE];
static size_t g_StoreNextAtIndex = 0; // searches made for this specific ImageId

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
	g_PastRequests[g_StoreNextAtIndex].aLeft = aLeft;
	g_PastRequests[g_StoreNextAtIndex].aTop = aTop;
	g_PastRequests[g_StoreNextAtIndex].aRight = aRight;
	g_PastRequests[g_StoreNextAtIndex].aBottom = aBottom;
	g_PastRequests[g_StoreNextAtIndex].LastImageId = ImageId;
	g_StoreNextAtIndex = (g_StoreNextAtIndex + 1) % MAX_SEARCH_HISTORY_SIZE;
}

void GetAdvisedNewCaptureSize(int& aLeft, int& aTop, int& aRight, int& aBottom)
{
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
}
