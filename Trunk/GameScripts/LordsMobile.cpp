#include "stdafx.h"

#if defined( _CONSOLE ) && defined(COMPILE_LORDS_SCRIPTS)

#ifdef TEST_OFFLINE_PARSING_OF_PICTURES
	char FullPath[2500];
#endif

#define JUST_CLICK_NO_PARSE_ADVANCED

#define REDUCE_PIXELPRECISION_MASK 0x00F0F0F0
#define RESYNC_ON_X_DIFF			15
#define RESYNC_ON_Y_DIFF			15

DWORD KoPlayerProcessId = 0;
HWND KoPlayerWND = 0;
int Ko[4];
StorablePlayerInfo CurPlayer;
StorableResourceTileInfo CurRss;
int ParseCastleInfo = 0;
int ParseProfileInfo = 0;
int ParseProfileInfo2 = 0;

void WaitScreeenDragFinish();

void KoLeftClick(int x, int y)
{
	LeftClick(Ko[0] + x, Ko[1] + y);
}

void DragScreenToLeft()
{
	int SkipDragAmount = 32;
	MouseDrag(Ko[0] + Ko[2] - SkipDragAmount, Ko[1] + Ko[3] / 2, Ko[0] + SkipDragAmount, Ko[1] + Ko[3] / 2);
	WaitScreeenDragFinish();
}

void DragScreenToRight()
{
	int SkipDragAmount = 32;
	MouseDrag(Ko[0] + 2, Ko[1] + Ko[3] / 2, Ko[0] + Ko[2] - SkipDragAmount, Ko[1] + Ko[3] / 2);
	WaitScreeenDragFinish();
}

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
//printf("at %d,%d has %X instead %X => %X %X\n", x, y, Pixel1, Color, (Pixel1 & REDUCE_PIXELPRECISION_MASK), (Color & REDUCE_PIXELPRECISION_MASK));
	if (Pixel1 == (Color & REDUCE_PIXELPRECISION_MASK))
		return 1;
	return 0;
}

int IsPixelAtPosCurScreenShotRel(int x, int y, COLORREF Color)
{
	if (x > CurScreenshot->GetWidth() || y > CurScreenshot->GetHeight())
		return 0;
	COLORREF Pixel1 = CurScreenshot->Pixels[y * CurScreenshot->GetWidth() + x];
//printf("at %d,%d has %X instead %X => %X %X\n", x, y, Pixel1, Color, (Pixel1 & REDUCE_PIXELPRECISION_MASK), (Color & REDUCE_PIXELPRECISION_MASK));
	if ((Pixel1 & REDUCE_PIXELPRECISION_MASK) == (Color & REDUCE_PIXELPRECISION_MASK))
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

int WaitPixelChangeColor(int x, int y, COLORREF Color, int Timeout)
{
	int ret = 0;
	int tSleep = 100;
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
		errno_t er = fopen_s(&LocalDumpFile, "Players.txt", "at");
	if (LocalDumpFile != 0)
	{
		fprintf(LocalDumpFile, "%d \t %d \t %d", CurPlayer.k, CurPlayer.x, CurPlayer.y);
		fprintf(LocalDumpFile, " \t %s \t %s", CurPlayer.Name, CurPlayer.Guild);
		fprintf(LocalDumpFile, " \t %d \t %d", CurPlayer.Might, CurPlayer.Kills);
		fprintf(LocalDumpFile, " \t %d \t %d", (int)CurPlayer.LastUpdateTimestamp, (int)CurPlayer.HasPrisoners);
		fprintf(LocalDumpFile, " \t %d \t %d \t %d", CurPlayer.VIPLevel, CurPlayer.GuildRank, CurPlayer.PlayerLevel);
		fprintf(LocalDumpFile, " \t %d \t %d \t %d", CurPlayer.SuccessfulAttacks, CurPlayer.FailedAttacks, CurPlayer.SuccessfulDefenses);
		fprintf(LocalDumpFile, " \t %d \t %d \t %d", CurPlayer.FailedDefenses, CurPlayer.TroopsKilled, CurPlayer.TroopsLost);
		fprintf(LocalDumpFile, " \t %d \t %d \t %d", CurPlayer.TroopsHealed, CurPlayer.TroopsWounded, CurPlayer.TurfsDestroyed);
		fprintf(LocalDumpFile, " \t %d \t %d \t %d", CurPlayer.TurfsLost, CurPlayer.MightDestroyed, 0);
#ifdef TEST_OFFLINE_PARSING_OF_PICTURES
		fprintf(LocalDumpFile, " \t %s", FullPath);
#endif
		fprintf(LocalDumpFile, "\n");
		fflush(LocalDumpFile);
	}
}

int CloseGenericPopup(int x, int y, int color, int SleepFor, int Timeout)
{
	int ret = 0;
	if (IsPixelAtPos(x, y, color))
	{
		KoLeftClick(x, y);
		//wait close window go away
		WaitPixelChangeColor(x, y, color, Timeout);
		Sleep(SleepFor);	//extra wait to make sure window will not interpret it as a delayed event and still process it
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
	int ClosedSomething;
	do {
		ClosedSomething = 0;
		//resource or castle popups still visible
		ClosedSomething += CloseGenericPopup(853, 127, STATIC_BGR_RGB(0x00FFBD36)); // resource or castle leftover popup
		ClosedSomething += CloseGenericPopup(1255, 43, STATIC_BGR_RGB(0x00FFBE38),400, 400); // full screen popups
		ClosedSomething += CloseGenericPopup(1235, 43, 0x00FFBE38); // full screen popups
		ClosedSomething += CloseGenericPopup(1255, 43, STATIC_BGR_RGB(0x009C504F), 400, 400); // full screen popups
		ClosedSomething += CloseGenericPopup(1235, 43, STATIC_BGR_RGB(0x00FFBE38), 400, 400); // full screen popups
		ClosedSomething += CloseGenericPopup(853, 118, STATIC_BGR_RGB(0x00FFBE39)); // if we clicked on rally / battle hall
		ClosedSomething += CloseGenericPopup(852, 119, STATIC_BGR_RGB(0x00FFBD37)); // if we clicked on scout
		ClosedSomething += CloseGenericPopup(853, 126, STATIC_BGR_RGB(0x00FFBD36)); // if we clicked on land
		ClosedSomething += CloseGenericPopup(819, 431, STATIC_BGR_RGB(0x00FFBA31)); // if we clicked on army
		ClosedSomething += CloseGenericPopup(1516, 473, STATIC_BGR_RGB(0x00FFBD36)); // if we clicked on forest info
		ClosedSomething += CloseGenericPopup(855, 146, STATIC_BGR_RGB(0x00FFBD37)); // daily login bonus popup
		ClosedSomething += CloseGenericPopup(854, 121, STATIC_BGR_RGB(0x00FFBA31)); // disconnected
		//debugging is life
		if (ClosedSomething)
			printf("We managed to close some unexpected popup. Continue Execution\n");
	} while (ClosedSomething != 0);
}

int VIPCharsLoaded = 0;
void GetVIPLevelFromCastlePopup()
{
	OCR_SetActiveFontSet(1, "K_C_M_VIP/");
	if (VIPCharsLoaded == 0)
	{
		VIPCharsLoaded = 1;
		OCR_LoadFontsFromDir("K_C_M_VIP", "KCM_");
	}
	OCR_SetMaxFontSize(20, 20);
	KeepColorsMinInRegion(66, 17, 90, 36, RGB(189, 174, 102));
	//SaveScreenshot();
	char *res = OCR_ReadTextLeftToRightSaveUnknownChars(66, 17, 90, 36);
	if (res[0] != '0')RemoveCharFromNumberString(res, ' ');
	CurPlayer.VIPLevel = atoi(res);
	if (OCR_FoundNewFont == 1)
		CurPlayer.SkipSave = 1;
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
	KeepColorsMinInRegion(125, 363, 274, 379, RGB(166, 172, 175));
	//SaveScreenshot();
	char *res = OCR_ReadTextLeftToRightSaveUnknownChars(125, 363, 274, 379);
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
	KeepColorsMinInRegion(250, 97, 390, 119, RGB(211, 211, 211));
	//SaveScreenshot();
	char *res;
	res = OCR_ReadTextLeftToRightSaveUnknownChars(193, 66, 350, 88);
	if (res[0] != '0')RemoveCharFromNumberString(res, ' ');
	if (res[0] != '0')RemoveCharFromNumberString(res, ',');
	CurPlayer.Might = atoi(res);
	res = OCR_ReadTextLeftToRightSaveUnknownChars(250, 97, 390, 119);
	if (res[0] != '0')RemoveCharFromNumberString(res, ' ');
	if (res[0] != '0')RemoveCharFromNumberString(res, ',');
	CurPlayer.Kills = atoi(res);
	if (OCR_FoundNewFont == 1)
		CurPlayer.SkipSave = 1;
}

void GetPlayerHasPrisoners()
{
//	printf("prisoners see pixel 0x%X\n", GetKoPixel(431, 415));
	if (IsPixelAtPos(431, 415, STATIC_BGR_RGB(0x00D6B263)) || IsPixelAtPos(431, 415, 0x00D6B263))
	{
		printf("Castle has prisoners\n");
		CurPlayer.HasPrisoners = time(NULL);
	}
}

void GetPlayerGuildRank()
{
//	for (int y = 23; y <= 26; y++)
//		for (int x = 23; x <= 26; x++)
//			IsPixelAtPosCurScreenShotRel(x, y, BGR(137, 137, 137));
#ifndef JUST_CLICK_NO_PARSE_ADVANCED
	if (IsPixelAtPosCurScreenShotRel(25, 25, BGR(53, 53, 53)) || IsPixelAtPosCurScreenShotRel(21, 19, BGR(49, 52, 49)))
		CurPlayer.GuildRank = 1;
	else if (IsPixelAtPosCurScreenShotRel(25, 25, BGR(137, 137, 137)) || IsPixelAtPosCurScreenShotRel(17, 20, BGR(41, 44, 57)))
		CurPlayer.GuildRank = 2;
	else if (IsPixelAtPosCurScreenShotRel(25, 25, BGR(104, 102, 84)) || IsPixelAtPosCurScreenShotRel(14, 21, BGR(49, 48, 8)))
		CurPlayer.GuildRank = 3;
	else if (IsPixelAtPosCurScreenShotRel(25, 25, BGR(53, 35, 16)) || IsPixelAtPosCurScreenShotRel(14, 21, BGR(49, 32, 0)))
		CurPlayer.GuildRank = 4;
	else if (IsPixelAtPosCurScreenShotRel(25, 25, BGR(86, 35, 35)) || IsPixelAtPosCurScreenShotRel(30, 31, BGR(114, 34, 17)))
		CurPlayer.GuildRank = 5;
#endif
}

void GetPlayerIsBurning()
{
	// this is an animation. It has a huge chance to fail
	if (IsPixelAtPos(434, 344, STATIC_BGR_RGB(0x00576469)) || IsPixelAtPos(440, 331, STATIC_BGR_RGB(0x0050565C)))
		CurPlayer.IsBurning = time(NULL);
}

int PlayerLevelCharsLoaded = 0;
void GetPlayerLevelProfilePopup()
{
	OCR_SetActiveFontSet(4, "K_C_M_PLevel/");
	if (PlayerLevelCharsLoaded == 0)
	{
		PlayerLevelCharsLoaded = 1;
		OCR_LoadFontsFromDir("K_C_M_PLevel", "KCM_");
	}
	OCR_SetMaxFontSize(20, 20);
	TakeScreenshot(Ko[0] + 446, Ko[1] + 225, Ko[0] + 476, Ko[1] + 248); //take screenshot of the unmurdered image. We will reprocess it later
	//SaveScreenshot();
	KeepColorsMinInRegion(-1, -1, -1, -1, RGB(168, 168, 103));
	//SaveScreenshot();
	char *res;
	res = OCR_ReadTextLeftToRightSaveUnknownChars(-1, -1, -1, -1);
	if (OCR_FoundNewFont == 0)
	{
		if (res[0] != '0')	RemoveCharFromNumberString(res, ' ');
		CurPlayer.PlayerLevel = atoi(res);
	}
	else
	{
		TakeScreenshot(Ko[0] + 446, Ko[1] + 225, Ko[0] + 476, Ko[1] + 248); //take screenshot of the unmurdered image. We will reprocess it later
		SaveScreenshot();
	}
}

int PlayerProfileCharsLoaded = 0;
void GetPlayerProfileInfo1()
{
	OCR_SetActiveFontSet(4, "K_C_M_PProfile/");
	if (PlayerProfileCharsLoaded == 0)
	{
		PlayerProfileCharsLoaded = 1;
		OCR_LoadFontsFromDir("K_C_M_PProfile", "KCM_");
	}
	OCR_SetMaxFontSize(20, 20);
	TakeScreenshot(Ko[0] + 653, Ko[1] + 292, Ko[0] + 850, Ko[1] + 683); //take screenshot of the unmurdered image. We will reprocess it later
//	SaveScreenshot();
	KeepColorsMinInRegion(-1, -1, -1, -1, RGB(231, 219, 150));
//	SaveScreenshot();
	char *res;
	int LogScreenshot = 0;
	res = OCR_ReadTextLeftToRightSaveUnknownChars(5, 6, 150, 24);
	if (res[0] != '0')RemoveCharFromNumberString(res, ' ');
	if (res[0] != '0')RemoveCharFromNumberString(res, ',');
	if (OCR_FoundNewFont == 0)
		CurPlayer.SuccessfulAttacks = atoi(res);
	else
		LogScreenshot += OCR_FoundNewFont;

	res = OCR_ReadTextLeftToRightSaveUnknownChars(5, 43, 150, 60);
	if (res[0] != '0')RemoveCharFromNumberString(res, ' ');
	if (res[0] != '0')RemoveCharFromNumberString(res, ',');
	LogScreenshot += OCR_FoundNewFont;
	if (OCR_FoundNewFont == 0)
		CurPlayer.FailedAttacks = atoi(res);
	else
		LogScreenshot += OCR_FoundNewFont;

	res = OCR_ReadTextLeftToRightSaveUnknownChars(5, 79, 150, 96);
	if (res[0] != '0')RemoveCharFromNumberString(res, ' ');
	if (res[0] != '0')RemoveCharFromNumberString(res, ',');
	LogScreenshot += OCR_FoundNewFont;
	if (OCR_FoundNewFont == 0)
		CurPlayer.SuccessfulDefenses = atoi(res);
	else
		LogScreenshot += OCR_FoundNewFont;

	res = OCR_ReadTextLeftToRightSaveUnknownChars(5, 115, 150, 132);
	if (res[0] != '0')RemoveCharFromNumberString(res, ' ');
	if (res[0] != '0')RemoveCharFromNumberString(res, ',');
	LogScreenshot += OCR_FoundNewFont;
	if (OCR_FoundNewFont == 0)
		CurPlayer.FailedDefenses = atoi(res);
	else
		LogScreenshot += OCR_FoundNewFont;

	res = OCR_ReadTextLeftToRightSaveUnknownChars(5, 187, 150, 204);
	if (res[0] != '0')RemoveCharFromNumberString(res, ' ');
	if (res[0] != '0')RemoveCharFromNumberString(res, ',');
	LogScreenshot += OCR_FoundNewFont;
	if (OCR_FoundNewFont == 0)
		CurPlayer.TroopsKilled = atoi(res);
	else
		LogScreenshot += OCR_FoundNewFont;

	res = OCR_ReadTextLeftToRightSaveUnknownChars(5, 259, 150, 276);
	if (res[0] != '0')RemoveCharFromNumberString(res, ' ');
	if (res[0] != '0')RemoveCharFromNumberString(res, ',');
	LogScreenshot += OCR_FoundNewFont;
	if (OCR_FoundNewFont == 0)
		CurPlayer.TroopsLost = atoi(res);
	else
		LogScreenshot += OCR_FoundNewFont;

	res = OCR_ReadTextLeftToRightSaveUnknownChars(5, 331, 150, 348);
	if (res[0] != '0')RemoveCharFromNumberString(res, ' ');
	if (res[0] != '0')RemoveCharFromNumberString(res, ',');
	LogScreenshot += OCR_FoundNewFont;
	if (OCR_FoundNewFont == 0)
		CurPlayer.TroopsHealed = atoi(res);
	else
		LogScreenshot += OCR_FoundNewFont;

	res = OCR_ReadTextLeftToRightSaveUnknownChars(5, 367, 150, 384);
	if (res[0] != '0')RemoveCharFromNumberString(res, ' ');
	if (res[0] != '0')RemoveCharFromNumberString(res, ',');
	LogScreenshot += OCR_FoundNewFont;
	if (OCR_FoundNewFont == 0)
		CurPlayer.TroopsWounded = atoi(res);
	else
		LogScreenshot += OCR_FoundNewFont;

	if (LogScreenshot != 0)
	{
		TakeScreenshot(Ko[0] + 653, Ko[1] + 292, Ko[0] + 850, Ko[1] + 683); //take screenshot of the unmurdered image. We will reprocess it later
		SaveScreenshot();
	}
	goto FINISH_THIS_LATER;
	Sleep(500);
	//drag the screen a bit up
	MouseDrag(Ko[0] + Ko[2] / 2, Ko[1] + Ko[3] / 2 + 100, Ko[0] + Ko[2] / 2, Ko[1] + Ko[3] / 3);
	KoLeftClick(Ko[0] + Ko[2] / 2, Ko[1] + Ko[3] / 3); // make the movement stop instantly
	Sleep(500);

	//new window to process
	TakeScreenshot(Ko[0] + 653, Ko[1] + 471, Ko[0] + 850, Ko[1] + 579); //take screenshot of the unmurdered image. We will reprocess it later
	SaveScreenshot();
	KeepColorsMinInRegion(-1, -1, -1, -1, RGB(231, 219, 150));
	SaveScreenshot();
	LogScreenshot = 0;

	res = OCR_ReadTextLeftToRightSaveUnknownChars(8, 15, 150, 32);
	if (res[0] != '0')RemoveCharFromNumberString(res, ' ');
	if (res[0] != '0')RemoveCharFromNumberString(res, ',');
	LogScreenshot += OCR_FoundNewFont;
	if (OCR_FoundNewFont == 0)
		CurPlayer.TurfsDestroyed = atoi(res);
	else
		LogScreenshot += OCR_FoundNewFont; 

	res = OCR_ReadTextLeftToRightSaveUnknownChars(8, 51, 150, 68);
	if (res[0] != '0')RemoveCharFromNumberString(res, ' ');
	if (res[0] != '0')RemoveCharFromNumberString(res, ',');
	LogScreenshot += OCR_FoundNewFont;
	if (OCR_FoundNewFont == 0)
		CurPlayer.TurfsLost = atoi(res);
	else
		LogScreenshot += OCR_FoundNewFont;

	res = OCR_ReadTextLeftToRightSaveUnknownChars(8, 87, 150, 104);
	if (res[0] != '0')RemoveCharFromNumberString(res, ' ');
	if (res[0] != '0')RemoveCharFromNumberString(res, ',');
	LogScreenshot += OCR_FoundNewFont;
	if (OCR_FoundNewFont == 0)
		CurPlayer.MightDestroyed = atoi(res);
	else
		LogScreenshot += OCR_FoundNewFont;

	if (LogScreenshot != 0)
	{
		TakeScreenshot(Ko[0] + 653, Ko[1] + 471, Ko[0] + 850, Ko[1] + 579); //take screenshot of the unmurdered image. We will reprocess it later
		SaveScreenshot();
	}
	FINISH_THIS_LATER:
	//close the popup ( we will close the second one later )
	Sleep(200);
	CloseAllPossiblePopups();
}


void GetProfileInfo()
{
	printf("Started parsing player info window\n");
	// click view profile. If we can not find the button than stop clicking random stuff
	if (!IsPixelAtPos(532, 421, STATIC_BGR_RGB(0x002A7584)) && !IsPixelAtPos(532, 421, (0x002A7584)))
	{
		printf("Could not find profile view button. Skipping profile scan\n");
		return;
	}
	KoLeftClick(532, 421);
	//player tag will pop up
	if (WaitPixelBecomeColor(442, 223, STATIC_BGR_RGB(0x00951B11)) == 0)
	{
		printf("Player level screen load timout !\n");
		return;
	}
#ifndef JUST_CLICK_NO_PARSE_ADVANCED
	printf("parsing player level\n");
	//parse player level
	GetPlayerLevelProfilePopup();
#endif
	if (ParseProfileInfo2 == 0)
	{
		printf("End parsing player info\n");
//		Sleep(300);
		CloseAllPossiblePopups();
		return;
	}
	//open advanced player info
	KoLeftClick(220, 125);
#ifndef JUST_CLICK_NO_PARSE_ADVANCED
	if (WaitPixelBecomeColor(528, 233, RGB(242, 196, 125)) == 0)
	{
//		IsPixelAtPos(528, 233, RGB(242, 196, 125));
		printf("Player advanced info screen load timout !\n");
		return;
	}
	//parse attacks and defenses
	GetPlayerProfileInfo1();
#endif
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
	//SaveScreenshot();
#endif

	memset(&CurPlayer, 0, sizeof(CurPlayer));

	GetPlayerHasPrisoners();
	GetPlayerGuildRank();
	GetPlayerIsBurning();
	GetPlayerNameFromCastlePopup();
	GetPlayerMightKillsFromCastlePopup();
	GetGuildNameFromCastlePopup();
	GetPlayerXYFromCastlePopup();
	GetVIPLevelFromCastlePopup();

	//must be last thing as we are changing popup windows
	if (ParseProfileInfo)
	{
		GetProfileInfo();
	}

	//we did not handle this one. Save it for later processing. Maybe we need to simply decode the new characters
	if (CurPlayer.SkipSave == 1)
	{
#ifndef TEST_OFFLINE_PARSING_OF_PICTURES
		TakeScreenshot(Ko[0] + PopupStartX, Ko[1] + PopupStartY, Ko[0] + PopupEndX, Ko[1] + PopupEndY); //take screenshot of the unmurdered image. We will reprocess it later
		SaveScreenshot();
#endif
	}
	else
	{
		CurPlayer.LastUpdateTimestamp = time(NULL);
		AppendDataToDB();
	}
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
	if (Coord > 1030)
		Coord = 1030;
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
	//in rare cases game blocks and jumping no longer works for some reason
	DragScreenToRight();
	KoLeftClick(700, 25);
	Sleep(1000);
	//enter X
	KoLeftClick(650, 255);
	Sleep(1000);
	EnterTeleportCoord(x);
	Sleep(1000);
	// y
	KoLeftClick(775, 255);
	Sleep(1000);
	EnterTeleportCoord(y);
	Sleep(1000);
	//push go
	KoLeftClick(645, 375);
	Sleep(2000);
}

void CaptureVisibleScreenGetPlayerLabels(int ExpectedKingdom, int ExpectedX, int ExpectedY)
{

	// this will probably only run once to get the process related details
	GetKoPlayerAndPos();

	// wait for it to get some focus 
	WaitKoPlayerGetFocus();

	//make sure we did not inherit something strange from last session
	CloseAllPossiblePopups();

	// depends on the window resolution. As the resolution increases this will increase also
	int JumpToTurefIconSize = 80;
	int InfiniteLoopDisabler = 10;
RestartScreenScan:
	InfiniteLoopDisabler--;
	if (InfiniteLoopDisabler < 0)
		return;
	TakeScreenshot(Ko[0] + JumpToTurefIconSize, Ko[1] + JumpToTurefIconSize, Ko[0] + Ko[2], Ko[1] + Ko[3]);
	//cut out the icon in the right upper corner
	{
		int Width = CurScreenshot->GetWidth();
		int Height = CurScreenshot->GetHeight();
		for (int i = 0; i < 130 && i < Height; i++)
			memset(&CurScreenshot->Pixels[i*Width + Width - 130], TRANSPARENT_COLOR, 130 * sizeof(int));
	}

	//remove water and everything else that is not a player tag
	KeepGradient3(RGB(33, 109, 148), 0.25f, RGB(16, 77, 113), 0.4f, RGB(40, 116, 155), 0.20f);
	//SaveScreenshot();
	int rad = 52;
	ImageSearch_Multipass_PixelCount3(0, 85, 5, 8, rad);

	//whole screen is only background without any loaded tile info ?
	if (SearchResultCount == 0)
	{
		Sleep(1000);
		goto RestartScreenScan;
	}

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
		if (GetAsyncKeyState(VK_INSERT))
			break;
		memset(&CurPlayer, 0, sizeof(CurPlayer));

		//pause until we get focus again. Might be required when we want to do something in the background
		WaitKoPlayerGetFocus();

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
		if (GetAsyncKeyState(VK_INSERT))
			break;

		//pause until we get focus again. Might be required when we want to do something in the background
		WaitKoPlayerGetFocus();

		if (CurPlayer.x > 0 && CurPlayer.y > 0 && (abs(CurPlayer.x - ExpectedX) > RESYNC_ON_X_DIFF || abs(CurPlayer.y - ExpectedY) > RESYNC_ON_Y_DIFF))
		{
			printf("\nWe are expecting to be at %d,%d, but we are at %d,%d?. Resync location\n\n", ExpectedX, ExpectedY, CurPlayer.x, CurPlayer.y);
			JumpToKingdomLocation(ExpectedKingdom, ExpectedX, ExpectedY);
			goto RestartScreenScan;
		}

	}

	//ake sure there are no popups, so drag can work it's magic
	CloseAllPossiblePopups();

}

void WaitScreeenDragFinish()
{
#define MonitoredPositionCount 4
	int MonitoredPixels[MonitoredPositionCount][3];
	memset(MonitoredPixels, 0, sizeof(MonitoredPixels));
	//define a few locations that we will monitor
	MonitoredPixels[0][0] = Ko[0] + 100;
	MonitoredPixels[0][1] = Ko[1] + 100;
	MonitoredPixels[1][0] = Ko[0] + 200;
	MonitoredPixels[1][1] = Ko[1] + 200;
	MonitoredPixels[2][0] = Ko[0] + 300;
	MonitoredPixels[2][1] = Ko[1] + 300;
	MonitoredPixels[3][0] = Ko[0] + 400;
	MonitoredPixels[3][1] = Ko[1] + 400;
	//get initial values
	for (int i = 0; i < MonitoredPositionCount; i++)
		MonitoredPixels[i][2] = GetKoPixel(MonitoredPixels[i][0], MonitoredPixels[i][1]);
	//see if at least some of them will have the same value over time. If so, we guess the screen stopped moving
	int SleepTime = 100;
	int WaitTimeout = 2000;
	int NeedsMoreWait = 1;
	while (WaitTimeout > 0 && NeedsMoreWait == 1)
	{
		Sleep(SleepTime);
		NeedsMoreWait -= SleepTime;
		int SameValueCount = 0;
		for (int i = 0; i < MonitoredPositionCount; i++)
		{
			int NewPixelVal = GetKoPixel(MonitoredPixels[i][0], MonitoredPixels[i][1]);
			if (MonitoredPixels[i][2] == NewPixelVal)
				SameValueCount++;
			MonitoredPixels[i][2] = NewPixelVal;
		}
		if (SameValueCount == MonitoredPositionCount / 2)
			NeedsMoreWait = 0;
	}
}

void ZoomOutToKingdomView()
{
}

void SaveKingdomScanStatus( int k, int x, int y)
{
	FILE *f;
	errno_t er = fopen_s( &f, "KingdomScanStatus.txt", "wb");
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

void RestoreKingdomScanStatus(int &k, int &x, int &y)
{
	k = COULD_NOT_LOAD_RESTORE_DATA;
	FILE *f;
	errno_t er = fopen_s(&f, "KingdomScanStatus.txt", "rb");
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

	int StepY = 10;
	if (StartY > EndY)
		StepY = -StepY;
	StartCounter();
	int Start = GetTimeTickI();
	for (int y = StartY; (StepY < 0 && y >= EndY) || (StepY > 0 && y <= EndY); y += StepY)
	{
		if (RestoreK == COULD_NOT_LOAD_RESTORE_DATA)
			JumpToKingdomLocation(Kingdom, StartX, y);
		for (int x = StartX; x <= EndX; x+=10)
		{
			//try to jump directly to a location where 
			if (RestoreK != COULD_NOT_LOAD_RESTORE_DATA)
			{
				if (( x != RestoreX || y != RestoreY ) && x == 0)
				{
					y = RestoreY;
					x = RestoreX;
					JumpToKingdomLocation(69, RestoreX, RestoreY);
				}
				RestoreK = COULD_NOT_LOAD_RESTORE_DATA;
			}

			int End = GetTimeTickI();
			printf("We made %d slides. We should be at x = %d. Time spent so far %d\n", x, x, (End - Start) / 1000 / 60);

			CurPlayer.x = 0;
			CaptureVisibleScreenGetPlayerLabels(Kingdom, x, y);

			//if we found a castle, check if we are on the same screen as expected. Resync to expected location in case we clicked on an army or something
			if (CurPlayer.x > 0 && CurPlayer.y > 0 && (abs(CurPlayer.x - x) > RESYNC_ON_X_DIFF || abs(CurPlayer.y - y) > RESYNC_ON_Y_DIFF))
			{
				printf("\nWe are expecting to be at %d,%d, but we are at %d,%d?. Resync location\n\n", x, y, CurPlayer.x, CurPlayer.y);
				JumpToKingdomLocation(69, x, y);
			}
			else
			{
				DragScreenToLeft(); // we function as expected, we can simply drag the screen to the left
//				Sleep(200);
			}

			//in case we close the program for some reason we could resume next time
			SaveKingdomScanStatus(Kingdom, x, y);

			//safety break from a possible infinite loop
			if (GetAsyncKeyState(VK_INSERT))
				return;

			//pause until we get focus again. Might be required when we want to do something in the background
			WaitKoPlayerGetFocus();
		}
	}
}

void OfflineTestCastlePopupParsing()
{
#ifdef TEST_OFFLINE_PARSING_OF_PICTURES
	memset(Ko, 0, sizeof(Ko));
	TakeScreenshot(0, 0, 401, 381);
//	std::string path = "h:/Lords/CastlepopupExamples9";
	std::string path = "CastlepopupExamples11";
	std::string search_path = path;
	search_path += "/*.*";
	std::string SkipUntilFile = "";
	int FoundFirstFile = SkipUntilFile.length() == 0;
	int SkipFirstN = 1500 * 0;
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
//			printf("%d)Parsing file : %s\n", Index, FullPath);
			LoadCacheOverScreenshot(FullPath, 0, 0);
			//test it
			ParseCastlePopup();
			if (CurPlayer.SkipSave == 1)
			{
				SaveScreenshot();
				printf("%d)Issue with file : %s\n", Index, FullPath);
				LoadCacheOverScreenshot(FullPath, 0, 0);
				SaveScreenshot();
			}
		}
	} while (::FindNextFile(hFind, &fd) && BatchProcessMaxCount > 0);
	::FindClose(hFind);
#endif
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
/*	{
		ParseProfileInfo = 1;
		ScanKingdomArea(69, 0, 0, 10, 10);
	}/**/
/*	{
		GetKoPlayerAndPos();
		WaitKoPlayerGetFocus();
		DragScreenToRight();
		return;
	}/**/
/*	{
		GetKoPlayerAndPos();
		WaitKoPlayerGetFocus();
		ParseCastlePopup();
		return;
	}/**/
/*	{
		GetKoPlayerAndPos();
		WaitKoPlayerGetFocus();
		GetProfileInfo();
		return;
	}/**/
/*	{
		GetKoPlayerAndPos();
		WaitKoPlayerGetFocus();
		MouseDrag(Ko[0] + Ko[2] / 2, Ko[1] + Ko[3] / 2 + 100, Ko[0] + Ko[2] / 2, Ko[1] + Ko[3] / 3 - 2);
		MouseDrag(Ko[0] + Ko[2] / 2, Ko[1] + Ko[3] / 3 - 2, Ko[0] + Ko[2] / 2, Ko[1] + Ko[3] / 3);
		WaitScreeenDragFinish();
		return;
	}/**/
/*	{
		GetKoPlayerAndPos();
		WaitKoPlayerGetFocus();
		ParseProfileInfo = ParseProfileInfo2 = 1;
		CaptureVisibleScreenGetPlayerLabels(-1, 354, 689);
		return;
	}/**/
/*	{
		GetKoPlayerAndPos();
		WaitKoPlayerGetFocus();
		GetPlayerProfileInfo1();
		return;
	}/**/
/*	{
		OfflineTestCastlePopupParsing();
		return;
	}/**/
	int Kingdom = 67, StartX = 0, StartY = 0, EndX = 500, EndY = 1000;
	FILE *f;
	errno_t er = fopen_s(&f, "ScanParams.txt", "rt");
	if (f)
	{
		fscanf_s(f, "%d\n", &Kingdom);
		fscanf_s(f, "%d\n", &StartX);
		fscanf_s(f, "%d\n", &StartY);
		fscanf_s(f, "%d\n", &EndX);
		fscanf_s(f, "%d\n", &EndY); 
		fscanf_s(f, "%d\n", &ParseCastleInfo);
		fscanf_s(f, "%d\n", &ParseProfileInfo);
		fscanf_s(f, "%d\n", &ParseProfileInfo2);
		fclose(f);
		if (ParseProfileInfo)
		{
			printf("advanced profile info parsing is enabled\n");
			if (ParseProfileInfo2)
				printf("Second level advanced profile info scanning is enabled\n");
		}
	}
	// aprox 7 mins / row
	// 40 * 50 in 35 mins => 57 screens / min
	// 9 row in 77 minutes
	ScanKingdomArea(Kingdom, StartX, StartY, EndX, EndY);

	printf("fliptablegoinghome.THE END\n");
	_getch();
}

#endif