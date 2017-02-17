#include "../stdafx.h"

#ifdef TEST_OFFLINE_PARSING_OF_PICTURES
char FullPath[2500];
#endif

#define REDUCE_PIXELPRECISION_MASK 0x00F0F0F0

DWORD KoPlayerProcessId = 0;
HWND KoPlayerWND = 0;
int Ko[4];
StorablePlayerInfo CurPlayer;

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

int IsKoPlayerInFocus()
{
	HWND CurWND = GetForegroundWindow();
	return (KoPlayerWND == CurWND);
}

void WaitKoPlayerGetFocus()
{
	while (IsKoPlayerInFocus() == 0 )
		Sleep(100);
}

COLORREF GetKoPixel(int x, int y)
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

int IsPixelAtPos(int x, int y, COLORREF Color)
{
	COLORREF Pixel1 = GetKoPixel(x, y);
	if (Pixel1 == (Color & REDUCE_PIXELPRECISION_MASK))
		return 1;
	return 0;
}

int WaitPixelBecomeColor(int x, int y, COLORREF Color)
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

int WaitPixelChangeColor(int x, int y, COLORREF Color)
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

//Just Dump Everything we can in the same text file. 
FILE *LocalDumpFile = 0;
void AppendDataToDB()
{
	if (LocalDumpFile == 0)
		LocalDumpFile = fopen("Players.txt", "at");
	if (LocalDumpFile != 0)
	{
		fprintf(LocalDumpFile, "%d \t %d \t %d", CurPlayer.k, CurPlayer.x, CurPlayer.y);
		fprintf(LocalDumpFile, " \t %s \t %s", CurPlayer.Name, CurPlayer.Guild);
		fprintf(LocalDumpFile, " \t %d \t %d", CurPlayer.Might, CurPlayer.Kills);
		fprintf(LocalDumpFile, " \t %d", CurPlayer.LastUpdateTimestamp);
#ifdef TEST_OFFLINE_PARSING_OF_PICTURES
		fprintf(LocalDumpFile, " \t %s", FullPath);
#endif
		fprintf(LocalDumpFile, "\n");
	}
}

int GuildCharsLoaded = 0;
void GetGuildNameFromCastlePopup()
{
	OCR_SetActiveFontSet(1,"K_C_M_guild/");
	if (GuildCharsLoaded == 0)
	{
		GuildCharsLoaded = 1;
		OCR_LoadFontsFromDir("K_C_M_guild", "KCM_");
	}
	OCR_SetMaxFontSize(20, 20);
	KeepColorsMinInRegion(73, 131, 370, 154, RGB(173, 174, 176));
	//SaveScreenshot();
	char *res = OCR_ReadTextLeftToRightSaveUnknownChars(73, 131, 370, 154);
	strcpy_s(CurPlayer.Guild, sizeof(CurPlayer.Guild), res);
	if (OCR_FoundNewFont == 1)
		CurPlayer.SkipSave = 1;
}

int NameCharsLoaded = 0;
void GetPlayerNameFromCastlePopup()
{
	OCR_SetActiveFontSet(2,"K_C_M_Playernames/");
	if (NameCharsLoaded == 0)
	{
		NameCharsLoaded = 1;
		OCR_LoadFontsFromDir("K_C_M_Playernames", "KCM_");
	}
	OCR_SetMaxFontSize(20, 20);
	KeepColorsMinInRegion(121, 16, 399, 44, RGB(194, 180, 55));
	//SaveScreenshot();
	char *res = OCR_ReadTextLeftToRightSaveUnknownChars(121, 16, 399, 44);
	strcpy_s(CurPlayer.Name, sizeof(CurPlayer.Name), res);
	if (OCR_FoundNewFont == 1)
		CurPlayer.SkipSave = 1;
}

int IsNumber(char A)
{
	if (A >= '0' && A <= '9')
		return 1;
	return 0;
}
void SkipToNumber(char *res, int &Ind)
{
	while (res[Ind] != 0 && IsNumber(res[Ind]) == 0)
		Ind++;
}
void AssembleNumber(char *res, int &Ind, int &N)
{
	N = 0;
	while (res[Ind] == ' ' || IsNumber(res[Ind]) == 1)
	{
		if (res[Ind] != ' ')
			N = N * 10 + res[Ind] - '0';
		Ind++;
	}
}
void ParseKXY(char *res, int &k, int &x, int &y)
{
	int Ind = 0;
	//parse until we find the first =
	SkipToNumber(res, Ind);
	//assemble K
	AssembleNumber(res, Ind, k);
	//parse until we find the first =
	SkipToNumber(res, Ind);
	//assemble x
	AssembleNumber(res, Ind, x);
	//parse until we find the first =
	SkipToNumber(res, Ind);
	//assemble y
	AssembleNumber(res, Ind, y);
}

int XYCharsLoaded = 0;
void GetPlayerXYFromCastlePopup()
{
	OCR_SetActiveFontSet(3,"K_C_M_xy/");
	if (XYCharsLoaded == 0)
	{
		XYCharsLoaded = 1;
		OCR_LoadFontsFromDir("K_C_M_xy", "KCM_");
	}
	OCR_SetMaxFontSize(20, 20);
	KeepColorsMinInRegion(129, 363, 268, 379, RGB(166, 172, 175));
	//SaveScreenshot();
	char *res = OCR_ReadTextLeftToRightSaveUnknownChars(129, 363, 268, 379);
	//printf("Tried to read player location %s\n", res);
	if (OCR_FoundNewFont == 1)
		CurPlayer.SkipSave = 1;
	else
		ParseKXY(res, CurPlayer.k, CurPlayer.x, CurPlayer.y);
}

int MightKillsCharsLoaded = 0;
void GetPlayerMightKillsFromCastlePopup()
{
	OCR_SetActiveFontSet(4,"K_C_M_MightKills/");
	if (MightKillsCharsLoaded == 0)
	{
		MightKillsCharsLoaded = 1;
		OCR_LoadFontsFromDir("K_C_M_MightKills", "KCM_");
	}
	OCR_SetMaxFontSize(20, 20);
	KeepColorsMinInRegion(193, 66, 350, 88, RGB(211, 211, 211));
	KeepColorsMinInRegion(258, 97, 390, 119, RGB(211, 211, 211));
	//SaveScreenshot();
	char *res;
	res = OCR_ReadTextLeftToRightSaveUnknownChars(193, 66, 350, 88);
	RemoveCharFromNumberString(res, ',');
	CurPlayer.Might = atoi(res);
	res = OCR_ReadTextLeftToRightSaveUnknownChars(258, 97, 390, 119);
	RemoveCharFromNumberString(res, ',');
	CurPlayer.Kills = atoi(res);
	if (OCR_FoundNewFont == 1)
		CurPlayer.SkipSave = 1;
}

void ParseCastlePopup()
{
#ifndef TEST_OFFLINE_PARSING_OF_PICTURES
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
#endif

	memset(&CurPlayer, 0, sizeof(CurPlayer));
	GetPlayerNameFromCastlePopup();
	GetPlayerMightKillsFromCastlePopup();
	GetGuildNameFromCastlePopup();
	GetPlayerXYFromCastlePopup();

	//we did not handle this one. Save it for later processing. Maybe we need to simply decode the new characters
	if (CurPlayer.SkipSave == 1)
	{
		SaveScreenshot();
	}
	else
	{
		CurPlayer.LastUpdateTimestamp = GetTickCount();
		AppendDataToDB();
	}
}

void KoLeftClick(int x, int y)
{
	LeftClick(Ko[0] + x, Ko[1] + y);
}

int CloseGenericPopup(int x, int y, int color)
{
	int ret = 0;
	if (IsPixelAtPos(x, y, color))
	{
		KoLeftClick(x, y);
		//wait close window go away
		WaitPixelChangeColor(x, y, color);
		Sleep(200);	//extra wait to make sure window will not interpret it as a delayed event and still process it
		ret = 1;
	}
	return ret;
}

void CloseRSSOrCastlePopup()
{
	CloseGenericPopup(853, 127, STATIC_BGR_RGB(0x00FFBD36));
}

void CloseAllPossiblePopups()
{
	int ClosedSomething = 0;
	//resource or castle popups still visible
	ClosedSomething += CloseGenericPopup(853, 127, STATIC_BGR_RGB(0x00FFBD36)); // resource or castle leftover popup
	ClosedSomething += CloseGenericPopup(1255, 43, STATIC_BGR_RGB(0x00FFBE38)); // full screen popups
	ClosedSomething += CloseGenericPopup(1235, 43, 0x00FFBE38); // full screen popups
	ClosedSomething += CloseGenericPopup(1255, 43, STATIC_BGR_RGB(0x009C504F)); // full screen popups
	ClosedSomething += CloseGenericPopup(1235, 43, STATIC_BGR_RGB(0x00FFBE38)); // full screen popups
	ClosedSomething += CloseGenericPopup(853, 118, STATIC_BGR_RGB(0x00FFBE39)); // if we clicked on rally / battle hall
	ClosedSomething += CloseGenericPopup(852, 119, STATIC_BGR_RGB(0x00FFBD37)); // if we clicked on scout
	ClosedSomething += CloseGenericPopup(853, 126, STATIC_BGR_RGB(0x00FFBD36)); // if we clicked on land
	ClosedSomething += CloseGenericPopup(819, 431, STATIC_BGR_RGB(0x00FFBA31)); // if we clicked on army
	ClosedSomething += CloseGenericPopup(1516, 473, STATIC_BGR_RGB(0x00FFBD36)); // if we clicked on forest info
//	ClosedSomething += CloseGenericPopup(1516, 473, STATIC_BGR_RGB(0x00FFBD36)); // connection lost
	//debugging is life
	if (ClosedSomething)
		printf("We managed to close some unexpected popup. Continue Execution\n");
}

void EnterTeleportCordDigit(int Digit)
{
	if (Digit == 1)
		KoLeftClick(870, 295);
	if (Digit == 2)
		KoLeftClick(955, 295);
	if (Digit == 3)
		KoLeftClick(1045, 295);

	if (Digit == 4)
		KoLeftClick(870, 360);
	if (Digit == 5)
		KoLeftClick(955, 360);
	if (Digit == 6)
		KoLeftClick(1045, 360);

	if (Digit == 7)
		KoLeftClick(870, 425);
	if (Digit == 8)
		KoLeftClick(955, 425);
	if (Digit == 9)
		KoLeftClick(1045, 425);

	if (Digit == 0)
		KoLeftClick(890, 490);
	Sleep(500);
}

void EnterTeleportCoord(int Coord)
{
	if (Coord > 1000)
		Coord = 1000;
	//convert number to vect
	int NumVect[9];
	int VectLen = 0;
	while (Coord > 0)
	{
		NumVect[VectLen++] = Coord % 10;
		Coord /= 10;
	}
	for (int i = VectLen - 1; i >= 0; i--)
		EnterTeleportCordDigit(NumVect[i]);
	// push the OK button
	KoLeftClick(1020, 490);
	Sleep(500);
}

void JumpToKingdomLocation(int Kingdom, int x, int y)
{
	KoLeftClick(700, 25);
	Sleep(500);
	//enter X
	KoLeftClick(650, 255);
	Sleep(500);
	EnterTeleportCoord(x);
	// y
	KoLeftClick(775, 255);
	Sleep(500);
	EnterTeleportCoord(y);
	//push go
	KoLeftClick(645, 375);
	Sleep(2000);
}

void WINAPI CaptureVisibleScreenGetPlayerLabels()
{

	// this will probably only run once to get the process related details
	GetKoPlayerAndPos();

	// wait for it to get some focus 
	WaitKoPlayerGetFocus();

	//make sure we did not inherit something strange from last session
	CloseAllPossiblePopups();

	// depends on the window resolution. As the resolution increases this will increase also
	int JumpToTurefIconSize = 80;
	TakeScreenshot(Ko[0] + JumpToTurefIconSize, Ko[1] + JumpToTurefIconSize, Ko[0] + Ko[2] - JumpToTurefIconSize, Ko[1] + Ko[3] - JumpToTurefIconSize);
/*	{
		SetGradientToColor(0xA59B63, 0.162f, 0x00FFFFFF);	// remove water
		KeepGradient(0x00946D21, 0.4f);						// keep tags only. Think about shielded players also
		//SaveScreenshot();
		ImageSearch_Multipass_PixelCount2(0, 60, 35, 5, 34, 21, 45);
	}/**/
	{
		KeepGradient3(RGB(33, 109, 148), 0.25f, RGB(16, 77, 113), 0.4f, RGB(40, 116, 155), 0.20f);
		ImageSearch_Multipass_PixelCount3(0, 85, 5, 8, 52);
	}/**/
	//ImageSearch_Multipass_PixelCount2(25, 25, 5, 8, 14, 45);
	//try to debug WTF situations
	if (SearchResultCount > 100)
	{
		SaveScreenshot();
		TakeScreenshot(Ko[0] + JumpToTurefIconSize, Ko[1] + JumpToTurefIconSize, Ko[0] + Ko[2] - JumpToTurefIconSize, Ko[1] + Ko[3] - JumpToTurefIconSize);
		SaveScreenshot();
		printf("Something is wrong. It's impossible to get more than 100 nodes per screen. We got %d\n", SearchResultCount);
		SearchResultCount = 0;
	}
	else
		printf("See %d node tags on this screen\n", SearchResultCount);
	//parse each node label
	for (int i = 0; i < SearchResultCount; i++)
	{
		//safety break from a possible infinite loop
		if (GetAsyncKeyState(VK_INSERT) || IsKoPlayerInFocus() == 0)
			break;

		//try to make sure we do not have any random popups at this stage of the parsing.
		CloseAllPossiblePopups();

		// click on the position of the level tag
		int x = SearchResultXYSAD[i][0];
		int y = SearchResultXYSAD[i][1];
		LeftClick(x, y);

		//close as soon as possible to not move our screen. Probably the whole scan is foobar as all click locations are messed up. It will read to random popup screens also 
		CloseGenericPopup(819, 431, STATIC_BGR_RGB(0x00FFBA31)); // if we clicked on army

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
		else if (IsPixelAtPos(613, 500, STATIC_BGR_RGB(0x00337677)) == 1 || IsPixelAtPos(614, 495, STATIC_BGR_RGB(0x00A54A4F)) == 1) // can send resource or can be scouted
		{
			printf("Found castle at %d %d\n", x, y);
			// parse if castle
			ParseCastlePopup();
			//close it
			CloseRSSOrCastlePopup();
		}
		else
		{
			printf("We clicked on something but have no idea what at %d %d. Ratio is %d\n", x, y, SearchResultXYSAD[i][2]);
		}
//		break;

		//safety break from a possible infinite loop
		if (GetAsyncKeyState(VK_INSERT) || IsKoPlayerInFocus() == 0)
			break;

	}

	//ake sure there are no popups, so drag can work it's magic
	CloseAllPossiblePopups();

}

void DragScreenToLeft()
{
	int SkipDragAmount = 32;
	MouseDrag(Ko[0] + Ko[2] - SkipDragAmount, Ko[1] + Ko[3] / 2, Ko[0] + SkipDragAmount, Ko[1] + Ko[3] / 2);
}

void ZoomOutToKingdomView()
{
}

void SaveKingdomScanStatus( int k, int x, int y)
{
	FILE *f = fopen("KingdomScanStatus.txt", "wb");
	if (f)
	{
		int buf[3];
		buf[0] = k;
		buf[1] = x;
		buf[2] = y;
		fwrite(buf, 1, 3 * sizeof(int), f);
		fclose(f);
	}
}

#define COULD_NOT_LOAD_RESTORE_DATA -2
void RestoreKingdomScanStatus(int &k, int &x, int &y)
{
	k = COULD_NOT_LOAD_RESTORE_DATA;
	FILE *f = fopen("KingdomScanStatus.txt", "rb");
	if (f)
	{
		int buf[3];
		fread(buf, 1, 3 * sizeof(int), f);
		k = buf[0];
		x = buf[1];
		y = buf[2];
		fclose(f);
	}
}

void ResetKingdomSaveStatus()
{
	SaveKingdomScanStatus(COULD_NOT_LOAD_RESTORE_DATA, COULD_NOT_LOAD_RESTORE_DATA, COULD_NOT_LOAD_RESTORE_DATA);
}

void ScanKingdomArea(int Kingdom, int StartX, int StartY, int EndX, int EndY)
{
	GetKoPlayerAndPos();
	WaitKoPlayerGetFocus();
	CloseAllPossiblePopups();
	ZoomOutToKingdomView();

	int RestoreK, RestoreX, RestoreY;
	RestoreKingdomScanStatus(RestoreK, RestoreX, RestoreY);
	//make sure this does not contain random junk
	memset(&CurPlayer, 0, sizeof(CurPlayer));

	StartCounter();
	int Start = GetTimeTickI();
	for (int y = StartY; y <= EndY; y += 10)
	{
		JumpToKingdomLocation(Kingdom, StartX, y);
		for (int x = StartX; x <= EndX; x+=10)
		{
			//try to jump directly to a location where 
			if (RestoreK != COULD_NOT_LOAD_RESTORE_DATA)
			{
				y = RestoreY;
				x = RestoreX;
				JumpToKingdomLocation(69, RestoreX, RestoreY);
				RestoreK = COULD_NOT_LOAD_RESTORE_DATA;
			}

			int End = GetTimeTickI();
			printf("We made %d slides. We should be at x = %d. Time spent so far %d\n", x, x, (End - Start) / 1000 / 60);

			CurPlayer.k = -1;	//mark it as invalid

			CaptureVisibleScreenGetPlayerLabels();

			//if we found a castle, check if we are on the same screen as expected. Resync to expected location in case we clicked on an army or something
			if (CurPlayer.k != -1 && (abs(CurPlayer.x - x) > 20 || abs(CurPlayer.y - y) > 10))
			{
				printf("\nWe are expecting to be at %d,%d, but we are at %d,%d?. Resync location\n\n", x, y, CurPlayer.x, CurPlayer.y);
				JumpToKingdomLocation(69, x, y);
			}
			else
				DragScreenToLeft(); // we function as expected, we can simply drag the screen to the left

			SaveKingdomScanStatus(Kingdom, x, y);
			//safety break from a possible infinite loop
			if (GetAsyncKeyState(VK_INSERT) || IsKoPlayerInFocus() == 0)
				return;
		}
	}
}

void OfflineTestCastlePopupParsing()
{
	memset(Ko, 0, sizeof(Ko));
	TakeScreenshot(0, 0, 401, 381);
	std::string path = "h:/Lords/CastlepopupExamples7";
	std::string search_path = path;
	search_path += "/*.*";
	std::string SkipUntilFile = "";
	int FoundFirstFile = SkipUntilFile.length() == 0;
	int SkipFirstN = 1500 * 2;
	int BatchProcessMaxCount = SkipFirstN + 1500;
//	int BatchProcessMaxCount = 1;
	int Index = 0;
	WIN32_FIND_DATA fd;
	HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
	if (hFind == INVALID_HANDLE_VALUE)
		return;
	do {
		// read all (real) files in current folder
		// , delete '!' read other 2 default folder . and ..
		if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			Index++;
			if (FoundFirstFile == 0)
			{
				if (strcmp(fd.cFileName, SkipUntilFile.c_str()) == 0)
					FoundFirstFile = 1;
				else
					continue;
			}
			BatchProcessMaxCount--;
			if (SkipFirstN-- > 0)
				continue;
			sprintf_s(FullPath, sizeof(FullPath), "%s/%s", path.c_str(), fd.cFileName);
			printf("%d)Parsing file : %s\n", Index, FullPath);
			LoadCacheOverScreenshot(FullPath, 0, 0);
			//test it
			ParseCastlePopup();
		}
	} while (::FindNextFile(hFind, &fd) && BatchProcessMaxCount > 0);
	::FindClose(hFind);
}

void RunLordsMobileTests()
{
	//GetKoPlayerAndPos();
/*	{
		GetKoPlayerAndPos();
		WaitKoPlayerGetFocus();
		DragScreenToLeft();
		return;
	}/**/
/*	{
		GetKoPlayerAndPos();
		WaitKoPlayerGetFocus();
		CloseAllPossiblePopups();
		return;
	}/**/
/*	{
		GetKoPlayerAndPos();
		WaitKoPlayerGetFocus();
		CloseAllPossiblePopups();
		ZoomOutToKingdomView();
		JumpToKingdomLocation(69, 0, 110);
	}/**/
	{
		OfflineTestCastlePopupParsing();
		return;
	}/**/
	// aprox 7 mins / row
	// 40 * 50 in 35 mins => 57 screens / min
	// 9 row in 77 minutes
	ScanKingdomArea(69, 0, 720, 500, 1000);

	printf("fliptablegoinghome.THE END\n");
	_getch();
}