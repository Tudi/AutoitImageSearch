#include "../stdafx.h"

void DetectPlayerShields()
{
/*	{
		//try to detect player shields
		TakeScreenshot(0, 0, 1025, 599);
		//		LoadCacheOverScreenshot("Screenshot_0015_1025_0599.bmp", 0, 0);
		LoadCacheOverScreenshot("Screenshot_0025_1025_0599.bmp", 0, 0);
		//KeepGradient(RGB(216, 215, 249), 2.5f);
		KeepColorsMinInRegion(-1, -1, -1, -1, RGB(216, 215, 249));
		SaveScreenshot();
		return;
	}/**/
}
void LocateAndRemoveWaterAndDetectPlayers()
{
/*	{
		//try to remove water zones
		TakeScreenshot(0, 0, 1025, 599);
		LoadCacheOverScreenshot("WaterExamples/Screenshot_0012_1025_0599.bmp", 0, 0);
		GetUniqueColorsInRegion(675, 254, 747, 348);
		SetGradientToColor(0x00A59B63, 0.162f, TRANSPARENT_COLOR);
		//KeepColorsMaxInRegion(-1, -1, -1, -1, RGB(99, 166, 181));
		SaveScreenshot();
		//check if we can still see player labels
		KeepGradient(RGB(33, 106, 148), 0.4f);
		char * tt = ImageSearch_Multiple_PixelCount(0, 50, 33, 21);
		printf("t = %s\n", tt);
		SaveScreenshot();
		return;
	}/**/

	/*	{
	//try to detect player labels
	TakeScreenshot(0, 0, 1282, 722);
	//		LoadCacheOverScreenshot("Screenshot_0015_1025_0599.bmp", 0, 0);
	//		LoadCacheOverScreenshot("Screenshot_0025_1025_0599.bmp", 0, 0);
	//		LoadCacheOverScreenshot("CastleTags1200.bmp", 0, 0);
	TakeScreenshot(0, 0, 1062, 500);
	LoadCacheOverScreenshot("CastleTags_4.bmp", 0, 0);
	//		char * ttt = ImageSearch_Multiple_Gradient(RGB(33, 106, 148), 60, 75, 181 - 151, 235 - 220);
	//remove water if there is any
	SetGradientToColor(0x00A59B63, 0.162f, TRANSPARENT_COLOR);
	SaveScreenshot();
	//remove anything else left than player tags
	KeepGradient(RGB(33, 109, 148), 0.4f);
	SaveScreenshot();
	//		float t = GetPixelRatioInArea(0, 311, 522, 345, 539);
	//		printf("t = %f\n", t);
	//return;
	//		char * tt = ImageSearch_Multiple_PixelCount(0, 75, 181 - 151, 235 - 220); // found less than half shielded
	char * tt = ImageSearch_Multipass_PixelCount(0, 60, 35, 5, 34, 21); // try to find good matches, than try to find worse and worse matches
	printf("t = %s\n", tt);
	SaveScreenshot();
	return;

	PushToColorKeepList(0x406060);
	PushToColorKeepList(0x406080);
	PushToColorKeepList(0x206080);
	PushToColorKeepList(0x204060);
	PushToColorKeepList(0x204040);
	PushToColorKeepList(0x204080);
	PushToColorKeepList(0x206060);
	PushToColorKeepList(0x2060A0);
	PushToColorKeepList(0x004060);
	PushToColorKeepList(0x004080);
	PushToColorKeepList(0x006060);
	PushToColorKeepList(0x006080);
	LoadCacheOverScreenshot("Screenshot_0015_1025_0599.bmp", 0, 0);
	ApplyColorBitmask(0x00E0E0E0);
	//SaveScreenshot();
	//KeepColor3SetBoth(0, 0x00FFFFFF, 0x206080, 0x4080, 0x4060);
	ApplyColorKeepList(0x00FFFFFF, 0);
	SaveScreenshot();

	LoadCacheOverScreenshot("Screenshot_0015_1025_0599.bmp", 0, 0);
	ApplyColorBitmask(0x00C0C0C0);
	//SaveScreenshot();
	KeepColor3SetBoth(0, 0x00FFFFFF, 0x00004080, 0x00004040, 0x00408080);
	//SaveScreenshot();

	LoadCacheOverScreenshot("Screenshot_0015_1025_0599.bmp", 0, 0);
	ApplyColorBitmask(0x00808080);
	//SaveScreenshot();
	KeepColor3SetBoth(0, 0x00FFFFFF, 0x00000080, 0x00008080, 0x00008080);
	//SaveScreenshot();
	}/**/
}

void ExtractPlayerName()
{
/*	{
		char *res;
		//OCR_LoadFontsFromFile("K_C_M_FontMap.txt");
		OCR_LoadFontsFromDir("K_C_M", "KCM_");
		OCR_LoadFontsFromDir("K_C_M_Old1", "K_C_M_");
		OCR_LoadFontsFromDir("K_C_M_old2", "KCM_");
		OCR_LoadFontsFromDir("K_C_M_try3", "KCM_");
		TakeScreenshot(0, 0, 1025, 599);
		OCR_SetMaxFontSize(20, 20);
		//LoadCacheOverScreenshot("CastlepopupExamples/Screenshot_0021_1025_0599.bmp", 0, 0);
		//LoadCacheOverScreenshot("CastlepopupExamples/Screenshot_0024_1025_0599.bmp", 0, 0);
		//LoadCacheOverScreenshot("CastlepopupExamples/Screenshot_0042_1025_0599.bmp", 0, 0);
		//LoadCacheOverScreenshot("CastlepopupExamples/Screenshot_0045_1025_0599.bmp", 0, 0);
		//LoadCacheOverScreenshot("CastlepopupExamples/Screenshot_0048_1025_0599.bmp", 0, 0);
		//LoadCacheOverScreenshot("CastlepopupExamples/Screenshot_0051_1025_0599.bmp", 0, 0);
		//SaveScreenshot();
//		KeepGradientRegion(RGB(239, 222, 65), 0.1f, 446, 182, 680, 205);
//		KeepGradientRegion(RGB(237, 237, 237), 0.015f, 405, 223, 680, 295);
//		KeepGradientRegion(RGB(237, 237, 237), 0.03f, 502, 469, 570, 482);
//		SaveScreenshot();
		//		KeepColorsMinInRegion(446, 182, 680, 205, 144, 133, 41);
		KeepColorsMinInRegion(446, 181, 680, 205, RGB(171, 160, 49));
		//SaveScreenshot();
#if 0==1
		//this was too low, 'ffi' gets merged into 1 char 
		KeepColorsMinInRegion(446, 223, 680, 241, RGB(154, 158, 159));
		KeepColorsMinInRegion(446, 249, 680, 265, RGB(154, 158, 159));
		KeepColorsMinInRegion(405, 276, 680, 295, RGB(154, 158, 159));
		KeepColorsMinInRegion(502, 469, 529, 482, RGB(154, 158, 159));
		KeepColorsMinInRegion(543, 469, 570, 482, RGB(154, 158, 159));
#elif 0==2
		KeepColorsMinInRegion(446, 223, 680, 240, RGB(182, 185, 186)); // might
		KeepColorsMinInRegion(446, 249, 680, 265, RGB(182, 185, 186)); // kills
		KeepColorsMinInRegion(405, 276, 680, 295, RGB(182, 185, 186)); // guild
		KeepColorsMinInRegion(502, 469, 529, 482, RGB(182, 185, 186)); // x
		KeepColorsMinInRegion(543, 469, 570, 482, RGB(182, 185, 186)); // y
#elif 22==22
		KeepColorsMinInRegion(446, 223, 680, 240, RGB(155, 158, 159)); // might
		KeepColorsMinInRegion(446, 249, 680, 265, RGB(155, 158, 159)); // kills
		KeepColorsMinInRegion(405, 276, 680, 295, RGB(155, 158, 159)); // guild
		KeepColorsMinInRegion(502, 469, 529, 482, RGB(155, 158, 159)); // x
		KeepColorsMinInRegion(543, 469, 570, 482, RGB(155, 158, 159)); // y
#elif 0==3
		KeepGradientRegion(RGB(155, 159, 162), 0.04f, 446, 223, 680, 241); // might
		KeepGradientRegion(RGB(155, 159, 162), 0.04f, 446, 249, 680, 266); // kills
		KeepGradientRegion(RGB(155, 159, 162), 0.04f, 405, 276, 680, 295); // guild
		KeepGradientRegion(RGB(155, 159, 162), 0.04f, 502, 469, 529, 482); // x
		KeepGradientRegion(RGB(155, 159, 162), 0.04f, 543, 469, 570, 482); // y
#elif 0==4
		KeepGradientRegionMinValue(RGB(172, 173, 174), 0.02f, 446, 223, 680, 241); // might
		KeepGradientRegionMinValue(RGB(172, 173, 174), 0.02f, 446, 249, 680, 266); // kills
		KeepGradientRegionMinValue(RGB(172, 173, 174), 0.02f, 405, 276, 680, 295); // guild
		KeepGradientRegionMinValue(RGB(172, 173, 174), 0.02f, 502, 469, 529, 482); // x
		KeepGradientRegionMinValue(RGB(172, 173, 174), 0.02f, 543, 469, 570, 482); // y
#elif 0==5
		KeepGradientRegionMinValue(RGB(155, 158, 161), 0.03f, 446, 223, 680, 241); // might
		KeepGradientRegionMinValue(RGB(155, 158, 161), 0.03f, 446, 249, 680, 266); // kills
		KeepGradientRegionMinValue(RGB(155, 158, 161), 0.03f, 405, 276, 680, 295); // guild
		KeepGradientRegionMinValue(RGB(155, 158, 161), 0.03f, 502, 469, 529, 482); // x
		KeepGradientRegionMinValue(RGB(155, 158, 161), 0.03f, 543, 469, 570, 482); // y
#elif 0==6
		RemoveIfHasStrongerNeighbours(3, 446, 223, 680, 241); // might
		RemoveIfHasStrongerNeighbours(3, 446, 249, 680, 266); // kills
		RemoveIfHasStrongerNeighbours(3, 405, 276, 680, 295); // guild
		RemoveIfHasStrongerNeighbours(3, 502, 469, 529, 482); // x
		RemoveIfHasStrongerNeighbours(3, 543, 469, 570, 482); // y
#elif 0==7
		ApplyColorBitmask(0x00F0F0F0);
		ErodeNotInLine(446, 223, 680, 241); // might
		ErodeNotInLine(446, 249, 680, 266); // kills
		ErodeNotInLine(405, 276, 680, 295); // guild
		ErodeNotInLine(502, 469, 529, 482); // x
		ErodeNotInLine(543, 469, 570, 482); // y
#elif 0==8
		// fonts fade out to black. We want to remove pixel that is close to black, but a pixel that would not break a line inside the font
		// remove non font color. Leave some very wide fonts
		// keep eroding them without breaking lines
		ErodeOnEdgeNeighbours(446, 223, 680, 241); // might
		ErodeOnEdgeNeighbours(446, 249, 680, 266); // kills
		ErodeOnEdgeNeighbours(405, 276, 680, 295); // guild
		ErodeOnEdgeNeighbours(502, 469, 529, 482); // x
		ErodeOnEdgeNeighbours(543, 469, 570, 482); // y
#endif
		//SaveScreenshot();
		//return;
		//ConvertToGrayScale();
		//DecreaseColorCount(1);
		//SaveScreenshot();
		//SaveScreenshot();
		//GetUniqueColorsInRegion(502, 223, 580, 237);
		//KeepColorSetRest(0x00000000, 0x00FFFFFF, 0x00000000);
		//SaveScreenshot();
		//ErrodeRegionToTransparent();
		//SaveScreenshot();
		//read name
		res = OCR_ReadTextLeftToRightSaveUnknownChars(446, 181, 680, 205);
		if (res != NULL)
		{
		printf("Name : %s\n", res);
		}
		//read might
		res = OCR_ReadTextLeftToRightSaveUnknownChars(502, 223, 680, 241);
		if (res != NULL)
		{
		printf("might : %s\n", res);
		RemoveCharFromNumberString(res, ',');
		printf("might : %s\n", res);
		}
		//read troops killed
		res = OCR_ReadTextLeftToRightSaveUnknownChars(554, 249, 680, 265);
		if (res != NULL)
		{
		printf("kills : %s\n", res);
		RemoveCharFromNumberString(res, ',');
		printf("kills : %s\n", res);
		}
		//read guild
		res = OCR_ReadTextLeftToRightSaveUnknownChars(405, 276, 680, 295);
		if (res != NULL)
		{
		printf("guild : %s\n", res);
		}
		//read location X
		res = OCR_ReadTextLeftToRightSaveUnknownChars(502, 469, 529, 482);
		if (res != NULL)
		{
		printf("x : %s\n", res);
		}
		//read location y
		res = OCR_ReadTextLeftToRightSaveUnknownChars(543, 469, 570, 482);
		if (res != NULL)
		{
		printf("y : %s\n", res);
		}
		return;
	}/**/

/*	{
		char *res;
		//OCR_LoadFontsFromFile("K_C_M_FontMap.txt");
		OCR_LoadFontsFromDir("K_C_M", "KCM_");
		TakeScreenshot(0, 0, 1025, 599);
		OCR_SetMaxFontSize(20, 20);
//		LoadCacheOverScreenshot("CastlepopupExamples/Screenshot_0036_0280_0325.bmp", 0, 0);
		LoadCacheOverScreenshot("CastlepopupExamples/Screenshot_0034_0280_0325.bmp", 0, 0);
		//read name
		res = OCR_ReadTextLeftToRightSaveUnknownChars(446 - 400, 182 - 165, 680 - 400, 205 - 165);
		printf("Name : %s\n", res);
		//read might
		res = OCR_ReadTextLeftToRightSaveUnknownChars(502 - 400, 223 - 165, 680 - 400, 240 - 165);
		printf("might : %s\n", res);
		RemoveCharFromNumberString(res, ',');
		printf("might : %s\n", res);
		//read troops killed
		res = OCR_ReadTextLeftToRightSaveUnknownChars(554 - 400, 249 - 165, 680 - 400, 265 - 165);
		printf("kills : %s\n", res);
		RemoveCharFromNumberString(res, ',');
		printf("kills : %s\n", res);
		//read guild
		res = OCR_ReadTextLeftToRightSaveUnknownChars(405 - 400, 276 - 165, 680 - 400, 295 - 165);
		printf("guild : %s\n", res);
		//read location X
		res = OCR_ReadTextLeftToRightSaveUnknownChars(502 - 400, 469 - 165, 529 - 400, 482 - 165);
		printf("x : %s\n", res);
		//read location y
		res = OCR_ReadTextLeftToRightSaveUnknownChars(543 - 400, 469 - 165, 570 - 400, 482 - 165);
		printf("y : %s\n", res);
	}/**/
/*	{
		char *res;
		TakeScreenshot(0, 0, 700, 700);
		OCR_SetMaxFontSize(20, 20);
		std::string path = "OCRLowresInput";
		std::string search_path = path;
		search_path += "/*.*";
		std::string SkipUntilFile = "";
		int FoundFirstFile = SkipUntilFile.length() == 0;
		int SkipFirstN = 0;
		int BatchProcessMaxCount = 200;
		int Index = 0;
		WIN32_FIND_DATA fd;
		HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
		if (hFind != INVALID_HANDLE_VALUE)
		{
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
//					if ((Index & 1) == 0)
//						continue;
					char FullPath[2500];
					sprintf_s(FullPath, sizeof(FullPath), "%s/%s", path.c_str(), fd.cFileName);
					printf("%d)Parsing file : %s\n", Index, FullPath);
					LoadCacheOverScreenshot(FullPath, 0, 0);
					//SaveScreenshot();
					//continue;
					//SaveScreenshot();
					//KeepColorsMinInRegion(446 - 400, 181 - 165, 680 - 400, 205 - 165, RGB(171, 160, 49));
					KeepColorsMinInRegion(369, 183, 650, 405, RGB(150, 151, 154));

					//SaveScreenshot();
					//continue;
					//res = OCR_ReadTextLeftToRightSaveUnknownChars(446 - 400, 181 - 165, 680 - 400, 205 - 165);
					//printf("%s\n", res);
					res = OCR_ReadTextLeftToRightSaveUnknownChars(406, 235, 630, 251);
					printf("%s\n", res);
				}
			} while (::FindNextFile(hFind, &fd) && BatchProcessMaxCount > 0);
			::FindClose(hFind);
		}

		//SaveScreenshot();
		return;
	}/**/
/*	{
		char *res;
		TakeScreenshot(0, 0, 1910, 1100);
		OCR_SetMaxFontSize(20, 20);
		std::string path = "OCRHighResInput";
		std::string search_path = path;
		search_path += "/*.*";
		std::string SkipUntilFile = "";
		int FoundFirstFile = SkipUntilFile.length() == 0;
		int SkipFirstN = 0;
		int BatchProcessMaxCount = 200;
		int Index = 0;
		WIN32_FIND_DATA fd;
		HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
		if (hFind != INVALID_HANDLE_VALUE)
		{
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
					//					if ((Index & 1) == 0)
					//						continue;
					char FullPath[2500];
					sprintf_s(FullPath, sizeof(FullPath), "%s/%s", path.c_str(), fd.cFileName);
					printf("%d)Parsing file : %s\n", Index, FullPath);
					LoadCacheOverScreenshot(FullPath, 0, 0);
					//SaveScreenshot();
					//continue;
					//SaveScreenshot();
					//KeepColorsMinInRegion(446 - 400, 181 - 165, 680 - 400, 205 - 165, RGB(171, 160, 49));
					KeepColorsMinInRegion(645, 355, 1195, 810, RGB(201, 201, 203));

					//SaveScreenshot();
					//continue;
					//res = OCR_ReadTextLeftToRightSaveUnknownChars(446 - 400, 181 - 165, 680 - 400, 205 - 165);
					//printf("%s\n", res);
					res = OCR_ReadTextLeftToRightSaveUnknownChars(722, 461, 1190, 490);
					printf("%s\n", res);
				}
			} while (::FindNextFile(hFind, &fd) && BatchProcessMaxCount > 0);
			::FindClose(hFind);
		}

		//SaveScreenshot();
		return;
	}/**/
	{
		char *res;
		OCR_SetActiveFontSet(2, "K_C_M_Playernames");
		OCR_LoadFontsFromDir("K_C_M_Playernames", "KCM_");
		OCR_LoadFontsFromDir("K_C_M_Playernames2", "KCM_");
		OCR_LoadFontsFromDir("K_C_M_Playernames3", "KCM_");
		TakeScreenshot(0, 0, 401, 381);
		OCR_SetMaxFontSize(20, 20);
		std::string path = "CastlepopupExamples5";
		std::string search_path = path;
		search_path += "/*.*";
		std::string SkipUntilFile = "";
		int FoundFirstFile = SkipUntilFile.length() == 0;
		int SkipFirstN = 0;
		int BatchProcessMaxCount = 1010;
		int Index = 0;
		WIN32_FIND_DATA fd;
		HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
		if (hFind != INVALID_HANDLE_VALUE)
		{
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
					char FullPath[2500];
					sprintf_s(FullPath, sizeof(FullPath), "%s/%s", path.c_str(), fd.cFileName);
					//printf("%d)Parsing file : %s\n", Index, FullPath);
					LoadCacheOverScreenshot(FullPath, 0, 0);
					//SaveScreenshot();
					//KeepColorsMinInRegion(121, 16, 390, 44, RGB(194, 180, 55));
					KeepColorsMinInRegion(121, 16, 390, 44, RGB(178, 165, 50));
					//SaveScreenshot();
					res = OCR_ReadTextLeftToRightSaveUnknownChars(121, 16, 390, 44);
					printf("%s\n", res);
					if (OCR_FoundNewFont > 0)
					{
						printf("Original Filename was %s\n", FullPath);
						SaveScreenshot();
					}
				}
			} while (::FindNextFile(hFind, &fd) && BatchProcessMaxCount > 0);
			::FindClose(hFind);
		}

		//SaveScreenshot();
		return;
	}/**/
}

void ExtractKillsMight()
{
	{
		char *res;
		OCR_SetActiveFontSet(4, "K_C_M_MightKills");
		OCR_LoadFontsFromDir("K_C_M_MightKills", "KCM_");
		OCR_LoadFontsFromDir("K_C_M_MightKills2", "KCM_");
		TakeScreenshot(0, 0, 401, 381);
		OCR_SetMaxFontSize(20, 20);
		std::string path = "CastlepopupExamples5";
		std::string search_path = path;
		search_path += "/*.*";
		std::string SkipUntilFile = "";
		int FoundFirstFile = SkipUntilFile.length() == 0;
		int SkipFirstN = 0;
		int BatchProcessMaxCount = 1010;
		int Index = 0;
		WIN32_FIND_DATA fd;
		HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
		if (hFind != INVALID_HANDLE_VALUE)
		{
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
					char FullPath[2500];
					sprintf_s(FullPath, sizeof(FullPath), "%s/%s", path.c_str(), fd.cFileName);
					printf("%d)Parsing file : %s\n", Index, FullPath);
					LoadCacheOverScreenshot(FullPath, 0, 0);
					//continue;
					//SaveScreenshot();
					KeepColorsMinInRegion(193, 66, 350, 88, RGB(211, 211, 211));
					KeepColorsMinInRegion(258, 97, 390, 119, RGB(211, 211, 211));
					//SaveScreenshot();
					//continue;
					int tOCR_FoundNewFont = 0;
					res = OCR_ReadTextLeftToRightSaveUnknownChars(193, 66, 350, 88);
					tOCR_FoundNewFont += OCR_FoundNewFont;
					printf("%s\n", res);
					res = OCR_ReadTextLeftToRightSaveUnknownChars(258, 97, 390, 119);
					tOCR_FoundNewFont += OCR_FoundNewFont;
					printf("%s\n", res);
					if (tOCR_FoundNewFont > 0)
						SaveScreenshot();
				}
			} while (::FindNextFile(hFind, &fd) && BatchProcessMaxCount > 0);
			::FindClose(hFind);
		}

		//SaveScreenshot();
		return;
	}/**/
}

void ExtractXY()
{
	{
		char *res;
		OCR_SetActiveFontSet(3, "K_C_M_xy");
		OCR_LoadFontsFromDir("K_C_M_xy", "KCM_");
		OCR_LoadFontsFromDir("K_C_M_xy2", "KCM_");
		TakeScreenshot(0, 0, 401, 381);
		OCR_SetMaxFontSize(20, 20);
		std::string path = "CastlepopupExamples5";
		std::string search_path = path;
		search_path += "/*.*";
		std::string SkipUntilFile = "";
		int FoundFirstFile = SkipUntilFile.length() == 0;
		int SkipFirstN = 0;
		int BatchProcessMaxCount = 1010;
		int Index = 0;
		WIN32_FIND_DATA fd;
		HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
		if (hFind != INVALID_HANDLE_VALUE)
		{
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
					char FullPath[2500];
					sprintf_s(FullPath, sizeof(FullPath), "%s/%s", path.c_str(), fd.cFileName);
					printf("%d)Parsing file : %s\n", Index, FullPath);
					LoadCacheOverScreenshot(FullPath, 0, 0);
					//SaveScreenshot();
					KeepColorsMinInRegion(139, 363, 266, 379, RGB(166, 172, 175));
					//SaveScreenshot();
					res = OCR_ReadTextLeftToRightSaveUnknownChars(139, 363, 266, 379);
					printf("%s\n", res);
					if (OCR_FoundNewFont == 1)
						SaveScreenshot();
				}
			} while (::FindNextFile(hFind, &fd) && BatchProcessMaxCount > 0);
			::FindClose(hFind);
		}
		//SaveScreenshot();
		return;
	}/**/
}

void ExtractGuild()
{
/*	{
		char *res;
		OCR_LoadFontsFromDir("K_C_M", "KCM_");
		OCR_LoadFontsFromDir("K_C_M_Old1", "K_C_M_");
		OCR_LoadFontsFromDir("K_C_M_old2", "KCM_");
		OCR_LoadFontsFromDir("K_C_M_try3", "KCM_");
		OCR_LoadFontsFromDir("K_C_M_try4", "KCM_");
		OCR_LoadFontsFromDir("K_C_M_try5", "KCM_");
		OCR_LoadFontsFromDir("K_C_M_try6", "KCM_");
		OCR_LoadFontsFromDir("K_C_M_try7", "KCM_");
		TakeScreenshot(0, 0, 300, 350);
		OCR_SetMaxFontSize(20, 20);
		ResetColorKeepList();
		PushToColorKeepList(RGB(176, 179, 181));
		//PushToColorKeepList(RGB(173, 175, 176));
		//PushToColorKeepList(RGB(173, 176, 176));
		PushToColorKeepList(RGB(171, 174, 176));
		PushToColorKeepList(RGB(170, 175, 177));
		PushToColorKeepList(RGB(170, 174, 177));
		PushToColorKeepList(RGB(167, 170, 172));
		std::string path = "CastlepopupExamples2";
		std::string search_path = path;
		search_path += "/*.*";
		std::string SkipUntilFile = "";
		int FoundFirstFile = SkipUntilFile.length() == 0;
		int SkipFirstN = 0;
		int BatchProcessMaxCount = 520;
		int Index = 0;
		WIN32_FIND_DATA fd;
		HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
		if (hFind != INVALID_HANDLE_VALUE)
		{
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
					if ( ( Index & 1 ) == 0)
						continue;
					char FullPath[2500];
					sprintf_s(FullPath, sizeof(FullPath), "%s/%s", path.c_str(), fd.cFileName);
					printf("%d)Parsing file : %s\n", Index, FullPath);
					LoadCacheOverScreenshot(FullPath, 0, 0);
					//SaveScreenshot();
					//continue;
					//SaveScreenshot();
					KeepColorsMinInRegion(446 - 400, 181 - 165, 680 - 400, 205 - 165, RGB(171, 160, 49));
					//KeepColorsMinInRegion(446 - 400, 181 - 165, 680 - 400, 205 - 165, RGB(182, 170, 53));
					KeepColorsMinInRegion(500 - 400, 223 - 165, 680 - 400, 240 - 165, 0xA09E9A); // might
					KeepColorsMinInRegion(446 - 400, 249 - 165, 680 - 400, 265 - 165, 0xA09E9A); // kills
					ApplyColorEliminateListToArea(0, 405 - 400, 276 - 165, 680 - 400, 295 - 165);
					KeepColorsMinInRegion(405 - 400, 276 - 165, 680 - 400, 295 - 165, 0xA09E9A); // guild
					//ApplyColorEliminateListToArea(0, 502 - 400, 469 - 165, 529 - 400, 482 - 165);
					KeepColorsMinInRegion(502 - 400, 469 - 165, 529 - 400, 482 - 165, 0xA09E9A); // x
					KeepColorsMinInRegion(543 - 400, 469 - 165, 570 - 400, 482 - 165, 0xA09E9A); // y
			//SaveScreenshot();
			//continue;
					res = OCR_ReadTextLeftToRightSaveUnknownChars(446 - 400, 181 - 165, 680 - 400, 205 - 165);
					printf("%s\n", res);
					res = OCR_ReadTextLeftToRightSaveUnknownChars(500 - 400, 223 - 165, 680 - 400, 240 - 165);
					printf("%s\n", res);
					res = OCR_ReadTextLeftToRightSaveUnknownChars(554 - 400, 249 - 165, 680 - 400, 265 - 165);
					printf("%s\n", res);
					res = OCR_ReadTextLeftToRightSaveUnknownChars(405 - 400, 276 - 165, 680 - 400, 295 - 165);
					printf("%s\n", res);
					res = OCR_ReadTextLeftToRightSaveUnknownChars(502 - 400, 469 - 165, 529 - 400, 482 - 165);
					printf("%s\n", res);
					res = OCR_ReadTextLeftToRightSaveUnknownChars(543 - 400, 469 - 165, 570 - 400, 482 - 165);
					printf("%s\n", res);
				}
			} while (::FindNextFile(hFind, &fd) && BatchProcessMaxCount > 0 );
			::FindClose(hFind);
		}

		//SaveScreenshot();
		return;
	}/**/

	{
		char *res;
		OCR_SetActiveFontSet(1, "K_C_M_guild");
		OCR_LoadFontsFromDir("K_C_M_guild", "KCM_");
		OCR_LoadFontsFromDir("K_C_M_guild2", "KCM_");
		TakeScreenshot(0, 0, 401, 381);
		OCR_SetMaxFontSize(20, 20);
		std::string path = "CastlepopupExamples5";
		std::string search_path = path;
		search_path += "/*.*";
		std::string SkipUntilFile = "";
		int FoundFirstFile = SkipUntilFile.length() == 0;
		int SkipFirstN = 0;
		int BatchProcessMaxCount = 1010;
		int Index = 0;
		WIN32_FIND_DATA fd;
		HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
		if (hFind != INVALID_HANDLE_VALUE)
		{
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
					char FullPath[2500];
					sprintf_s(FullPath, sizeof(FullPath), "%s/%s", path.c_str(), fd.cFileName);
					printf("%d)Parsing file : %s\n", Index, FullPath);
					LoadCacheOverScreenshot(FullPath, 0, 0);
					//SaveScreenshot();
					KeepColorsMinInRegion(73, 131, 370, 154, RGB(173, 174, 176));
					//SaveScreenshot();
					res = OCR_ReadTextLeftToRightSaveUnknownChars(73, 131, 370, 154);
					printf("%s\n", res);
					if (OCR_FoundNewFont > 0)
					{
						printf("Original Filename was %s\n", FullPath);
						SaveScreenshot();
					}
				}
			} while (::FindNextFile(hFind, &fd) && BatchProcessMaxCount > 0);
			::FindClose(hFind);
		}

		//SaveScreenshot();
		return;
	}/**/
}

void RunLordsTesting()
{
	//ExtractXY();
	//ExtractKillsMight();
	ExtractPlayerName();
	//ExtractGuild();
}