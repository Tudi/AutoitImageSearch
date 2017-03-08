#include "../stdafx.h"

void GetProfileInfo_3()
{
	if (ParseProfileInfo2 == 0)
		return;
	//open advanced player info
	KoLeftClick(220, 125);
	//wait a bit than close it
}

void GetProfileInfo_2()
{
	if (ParseProfileInfo == 0 && ParseProfileInfo2 == 0)
		return;
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
	printf("End parsing player info\n");
	//		Sleep(300);	// game tends to slow down over time. We need to add some dynamic sleep until loading finishes
}

void ParseCastlePopup2()
{
	//this is the popup that happens when we click on a castle
/*	if (WaitPixelBecomeColor(566, 270, 0x00FFFFFF) == 0 && WaitPixelBecomeColor(569, 301, 0x00FFFFFF) == 0)
	{
		printf("Castle popup load timemout. Skipping parsing\n");
		return;
	}*/
	//must be last thing as we are changing popup windows
}

void CaptureVisibleScreenGetPlayerLabels2(int ExpectedKingdom, int ExpectedX, int ExpectedY)
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
	KeepGradient3(RGB(33, 109, 148), 0.12f, RGB(16, 77, 113), 0.4f, RGB(40, 116, 155), 0.20f);
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
		}
		else if (IsPixelAtPos(613, 500, STATIC_BGR_RGB(0x00337677)) == 1 || IsPixelAtPos(614, 495, STATIC_BGR_RGB(0x00A54A4F)) == 1) // can send resource or can be scouted
		{
			printf("Found castle at %d %d\n", x, y);
			// parse if castle
			ParseCastlePopup2();
			GetProfileInfo_2(); // in case it was wanted to obtain player level
			GetProfileInfo_3(); // advanced statistics like death count...
		}
		else
		{
			printf("We clicked on something but have no idea what at %d %d. Ratio is %d\n", x, y, SearchResultXYSAD[i][2]);
		}

		//safety break from a possible infinite loop
		if (GetAsyncKeyState(VK_INSERT))
			break;

		//pause until we get focus again. Might be required when we want to do something in the background
		WaitKoPlayerGetFocus();
	}

	//ake sure there are no popups, so drag can work it's magic
	CloseAllPossiblePopups();

}

void ScanKingdomArea2(int Kingdom, int StartX, int StartY, int EndX, int EndY)
{
	GetKoPlayerAndPos();
	WaitKoPlayerGetFocus();
	CloseAllPossiblePopups();

	int RestoreK, RestoreX, RestoreY;
	RestoreKingdomScanStatus(RestoreK, RestoreX, RestoreY);

	int StepY = 10;
	if (StartY > EndY)
		StepY = -StepY;
	StartCounter();
	int Start = GetTimeTickI();
	for (int y = StartY; (StepY < 0 && y >= EndY) || (StepY > 0 && y <= EndY); y += StepY)
	{
		if (RestoreK == COULD_NOT_LOAD_RESTORE_DATA)
			JumpToKingdomLocation(Kingdom, StartX, y);
		for (int x = StartX; x <= EndX; x += 10)
		{
			//try to jump directly to a location where 
			if (RestoreK != COULD_NOT_LOAD_RESTORE_DATA)
			{
				if ((x != RestoreX || y != RestoreY) && x == 0)
				{
					y = RestoreY;
					x = RestoreX;
					JumpToKingdomLocation(69, RestoreX, RestoreY);
				}
				RestoreK = COULD_NOT_LOAD_RESTORE_DATA;
			}

			int End = GetTimeTickI();
			printf("We made %d slides. We should be at x = %d. Time spent so far %d\n", x, x, (End - Start) / 1000 / 60);

			CaptureVisibleScreenGetPlayerLabels2(Kingdom, x, y);

			DragScreenToLeft(); // we function as expected, we can simply drag the screen to the left

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

void RunLordsMobileTestsNoOCR()
{
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
	ScanKingdomArea2(Kingdom, StartX, StartY, EndX, EndY);

	printf("fliptablegoinghome.THE END\n");
	_getch();
}