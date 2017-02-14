#include "../stdafx.h"

#define REDUCE_PIXELPRECISION_MASK 0x00F0F0F0
#define RGB_GET_R(Color) ( Color & 0xFF )
#define RGB_GET_G(Color) ( (Color >> 8 ) & 0xFF)
#define RGB_GET_B(Color) ( (Color >> 16 )& 0xFF)
#define STATIC_BGR_RGB(Color) (( RGB_GET_R( Color ) << 16 ) | ( RGB_GET_G( Color ) << 8 ) | ( RGB_GET_B( Color ) ) )

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
	HDC hDC = GetDC(0);				//I'm not sure if this is right or what exactly it does.
	COLORREF Color = GetPixel(hDC, Ko[0] + x, Ko[1] + y);
	ReleaseDC(0, hDC);
	Color = Color & REDUCE_PIXELPRECISION_MASK;
	return Color;
/*	//swap bytes in case we got it in BGR
	int B = GetRValue(Color);
	int G = GetGValue(Color);
	int R = GetBValue(Color);
	//return the real RGB
	return RGB(R,G,B);*/
}

int IsPixelAtPos(int x, int y, int Color)
{
	int Pixel1 = GetKoPixel(x, y);
	if (Pixel1 == (Color & REDUCE_PIXELPRECISION_MASK))
		return 1;
	return 0;
}

int WaitPixelBecomeColor(int x, int y, int Color)
{
	int tSleep = 100;
	int Timeout = 3000;
	Color = Color & REDUCE_PIXELPRECISION_MASK;
	while (Timeout > 0 && IsPixelAtPos(x, y, Color) == 0 )
	{
		Sleep(tSleep);
		Timeout -= tSleep;
	}
	return IsPixelAtPos(x, y, Color);
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

void ParseCastlePopup()
{
	if (WaitPixelBecomeColor(566, 270, 0x00FFFFFF) == 0 && WaitPixelBecomeColor(569, 301, 0x00FFFFFF) == 0)
	{
		printf("Castle popup load timemout. Skipping parsing\n");
		return;
	}
	int PopupStartX = 440;
	int PopupStartY = 200;
	int PopupEndX = 840;
	int PopupEndY = 580;
	TakeScreenshot(Ko[0] + PopupStartX, Ko[1] + PopupStartY, Ko[0] + PopupEndX, Ko[1] + PopupEndY);
	SaveScreenshot();
}

void KoLeftClick(int x, int y)
{
	LeftClick(Ko[0] + x, Ko[1] + y);
}

void CloseGenericPopup(int x, int y, int color)
{
	if (IsPixelAtPos(x, y, color))
	{
		KoLeftClick(x, y);
		//wait close window go away
		WaitPixelChangeColor(x, y, color);
		Sleep(200);	//extra wait to make sure window will not interpret it as a delayed event and still process it
	}
}

void CloseRSSOrCastlePopup()
{
	CloseGenericPopup(853, 127, STATIC_BGR_RGB(0x00FFBD36));
}

void CloseAllPossiblePopups()
{
	//resource or castle popups still visible
	CloseRSSOrCastlePopup();
	CloseGenericPopup(1255, 43, STATIC_BGR_RGB(0x00FFBE38)); // full screen popups
	CloseGenericPopup(853, 118, STATIC_BGR_RGB(0x00FFBE39)); // if we clicked on rally / battle hall
	CloseGenericPopup(852, 119, STATIC_BGR_RGB(0x00FFBD37)); // if we clicked on scout
	CloseGenericPopup(853, 126, STATIC_BGR_RGB(0x00FFBD36)); // if we clicked on land
	CloseGenericPopup(819, 431, STATIC_BGR_RGB(0x00FFBA31)); // if we clicked on army
}
/*
void Detect()
{
	POINT p;
	HDC hDC = GetDC(0);
	int x, y;

	while (!GetAsyncKeyState(VK_INSERT)) // Press insert to stop
	{
		GetCursorPos(&p);
		x = p.x;
		y = p.y;
		hDC = GetDC(0);
		std::cout << x << " " << y << " " << GetPixel(hDC, x, y) << std::endl;
		Sleep(50);
	}
	ReleaseDC(0, hDC);
}*/

void CaptureVisibleScreenGetPlayerLabels()
{
//	Detect();
//	return;

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
	else
		printf("See %d node tags on this screen\n", SearchResultCount);
	//parse each node label
	for (int i = 0; i < SearchResultCount; i++)
	{
		//try to make sure we do not have any random popups at this stage of the parsing.

		// click on the position of the level tag
		int x = SearchResultXYSAD[i][0];
		int y = SearchResultXYSAD[i][1];
		LeftClick(x, y);

		// wait to see if a popup opens
		if (WaitPixelBecomeColor(852, 126, STATIC_BGR_RGB(0x00FFBD36)) == 0)
		{
			printf("Popup did not become visible ? See pixel 0x%X . Wanted 0x%X \n", GetKoPixel(852, 126), STATIC_BGR_RGB(0x00FFBD36));
		}

		// check what type of popup opened
		if (IsPixelAtPos(817, 297, STATIC_BGR_RGB(0x00EFC471)) == 1)	// the golden i
		{
			printf("Found resource node at %d %d\n", x, y);
			//close it
			CloseRSSOrCastlePopup();
		}
		else if (IsPixelAtPos(518, 215, STATIC_BGR_RGB(0x00F3D51E) ) == 1) // the VIP star
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
//		break;
	}
}

void RunLordsMobileTests()
{
	//GetKoPlayerAndPos();
	CaptureVisibleScreenGetPlayerLabels();
}