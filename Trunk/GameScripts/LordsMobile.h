#pragma once

//#define TEST_OFFLINE_PARSING_OF_PICTURES
#define COULD_NOT_LOAD_RESTORE_DATA -2

void RunLordsMobileTests();
void RunLordsMobileTestsNoOCR();

void ScanKingdomArea(int Kingdom, int StartX, int StartY, int EndX, int EndY);
void ResetKingdomSaveStatus();
void RestoreKingdomScanStatus(int &k, int &x, int &y);
void SaveKingdomScanStatus(int k, int x, int y);
void WaitScreeenDragFinish();
void JumpToKingdomLocation(int Kingdom, int x, int y);
void EnterTeleportCoord(int Coord);
void EnterTeleportCordDigit(int Digit);
void AssembleNumber(char *res, int &Ind, int &N);
void CloseAllPossiblePopups();
int WaitPixelChangeColor(int x, int y, COLORREF Color, int Timeout = 3000);
int WaitPixelBecomeColor(int x, int y, COLORREF Color);
void WaitKoPlayerGetFocus();
void GetKoPlayerAndPos();
int IsPixelAtPos(int x, int y, COLORREF Color);
void KoLeftClick(int x, int y);
int CloseGenericPopup(int x, int y, int color, int SleepFor = 100, int Timeout = 3000);
COLORREF GetKoPixel(int x, int y);
void DragScreenToLeft();

struct StorablePlayerInfo
{
	int SkipSave;
	int k, x, y;
	int Might;
	int Kills;
	char Name[500];
	char Guild[500];
	time_t LastUpdateTimestamp;
	// to be comming soon
	time_t IsBurning;
	time_t HasPrisoners;
	int CastleLevel;
	int PlayerLevel;
	//char PlayerNick[100];
	int VIPLevel;
	int GuildRank;
	int SuccessfulAttacks;
	int FailedAttacks;
	int SuccessfulDefenses;
	int FailedDefenses;
	int TroopsKilled;
	int TroopsLost;
	int TroopsHealed;
	int TroopsWounded;
	int TurfsDestroyed;
	int TurfsLost;
	int MightDestroyed;

	int ResourceGathered;
	int ColloseumRank;
	int ColloseumBestRank;
	int ColloseumWonRank;
	int Bounty;
	int IsShielded;
};

struct StorableResourceTileInfo
{
	int Type;
	int Level;
	int RemainingRss;
	char PlayerName[100];
	time_t LastUpdateTimestamp;
};

extern StorablePlayerInfo CurPlayer;
extern StorableResourceTileInfo CurRss;
extern int ParseProfileInfo;
extern int ParseProfileInfo2;
extern int Ko[4];