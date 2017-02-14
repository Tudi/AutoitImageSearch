#include "../stdafx.h"

#define REDUCE_PIXELPRECISION_MASK 0x00F0F0F0

DWORD KoPlayerProcessId = 0;
HWND KoPlayerWND = 0;
int Ko[4];

void GetKoPlayerAndPos()
{
	if (KoPlayerProcessId == 0)
	{
		memset(Ko, 0, sizeof(Ko));
		KoPlayerProcessId = GetProcessByExeName("KOPLAYER.exe");
		if (KoPlayerProcessId == 0)
			return;	// bad bad bad
		KoPlayerWND = FindMainHWND(KoPlayerProcessId);
		if (KoPlayerWND == NULL)
			return; // bad bad bad
		RECT rect;
		if (GetWindowRect(KoPlayerWND, &rect))
		{
			Ko[0] = rect.left + 2;
			Ko[1] = rect.top + 38;
			Ko[2] = rect.right - rect.left - 63;
			Ko[3] = rect.bottom - rect.top - 38 - 2 - 1;
		}
	}
}

void WaitKoPlayerGetFocus()
{
	HWND CurWND = GetForegroundWindow();
	while (KoPlayerWND != CurWND)
	{
		Sleep(1000);
		CurWND = GetForegroundWindow();
	}
}

DWORD GetKoPixel(int x, int y)
{
	HDC hDC = GetDC(KoPlayerWND);  //I'm not sure if this is right or what exactly it does.
	DWORD ret = GetPixel(hDC, Ko[0] + x, Ko[1] + y);
	ReleaseDC(KoPlayerWND, hDC);
	ret = ret & REDUCE_PIXELPRECISION_MASK;
	return ret;
}

void WaitPixelBecomeColor(int x, int y, int Color)
{
	int tSleep = 100;
	int Timeout = 3000;
	Color = Color & REDUCE_PIXELPRECISION_MASK;
	while (Timeout > 0 && GetKoPixel(x, y) != Color)
	{
		Sleep(tSleep);
		Timeout -= tSleep;
	}
}

int WaitPixelChangeColor(int x, int y, int Color)
{
	int ret = 0;
	int tSleep = 100;
	int Timeout = 3000;
	Color = Color & REDUCE_PIXELPRECISION_MASK;
	while (Timeout > 0 && GetKoPixel(x, y) == Color)
	{
		Sleep(tSleep);
		Timeout -= tSleep;
		ret = 1;
	}
	return ret;
}

int IsPixelAtPos(int x, int y, int Color)
{
	int Pixel1 = GetKoPixel(x, y);
	if (Pixel1 == (Color & REDUCE_PIXELPRECISION_MASK))
		return 1;
	return 0;
}

void ParseCastlePopup()
{
	WaitPixelChangeColor(566, 270, 0x00FFFFFF);
	WaitPixelChangeColor(569, 301, 0x00FFFFFF);
	int PopupStartX = 440;
	int PopupStartY = 200;
	int PopupEndX = 840;
	int PopupEndY = 580;
	TakeScreenshot(Ko[0] + PopupStartX, Ko[1] + PopupStartY, Ko[0] + PopupEndX, Ko[1] + PopupEndY);
	SaveScreenshot();
}

void CloseGenericPopup(int x, int y, int color)
{
	if (IsPixelAtPos(x, y, color))
	{
		LeftClick(x, y);
		//wait close window go away
		WaitPixelChangeColor(x, y, color);
		Sleep(200);	//extra wait to make sure window will not interpret it as a delayed event and still process it
	}
}

void CloseRSSOrCastlePopup()
{
	CloseGenericPopup(853, 127, 0x00FFBD36);
}

void CloseAllPossiblePopups()
{
	//resource or castle popups still visible
	CloseRSSOrCastlePopup();
	CloseGenericPopup(1255, 43, 0x00FFBE38); // full screen popups
	CloseGenericPopup(853, 118, 0x00FFBE39); // if we clicked on rally / battle hall
	CloseGenericPopup(852, 119, 0x00FFBD37); // if we clicked on scout
	CloseGenericPopup(853, 126, 0x00FFBD36); // if we clicked on land
	CloseGenericPopup(819, 431, 0x00FFBA31); // if we clicked on army
}

void CaptureVisibleScreenGetPlayerLabels()
{
	// this will probably only run once to get the process related details
	GetKoPlayerAndPos();

	// wait for it to get some focus 
	WaitKoPlayerGetFocus();

	// depends on the window resolution. As the resolution increases this will increase also
	int JumpToTurefIconSize = 80;
	TakeScreenshot(Ko[0]+JumpToTurefIconSize, Ko[1]+JumpToTurefIconSize, Ko[0] + Ko[2] - JumpToTurefIconSize, Ko[1] + Ko[3] - JumpToTurefIconSize);
	SetGradientToColor(0xA59B63, 0.162f, 0x00FFFFFF);
	KeepGradient(0x00946D21, 0.4f);
//SaveScreenshot();
	ImageSearch_Multipass_PixelCount2(0, 60, 35, 5, 34, 21);
	//try to debug WTF situations
	if (SearchResultCount > 100)
	{
		SaveScreenshot();
		printf("Something is wrong. It's impossible to get more than 100 nodes per screen. We got %d\n", SearchResultCount);
		SearchResultCount = 0;
	}
	//parse each node label
	for (int i = 0; i < SearchResultCount; i++)
	{
		//try to make sure we do not have any random popups at this stage of the parsing.

		// click on the position of the level tag
		int x = SearchResultXYSAD[i][0];
		int y = SearchResultXYSAD[i][1];
		LeftClick(x, y);

		// wait to see if a popup opens
		WaitPixelBecomeColor(852, 126, 0x00FFBD36);

		// check what type of popup opened
		if (IsPixelAtPos(817, 297, 0x00EFC471) == 1)	// the golden i
		{
			printf("Found resource node at %d %d\n", x, y);
			//close it
			CloseRSSOrCastlePopup();
		}
		else if (IsPixelAtPos(518, 215, 0x00F3D51E) == 1) // the VIP star
		{
			printf("Found castle at %d %d\n", x, y);
			// parse if castle
			ParseCastlePopup();
			//close it
			CloseRSSOrCastlePopup();
		}
		else
		{
			printf("We clicked on something but have no idea what\n");
		}
		break;
	}
}

void RunLordsMobileTests()
{
	//GetKoPlayerAndPos();
	CaptureVisibleScreenGetPlayerLabels();
}