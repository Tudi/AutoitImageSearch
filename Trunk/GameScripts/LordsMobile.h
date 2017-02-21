#pragma once

//#define TEST_OFFLINE_PARSING_OF_PICTURES

void RunLordsMobileTests();

struct StorablePlayerInfo
{
	int SkipSave;
	int k, x, y;
	int Might;
	int Kills;
	char Name[100];
	char Guild[100];
	int LastUpdateTimestamp;
	// to be comming soon
	int IsBurning;
	int HasPrisoners;
	int CastleLevel;
	int PlayerLevel;
	//char PlayerNick[100];
	int VIP;
	int SuccessfulAttacks;
	int FailedAttacks;
	int SuccessfulDefenses;
	int FailedDefenses;
	int TroopsKilled;
	int TroopsLost;
	int TroopsHealed;
	int TurfsDestroyed;
	int TurfsLost;
	int MightDestroyed;
	int ResourceGathered;
	int ColloseumRank;
	int ColloseumBestRank;
	int ColloseumWonRank;
	int Bounty;
};

struct StorableResourceTileInfo
{
	int Type;
	int Level;
	int RemainingRss;
	char PlayerName[100];
};

extern StorablePlayerInfo CurPlayer;
