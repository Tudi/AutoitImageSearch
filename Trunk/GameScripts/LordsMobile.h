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
