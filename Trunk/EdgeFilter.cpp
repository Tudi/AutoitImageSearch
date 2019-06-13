#include "stdafx.h"
#include "EdgeFilter.h"
#include <list>

std::list<LineFilter*> LO_Stores;
LineFilter *LO_Active = NULL;

static signed char LinePoints[8][72][18];
char IsLinePointsInitialized = 0;
void InitLinePoints();

void LineFilter_Init(int LineFilterPixelsIndex)
{
	//do we have an active store for this object ?
	for(auto itr = LO_Stores.begin(); itr != LO_Stores.end(); itr++)
		if ((*itr)->MyIndex == LineFilterPixelsIndex)
		{
			LO_Active = (*itr);
			return;
		}
	LO_Active = new LineFilter();
	LO_Stores.push_front(LO_Active);
	LO_Active->MyIndex = LineFilterPixelsIndex;
}

RedBlackTreeNode* Factory_New_Linestore(char *Key, int KeySize)
{
	LineStore *ret = new LineStore(KeySize);
	memcpy(ret->Line, Key, KeySize);
	ret->TotalFoundCount = 1;
	ret->TotalPictureFoundCount = 1;
	RedBlackTreeNode *ret2 = new RedBlackTreeNode();
	ret2->Value = ret;
	ret2->Key = ret->Line;
	return ret2;
}

void LineFilter_LearnLine(LineFilter *LO_Active, char *Line, int LineLen)
{
	LineStore *ExistingLine = (LineStore *)LO_Active->Lines[LineLen - MIN_LINE_LENGTH_PIXELS]->FindNode(Line);
	if (ExistingLine != NULL)
	{
		ExistingLine->TotalFoundCount++;
		return;
	}
	RedBlackTreeNode *newNode = Factory_New_Linestore(Line, LineLen * PIXEL_BYTE_COUNT);
	LO_Active->Lines[LineLen - MIN_LINE_LENGTH_PIXELS]->AddNode(newNode);
}

//from temporary to active
void MergePictureStatistics(LineFilter *Ori, LineFilter *Temp)
{
	for (int i = MIN_LINE_LENGTH_PIXELS; i < MAX_LINE_LENGTH_PIXELS; i++)
	{
		std::list<RedBlackTreeNode*> *NodeList = Temp->Lines[i - MIN_LINE_LENGTH_PIXELS]->GetNodeList();
		for (auto itr = NodeList->begin(); itr != NodeList->end(); itr++)
		{
			LineStore *TempLine = (LineStore*)(*itr)->Value;
			LineStore *ExistingLine = (LineStore *)Ori->Lines[i - MIN_LINE_LENGTH_PIXELS]->FindNode(TempLine->Line);
			if (ExistingLine != NULL)
			{
				ExistingLine->TotalFoundCount += TempLine->TotalFoundCount;
				ExistingLine->TotalPictureFoundCount += TempLine->TotalPictureFoundCount;
				continue;
			}
			RedBlackTreeNode *NewNode = Factory_New_Linestore(TempLine->Line, i * PIXEL_BYTE_COUNT );
			LineStore *NewLine = (LineStore *)NewNode->Value;
			NewLine->TotalFoundCount = TempLine->TotalFoundCount;
			NewLine->TotalPictureFoundCount = TempLine->TotalPictureFoundCount;
			Ori->Lines[i - MIN_LINE_LENGTH_PIXELS]->AddNode(NewNode);
		}
	}
	Ori->LinesAdded += Temp->LinesAdded;
	Ori->NumberOfImagesLoaded += Temp->NumberOfImagesLoaded;
}

void CopyPixel(LPCOLORREF src, char *dst)
{
	*(LPCOLORREF)&dst[0] = src[0]; // also copy alpha channel that is on byte 4, we will overwrite this later
}

LineFilter *LineFilterParseImage(char *aFileName)
{
	InitLinePoints();

	CachedPicture *cache = CachePicturePrintErrors(aFileName, __FUNCTION__);
	if (cache == NULL)
		return NULL;

	int PixelCount = cache->Width * cache->Height;

	//blurr the image with 3x3 mask, middle gets added 8 times
//	LPCOLORREF new_Pixels = BlurrImage(1, 7, cache->Pixels, cache->Width, cache->Height);
//	_aligned_free(cache->Pixels);
//	cache->Pixels = new_Pixels;

	//reduce color count to half
	ColorReduceCache(aFileName,6);

	//remove gradient from the image
//	GradientRemoveCache(aFileName);

//	GradientReduceCache(aFileName, 3);

	LineFilter *UsedLineFilter = new LineFilter();

	int ValuesAdded = 0;

	//break up the image into multiple lines
	char *LineBuff = (char *)_aligned_malloc(MAX_LINE_LENGTH_PIXELS * 4 + SSE_PADDING, SSE_ALIGNMENT);
	for (int LineLength = MIN_LINE_LENGTH_PIXELS; LineLength < MAX_LINE_LENGTH_PIXELS; LineLength++)
	{
		int NumberOfPossibleLines = 2 * (LineLength + 1) + 2 * (LineLength - 1);
		for (int y = LineLength; y < cache->Height - LineLength; y++)
			for (int x = LineLength; x < cache->Width - LineLength; x++)
			{
				CopyPixel(&cache->Pixels[y * cache->Width + x], &LineBuff[0]);// pixel 0,0
				//construct all the possible lines
				for (int CurLine = 0; CurLine < NumberOfPossibleLines; CurLine++)
				{
					//construct this line
					for (int PixelNr = 0; PixelNr < LineLength-1; PixelNr++)
					{
						int YMod = LinePoints[LineLength - MIN_LINE_LENGTH_PIXELS][CurLine][PixelNr * 2 + 0];
						int XMod = LinePoints[LineLength - MIN_LINE_LENGTH_PIXELS][CurLine][PixelNr * 2 + 1];
						CopyPixel(&cache->Pixels[(y + YMod) * cache->Width + x + XMod], &LineBuff[(PixelNr+1)*PIXEL_BYTE_COUNT]);
					}
					//add the line to the statistics
					LineFilter_LearnLine(UsedLineFilter, LineBuff, LineLength);
					ValuesAdded++;
				}
			}
	}
	_aligned_free(LineBuff);

	//number of pixels we added to statistcs
	UsedLineFilter->LinesAdded += ValuesAdded;
	UsedLineFilter->NumberOfImagesLoaded++;

	return UsedLineFilter;
}

void LineFilter_AddImage(int ObjectIndex, char *aFileName)
{
	LineFilter_Init(ObjectIndex);
	LineFilter *UsedLineFilter = LineFilterParseImage(aFileName);
	if (UsedLineFilter == NULL)
		return;
	MergePictureStatistics(LO_Active, UsedLineFilter);
	delete UsedLineFilter;
}

//from temporary to active
void MergePictureStatisticsElimianteNonCommon(LineFilter *Ori, LineFilter *Temp)
{
	for (int i = 0; i < MAX_LINE_LENGTH_PIXELS - MIN_LINE_LENGTH_PIXELS; i++)
	{
		std::list<RedBlackTreeNode*> *NodeList = Ori->Lines[i]->GetNodeList();
		for (auto itr = NodeList->begin(); itr != NodeList->end(); itr++)
		{
			LineStore *ExistingLine = (LineStore *)Temp->Lines[i]->FindNode((*itr)->Key);
			//soft erase if this line does not exist in the new image
			if (ExistingLine == NULL)
			{
				LineStore *TempLine = (LineStore*)(*itr)->Value;
				TempLine->TotalFoundCount = 0;
				TempLine->TotalPictureFoundCount = 0;
			}
		}
	}
}

void LineFilter_AddImageEliminateNonCommon(int ObjectIndex, char *aFileName)
{
	LineFilter_Init(ObjectIndex);
	LineFilter *UsedLineFilter = LineFilterParseImage(aFileName);
	if (UsedLineFilter == NULL)
		return;
	MergePictureStatisticsElimianteNonCommon(LO_Active, UsedLineFilter);
	delete UsedLineFilter;
}

void LineFilter_MarkObjectProbability(int ObjectIndex, char *aFileName)
{
	LineFilter_Init(ObjectIndex);
	CachedPicture *cache = CachePicturePrintErrors(aFileName, __FUNCTION__);
	if (cache == NULL)
		return;

	LPCOLORREF NewPicture = (LPCOLORREF)_aligned_malloc(cache->Width * cache->Height * sizeof(COLORREF) + SSE_PADDING, SSE_ALIGNMENT);
	memset(NewPicture, 0, cache->Width * cache->Height * sizeof(COLORREF) + SSE_PADDING);

	char *LineBuff = (char *)_aligned_malloc(MAX_LINE_LENGTH_PIXELS * 4 + SSE_PADDING, SSE_ALIGNMENT);
	for (int LineLength = MIN_LINE_LENGTH_PIXELS; LineLength < MAX_LINE_LENGTH_PIXELS; LineLength++)
	{
		int NumberOfPossibleLines = 2 * (LineLength + 1) + 2 * (LineLength - 1);
		for (int y = LineLength; y < cache->Height - LineLength; y++)
			for (int x = LineLength; x < cache->Width - LineLength; x++)
			{
				CopyPixel(&cache->Pixels[y * cache->Width + x], &LineBuff[0]);// pixel 0,0
				//construct all the possible lines
				for (int CurLine = 0; CurLine < NumberOfPossibleLines; CurLine++)
				{
					//construct this line
					for (int PixelNr = 0; PixelNr < LineLength - 1; PixelNr++)
					{
						int YMod = LinePoints[LineLength - MIN_LINE_LENGTH_PIXELS][CurLine][PixelNr * 2 + 0];
						int XMod = LinePoints[LineLength - MIN_LINE_LENGTH_PIXELS][CurLine][PixelNr * 2 + 1];
						CopyPixel(&cache->Pixels[(y + YMod) * cache->Width + x + XMod], &LineBuff[(PixelNr + 1) * PIXEL_BYTE_COUNT]);
					}

					//get statistics about this line
					LineStore *ls = (LineStore*)LO_Active->Lines[LineLength - MIN_LINE_LENGTH_PIXELS]->FindNode(LineBuff);
					if (ls == NULL)
						continue;
					if (ls->TotalFoundCount == 0)
						continue;

					//we found this line at this position. Calculate how much we trust this line that it belongs to the object
//					float LineFoundPCT = (float)ls->TotalFoundCount / (float)LO_Active->LinesAdded; //from all the lines we seen in our life, how frequent was this one ? Maybe it's a unique line
					float LineFoundPicturePCT = (float)ls->TotalPictureFoundCount / (float)LO_Active->NumberOfImagesLoaded; // did we see it in all training pictures ?

					//mark this as a recognized line
					unsigned char WhiteScale = LineFoundPicturePCT * 255;
					if (WhiteScale < 10)
						WhiteScale = 10; //something to be visible to the human eye
					int RGBWhite = RGB(WhiteScale, WhiteScale, WhiteScale);
					NewPicture[y * cache->Width + x] = RGBWhite;
					for (int PixelNr = 0; PixelNr < LineLength - 1; PixelNr++)
					{
						int YMod = LinePoints[LineLength - MIN_LINE_LENGTH_PIXELS][CurLine][PixelNr * 2 + 0];
						int XMod = LinePoints[LineLength - MIN_LINE_LENGTH_PIXELS][CurLine][PixelNr * 2 + 1];
						NewPicture[(y + YMod)* cache->Width + x + XMod] = RGBWhite;
					}
				}
			}
	}
	_aligned_free(LineBuff);

	_aligned_free(cache->Pixels);
	cache->Pixels = NewPicture;
}

void InitLinePoints()
{
	if (IsLinePointsInitialized != 0)
		return;
	IsLinePointsInitialized = 1;
	//1st index is line length
	//2nd index is line index
	//3rd index is x than y
	LinePoints[0][0][0] = -1; LinePoints[0][0][1] = -1;
	LinePoints[0][1][0] = -1; LinePoints[0][1][1] = 0;
	LinePoints[0][2][0] = -1; LinePoints[0][2][1] = 0;
	LinePoints[0][3][0] = -1; LinePoints[0][3][1] = 0;
	LinePoints[0][4][0] = -1; LinePoints[0][4][1] = 1;
	LinePoints[0][5][0] = 0; LinePoints[0][5][1] = -1;
	LinePoints[0][6][0] = 0; LinePoints[0][6][1] = 1;
	LinePoints[0][7][0] = 0; LinePoints[0][7][1] = -1;
	LinePoints[0][8][0] = 0; LinePoints[0][8][1] = 1;
	LinePoints[0][9][0] = 0; LinePoints[0][9][1] = -1;
	LinePoints[0][10][0] = 0; LinePoints[0][10][1] = 1;
	LinePoints[0][11][0] = 1; LinePoints[0][11][1] = -1;
	LinePoints[0][12][0] = 1; LinePoints[0][12][1] = 0;
	LinePoints[0][13][0] = 1; LinePoints[0][13][1] = 0;
	LinePoints[0][14][0] = 1; LinePoints[0][14][1] = 0;
	LinePoints[0][15][0] = 1; LinePoints[0][15][1] = 1;
	LinePoints[1][0][0] = -1; LinePoints[1][0][1] = -1; LinePoints[1][0][2] = -2; LinePoints[1][0][3] = -2;
	LinePoints[1][1][0] = -1; LinePoints[1][1][1] = 0; LinePoints[1][1][2] = -2; LinePoints[1][1][3] = -1;
	LinePoints[1][2][0] = -1; LinePoints[1][2][1] = 0; LinePoints[1][2][2] = -2; LinePoints[1][2][3] = 0;
	LinePoints[1][3][0] = -1; LinePoints[1][3][1] = 0; LinePoints[1][3][2] = -2; LinePoints[1][3][3] = 0;
	LinePoints[1][4][0] = -1; LinePoints[1][4][1] = 0; LinePoints[1][4][2] = -2; LinePoints[1][4][3] = 0;
	LinePoints[1][5][0] = -1; LinePoints[1][5][1] = 0; LinePoints[1][5][2] = -2; LinePoints[1][5][3] = 1;
	LinePoints[1][6][0] = -1; LinePoints[1][6][1] = 1; LinePoints[1][6][2] = -2; LinePoints[1][6][3] = 2;
	LinePoints[1][7][0] = 0; LinePoints[1][7][1] = -1; LinePoints[1][7][2] = -1; LinePoints[1][7][3] = -2;
	LinePoints[1][8][0] = 0; LinePoints[1][8][1] = 1; LinePoints[1][8][2] = -1; LinePoints[1][8][3] = 2;
	LinePoints[1][9][0] = 0; LinePoints[1][9][1] = -1; LinePoints[1][9][2] = 0; LinePoints[1][9][3] = -2;
	LinePoints[1][10][0] = 0; LinePoints[1][10][1] = 1; LinePoints[1][10][2] = 0; LinePoints[1][10][3] = 2;
	LinePoints[1][11][0] = 0; LinePoints[1][11][1] = -1; LinePoints[1][11][2] = 0; LinePoints[1][11][3] = -2;
	LinePoints[1][12][0] = 0; LinePoints[1][12][1] = 1; LinePoints[1][12][2] = 0; LinePoints[1][12][3] = 2;
	LinePoints[1][13][0] = 0; LinePoints[1][13][1] = -1; LinePoints[1][13][2] = 0; LinePoints[1][13][3] = -2;
	LinePoints[1][14][0] = 0; LinePoints[1][14][1] = 1; LinePoints[1][14][2] = 0; LinePoints[1][14][3] = 2;
	LinePoints[1][15][0] = 0; LinePoints[1][15][1] = -1; LinePoints[1][15][2] = 1; LinePoints[1][15][3] = -2;
	LinePoints[1][16][0] = 0; LinePoints[1][16][1] = 1; LinePoints[1][16][2] = 1; LinePoints[1][16][3] = 2;
	LinePoints[1][17][0] = 1; LinePoints[1][17][1] = -1; LinePoints[1][17][2] = 2; LinePoints[1][17][3] = -2;
	LinePoints[1][18][0] = 1; LinePoints[1][18][1] = 0; LinePoints[1][18][2] = 2; LinePoints[1][18][3] = -1;
	LinePoints[1][19][0] = 1; LinePoints[1][19][1] = 0; LinePoints[1][19][2] = 2; LinePoints[1][19][3] = 0;
	LinePoints[1][20][0] = 1; LinePoints[1][20][1] = 0; LinePoints[1][20][2] = 2; LinePoints[1][20][3] = 0;
	LinePoints[1][21][0] = 1; LinePoints[1][21][1] = 0; LinePoints[1][21][2] = 2; LinePoints[1][21][3] = 0;
	LinePoints[1][22][0] = 1; LinePoints[1][22][1] = 0; LinePoints[1][22][2] = 2; LinePoints[1][22][3] = 1;
	LinePoints[1][23][0] = 1; LinePoints[1][23][1] = 1; LinePoints[1][23][2] = 2; LinePoints[1][23][3] = 2;
	LinePoints[2][0][0] = -1; LinePoints[2][0][1] = -1; LinePoints[2][0][2] = -2; LinePoints[2][0][3] = -2; LinePoints[2][0][4] = -3; LinePoints[2][0][5] = -3;
	LinePoints[2][1][0] = -1; LinePoints[2][1][1] = 0; LinePoints[2][1][2] = -2; LinePoints[2][1][3] = -1; LinePoints[2][1][4] = -3; LinePoints[2][1][5] = -2;
	LinePoints[2][2][0] = -1; LinePoints[2][2][1] = 0; LinePoints[2][2][2] = -2; LinePoints[2][2][3] = -1; LinePoints[2][2][4] = -3; LinePoints[2][2][5] = -1;
	LinePoints[2][3][0] = -1; LinePoints[2][3][1] = 0; LinePoints[2][3][2] = -2; LinePoints[2][3][3] = 0; LinePoints[2][3][4] = -3; LinePoints[2][3][5] = 0;
	LinePoints[2][4][0] = -1; LinePoints[2][4][1] = 0; LinePoints[2][4][2] = -2; LinePoints[2][4][3] = 0; LinePoints[2][4][4] = -3; LinePoints[2][4][5] = 0;
	LinePoints[2][5][0] = -1; LinePoints[2][5][1] = 0; LinePoints[2][5][2] = -2; LinePoints[2][5][3] = 0; LinePoints[2][5][4] = -3; LinePoints[2][5][5] = 0;
	LinePoints[2][6][0] = -1; LinePoints[2][6][1] = 0; LinePoints[2][6][2] = -2; LinePoints[2][6][3] = 1; LinePoints[2][6][4] = -3; LinePoints[2][6][5] = 1;
	LinePoints[2][7][0] = -1; LinePoints[2][7][1] = 0; LinePoints[2][7][2] = -2; LinePoints[2][7][3] = 1; LinePoints[2][7][4] = -3; LinePoints[2][7][5] = 2;
	LinePoints[2][8][0] = -1; LinePoints[2][8][1] = 1; LinePoints[2][8][2] = -2; LinePoints[2][8][3] = 2; LinePoints[2][8][4] = -3; LinePoints[2][8][5] = 3;
	LinePoints[2][9][0] = 0; LinePoints[2][9][1] = -1; LinePoints[2][9][2] = -1; LinePoints[2][9][3] = -2; LinePoints[2][9][4] = -2; LinePoints[2][9][5] = -3;
	LinePoints[2][10][0] = 0; LinePoints[2][10][1] = 1; LinePoints[2][10][2] = -1; LinePoints[2][10][3] = 2; LinePoints[2][10][4] = -2; LinePoints[2][10][5] = 3;
	LinePoints[2][11][0] = 0; LinePoints[2][11][1] = -1; LinePoints[2][11][2] = -1; LinePoints[2][11][3] = -2; LinePoints[2][11][4] = -1; LinePoints[2][11][5] = -3;
	LinePoints[2][12][0] = 0; LinePoints[2][12][1] = 1; LinePoints[2][12][2] = -1; LinePoints[2][12][3] = 2; LinePoints[2][12][4] = -1; LinePoints[2][12][5] = 3;
	LinePoints[2][13][0] = 0; LinePoints[2][13][1] = -1; LinePoints[2][13][2] = 0; LinePoints[2][13][3] = -2; LinePoints[2][13][4] = 0; LinePoints[2][13][5] = -3;
	LinePoints[2][14][0] = 0; LinePoints[2][14][1] = 1; LinePoints[2][14][2] = 0; LinePoints[2][14][3] = 2; LinePoints[2][14][4] = 0; LinePoints[2][14][5] = 3;
	LinePoints[2][15][0] = 0; LinePoints[2][15][1] = -1; LinePoints[2][15][2] = 0; LinePoints[2][15][3] = -2; LinePoints[2][15][4] = 0; LinePoints[2][15][5] = -3;
	LinePoints[2][16][0] = 0; LinePoints[2][16][1] = 1; LinePoints[2][16][2] = 0; LinePoints[2][16][3] = 2; LinePoints[2][16][4] = 0; LinePoints[2][16][5] = 3;
	LinePoints[2][17][0] = 0; LinePoints[2][17][1] = -1; LinePoints[2][17][2] = 0; LinePoints[2][17][3] = -2; LinePoints[2][17][4] = 0; LinePoints[2][17][5] = -3;
	LinePoints[2][18][0] = 0; LinePoints[2][18][1] = 1; LinePoints[2][18][2] = 0; LinePoints[2][18][3] = 2; LinePoints[2][18][4] = 0; LinePoints[2][18][5] = 3;
	LinePoints[2][19][0] = 0; LinePoints[2][19][1] = -1; LinePoints[2][19][2] = 1; LinePoints[2][19][3] = -2; LinePoints[2][19][4] = 1; LinePoints[2][19][5] = -3;
	LinePoints[2][20][0] = 0; LinePoints[2][20][1] = 1; LinePoints[2][20][2] = 1; LinePoints[2][20][3] = 2; LinePoints[2][20][4] = 1; LinePoints[2][20][5] = 3;
	LinePoints[2][21][0] = 0; LinePoints[2][21][1] = -1; LinePoints[2][21][2] = 1; LinePoints[2][21][3] = -2; LinePoints[2][21][4] = 2; LinePoints[2][21][5] = -3;
	LinePoints[2][22][0] = 0; LinePoints[2][22][1] = 1; LinePoints[2][22][2] = 1; LinePoints[2][22][3] = 2; LinePoints[2][22][4] = 2; LinePoints[2][22][5] = 3;
	LinePoints[2][23][0] = 1; LinePoints[2][23][1] = -1; LinePoints[2][23][2] = 2; LinePoints[2][23][3] = -2; LinePoints[2][23][4] = 3; LinePoints[2][23][5] = -3;
	LinePoints[2][24][0] = 1; LinePoints[2][24][1] = 0; LinePoints[2][24][2] = 2; LinePoints[2][24][3] = -1; LinePoints[2][24][4] = 3; LinePoints[2][24][5] = -2;
	LinePoints[2][25][0] = 1; LinePoints[2][25][1] = 0; LinePoints[2][25][2] = 2; LinePoints[2][25][3] = -1; LinePoints[2][25][4] = 3; LinePoints[2][25][5] = -1;
	LinePoints[2][26][0] = 1; LinePoints[2][26][1] = 0; LinePoints[2][26][2] = 2; LinePoints[2][26][3] = 0; LinePoints[2][26][4] = 3; LinePoints[2][26][5] = 0;
	LinePoints[2][27][0] = 1; LinePoints[2][27][1] = 0; LinePoints[2][27][2] = 2; LinePoints[2][27][3] = 0; LinePoints[2][27][4] = 3; LinePoints[2][27][5] = 0;
	LinePoints[2][28][0] = 1; LinePoints[2][28][1] = 0; LinePoints[2][28][2] = 2; LinePoints[2][28][3] = 0; LinePoints[2][28][4] = 3; LinePoints[2][28][5] = 0;
	LinePoints[2][29][0] = 1; LinePoints[2][29][1] = 0; LinePoints[2][29][2] = 2; LinePoints[2][29][3] = 1; LinePoints[2][29][4] = 3; LinePoints[2][29][5] = 1;
	LinePoints[2][30][0] = 1; LinePoints[2][30][1] = 0; LinePoints[2][30][2] = 2; LinePoints[2][30][3] = 1; LinePoints[2][30][4] = 3; LinePoints[2][30][5] = 2;
	LinePoints[2][31][0] = 1; LinePoints[2][31][1] = 1; LinePoints[2][31][2] = 2; LinePoints[2][31][3] = 2; LinePoints[2][31][4] = 3; LinePoints[2][31][5] = 3;
	LinePoints[3][0][0] = -1; LinePoints[3][0][1] = -1; LinePoints[3][0][2] = -2; LinePoints[3][0][3] = -2; LinePoints[3][0][4] = -3; LinePoints[3][0][5] = -3; LinePoints[3][0][6] = -4; LinePoints[3][0][7] = -4;
	LinePoints[3][1][0] = -1; LinePoints[3][1][1] = 0; LinePoints[3][1][2] = -2; LinePoints[3][1][3] = -1; LinePoints[3][1][4] = -3; LinePoints[3][1][5] = -2; LinePoints[3][1][6] = -4; LinePoints[3][1][7] = -3;
	LinePoints[3][2][0] = -1; LinePoints[3][2][1] = 0; LinePoints[3][2][2] = -2; LinePoints[3][2][3] = -1; LinePoints[3][2][4] = -3; LinePoints[3][2][5] = -1; LinePoints[3][2][6] = -4; LinePoints[3][2][7] = -2;
	LinePoints[3][3][0] = -1; LinePoints[3][3][1] = 0; LinePoints[3][3][2] = -2; LinePoints[3][3][3] = 0; LinePoints[3][3][4] = -3; LinePoints[3][3][5] = -1; LinePoints[3][3][6] = -4; LinePoints[3][3][7] = -1;
	LinePoints[3][4][0] = -1; LinePoints[3][4][1] = 0; LinePoints[3][4][2] = -2; LinePoints[3][4][3] = 0; LinePoints[3][4][4] = -3; LinePoints[3][4][5] = 0; LinePoints[3][4][6] = -4; LinePoints[3][4][7] = 0;
	LinePoints[3][5][0] = -1; LinePoints[3][5][1] = 0; LinePoints[3][5][2] = -2; LinePoints[3][5][3] = 0; LinePoints[3][5][4] = -3; LinePoints[3][5][5] = 0; LinePoints[3][5][6] = -4; LinePoints[3][5][7] = 0;
	LinePoints[3][6][0] = -1; LinePoints[3][6][1] = 0; LinePoints[3][6][2] = -2; LinePoints[3][6][3] = 0; LinePoints[3][6][4] = -3; LinePoints[3][6][5] = 0; LinePoints[3][6][6] = -4; LinePoints[3][6][7] = 0;
	LinePoints[3][7][0] = -1; LinePoints[3][7][1] = 0; LinePoints[3][7][2] = -2; LinePoints[3][7][3] = 0; LinePoints[3][7][4] = -3; LinePoints[3][7][5] = 1; LinePoints[3][7][6] = -4; LinePoints[3][7][7] = 1;
	LinePoints[3][8][0] = -1; LinePoints[3][8][1] = 0; LinePoints[3][8][2] = -2; LinePoints[3][8][3] = 1; LinePoints[3][8][4] = -3; LinePoints[3][8][5] = 1; LinePoints[3][8][6] = -4; LinePoints[3][8][7] = 2;
	LinePoints[3][9][0] = -1; LinePoints[3][9][1] = 0; LinePoints[3][9][2] = -2; LinePoints[3][9][3] = 1; LinePoints[3][9][4] = -3; LinePoints[3][9][5] = 2; LinePoints[3][9][6] = -4; LinePoints[3][9][7] = 3;
	LinePoints[3][10][0] = -1; LinePoints[3][10][1] = 1; LinePoints[3][10][2] = -2; LinePoints[3][10][3] = 2; LinePoints[3][10][4] = -3; LinePoints[3][10][5] = 3; LinePoints[3][10][6] = -4; LinePoints[3][10][7] = 4;
	LinePoints[3][11][0] = 0; LinePoints[3][11][1] = -1; LinePoints[3][11][2] = -1; LinePoints[3][11][3] = -2; LinePoints[3][11][4] = -2; LinePoints[3][11][5] = -3; LinePoints[3][11][6] = -3; LinePoints[3][11][7] = -4;
	LinePoints[3][12][0] = 0; LinePoints[3][12][1] = 1; LinePoints[3][12][2] = -1; LinePoints[3][12][3] = 2; LinePoints[3][12][4] = -2; LinePoints[3][12][5] = 3; LinePoints[3][12][6] = -3; LinePoints[3][12][7] = 4;
	LinePoints[3][13][0] = 0; LinePoints[3][13][1] = -1; LinePoints[3][13][2] = -1; LinePoints[3][13][3] = -2; LinePoints[3][13][4] = -1; LinePoints[3][13][5] = -3; LinePoints[3][13][6] = -2; LinePoints[3][13][7] = -4;
	LinePoints[3][14][0] = 0; LinePoints[3][14][1] = 1; LinePoints[3][14][2] = -1; LinePoints[3][14][3] = 2; LinePoints[3][14][4] = -1; LinePoints[3][14][5] = 3; LinePoints[3][14][6] = -2; LinePoints[3][14][7] = 4;
	LinePoints[3][15][0] = 0; LinePoints[3][15][1] = -1; LinePoints[3][15][2] = 0; LinePoints[3][15][3] = -2; LinePoints[3][15][4] = -1; LinePoints[3][15][5] = -3; LinePoints[3][15][6] = -1; LinePoints[3][15][7] = -4;
	LinePoints[3][16][0] = 0; LinePoints[3][16][1] = 1; LinePoints[3][16][2] = 0; LinePoints[3][16][3] = 2; LinePoints[3][16][4] = -1; LinePoints[3][16][5] = 3; LinePoints[3][16][6] = -1; LinePoints[3][16][7] = 4;
	LinePoints[3][17][0] = 0; LinePoints[3][17][1] = -1; LinePoints[3][17][2] = 0; LinePoints[3][17][3] = -2; LinePoints[3][17][4] = 0; LinePoints[3][17][5] = -3; LinePoints[3][17][6] = 0; LinePoints[3][17][7] = -4;
	LinePoints[3][18][0] = 0; LinePoints[3][18][1] = 1; LinePoints[3][18][2] = 0; LinePoints[3][18][3] = 2; LinePoints[3][18][4] = 0; LinePoints[3][18][5] = 3; LinePoints[3][18][6] = 0; LinePoints[3][18][7] = 4;
	LinePoints[3][19][0] = 0; LinePoints[3][19][1] = -1; LinePoints[3][19][2] = 0; LinePoints[3][19][3] = -2; LinePoints[3][19][4] = 0; LinePoints[3][19][5] = -3; LinePoints[3][19][6] = 0; LinePoints[3][19][7] = -4;
	LinePoints[3][20][0] = 0; LinePoints[3][20][1] = 1; LinePoints[3][20][2] = 0; LinePoints[3][20][3] = 2; LinePoints[3][20][4] = 0; LinePoints[3][20][5] = 3; LinePoints[3][20][6] = 0; LinePoints[3][20][7] = 4;
	LinePoints[3][21][0] = 0; LinePoints[3][21][1] = -1; LinePoints[3][21][2] = 0; LinePoints[3][21][3] = -2; LinePoints[3][21][4] = 0; LinePoints[3][21][5] = -3; LinePoints[3][21][6] = 0; LinePoints[3][21][7] = -4;
	LinePoints[3][22][0] = 0; LinePoints[3][22][1] = 1; LinePoints[3][22][2] = 0; LinePoints[3][22][3] = 2; LinePoints[3][22][4] = 0; LinePoints[3][22][5] = 3; LinePoints[3][22][6] = 0; LinePoints[3][22][7] = 4;
	LinePoints[3][23][0] = 0; LinePoints[3][23][1] = -1; LinePoints[3][23][2] = 0; LinePoints[3][23][3] = -2; LinePoints[3][23][4] = 1; LinePoints[3][23][5] = -3; LinePoints[3][23][6] = 1; LinePoints[3][23][7] = -4;
	LinePoints[3][24][0] = 0; LinePoints[3][24][1] = 1; LinePoints[3][24][2] = 0; LinePoints[3][24][3] = 2; LinePoints[3][24][4] = 1; LinePoints[3][24][5] = 3; LinePoints[3][24][6] = 1; LinePoints[3][24][7] = 4;
	LinePoints[3][25][0] = 0; LinePoints[3][25][1] = -1; LinePoints[3][25][2] = 1; LinePoints[3][25][3] = -2; LinePoints[3][25][4] = 1; LinePoints[3][25][5] = -3; LinePoints[3][25][6] = 2; LinePoints[3][25][7] = -4;
	LinePoints[3][26][0] = 0; LinePoints[3][26][1] = 1; LinePoints[3][26][2] = 1; LinePoints[3][26][3] = 2; LinePoints[3][26][4] = 1; LinePoints[3][26][5] = 3; LinePoints[3][26][6] = 2; LinePoints[3][26][7] = 4;
	LinePoints[3][27][0] = 0; LinePoints[3][27][1] = -1; LinePoints[3][27][2] = 1; LinePoints[3][27][3] = -2; LinePoints[3][27][4] = 2; LinePoints[3][27][5] = -3; LinePoints[3][27][6] = 3; LinePoints[3][27][7] = -4;
	LinePoints[3][28][0] = 0; LinePoints[3][28][1] = 1; LinePoints[3][28][2] = 1; LinePoints[3][28][3] = 2; LinePoints[3][28][4] = 2; LinePoints[3][28][5] = 3; LinePoints[3][28][6] = 3; LinePoints[3][28][7] = 4;
	LinePoints[3][29][0] = 1; LinePoints[3][29][1] = -1; LinePoints[3][29][2] = 2; LinePoints[3][29][3] = -2; LinePoints[3][29][4] = 3; LinePoints[3][29][5] = -3; LinePoints[3][29][6] = 4; LinePoints[3][29][7] = -4;
	LinePoints[3][30][0] = 1; LinePoints[3][30][1] = 0; LinePoints[3][30][2] = 2; LinePoints[3][30][3] = -1; LinePoints[3][30][4] = 3; LinePoints[3][30][5] = -2; LinePoints[3][30][6] = 4; LinePoints[3][30][7] = -3;
	LinePoints[3][31][0] = 1; LinePoints[3][31][1] = 0; LinePoints[3][31][2] = 2; LinePoints[3][31][3] = -1; LinePoints[3][31][4] = 3; LinePoints[3][31][5] = -1; LinePoints[3][31][6] = 4; LinePoints[3][31][7] = -2;
	LinePoints[3][32][0] = 1; LinePoints[3][32][1] = 0; LinePoints[3][32][2] = 2; LinePoints[3][32][3] = 0; LinePoints[3][32][4] = 3; LinePoints[3][32][5] = -1; LinePoints[3][32][6] = 4; LinePoints[3][32][7] = -1;
	LinePoints[3][33][0] = 1; LinePoints[3][33][1] = 0; LinePoints[3][33][2] = 2; LinePoints[3][33][3] = 0; LinePoints[3][33][4] = 3; LinePoints[3][33][5] = 0; LinePoints[3][33][6] = 4; LinePoints[3][33][7] = 0;
	LinePoints[3][34][0] = 1; LinePoints[3][34][1] = 0; LinePoints[3][34][2] = 2; LinePoints[3][34][3] = 0; LinePoints[3][34][4] = 3; LinePoints[3][34][5] = 0; LinePoints[3][34][6] = 4; LinePoints[3][34][7] = 0;
	LinePoints[3][35][0] = 1; LinePoints[3][35][1] = 0; LinePoints[3][35][2] = 2; LinePoints[3][35][3] = 0; LinePoints[3][35][4] = 3; LinePoints[3][35][5] = 0; LinePoints[3][35][6] = 4; LinePoints[3][35][7] = 0;
	LinePoints[3][36][0] = 1; LinePoints[3][36][1] = 0; LinePoints[3][36][2] = 2; LinePoints[3][36][3] = 0; LinePoints[3][36][4] = 3; LinePoints[3][36][5] = 1; LinePoints[3][36][6] = 4; LinePoints[3][36][7] = 1;
	LinePoints[3][37][0] = 1; LinePoints[3][37][1] = 0; LinePoints[3][37][2] = 2; LinePoints[3][37][3] = 1; LinePoints[3][37][4] = 3; LinePoints[3][37][5] = 1; LinePoints[3][37][6] = 4; LinePoints[3][37][7] = 2;
	LinePoints[3][38][0] = 1; LinePoints[3][38][1] = 0; LinePoints[3][38][2] = 2; LinePoints[3][38][3] = 1; LinePoints[3][38][4] = 3; LinePoints[3][38][5] = 2; LinePoints[3][38][6] = 4; LinePoints[3][38][7] = 3;
	LinePoints[3][39][0] = 1; LinePoints[3][39][1] = 1; LinePoints[3][39][2] = 2; LinePoints[3][39][3] = 2; LinePoints[3][39][4] = 3; LinePoints[3][39][5] = 3; LinePoints[3][39][6] = 4; LinePoints[3][39][7] = 4;
	LinePoints[4][0][0] = -1; LinePoints[4][0][1] = -1; LinePoints[4][0][2] = -2; LinePoints[4][0][3] = -2; LinePoints[4][0][4] = -3; LinePoints[4][0][5] = -3; LinePoints[4][0][6] = -4; LinePoints[4][0][7] = -4; LinePoints[4][0][8] = -5; LinePoints[4][0][9] = -5;
	LinePoints[4][1][0] = -1; LinePoints[4][1][1] = 0; LinePoints[4][1][2] = -2; LinePoints[4][1][3] = -1; LinePoints[4][1][4] = -3; LinePoints[4][1][5] = -2; LinePoints[4][1][6] = -4; LinePoints[4][1][7] = -3; LinePoints[4][1][8] = -5; LinePoints[4][1][9] = -4;
	LinePoints[4][2][0] = -1; LinePoints[4][2][1] = 0; LinePoints[4][2][2] = -2; LinePoints[4][2][3] = -1; LinePoints[4][2][4] = -3; LinePoints[4][2][5] = -2; LinePoints[4][2][6] = -4; LinePoints[4][2][7] = -2; LinePoints[4][2][8] = -5; LinePoints[4][2][9] = -3;
	LinePoints[4][3][0] = -1; LinePoints[4][3][1] = 0; LinePoints[4][3][2] = -2; LinePoints[4][3][3] = -1; LinePoints[4][3][4] = -3; LinePoints[4][3][5] = -1; LinePoints[4][3][6] = -4; LinePoints[4][3][7] = -2; LinePoints[4][3][8] = -5; LinePoints[4][3][9] = -2;
	LinePoints[4][4][0] = -1; LinePoints[4][4][1] = 0; LinePoints[4][4][2] = -2; LinePoints[4][4][3] = 0; LinePoints[4][4][4] = -3; LinePoints[4][4][5] = -1; LinePoints[4][4][6] = -4; LinePoints[4][4][7] = -1; LinePoints[4][4][8] = -5; LinePoints[4][4][9] = -1;
	LinePoints[4][5][0] = -1; LinePoints[4][5][1] = 0; LinePoints[4][5][2] = -2; LinePoints[4][5][3] = 0; LinePoints[4][5][4] = -3; LinePoints[4][5][5] = 0; LinePoints[4][5][6] = -4; LinePoints[4][5][7] = 0; LinePoints[4][5][8] = -5; LinePoints[4][5][9] = 0;
	LinePoints[4][6][0] = -1; LinePoints[4][6][1] = 0; LinePoints[4][6][2] = -2; LinePoints[4][6][3] = 0; LinePoints[4][6][4] = -3; LinePoints[4][6][5] = 0; LinePoints[4][6][6] = -4; LinePoints[4][6][7] = 0; LinePoints[4][6][8] = -5; LinePoints[4][6][9] = 0;
	LinePoints[4][7][0] = -1; LinePoints[4][7][1] = 0; LinePoints[4][7][2] = -2; LinePoints[4][7][3] = 0; LinePoints[4][7][4] = -3; LinePoints[4][7][5] = 0; LinePoints[4][7][6] = -4; LinePoints[4][7][7] = 0; LinePoints[4][7][8] = -5; LinePoints[4][7][9] = 0;
	LinePoints[4][8][0] = -1; LinePoints[4][8][1] = 0; LinePoints[4][8][2] = -2; LinePoints[4][8][3] = 0; LinePoints[4][8][4] = -3; LinePoints[4][8][5] = 1; LinePoints[4][8][6] = -4; LinePoints[4][8][7] = 1; LinePoints[4][8][8] = -5; LinePoints[4][8][9] = 1;
	LinePoints[4][9][0] = -1; LinePoints[4][9][1] = 0; LinePoints[4][9][2] = -2; LinePoints[4][9][3] = 1; LinePoints[4][9][4] = -3; LinePoints[4][9][5] = 1; LinePoints[4][9][6] = -4; LinePoints[4][9][7] = 2; LinePoints[4][9][8] = -5; LinePoints[4][9][9] = 2;
	LinePoints[4][10][0] = -1; LinePoints[4][10][1] = 0; LinePoints[4][10][2] = -2; LinePoints[4][10][3] = 1; LinePoints[4][10][4] = -3; LinePoints[4][10][5] = 2; LinePoints[4][10][6] = -4; LinePoints[4][10][7] = 2; LinePoints[4][10][8] = -5; LinePoints[4][10][9] = 3;
	LinePoints[4][11][0] = -1; LinePoints[4][11][1] = 0; LinePoints[4][11][2] = -2; LinePoints[4][11][3] = 1; LinePoints[4][11][4] = -3; LinePoints[4][11][5] = 2; LinePoints[4][11][6] = -4; LinePoints[4][11][7] = 3; LinePoints[4][11][8] = -5; LinePoints[4][11][9] = 4;
	LinePoints[4][12][0] = -1; LinePoints[4][12][1] = 1; LinePoints[4][12][2] = -2; LinePoints[4][12][3] = 2; LinePoints[4][12][4] = -3; LinePoints[4][12][5] = 3; LinePoints[4][12][6] = -4; LinePoints[4][12][7] = 4; LinePoints[4][12][8] = -5; LinePoints[4][12][9] = 5;
	LinePoints[4][13][0] = 0; LinePoints[4][13][1] = -1; LinePoints[4][13][2] = -1; LinePoints[4][13][3] = -2; LinePoints[4][13][4] = -2; LinePoints[4][13][5] = -3; LinePoints[4][13][6] = -3; LinePoints[4][13][7] = -4; LinePoints[4][13][8] = -4; LinePoints[4][13][9] = -5;
	LinePoints[4][14][0] = 0; LinePoints[4][14][1] = 1; LinePoints[4][14][2] = -1; LinePoints[4][14][3] = 2; LinePoints[4][14][4] = -2; LinePoints[4][14][5] = 3; LinePoints[4][14][6] = -3; LinePoints[4][14][7] = 4; LinePoints[4][14][8] = -4; LinePoints[4][14][9] = 5;
	LinePoints[4][15][0] = 0; LinePoints[4][15][1] = -1; LinePoints[4][15][2] = -1; LinePoints[4][15][3] = -2; LinePoints[4][15][4] = -2; LinePoints[4][15][5] = -3; LinePoints[4][15][6] = -2; LinePoints[4][15][7] = -4; LinePoints[4][15][8] = -3; LinePoints[4][15][9] = -5;
	LinePoints[4][16][0] = 0; LinePoints[4][16][1] = 1; LinePoints[4][16][2] = -1; LinePoints[4][16][3] = 2; LinePoints[4][16][4] = -2; LinePoints[4][16][5] = 3; LinePoints[4][16][6] = -2; LinePoints[4][16][7] = 4; LinePoints[4][16][8] = -3; LinePoints[4][16][9] = 5;
	LinePoints[4][17][0] = 0; LinePoints[4][17][1] = -1; LinePoints[4][17][2] = -1; LinePoints[4][17][3] = -2; LinePoints[4][17][4] = -1; LinePoints[4][17][5] = -3; LinePoints[4][17][6] = -2; LinePoints[4][17][7] = -4; LinePoints[4][17][8] = -2; LinePoints[4][17][9] = -5;
	LinePoints[4][18][0] = 0; LinePoints[4][18][1] = 1; LinePoints[4][18][2] = -1; LinePoints[4][18][3] = 2; LinePoints[4][18][4] = -1; LinePoints[4][18][5] = 3; LinePoints[4][18][6] = -2; LinePoints[4][18][7] = 4; LinePoints[4][18][8] = -2; LinePoints[4][18][9] = 5;
	LinePoints[4][19][0] = 0; LinePoints[4][19][1] = -1; LinePoints[4][19][2] = 0; LinePoints[4][19][3] = -2; LinePoints[4][19][4] = -1; LinePoints[4][19][5] = -3; LinePoints[4][19][6] = -1; LinePoints[4][19][7] = -4; LinePoints[4][19][8] = -1; LinePoints[4][19][9] = -5;
	LinePoints[4][20][0] = 0; LinePoints[4][20][1] = 1; LinePoints[4][20][2] = 0; LinePoints[4][20][3] = 2; LinePoints[4][20][4] = -1; LinePoints[4][20][5] = 3; LinePoints[4][20][6] = -1; LinePoints[4][20][7] = 4; LinePoints[4][20][8] = -1; LinePoints[4][20][9] = 5;
	LinePoints[4][21][0] = 0; LinePoints[4][21][1] = -1; LinePoints[4][21][2] = 0; LinePoints[4][21][3] = -2; LinePoints[4][21][4] = 0; LinePoints[4][21][5] = -3; LinePoints[4][21][6] = 0; LinePoints[4][21][7] = -4; LinePoints[4][21][8] = 0; LinePoints[4][21][9] = -5;
	LinePoints[4][22][0] = 0; LinePoints[4][22][1] = 1; LinePoints[4][22][2] = 0; LinePoints[4][22][3] = 2; LinePoints[4][22][4] = 0; LinePoints[4][22][5] = 3; LinePoints[4][22][6] = 0; LinePoints[4][22][7] = 4; LinePoints[4][22][8] = 0; LinePoints[4][22][9] = 5;
	LinePoints[4][23][0] = 0; LinePoints[4][23][1] = -1; LinePoints[4][23][2] = 0; LinePoints[4][23][3] = -2; LinePoints[4][23][4] = 0; LinePoints[4][23][5] = -3; LinePoints[4][23][6] = 0; LinePoints[4][23][7] = -4; LinePoints[4][23][8] = 0; LinePoints[4][23][9] = -5;
	LinePoints[4][24][0] = 0; LinePoints[4][24][1] = 1; LinePoints[4][24][2] = 0; LinePoints[4][24][3] = 2; LinePoints[4][24][4] = 0; LinePoints[4][24][5] = 3; LinePoints[4][24][6] = 0; LinePoints[4][24][7] = 4; LinePoints[4][24][8] = 0; LinePoints[4][24][9] = 5;
	LinePoints[4][25][0] = 0; LinePoints[4][25][1] = -1; LinePoints[4][25][2] = 0; LinePoints[4][25][3] = -2; LinePoints[4][25][4] = 0; LinePoints[4][25][5] = -3; LinePoints[4][25][6] = 0; LinePoints[4][25][7] = -4; LinePoints[4][25][8] = 0; LinePoints[4][25][9] = -5;
	LinePoints[4][26][0] = 0; LinePoints[4][26][1] = 1; LinePoints[4][26][2] = 0; LinePoints[4][26][3] = 2; LinePoints[4][26][4] = 0; LinePoints[4][26][5] = 3; LinePoints[4][26][6] = 0; LinePoints[4][26][7] = 4; LinePoints[4][26][8] = 0; LinePoints[4][26][9] = 5;
	LinePoints[4][27][0] = 0; LinePoints[4][27][1] = -1; LinePoints[4][27][2] = 0; LinePoints[4][27][3] = -2; LinePoints[4][27][4] = 1; LinePoints[4][27][5] = -3; LinePoints[4][27][6] = 1; LinePoints[4][27][7] = -4; LinePoints[4][27][8] = 1; LinePoints[4][27][9] = -5;
	LinePoints[4][28][0] = 0; LinePoints[4][28][1] = 1; LinePoints[4][28][2] = 0; LinePoints[4][28][3] = 2; LinePoints[4][28][4] = 1; LinePoints[4][28][5] = 3; LinePoints[4][28][6] = 1; LinePoints[4][28][7] = 4; LinePoints[4][28][8] = 1; LinePoints[4][28][9] = 5;
	LinePoints[4][29][0] = 0; LinePoints[4][29][1] = -1; LinePoints[4][29][2] = 1; LinePoints[4][29][3] = -2; LinePoints[4][29][4] = 1; LinePoints[4][29][5] = -3; LinePoints[4][29][6] = 2; LinePoints[4][29][7] = -4; LinePoints[4][29][8] = 2; LinePoints[4][29][9] = -5;
	LinePoints[4][30][0] = 0; LinePoints[4][30][1] = 1; LinePoints[4][30][2] = 1; LinePoints[4][30][3] = 2; LinePoints[4][30][4] = 1; LinePoints[4][30][5] = 3; LinePoints[4][30][6] = 2; LinePoints[4][30][7] = 4; LinePoints[4][30][8] = 2; LinePoints[4][30][9] = 5;
	LinePoints[4][31][0] = 0; LinePoints[4][31][1] = -1; LinePoints[4][31][2] = 1; LinePoints[4][31][3] = -2; LinePoints[4][31][4] = 2; LinePoints[4][31][5] = -3; LinePoints[4][31][6] = 2; LinePoints[4][31][7] = -4; LinePoints[4][31][8] = 3; LinePoints[4][31][9] = -5;
	LinePoints[4][32][0] = 0; LinePoints[4][32][1] = 1; LinePoints[4][32][2] = 1; LinePoints[4][32][3] = 2; LinePoints[4][32][4] = 2; LinePoints[4][32][5] = 3; LinePoints[4][32][6] = 2; LinePoints[4][32][7] = 4; LinePoints[4][32][8] = 3; LinePoints[4][32][9] = 5;
	LinePoints[4][33][0] = 0; LinePoints[4][33][1] = -1; LinePoints[4][33][2] = 1; LinePoints[4][33][3] = -2; LinePoints[4][33][4] = 2; LinePoints[4][33][5] = -3; LinePoints[4][33][6] = 3; LinePoints[4][33][7] = -4; LinePoints[4][33][8] = 4; LinePoints[4][33][9] = -5;
	LinePoints[4][34][0] = 0; LinePoints[4][34][1] = 1; LinePoints[4][34][2] = 1; LinePoints[4][34][3] = 2; LinePoints[4][34][4] = 2; LinePoints[4][34][5] = 3; LinePoints[4][34][6] = 3; LinePoints[4][34][7] = 4; LinePoints[4][34][8] = 4; LinePoints[4][34][9] = 5;
	LinePoints[4][35][0] = 1; LinePoints[4][35][1] = -1; LinePoints[4][35][2] = 2; LinePoints[4][35][3] = -2; LinePoints[4][35][4] = 3; LinePoints[4][35][5] = -3; LinePoints[4][35][6] = 4; LinePoints[4][35][7] = -4; LinePoints[4][35][8] = 5; LinePoints[4][35][9] = -5;
	LinePoints[4][36][0] = 1; LinePoints[4][36][1] = 0; LinePoints[4][36][2] = 2; LinePoints[4][36][3] = -1; LinePoints[4][36][4] = 3; LinePoints[4][36][5] = -2; LinePoints[4][36][6] = 4; LinePoints[4][36][7] = -3; LinePoints[4][36][8] = 5; LinePoints[4][36][9] = -4;
	LinePoints[4][37][0] = 1; LinePoints[4][37][1] = 0; LinePoints[4][37][2] = 2; LinePoints[4][37][3] = -1; LinePoints[4][37][4] = 3; LinePoints[4][37][5] = -2; LinePoints[4][37][6] = 4; LinePoints[4][37][7] = -2; LinePoints[4][37][8] = 5; LinePoints[4][37][9] = -3;
	LinePoints[4][38][0] = 1; LinePoints[4][38][1] = 0; LinePoints[4][38][2] = 2; LinePoints[4][38][3] = -1; LinePoints[4][38][4] = 3; LinePoints[4][38][5] = -1; LinePoints[4][38][6] = 4; LinePoints[4][38][7] = -2; LinePoints[4][38][8] = 5; LinePoints[4][38][9] = -2;
	LinePoints[4][39][0] = 1; LinePoints[4][39][1] = 0; LinePoints[4][39][2] = 2; LinePoints[4][39][3] = 0; LinePoints[4][39][4] = 3; LinePoints[4][39][5] = -1; LinePoints[4][39][6] = 4; LinePoints[4][39][7] = -1; LinePoints[4][39][8] = 5; LinePoints[4][39][9] = -1;
	LinePoints[4][40][0] = 1; LinePoints[4][40][1] = 0; LinePoints[4][40][2] = 2; LinePoints[4][40][3] = 0; LinePoints[4][40][4] = 3; LinePoints[4][40][5] = 0; LinePoints[4][40][6] = 4; LinePoints[4][40][7] = 0; LinePoints[4][40][8] = 5; LinePoints[4][40][9] = 0;
	LinePoints[4][41][0] = 1; LinePoints[4][41][1] = 0; LinePoints[4][41][2] = 2; LinePoints[4][41][3] = 0; LinePoints[4][41][4] = 3; LinePoints[4][41][5] = 0; LinePoints[4][41][6] = 4; LinePoints[4][41][7] = 0; LinePoints[4][41][8] = 5; LinePoints[4][41][9] = 0;
	LinePoints[4][42][0] = 1; LinePoints[4][42][1] = 0; LinePoints[4][42][2] = 2; LinePoints[4][42][3] = 0; LinePoints[4][42][4] = 3; LinePoints[4][42][5] = 0; LinePoints[4][42][6] = 4; LinePoints[4][42][7] = 0; LinePoints[4][42][8] = 5; LinePoints[4][42][9] = 0;
	LinePoints[4][43][0] = 1; LinePoints[4][43][1] = 0; LinePoints[4][43][2] = 2; LinePoints[4][43][3] = 0; LinePoints[4][43][4] = 3; LinePoints[4][43][5] = 1; LinePoints[4][43][6] = 4; LinePoints[4][43][7] = 1; LinePoints[4][43][8] = 5; LinePoints[4][43][9] = 1;
	LinePoints[4][44][0] = 1; LinePoints[4][44][1] = 0; LinePoints[4][44][2] = 2; LinePoints[4][44][3] = 1; LinePoints[4][44][4] = 3; LinePoints[4][44][5] = 1; LinePoints[4][44][6] = 4; LinePoints[4][44][7] = 2; LinePoints[4][44][8] = 5; LinePoints[4][44][9] = 2;
	LinePoints[4][45][0] = 1; LinePoints[4][45][1] = 0; LinePoints[4][45][2] = 2; LinePoints[4][45][3] = 1; LinePoints[4][45][4] = 3; LinePoints[4][45][5] = 2; LinePoints[4][45][6] = 4; LinePoints[4][45][7] = 2; LinePoints[4][45][8] = 5; LinePoints[4][45][9] = 3;
	LinePoints[4][46][0] = 1; LinePoints[4][46][1] = 0; LinePoints[4][46][2] = 2; LinePoints[4][46][3] = 1; LinePoints[4][46][4] = 3; LinePoints[4][46][5] = 2; LinePoints[4][46][6] = 4; LinePoints[4][46][7] = 3; LinePoints[4][46][8] = 5; LinePoints[4][46][9] = 4;
	LinePoints[4][47][0] = 1; LinePoints[4][47][1] = 1; LinePoints[4][47][2] = 2; LinePoints[4][47][3] = 2; LinePoints[4][47][4] = 3; LinePoints[4][47][5] = 3; LinePoints[4][47][6] = 4; LinePoints[4][47][7] = 4; LinePoints[4][47][8] = 5; LinePoints[4][47][9] = 5;
	LinePoints[5][0][0] = -1; LinePoints[5][0][1] = -1; LinePoints[5][0][2] = -2; LinePoints[5][0][3] = -2; LinePoints[5][0][4] = -3; LinePoints[5][0][5] = -3; LinePoints[5][0][6] = -4; LinePoints[5][0][7] = -4; LinePoints[5][0][8] = -5; LinePoints[5][0][9] = -5; LinePoints[5][0][10] = -6; LinePoints[5][0][11] = -6;
	LinePoints[5][1][0] = -1; LinePoints[5][1][1] = 0; LinePoints[5][1][2] = -2; LinePoints[5][1][3] = -1; LinePoints[5][1][4] = -3; LinePoints[5][1][5] = -2; LinePoints[5][1][6] = -4; LinePoints[5][1][7] = -3; LinePoints[5][1][8] = -5; LinePoints[5][1][9] = -4; LinePoints[5][1][10] = -6; LinePoints[5][1][11] = -5;
	LinePoints[5][2][0] = -1; LinePoints[5][2][1] = 0; LinePoints[5][2][2] = -2; LinePoints[5][2][3] = -1; LinePoints[5][2][4] = -3; LinePoints[5][2][5] = -2; LinePoints[5][2][6] = -4; LinePoints[5][2][7] = -2; LinePoints[5][2][8] = -5; LinePoints[5][2][9] = -3; LinePoints[5][2][10] = -6; LinePoints[5][2][11] = -4;
	LinePoints[5][3][0] = -1; LinePoints[5][3][1] = 0; LinePoints[5][3][2] = -2; LinePoints[5][3][3] = -1; LinePoints[5][3][4] = -3; LinePoints[5][3][5] = -1; LinePoints[5][3][6] = -4; LinePoints[5][3][7] = -2; LinePoints[5][3][8] = -5; LinePoints[5][3][9] = -2; LinePoints[5][3][10] = -6; LinePoints[5][3][11] = -3;
	LinePoints[5][4][0] = -1; LinePoints[5][4][1] = 0; LinePoints[5][4][2] = -2; LinePoints[5][4][3] = 0; LinePoints[5][4][4] = -3; LinePoints[5][4][5] = -1; LinePoints[5][4][6] = -4; LinePoints[5][4][7] = -1; LinePoints[5][4][8] = -5; LinePoints[5][4][9] = -2; LinePoints[5][4][10] = -6; LinePoints[5][4][11] = -2;
	LinePoints[5][5][0] = -1; LinePoints[5][5][1] = 0; LinePoints[5][5][2] = -2; LinePoints[5][5][3] = 0; LinePoints[5][5][4] = -3; LinePoints[5][5][5] = 0; LinePoints[5][5][6] = -4; LinePoints[5][5][7] = -1; LinePoints[5][5][8] = -5; LinePoints[5][5][9] = -1; LinePoints[5][5][10] = -6; LinePoints[5][5][11] = -1;
	LinePoints[5][6][0] = -1; LinePoints[5][6][1] = 0; LinePoints[5][6][2] = -2; LinePoints[5][6][3] = 0; LinePoints[5][6][4] = -3; LinePoints[5][6][5] = 0; LinePoints[5][6][6] = -4; LinePoints[5][6][7] = 0; LinePoints[5][6][8] = -5; LinePoints[5][6][9] = 0; LinePoints[5][6][10] = -6; LinePoints[5][6][11] = 0;
	LinePoints[5][7][0] = -1; LinePoints[5][7][1] = 0; LinePoints[5][7][2] = -2; LinePoints[5][7][3] = 0; LinePoints[5][7][4] = -3; LinePoints[5][7][5] = 0; LinePoints[5][7][6] = -4; LinePoints[5][7][7] = 0; LinePoints[5][7][8] = -5; LinePoints[5][7][9] = 0; LinePoints[5][7][10] = -6; LinePoints[5][7][11] = 0;
	LinePoints[5][8][0] = -1; LinePoints[5][8][1] = 0; LinePoints[5][8][2] = -2; LinePoints[5][8][3] = 0; LinePoints[5][8][4] = -3; LinePoints[5][8][5] = 0; LinePoints[5][8][6] = -4; LinePoints[5][8][7] = 0; LinePoints[5][8][8] = -5; LinePoints[5][8][9] = 0; LinePoints[5][8][10] = -6; LinePoints[5][8][11] = 0;
	LinePoints[5][9][0] = -1; LinePoints[5][9][1] = 0; LinePoints[5][9][2] = -2; LinePoints[5][9][3] = 0; LinePoints[5][9][4] = -3; LinePoints[5][9][5] = 0; LinePoints[5][9][6] = -4; LinePoints[5][9][7] = 1; LinePoints[5][9][8] = -5; LinePoints[5][9][9] = 1; LinePoints[5][9][10] = -6; LinePoints[5][9][11] = 1;
	LinePoints[5][10][0] = -1; LinePoints[5][10][1] = 0; LinePoints[5][10][2] = -2; LinePoints[5][10][3] = 0; LinePoints[5][10][4] = -3; LinePoints[5][10][5] = 1; LinePoints[5][10][6] = -4; LinePoints[5][10][7] = 1; LinePoints[5][10][8] = -5; LinePoints[5][10][9] = 2; LinePoints[5][10][10] = -6; LinePoints[5][10][11] = 2;
	LinePoints[5][11][0] = -1; LinePoints[5][11][1] = 0; LinePoints[5][11][2] = -2; LinePoints[5][11][3] = 1; LinePoints[5][11][4] = -3; LinePoints[5][11][5] = 1; LinePoints[5][11][6] = -4; LinePoints[5][11][7] = 2; LinePoints[5][11][8] = -5; LinePoints[5][11][9] = 2; LinePoints[5][11][10] = -6; LinePoints[5][11][11] = 3;
	LinePoints[5][12][0] = -1; LinePoints[5][12][1] = 0; LinePoints[5][12][2] = -2; LinePoints[5][12][3] = 1; LinePoints[5][12][4] = -3; LinePoints[5][12][5] = 2; LinePoints[5][12][6] = -4; LinePoints[5][12][7] = 2; LinePoints[5][12][8] = -5; LinePoints[5][12][9] = 3; LinePoints[5][12][10] = -6; LinePoints[5][12][11] = 4;
	LinePoints[5][13][0] = -1; LinePoints[5][13][1] = 0; LinePoints[5][13][2] = -2; LinePoints[5][13][3] = 1; LinePoints[5][13][4] = -3; LinePoints[5][13][5] = 2; LinePoints[5][13][6] = -4; LinePoints[5][13][7] = 3; LinePoints[5][13][8] = -5; LinePoints[5][13][9] = 4; LinePoints[5][13][10] = -6; LinePoints[5][13][11] = 5;
	LinePoints[5][14][0] = -1; LinePoints[5][14][1] = 1; LinePoints[5][14][2] = -2; LinePoints[5][14][3] = 2; LinePoints[5][14][4] = -3; LinePoints[5][14][5] = 3; LinePoints[5][14][6] = -4; LinePoints[5][14][7] = 4; LinePoints[5][14][8] = -5; LinePoints[5][14][9] = 5; LinePoints[5][14][10] = -6; LinePoints[5][14][11] = 6;
	LinePoints[5][15][0] = 0; LinePoints[5][15][1] = -1; LinePoints[5][15][2] = -1; LinePoints[5][15][3] = -2; LinePoints[5][15][4] = -2; LinePoints[5][15][5] = -3; LinePoints[5][15][6] = -3; LinePoints[5][15][7] = -4; LinePoints[5][15][8] = -4; LinePoints[5][15][9] = -5; LinePoints[5][15][10] = -5; LinePoints[5][15][11] = -6;
	LinePoints[5][16][0] = 0; LinePoints[5][16][1] = 1; LinePoints[5][16][2] = -1; LinePoints[5][16][3] = 2; LinePoints[5][16][4] = -2; LinePoints[5][16][5] = 3; LinePoints[5][16][6] = -3; LinePoints[5][16][7] = 4; LinePoints[5][16][8] = -4; LinePoints[5][16][9] = 5; LinePoints[5][16][10] = -5; LinePoints[5][16][11] = 6;
	LinePoints[5][17][0] = 0; LinePoints[5][17][1] = -1; LinePoints[5][17][2] = -1; LinePoints[5][17][3] = -2; LinePoints[5][17][4] = -2; LinePoints[5][17][5] = -3; LinePoints[5][17][6] = -2; LinePoints[5][17][7] = -4; LinePoints[5][17][8] = -3; LinePoints[5][17][9] = -5; LinePoints[5][17][10] = -4; LinePoints[5][17][11] = -6;
	LinePoints[5][18][0] = 0; LinePoints[5][18][1] = 1; LinePoints[5][18][2] = -1; LinePoints[5][18][3] = 2; LinePoints[5][18][4] = -2; LinePoints[5][18][5] = 3; LinePoints[5][18][6] = -2; LinePoints[5][18][7] = 4; LinePoints[5][18][8] = -3; LinePoints[5][18][9] = 5; LinePoints[5][18][10] = -4; LinePoints[5][18][11] = 6;
	LinePoints[5][19][0] = 0; LinePoints[5][19][1] = -1; LinePoints[5][19][2] = -1; LinePoints[5][19][3] = -2; LinePoints[5][19][4] = -1; LinePoints[5][19][5] = -3; LinePoints[5][19][6] = -2; LinePoints[5][19][7] = -4; LinePoints[5][19][8] = -2; LinePoints[5][19][9] = -5; LinePoints[5][19][10] = -3; LinePoints[5][19][11] = -6;
	LinePoints[5][20][0] = 0; LinePoints[5][20][1] = 1; LinePoints[5][20][2] = -1; LinePoints[5][20][3] = 2; LinePoints[5][20][4] = -1; LinePoints[5][20][5] = 3; LinePoints[5][20][6] = -2; LinePoints[5][20][7] = 4; LinePoints[5][20][8] = -2; LinePoints[5][20][9] = 5; LinePoints[5][20][10] = -3; LinePoints[5][20][11] = 6;
	LinePoints[5][21][0] = 0; LinePoints[5][21][1] = -1; LinePoints[5][21][2] = 0; LinePoints[5][21][3] = -2; LinePoints[5][21][4] = -1; LinePoints[5][21][5] = -3; LinePoints[5][21][6] = -1; LinePoints[5][21][7] = -4; LinePoints[5][21][8] = -2; LinePoints[5][21][9] = -5; LinePoints[5][21][10] = -2; LinePoints[5][21][11] = -6;
	LinePoints[5][22][0] = 0; LinePoints[5][22][1] = 1; LinePoints[5][22][2] = 0; LinePoints[5][22][3] = 2; LinePoints[5][22][4] = -1; LinePoints[5][22][5] = 3; LinePoints[5][22][6] = -1; LinePoints[5][22][7] = 4; LinePoints[5][22][8] = -2; LinePoints[5][22][9] = 5; LinePoints[5][22][10] = -2; LinePoints[5][22][11] = 6;
	LinePoints[5][23][0] = 0; LinePoints[5][23][1] = -1; LinePoints[5][23][2] = 0; LinePoints[5][23][3] = -2; LinePoints[5][23][4] = 0; LinePoints[5][23][5] = -3; LinePoints[5][23][6] = -1; LinePoints[5][23][7] = -4; LinePoints[5][23][8] = -1; LinePoints[5][23][9] = -5; LinePoints[5][23][10] = -1; LinePoints[5][23][11] = -6;
	LinePoints[5][24][0] = 0; LinePoints[5][24][1] = 1; LinePoints[5][24][2] = 0; LinePoints[5][24][3] = 2; LinePoints[5][24][4] = 0; LinePoints[5][24][5] = 3; LinePoints[5][24][6] = -1; LinePoints[5][24][7] = 4; LinePoints[5][24][8] = -1; LinePoints[5][24][9] = 5; LinePoints[5][24][10] = -1; LinePoints[5][24][11] = 6;
	LinePoints[5][25][0] = 0; LinePoints[5][25][1] = -1; LinePoints[5][25][2] = 0; LinePoints[5][25][3] = -2; LinePoints[5][25][4] = 0; LinePoints[5][25][5] = -3; LinePoints[5][25][6] = 0; LinePoints[5][25][7] = -4; LinePoints[5][25][8] = 0; LinePoints[5][25][9] = -5; LinePoints[5][25][10] = 0; LinePoints[5][25][11] = -6;
	LinePoints[5][26][0] = 0; LinePoints[5][26][1] = 1; LinePoints[5][26][2] = 0; LinePoints[5][26][3] = 2; LinePoints[5][26][4] = 0; LinePoints[5][26][5] = 3; LinePoints[5][26][6] = 0; LinePoints[5][26][7] = 4; LinePoints[5][26][8] = 0; LinePoints[5][26][9] = 5; LinePoints[5][26][10] = 0; LinePoints[5][26][11] = 6;
	LinePoints[5][27][0] = 0; LinePoints[5][27][1] = -1; LinePoints[5][27][2] = 0; LinePoints[5][27][3] = -2; LinePoints[5][27][4] = 0; LinePoints[5][27][5] = -3; LinePoints[5][27][6] = 0; LinePoints[5][27][7] = -4; LinePoints[5][27][8] = 0; LinePoints[5][27][9] = -5; LinePoints[5][27][10] = 0; LinePoints[5][27][11] = -6;
	LinePoints[5][28][0] = 0; LinePoints[5][28][1] = 1; LinePoints[5][28][2] = 0; LinePoints[5][28][3] = 2; LinePoints[5][28][4] = 0; LinePoints[5][28][5] = 3; LinePoints[5][28][6] = 0; LinePoints[5][28][7] = 4; LinePoints[5][28][8] = 0; LinePoints[5][28][9] = 5; LinePoints[5][28][10] = 0; LinePoints[5][28][11] = 6;
	LinePoints[5][29][0] = 0; LinePoints[5][29][1] = -1; LinePoints[5][29][2] = 0; LinePoints[5][29][3] = -2; LinePoints[5][29][4] = 0; LinePoints[5][29][5] = -3; LinePoints[5][29][6] = 0; LinePoints[5][29][7] = -4; LinePoints[5][29][8] = 0; LinePoints[5][29][9] = -5; LinePoints[5][29][10] = 0; LinePoints[5][29][11] = -6;
	LinePoints[5][30][0] = 0; LinePoints[5][30][1] = 1; LinePoints[5][30][2] = 0; LinePoints[5][30][3] = 2; LinePoints[5][30][4] = 0; LinePoints[5][30][5] = 3; LinePoints[5][30][6] = 0; LinePoints[5][30][7] = 4; LinePoints[5][30][8] = 0; LinePoints[5][30][9] = 5; LinePoints[5][30][10] = 0; LinePoints[5][30][11] = 6;
	LinePoints[5][31][0] = 0; LinePoints[5][31][1] = -1; LinePoints[5][31][2] = 0; LinePoints[5][31][3] = -2; LinePoints[5][31][4] = 0; LinePoints[5][31][5] = -3; LinePoints[5][31][6] = 1; LinePoints[5][31][7] = -4; LinePoints[5][31][8] = 1; LinePoints[5][31][9] = -5; LinePoints[5][31][10] = 1; LinePoints[5][31][11] = -6;
	LinePoints[5][32][0] = 0; LinePoints[5][32][1] = 1; LinePoints[5][32][2] = 0; LinePoints[5][32][3] = 2; LinePoints[5][32][4] = 0; LinePoints[5][32][5] = 3; LinePoints[5][32][6] = 1; LinePoints[5][32][7] = 4; LinePoints[5][32][8] = 1; LinePoints[5][32][9] = 5; LinePoints[5][32][10] = 1; LinePoints[5][32][11] = 6;
	LinePoints[5][33][0] = 0; LinePoints[5][33][1] = -1; LinePoints[5][33][2] = 0; LinePoints[5][33][3] = -2; LinePoints[5][33][4] = 1; LinePoints[5][33][5] = -3; LinePoints[5][33][6] = 1; LinePoints[5][33][7] = -4; LinePoints[5][33][8] = 2; LinePoints[5][33][9] = -5; LinePoints[5][33][10] = 2; LinePoints[5][33][11] = -6;
	LinePoints[5][34][0] = 0; LinePoints[5][34][1] = 1; LinePoints[5][34][2] = 0; LinePoints[5][34][3] = 2; LinePoints[5][34][4] = 1; LinePoints[5][34][5] = 3; LinePoints[5][34][6] = 1; LinePoints[5][34][7] = 4; LinePoints[5][34][8] = 2; LinePoints[5][34][9] = 5; LinePoints[5][34][10] = 2; LinePoints[5][34][11] = 6;
	LinePoints[5][35][0] = 0; LinePoints[5][35][1] = -1; LinePoints[5][35][2] = 1; LinePoints[5][35][3] = -2; LinePoints[5][35][4] = 1; LinePoints[5][35][5] = -3; LinePoints[5][35][6] = 2; LinePoints[5][35][7] = -4; LinePoints[5][35][8] = 2; LinePoints[5][35][9] = -5; LinePoints[5][35][10] = 3; LinePoints[5][35][11] = -6;
	LinePoints[5][36][0] = 0; LinePoints[5][36][1] = 1; LinePoints[5][36][2] = 1; LinePoints[5][36][3] = 2; LinePoints[5][36][4] = 1; LinePoints[5][36][5] = 3; LinePoints[5][36][6] = 2; LinePoints[5][36][7] = 4; LinePoints[5][36][8] = 2; LinePoints[5][36][9] = 5; LinePoints[5][36][10] = 3; LinePoints[5][36][11] = 6;
	LinePoints[5][37][0] = 0; LinePoints[5][37][1] = -1; LinePoints[5][37][2] = 1; LinePoints[5][37][3] = -2; LinePoints[5][37][4] = 2; LinePoints[5][37][5] = -3; LinePoints[5][37][6] = 2; LinePoints[5][37][7] = -4; LinePoints[5][37][8] = 3; LinePoints[5][37][9] = -5; LinePoints[5][37][10] = 4; LinePoints[5][37][11] = -6;
	LinePoints[5][38][0] = 0; LinePoints[5][38][1] = 1; LinePoints[5][38][2] = 1; LinePoints[5][38][3] = 2; LinePoints[5][38][4] = 2; LinePoints[5][38][5] = 3; LinePoints[5][38][6] = 2; LinePoints[5][38][7] = 4; LinePoints[5][38][8] = 3; LinePoints[5][38][9] = 5; LinePoints[5][38][10] = 4; LinePoints[5][38][11] = 6;
	LinePoints[5][39][0] = 0; LinePoints[5][39][1] = -1; LinePoints[5][39][2] = 1; LinePoints[5][39][3] = -2; LinePoints[5][39][4] = 2; LinePoints[5][39][5] = -3; LinePoints[5][39][6] = 3; LinePoints[5][39][7] = -4; LinePoints[5][39][8] = 4; LinePoints[5][39][9] = -5; LinePoints[5][39][10] = 5; LinePoints[5][39][11] = -6;
	LinePoints[5][40][0] = 0; LinePoints[5][40][1] = 1; LinePoints[5][40][2] = 1; LinePoints[5][40][3] = 2; LinePoints[5][40][4] = 2; LinePoints[5][40][5] = 3; LinePoints[5][40][6] = 3; LinePoints[5][40][7] = 4; LinePoints[5][40][8] = 4; LinePoints[5][40][9] = 5; LinePoints[5][40][10] = 5; LinePoints[5][40][11] = 6;
	LinePoints[5][41][0] = 1; LinePoints[5][41][1] = -1; LinePoints[5][41][2] = 2; LinePoints[5][41][3] = -2; LinePoints[5][41][4] = 3; LinePoints[5][41][5] = -3; LinePoints[5][41][6] = 4; LinePoints[5][41][7] = -4; LinePoints[5][41][8] = 5; LinePoints[5][41][9] = -5; LinePoints[5][41][10] = 6; LinePoints[5][41][11] = -6;
	LinePoints[5][42][0] = 1; LinePoints[5][42][1] = 0; LinePoints[5][42][2] = 2; LinePoints[5][42][3] = -1; LinePoints[5][42][4] = 3; LinePoints[5][42][5] = -2; LinePoints[5][42][6] = 4; LinePoints[5][42][7] = -3; LinePoints[5][42][8] = 5; LinePoints[5][42][9] = -4; LinePoints[5][42][10] = 6; LinePoints[5][42][11] = -5;
	LinePoints[5][43][0] = 1; LinePoints[5][43][1] = 0; LinePoints[5][43][2] = 2; LinePoints[5][43][3] = -1; LinePoints[5][43][4] = 3; LinePoints[5][43][5] = -2; LinePoints[5][43][6] = 4; LinePoints[5][43][7] = -2; LinePoints[5][43][8] = 5; LinePoints[5][43][9] = -3; LinePoints[5][43][10] = 6; LinePoints[5][43][11] = -4;
	LinePoints[5][44][0] = 1; LinePoints[5][44][1] = 0; LinePoints[5][44][2] = 2; LinePoints[5][44][3] = -1; LinePoints[5][44][4] = 3; LinePoints[5][44][5] = -1; LinePoints[5][44][6] = 4; LinePoints[5][44][7] = -2; LinePoints[5][44][8] = 5; LinePoints[5][44][9] = -2; LinePoints[5][44][10] = 6; LinePoints[5][44][11] = -3;
	LinePoints[5][45][0] = 1; LinePoints[5][45][1] = 0; LinePoints[5][45][2] = 2; LinePoints[5][45][3] = 0; LinePoints[5][45][4] = 3; LinePoints[5][45][5] = -1; LinePoints[5][45][6] = 4; LinePoints[5][45][7] = -1; LinePoints[5][45][8] = 5; LinePoints[5][45][9] = -2; LinePoints[5][45][10] = 6; LinePoints[5][45][11] = -2;
	LinePoints[5][46][0] = 1; LinePoints[5][46][1] = 0; LinePoints[5][46][2] = 2; LinePoints[5][46][3] = 0; LinePoints[5][46][4] = 3; LinePoints[5][46][5] = 0; LinePoints[5][46][6] = 4; LinePoints[5][46][7] = -1; LinePoints[5][46][8] = 5; LinePoints[5][46][9] = -1; LinePoints[5][46][10] = 6; LinePoints[5][46][11] = -1;
	LinePoints[5][47][0] = 1; LinePoints[5][47][1] = 0; LinePoints[5][47][2] = 2; LinePoints[5][47][3] = 0; LinePoints[5][47][4] = 3; LinePoints[5][47][5] = 0; LinePoints[5][47][6] = 4; LinePoints[5][47][7] = 0; LinePoints[5][47][8] = 5; LinePoints[5][47][9] = 0; LinePoints[5][47][10] = 6; LinePoints[5][47][11] = 0;
	LinePoints[5][48][0] = 1; LinePoints[5][48][1] = 0; LinePoints[5][48][2] = 2; LinePoints[5][48][3] = 0; LinePoints[5][48][4] = 3; LinePoints[5][48][5] = 0; LinePoints[5][48][6] = 4; LinePoints[5][48][7] = 0; LinePoints[5][48][8] = 5; LinePoints[5][48][9] = 0; LinePoints[5][48][10] = 6; LinePoints[5][48][11] = 0;
	LinePoints[5][49][0] = 1; LinePoints[5][49][1] = 0; LinePoints[5][49][2] = 2; LinePoints[5][49][3] = 0; LinePoints[5][49][4] = 3; LinePoints[5][49][5] = 0; LinePoints[5][49][6] = 4; LinePoints[5][49][7] = 0; LinePoints[5][49][8] = 5; LinePoints[5][49][9] = 0; LinePoints[5][49][10] = 6; LinePoints[5][49][11] = 0;
	LinePoints[5][50][0] = 1; LinePoints[5][50][1] = 0; LinePoints[5][50][2] = 2; LinePoints[5][50][3] = 0; LinePoints[5][50][4] = 3; LinePoints[5][50][5] = 0; LinePoints[5][50][6] = 4; LinePoints[5][50][7] = 1; LinePoints[5][50][8] = 5; LinePoints[5][50][9] = 1; LinePoints[5][50][10] = 6; LinePoints[5][50][11] = 1;
	LinePoints[5][51][0] = 1; LinePoints[5][51][1] = 0; LinePoints[5][51][2] = 2; LinePoints[5][51][3] = 0; LinePoints[5][51][4] = 3; LinePoints[5][51][5] = 1; LinePoints[5][51][6] = 4; LinePoints[5][51][7] = 1; LinePoints[5][51][8] = 5; LinePoints[5][51][9] = 2; LinePoints[5][51][10] = 6; LinePoints[5][51][11] = 2;
	LinePoints[5][52][0] = 1; LinePoints[5][52][1] = 0; LinePoints[5][52][2] = 2; LinePoints[5][52][3] = 1; LinePoints[5][52][4] = 3; LinePoints[5][52][5] = 1; LinePoints[5][52][6] = 4; LinePoints[5][52][7] = 2; LinePoints[5][52][8] = 5; LinePoints[5][52][9] = 2; LinePoints[5][52][10] = 6; LinePoints[5][52][11] = 3;
	LinePoints[5][53][0] = 1; LinePoints[5][53][1] = 0; LinePoints[5][53][2] = 2; LinePoints[5][53][3] = 1; LinePoints[5][53][4] = 3; LinePoints[5][53][5] = 2; LinePoints[5][53][6] = 4; LinePoints[5][53][7] = 2; LinePoints[5][53][8] = 5; LinePoints[5][53][9] = 3; LinePoints[5][53][10] = 6; LinePoints[5][53][11] = 4;
	LinePoints[5][54][0] = 1; LinePoints[5][54][1] = 0; LinePoints[5][54][2] = 2; LinePoints[5][54][3] = 1; LinePoints[5][54][4] = 3; LinePoints[5][54][5] = 2; LinePoints[5][54][6] = 4; LinePoints[5][54][7] = 3; LinePoints[5][54][8] = 5; LinePoints[5][54][9] = 4; LinePoints[5][54][10] = 6; LinePoints[5][54][11] = 5;
	LinePoints[5][55][0] = 1; LinePoints[5][55][1] = 1; LinePoints[5][55][2] = 2; LinePoints[5][55][3] = 2; LinePoints[5][55][4] = 3; LinePoints[5][55][5] = 3; LinePoints[5][55][6] = 4; LinePoints[5][55][7] = 4; LinePoints[5][55][8] = 5; LinePoints[5][55][9] = 5; LinePoints[5][55][10] = 6; LinePoints[5][55][11] = 6;
	LinePoints[6][0][0] = -1; LinePoints[6][0][1] = -1; LinePoints[6][0][2] = -2; LinePoints[6][0][3] = -2; LinePoints[6][0][4] = -3; LinePoints[6][0][5] = -3; LinePoints[6][0][6] = -4; LinePoints[6][0][7] = -4; LinePoints[6][0][8] = -5; LinePoints[6][0][9] = -5; LinePoints[6][0][10] = -6; LinePoints[6][0][11] = -6; LinePoints[6][0][12] = -7; LinePoints[6][0][13] = -7;
	LinePoints[6][1][0] = -1; LinePoints[6][1][1] = 0; LinePoints[6][1][2] = -2; LinePoints[6][1][3] = -1; LinePoints[6][1][4] = -3; LinePoints[6][1][5] = -2; LinePoints[6][1][6] = -4; LinePoints[6][1][7] = -3; LinePoints[6][1][8] = -5; LinePoints[6][1][9] = -4; LinePoints[6][1][10] = -6; LinePoints[6][1][11] = -5; LinePoints[6][1][12] = -7; LinePoints[6][1][13] = -6;
	LinePoints[6][2][0] = -1; LinePoints[6][2][1] = 0; LinePoints[6][2][2] = -2; LinePoints[6][2][3] = -1; LinePoints[6][2][4] = -3; LinePoints[6][2][5] = -2; LinePoints[6][2][6] = -4; LinePoints[6][2][7] = -3; LinePoints[6][2][8] = -5; LinePoints[6][2][9] = -3; LinePoints[6][2][10] = -6; LinePoints[6][2][11] = -4; LinePoints[6][2][12] = -7; LinePoints[6][2][13] = -5;
	LinePoints[6][3][0] = -1; LinePoints[6][3][1] = 0; LinePoints[6][3][2] = -2; LinePoints[6][3][3] = -1; LinePoints[6][3][4] = -3; LinePoints[6][3][5] = -1; LinePoints[6][3][6] = -4; LinePoints[6][3][7] = -2; LinePoints[6][3][8] = -5; LinePoints[6][3][9] = -3; LinePoints[6][3][10] = -6; LinePoints[6][3][11] = -3; LinePoints[6][3][12] = -7; LinePoints[6][3][13] = -4;
	LinePoints[6][4][0] = -1; LinePoints[6][4][1] = 0; LinePoints[6][4][2] = -2; LinePoints[6][4][3] = -1; LinePoints[6][4][4] = -3; LinePoints[6][4][5] = -1; LinePoints[6][4][6] = -4; LinePoints[6][4][7] = -2; LinePoints[6][4][8] = -5; LinePoints[6][4][9] = -2; LinePoints[6][4][10] = -6; LinePoints[6][4][11] = -3; LinePoints[6][4][12] = -7; LinePoints[6][4][13] = -3;
	LinePoints[6][5][0] = -1; LinePoints[6][5][1] = 0; LinePoints[6][5][2] = -2; LinePoints[6][5][3] = 0; LinePoints[6][5][4] = -3; LinePoints[6][5][5] = -1; LinePoints[6][5][6] = -4; LinePoints[6][5][7] = -1; LinePoints[6][5][8] = -5; LinePoints[6][5][9] = -1; LinePoints[6][5][10] = -6; LinePoints[6][5][11] = -2; LinePoints[6][5][12] = -7; LinePoints[6][5][13] = -2;
	LinePoints[6][6][0] = -1; LinePoints[6][6][1] = 0; LinePoints[6][6][2] = -2; LinePoints[6][6][3] = 0; LinePoints[6][6][4] = -3; LinePoints[6][6][5] = 0; LinePoints[6][6][6] = -4; LinePoints[6][6][7] = -1; LinePoints[6][6][8] = -5; LinePoints[6][6][9] = -1; LinePoints[6][6][10] = -6; LinePoints[6][6][11] = -1; LinePoints[6][6][12] = -7; LinePoints[6][6][13] = -1;
	LinePoints[6][7][0] = -1; LinePoints[6][7][1] = 0; LinePoints[6][7][2] = -2; LinePoints[6][7][3] = 0; LinePoints[6][7][4] = -3; LinePoints[6][7][5] = 0; LinePoints[6][7][6] = -4; LinePoints[6][7][7] = 0; LinePoints[6][7][8] = -5; LinePoints[6][7][9] = 0; LinePoints[6][7][10] = -6; LinePoints[6][7][11] = 0; LinePoints[6][7][12] = -7; LinePoints[6][7][13] = 0;
	LinePoints[6][8][0] = -1; LinePoints[6][8][1] = 0; LinePoints[6][8][2] = -2; LinePoints[6][8][3] = 0; LinePoints[6][8][4] = -3; LinePoints[6][8][5] = 0; LinePoints[6][8][6] = -4; LinePoints[6][8][7] = 0; LinePoints[6][8][8] = -5; LinePoints[6][8][9] = 0; LinePoints[6][8][10] = -6; LinePoints[6][8][11] = 0; LinePoints[6][8][12] = -7; LinePoints[6][8][13] = 0;
	LinePoints[6][9][0] = -1; LinePoints[6][9][1] = 0; LinePoints[6][9][2] = -2; LinePoints[6][9][3] = 0; LinePoints[6][9][4] = -3; LinePoints[6][9][5] = 0; LinePoints[6][9][6] = -4; LinePoints[6][9][7] = 0; LinePoints[6][9][8] = -5; LinePoints[6][9][9] = 0; LinePoints[6][9][10] = -6; LinePoints[6][9][11] = 0; LinePoints[6][9][12] = -7; LinePoints[6][9][13] = 0;
	LinePoints[6][10][0] = -1; LinePoints[6][10][1] = 0; LinePoints[6][10][2] = -2; LinePoints[6][10][3] = 0; LinePoints[6][10][4] = -3; LinePoints[6][10][5] = 0; LinePoints[6][10][6] = -4; LinePoints[6][10][7] = 1; LinePoints[6][10][8] = -5; LinePoints[6][10][9] = 1; LinePoints[6][10][10] = -6; LinePoints[6][10][11] = 1; LinePoints[6][10][12] = -7; LinePoints[6][10][13] = 1;
	LinePoints[6][11][0] = -1; LinePoints[6][11][1] = 0; LinePoints[6][11][2] = -2; LinePoints[6][11][3] = 0; LinePoints[6][11][4] = -3; LinePoints[6][11][5] = 1; LinePoints[6][11][6] = -4; LinePoints[6][11][7] = 1; LinePoints[6][11][8] = -5; LinePoints[6][11][9] = 1; LinePoints[6][11][10] = -6; LinePoints[6][11][11] = 2; LinePoints[6][11][12] = -7; LinePoints[6][11][13] = 2;
	LinePoints[6][12][0] = -1; LinePoints[6][12][1] = 0; LinePoints[6][12][2] = -2; LinePoints[6][12][3] = 1; LinePoints[6][12][4] = -3; LinePoints[6][12][5] = 1; LinePoints[6][12][6] = -4; LinePoints[6][12][7] = 2; LinePoints[6][12][8] = -5; LinePoints[6][12][9] = 2; LinePoints[6][12][10] = -6; LinePoints[6][12][11] = 3; LinePoints[6][12][12] = -7; LinePoints[6][12][13] = 3;
	LinePoints[6][13][0] = -1; LinePoints[6][13][1] = 0; LinePoints[6][13][2] = -2; LinePoints[6][13][3] = 1; LinePoints[6][13][4] = -3; LinePoints[6][13][5] = 1; LinePoints[6][13][6] = -4; LinePoints[6][13][7] = 2; LinePoints[6][13][8] = -5; LinePoints[6][13][9] = 3; LinePoints[6][13][10] = -6; LinePoints[6][13][11] = 3; LinePoints[6][13][12] = -7; LinePoints[6][13][13] = 4;
	LinePoints[6][14][0] = -1; LinePoints[6][14][1] = 0; LinePoints[6][14][2] = -2; LinePoints[6][14][3] = 1; LinePoints[6][14][4] = -3; LinePoints[6][14][5] = 2; LinePoints[6][14][6] = -4; LinePoints[6][14][7] = 3; LinePoints[6][14][8] = -5; LinePoints[6][14][9] = 3; LinePoints[6][14][10] = -6; LinePoints[6][14][11] = 4; LinePoints[6][14][12] = -7; LinePoints[6][14][13] = 5;
	LinePoints[6][15][0] = -1; LinePoints[6][15][1] = 0; LinePoints[6][15][2] = -2; LinePoints[6][15][3] = 1; LinePoints[6][15][4] = -3; LinePoints[6][15][5] = 2; LinePoints[6][15][6] = -4; LinePoints[6][15][7] = 3; LinePoints[6][15][8] = -5; LinePoints[6][15][9] = 4; LinePoints[6][15][10] = -6; LinePoints[6][15][11] = 5; LinePoints[6][15][12] = -7; LinePoints[6][15][13] = 6;
	LinePoints[6][16][0] = -1; LinePoints[6][16][1] = 1; LinePoints[6][16][2] = -2; LinePoints[6][16][3] = 2; LinePoints[6][16][4] = -3; LinePoints[6][16][5] = 3; LinePoints[6][16][6] = -4; LinePoints[6][16][7] = 4; LinePoints[6][16][8] = -5; LinePoints[6][16][9] = 5; LinePoints[6][16][10] = -6; LinePoints[6][16][11] = 6; LinePoints[6][16][12] = -7; LinePoints[6][16][13] = 7;
	LinePoints[6][17][0] = 0; LinePoints[6][17][1] = -1; LinePoints[6][17][2] = -1; LinePoints[6][17][3] = -2; LinePoints[6][17][4] = -2; LinePoints[6][17][5] = -3; LinePoints[6][17][6] = -3; LinePoints[6][17][7] = -4; LinePoints[6][17][8] = -4; LinePoints[6][17][9] = -5; LinePoints[6][17][10] = -5; LinePoints[6][17][11] = -6; LinePoints[6][17][12] = -6; LinePoints[6][17][13] = -7;
	LinePoints[6][18][0] = 0; LinePoints[6][18][1] = 1; LinePoints[6][18][2] = -1; LinePoints[6][18][3] = 2; LinePoints[6][18][4] = -2; LinePoints[6][18][5] = 3; LinePoints[6][18][6] = -3; LinePoints[6][18][7] = 4; LinePoints[6][18][8] = -4; LinePoints[6][18][9] = 5; LinePoints[6][18][10] = -5; LinePoints[6][18][11] = 6; LinePoints[6][18][12] = -6; LinePoints[6][18][13] = 7;
	LinePoints[6][19][0] = 0; LinePoints[6][19][1] = -1; LinePoints[6][19][2] = -1; LinePoints[6][19][3] = -2; LinePoints[6][19][4] = -2; LinePoints[6][19][5] = -3; LinePoints[6][19][6] = -3; LinePoints[6][19][7] = -4; LinePoints[6][19][8] = -3; LinePoints[6][19][9] = -5; LinePoints[6][19][10] = -4; LinePoints[6][19][11] = -6; LinePoints[6][19][12] = -5; LinePoints[6][19][13] = -7;
	LinePoints[6][20][0] = 0; LinePoints[6][20][1] = 1; LinePoints[6][20][2] = -1; LinePoints[6][20][3] = 2; LinePoints[6][20][4] = -2; LinePoints[6][20][5] = 3; LinePoints[6][20][6] = -3; LinePoints[6][20][7] = 4; LinePoints[6][20][8] = -3; LinePoints[6][20][9] = 5; LinePoints[6][20][10] = -4; LinePoints[6][20][11] = 6; LinePoints[6][20][12] = -5; LinePoints[6][20][13] = 7;
	LinePoints[6][21][0] = 0; LinePoints[6][21][1] = -1; LinePoints[6][21][2] = -1; LinePoints[6][21][3] = -2; LinePoints[6][21][4] = -1; LinePoints[6][21][5] = -3; LinePoints[6][21][6] = -2; LinePoints[6][21][7] = -4; LinePoints[6][21][8] = -3; LinePoints[6][21][9] = -5; LinePoints[6][21][10] = -3; LinePoints[6][21][11] = -6; LinePoints[6][21][12] = -4; LinePoints[6][21][13] = -7;
	LinePoints[6][22][0] = 0; LinePoints[6][22][1] = 1; LinePoints[6][22][2] = -1; LinePoints[6][22][3] = 2; LinePoints[6][22][4] = -1; LinePoints[6][22][5] = 3; LinePoints[6][22][6] = -2; LinePoints[6][22][7] = 4; LinePoints[6][22][8] = -3; LinePoints[6][22][9] = 5; LinePoints[6][22][10] = -3; LinePoints[6][22][11] = 6; LinePoints[6][22][12] = -4; LinePoints[6][22][13] = 7;
	LinePoints[6][23][0] = 0; LinePoints[6][23][1] = -1; LinePoints[6][23][2] = -1; LinePoints[6][23][3] = -2; LinePoints[6][23][4] = -1; LinePoints[6][23][5] = -3; LinePoints[6][23][6] = -2; LinePoints[6][23][7] = -4; LinePoints[6][23][8] = -2; LinePoints[6][23][9] = -5; LinePoints[6][23][10] = -3; LinePoints[6][23][11] = -6; LinePoints[6][23][12] = -3; LinePoints[6][23][13] = -7;
	LinePoints[6][24][0] = 0; LinePoints[6][24][1] = 1; LinePoints[6][24][2] = -1; LinePoints[6][24][3] = 2; LinePoints[6][24][4] = -1; LinePoints[6][24][5] = 3; LinePoints[6][24][6] = -2; LinePoints[6][24][7] = 4; LinePoints[6][24][8] = -2; LinePoints[6][24][9] = 5; LinePoints[6][24][10] = -3; LinePoints[6][24][11] = 6; LinePoints[6][24][12] = -3; LinePoints[6][24][13] = 7;
	LinePoints[6][25][0] = 0; LinePoints[6][25][1] = -1; LinePoints[6][25][2] = 0; LinePoints[6][25][3] = -2; LinePoints[6][25][4] = -1; LinePoints[6][25][5] = -3; LinePoints[6][25][6] = -1; LinePoints[6][25][7] = -4; LinePoints[6][25][8] = -1; LinePoints[6][25][9] = -5; LinePoints[6][25][10] = -2; LinePoints[6][25][11] = -6; LinePoints[6][25][12] = -2; LinePoints[6][25][13] = -7;
	LinePoints[6][26][0] = 0; LinePoints[6][26][1] = 1; LinePoints[6][26][2] = 0; LinePoints[6][26][3] = 2; LinePoints[6][26][4] = -1; LinePoints[6][26][5] = 3; LinePoints[6][26][6] = -1; LinePoints[6][26][7] = 4; LinePoints[6][26][8] = -1; LinePoints[6][26][9] = 5; LinePoints[6][26][10] = -2; LinePoints[6][26][11] = 6; LinePoints[6][26][12] = -2; LinePoints[6][26][13] = 7;
	LinePoints[6][27][0] = 0; LinePoints[6][27][1] = -1; LinePoints[6][27][2] = 0; LinePoints[6][27][3] = -2; LinePoints[6][27][4] = 0; LinePoints[6][27][5] = -3; LinePoints[6][27][6] = -1; LinePoints[6][27][7] = -4; LinePoints[6][27][8] = -1; LinePoints[6][27][9] = -5; LinePoints[6][27][10] = -1; LinePoints[6][27][11] = -6; LinePoints[6][27][12] = -1; LinePoints[6][27][13] = -7;
	LinePoints[6][28][0] = 0; LinePoints[6][28][1] = 1; LinePoints[6][28][2] = 0; LinePoints[6][28][3] = 2; LinePoints[6][28][4] = 0; LinePoints[6][28][5] = 3; LinePoints[6][28][6] = -1; LinePoints[6][28][7] = 4; LinePoints[6][28][8] = -1; LinePoints[6][28][9] = 5; LinePoints[6][28][10] = -1; LinePoints[6][28][11] = 6; LinePoints[6][28][12] = -1; LinePoints[6][28][13] = 7;
	LinePoints[6][29][0] = 0; LinePoints[6][29][1] = -1; LinePoints[6][29][2] = 0; LinePoints[6][29][3] = -2; LinePoints[6][29][4] = 0; LinePoints[6][29][5] = -3; LinePoints[6][29][6] = 0; LinePoints[6][29][7] = -4; LinePoints[6][29][8] = 0; LinePoints[6][29][9] = -5; LinePoints[6][29][10] = 0; LinePoints[6][29][11] = -6; LinePoints[6][29][12] = 0; LinePoints[6][29][13] = -7;
	LinePoints[6][30][0] = 0; LinePoints[6][30][1] = 1; LinePoints[6][30][2] = 0; LinePoints[6][30][3] = 2; LinePoints[6][30][4] = 0; LinePoints[6][30][5] = 3; LinePoints[6][30][6] = 0; LinePoints[6][30][7] = 4; LinePoints[6][30][8] = 0; LinePoints[6][30][9] = 5; LinePoints[6][30][10] = 0; LinePoints[6][30][11] = 6; LinePoints[6][30][12] = 0; LinePoints[6][30][13] = 7;
	LinePoints[6][31][0] = 0; LinePoints[6][31][1] = -1; LinePoints[6][31][2] = 0; LinePoints[6][31][3] = -2; LinePoints[6][31][4] = 0; LinePoints[6][31][5] = -3; LinePoints[6][31][6] = 0; LinePoints[6][31][7] = -4; LinePoints[6][31][8] = 0; LinePoints[6][31][9] = -5; LinePoints[6][31][10] = 0; LinePoints[6][31][11] = -6; LinePoints[6][31][12] = 0; LinePoints[6][31][13] = -7;
	LinePoints[6][32][0] = 0; LinePoints[6][32][1] = 1; LinePoints[6][32][2] = 0; LinePoints[6][32][3] = 2; LinePoints[6][32][4] = 0; LinePoints[6][32][5] = 3; LinePoints[6][32][6] = 0; LinePoints[6][32][7] = 4; LinePoints[6][32][8] = 0; LinePoints[6][32][9] = 5; LinePoints[6][32][10] = 0; LinePoints[6][32][11] = 6; LinePoints[6][32][12] = 0; LinePoints[6][32][13] = 7;
	LinePoints[6][33][0] = 0; LinePoints[6][33][1] = -1; LinePoints[6][33][2] = 0; LinePoints[6][33][3] = -2; LinePoints[6][33][4] = 0; LinePoints[6][33][5] = -3; LinePoints[6][33][6] = 0; LinePoints[6][33][7] = -4; LinePoints[6][33][8] = 0; LinePoints[6][33][9] = -5; LinePoints[6][33][10] = 0; LinePoints[6][33][11] = -6; LinePoints[6][33][12] = 0; LinePoints[6][33][13] = -7;
	LinePoints[6][34][0] = 0; LinePoints[6][34][1] = 1; LinePoints[6][34][2] = 0; LinePoints[6][34][3] = 2; LinePoints[6][34][4] = 0; LinePoints[6][34][5] = 3; LinePoints[6][34][6] = 0; LinePoints[6][34][7] = 4; LinePoints[6][34][8] = 0; LinePoints[6][34][9] = 5; LinePoints[6][34][10] = 0; LinePoints[6][34][11] = 6; LinePoints[6][34][12] = 0; LinePoints[6][34][13] = 7;
	LinePoints[6][35][0] = 0; LinePoints[6][35][1] = -1; LinePoints[6][35][2] = 0; LinePoints[6][35][3] = -2; LinePoints[6][35][4] = 0; LinePoints[6][35][5] = -3; LinePoints[6][35][6] = 1; LinePoints[6][35][7] = -4; LinePoints[6][35][8] = 1; LinePoints[6][35][9] = -5; LinePoints[6][35][10] = 1; LinePoints[6][35][11] = -6; LinePoints[6][35][12] = 1; LinePoints[6][35][13] = -7;
	LinePoints[6][36][0] = 0; LinePoints[6][36][1] = 1; LinePoints[6][36][2] = 0; LinePoints[6][36][3] = 2; LinePoints[6][36][4] = 0; LinePoints[6][36][5] = 3; LinePoints[6][36][6] = 1; LinePoints[6][36][7] = 4; LinePoints[6][36][8] = 1; LinePoints[6][36][9] = 5; LinePoints[6][36][10] = 1; LinePoints[6][36][11] = 6; LinePoints[6][36][12] = 1; LinePoints[6][36][13] = 7;
	LinePoints[6][37][0] = 0; LinePoints[6][37][1] = -1; LinePoints[6][37][2] = 0; LinePoints[6][37][3] = -2; LinePoints[6][37][4] = 1; LinePoints[6][37][5] = -3; LinePoints[6][37][6] = 1; LinePoints[6][37][7] = -4; LinePoints[6][37][8] = 1; LinePoints[6][37][9] = -5; LinePoints[6][37][10] = 2; LinePoints[6][37][11] = -6; LinePoints[6][37][12] = 2; LinePoints[6][37][13] = -7;
	LinePoints[6][38][0] = 0; LinePoints[6][38][1] = 1; LinePoints[6][38][2] = 0; LinePoints[6][38][3] = 2; LinePoints[6][38][4] = 1; LinePoints[6][38][5] = 3; LinePoints[6][38][6] = 1; LinePoints[6][38][7] = 4; LinePoints[6][38][8] = 1; LinePoints[6][38][9] = 5; LinePoints[6][38][10] = 2; LinePoints[6][38][11] = 6; LinePoints[6][38][12] = 2; LinePoints[6][38][13] = 7;
	LinePoints[6][39][0] = 0; LinePoints[6][39][1] = -1; LinePoints[6][39][2] = 1; LinePoints[6][39][3] = -2; LinePoints[6][39][4] = 1; LinePoints[6][39][5] = -3; LinePoints[6][39][6] = 2; LinePoints[6][39][7] = -4; LinePoints[6][39][8] = 2; LinePoints[6][39][9] = -5; LinePoints[6][39][10] = 3; LinePoints[6][39][11] = -6; LinePoints[6][39][12] = 3; LinePoints[6][39][13] = -7;
	LinePoints[6][40][0] = 0; LinePoints[6][40][1] = 1; LinePoints[6][40][2] = 1; LinePoints[6][40][3] = 2; LinePoints[6][40][4] = 1; LinePoints[6][40][5] = 3; LinePoints[6][40][6] = 2; LinePoints[6][40][7] = 4; LinePoints[6][40][8] = 2; LinePoints[6][40][9] = 5; LinePoints[6][40][10] = 3; LinePoints[6][40][11] = 6; LinePoints[6][40][12] = 3; LinePoints[6][40][13] = 7;
	LinePoints[6][41][0] = 0; LinePoints[6][41][1] = -1; LinePoints[6][41][2] = 1; LinePoints[6][41][3] = -2; LinePoints[6][41][4] = 1; LinePoints[6][41][5] = -3; LinePoints[6][41][6] = 2; LinePoints[6][41][7] = -4; LinePoints[6][41][8] = 3; LinePoints[6][41][9] = -5; LinePoints[6][41][10] = 3; LinePoints[6][41][11] = -6; LinePoints[6][41][12] = 4; LinePoints[6][41][13] = -7;
	LinePoints[6][42][0] = 0; LinePoints[6][42][1] = 1; LinePoints[6][42][2] = 1; LinePoints[6][42][3] = 2; LinePoints[6][42][4] = 1; LinePoints[6][42][5] = 3; LinePoints[6][42][6] = 2; LinePoints[6][42][7] = 4; LinePoints[6][42][8] = 3; LinePoints[6][42][9] = 5; LinePoints[6][42][10] = 3; LinePoints[6][42][11] = 6; LinePoints[6][42][12] = 4; LinePoints[6][42][13] = 7;
	LinePoints[6][43][0] = 0; LinePoints[6][43][1] = -1; LinePoints[6][43][2] = 1; LinePoints[6][43][3] = -2; LinePoints[6][43][4] = 2; LinePoints[6][43][5] = -3; LinePoints[6][43][6] = 3; LinePoints[6][43][7] = -4; LinePoints[6][43][8] = 3; LinePoints[6][43][9] = -5; LinePoints[6][43][10] = 4; LinePoints[6][43][11] = -6; LinePoints[6][43][12] = 5; LinePoints[6][43][13] = -7;
	LinePoints[6][44][0] = 0; LinePoints[6][44][1] = 1; LinePoints[6][44][2] = 1; LinePoints[6][44][3] = 2; LinePoints[6][44][4] = 2; LinePoints[6][44][5] = 3; LinePoints[6][44][6] = 3; LinePoints[6][44][7] = 4; LinePoints[6][44][8] = 3; LinePoints[6][44][9] = 5; LinePoints[6][44][10] = 4; LinePoints[6][44][11] = 6; LinePoints[6][44][12] = 5; LinePoints[6][44][13] = 7;
	LinePoints[6][45][0] = 0; LinePoints[6][45][1] = -1; LinePoints[6][45][2] = 1; LinePoints[6][45][3] = -2; LinePoints[6][45][4] = 2; LinePoints[6][45][5] = -3; LinePoints[6][45][6] = 3; LinePoints[6][45][7] = -4; LinePoints[6][45][8] = 4; LinePoints[6][45][9] = -5; LinePoints[6][45][10] = 5; LinePoints[6][45][11] = -6; LinePoints[6][45][12] = 6; LinePoints[6][45][13] = -7;
	LinePoints[6][46][0] = 0; LinePoints[6][46][1] = 1; LinePoints[6][46][2] = 1; LinePoints[6][46][3] = 2; LinePoints[6][46][4] = 2; LinePoints[6][46][5] = 3; LinePoints[6][46][6] = 3; LinePoints[6][46][7] = 4; LinePoints[6][46][8] = 4; LinePoints[6][46][9] = 5; LinePoints[6][46][10] = 5; LinePoints[6][46][11] = 6; LinePoints[6][46][12] = 6; LinePoints[6][46][13] = 7;
	LinePoints[6][47][0] = 1; LinePoints[6][47][1] = -1; LinePoints[6][47][2] = 2; LinePoints[6][47][3] = -2; LinePoints[6][47][4] = 3; LinePoints[6][47][5] = -3; LinePoints[6][47][6] = 4; LinePoints[6][47][7] = -4; LinePoints[6][47][8] = 5; LinePoints[6][47][9] = -5; LinePoints[6][47][10] = 6; LinePoints[6][47][11] = -6; LinePoints[6][47][12] = 7; LinePoints[6][47][13] = -7;
	LinePoints[6][48][0] = 1; LinePoints[6][48][1] = 0; LinePoints[6][48][2] = 2; LinePoints[6][48][3] = -1; LinePoints[6][48][4] = 3; LinePoints[6][48][5] = -2; LinePoints[6][48][6] = 4; LinePoints[6][48][7] = -3; LinePoints[6][48][8] = 5; LinePoints[6][48][9] = -4; LinePoints[6][48][10] = 6; LinePoints[6][48][11] = -5; LinePoints[6][48][12] = 7; LinePoints[6][48][13] = -6;
	LinePoints[6][49][0] = 1; LinePoints[6][49][1] = 0; LinePoints[6][49][2] = 2; LinePoints[6][49][3] = -1; LinePoints[6][49][4] = 3; LinePoints[6][49][5] = -2; LinePoints[6][49][6] = 4; LinePoints[6][49][7] = -3; LinePoints[6][49][8] = 5; LinePoints[6][49][9] = -3; LinePoints[6][49][10] = 6; LinePoints[6][49][11] = -4; LinePoints[6][49][12] = 7; LinePoints[6][49][13] = -5;
	LinePoints[6][50][0] = 1; LinePoints[6][50][1] = 0; LinePoints[6][50][2] = 2; LinePoints[6][50][3] = -1; LinePoints[6][50][4] = 3; LinePoints[6][50][5] = -1; LinePoints[6][50][6] = 4; LinePoints[6][50][7] = -2; LinePoints[6][50][8] = 5; LinePoints[6][50][9] = -3; LinePoints[6][50][10] = 6; LinePoints[6][50][11] = -3; LinePoints[6][50][12] = 7; LinePoints[6][50][13] = -4;
	LinePoints[6][51][0] = 1; LinePoints[6][51][1] = 0; LinePoints[6][51][2] = 2; LinePoints[6][51][3] = -1; LinePoints[6][51][4] = 3; LinePoints[6][51][5] = -1; LinePoints[6][51][6] = 4; LinePoints[6][51][7] = -2; LinePoints[6][51][8] = 5; LinePoints[6][51][9] = -2; LinePoints[6][51][10] = 6; LinePoints[6][51][11] = -3; LinePoints[6][51][12] = 7; LinePoints[6][51][13] = -3;
	LinePoints[6][52][0] = 1; LinePoints[6][52][1] = 0; LinePoints[6][52][2] = 2; LinePoints[6][52][3] = 0; LinePoints[6][52][4] = 3; LinePoints[6][52][5] = -1; LinePoints[6][52][6] = 4; LinePoints[6][52][7] = -1; LinePoints[6][52][8] = 5; LinePoints[6][52][9] = -1; LinePoints[6][52][10] = 6; LinePoints[6][52][11] = -2; LinePoints[6][52][12] = 7; LinePoints[6][52][13] = -2;
	LinePoints[6][53][0] = 1; LinePoints[6][53][1] = 0; LinePoints[6][53][2] = 2; LinePoints[6][53][3] = 0; LinePoints[6][53][4] = 3; LinePoints[6][53][5] = 0; LinePoints[6][53][6] = 4; LinePoints[6][53][7] = -1; LinePoints[6][53][8] = 5; LinePoints[6][53][9] = -1; LinePoints[6][53][10] = 6; LinePoints[6][53][11] = -1; LinePoints[6][53][12] = 7; LinePoints[6][53][13] = -1;
	LinePoints[6][54][0] = 1; LinePoints[6][54][1] = 0; LinePoints[6][54][2] = 2; LinePoints[6][54][3] = 0; LinePoints[6][54][4] = 3; LinePoints[6][54][5] = 0; LinePoints[6][54][6] = 4; LinePoints[6][54][7] = 0; LinePoints[6][54][8] = 5; LinePoints[6][54][9] = 0; LinePoints[6][54][10] = 6; LinePoints[6][54][11] = 0; LinePoints[6][54][12] = 7; LinePoints[6][54][13] = 0;
	LinePoints[6][55][0] = 1; LinePoints[6][55][1] = 0; LinePoints[6][55][2] = 2; LinePoints[6][55][3] = 0; LinePoints[6][55][4] = 3; LinePoints[6][55][5] = 0; LinePoints[6][55][6] = 4; LinePoints[6][55][7] = 0; LinePoints[6][55][8] = 5; LinePoints[6][55][9] = 0; LinePoints[6][55][10] = 6; LinePoints[6][55][11] = 0; LinePoints[6][55][12] = 7; LinePoints[6][55][13] = 0;
	LinePoints[6][56][0] = 1; LinePoints[6][56][1] = 0; LinePoints[6][56][2] = 2; LinePoints[6][56][3] = 0; LinePoints[6][56][4] = 3; LinePoints[6][56][5] = 0; LinePoints[6][56][6] = 4; LinePoints[6][56][7] = 0; LinePoints[6][56][8] = 5; LinePoints[6][56][9] = 0; LinePoints[6][56][10] = 6; LinePoints[6][56][11] = 0; LinePoints[6][56][12] = 7; LinePoints[6][56][13] = 0;
	LinePoints[6][57][0] = 1; LinePoints[6][57][1] = 0; LinePoints[6][57][2] = 2; LinePoints[6][57][3] = 0; LinePoints[6][57][4] = 3; LinePoints[6][57][5] = 0; LinePoints[6][57][6] = 4; LinePoints[6][57][7] = 1; LinePoints[6][57][8] = 5; LinePoints[6][57][9] = 1; LinePoints[6][57][10] = 6; LinePoints[6][57][11] = 1; LinePoints[6][57][12] = 7; LinePoints[6][57][13] = 1;
	LinePoints[6][58][0] = 1; LinePoints[6][58][1] = 0; LinePoints[6][58][2] = 2; LinePoints[6][58][3] = 0; LinePoints[6][58][4] = 3; LinePoints[6][58][5] = 1; LinePoints[6][58][6] = 4; LinePoints[6][58][7] = 1; LinePoints[6][58][8] = 5; LinePoints[6][58][9] = 1; LinePoints[6][58][10] = 6; LinePoints[6][58][11] = 2; LinePoints[6][58][12] = 7; LinePoints[6][58][13] = 2;
	LinePoints[6][59][0] = 1; LinePoints[6][59][1] = 0; LinePoints[6][59][2] = 2; LinePoints[6][59][3] = 1; LinePoints[6][59][4] = 3; LinePoints[6][59][5] = 1; LinePoints[6][59][6] = 4; LinePoints[6][59][7] = 2; LinePoints[6][59][8] = 5; LinePoints[6][59][9] = 2; LinePoints[6][59][10] = 6; LinePoints[6][59][11] = 3; LinePoints[6][59][12] = 7; LinePoints[6][59][13] = 3;
	LinePoints[6][60][0] = 1; LinePoints[6][60][1] = 0; LinePoints[6][60][2] = 2; LinePoints[6][60][3] = 1; LinePoints[6][60][4] = 3; LinePoints[6][60][5] = 1; LinePoints[6][60][6] = 4; LinePoints[6][60][7] = 2; LinePoints[6][60][8] = 5; LinePoints[6][60][9] = 3; LinePoints[6][60][10] = 6; LinePoints[6][60][11] = 3; LinePoints[6][60][12] = 7; LinePoints[6][60][13] = 4;
	LinePoints[6][61][0] = 1; LinePoints[6][61][1] = 0; LinePoints[6][61][2] = 2; LinePoints[6][61][3] = 1; LinePoints[6][61][4] = 3; LinePoints[6][61][5] = 2; LinePoints[6][61][6] = 4; LinePoints[6][61][7] = 3; LinePoints[6][61][8] = 5; LinePoints[6][61][9] = 3; LinePoints[6][61][10] = 6; LinePoints[6][61][11] = 4; LinePoints[6][61][12] = 7; LinePoints[6][61][13] = 5;
	LinePoints[6][62][0] = 1; LinePoints[6][62][1] = 0; LinePoints[6][62][2] = 2; LinePoints[6][62][3] = 1; LinePoints[6][62][4] = 3; LinePoints[6][62][5] = 2; LinePoints[6][62][6] = 4; LinePoints[6][62][7] = 3; LinePoints[6][62][8] = 5; LinePoints[6][62][9] = 4; LinePoints[6][62][10] = 6; LinePoints[6][62][11] = 5; LinePoints[6][62][12] = 7; LinePoints[6][62][13] = 6;
	LinePoints[6][63][0] = 1; LinePoints[6][63][1] = 1; LinePoints[6][63][2] = 2; LinePoints[6][63][3] = 2; LinePoints[6][63][4] = 3; LinePoints[6][63][5] = 3; LinePoints[6][63][6] = 4; LinePoints[6][63][7] = 4; LinePoints[6][63][8] = 5; LinePoints[6][63][9] = 5; LinePoints[6][63][10] = 6; LinePoints[6][63][11] = 6; LinePoints[6][63][12] = 7; LinePoints[6][63][13] = 7;
	LinePoints[7][0][0] = -1; LinePoints[7][0][1] = -1; LinePoints[7][0][2] = -2; LinePoints[7][0][3] = -2; LinePoints[7][0][4] = -3; LinePoints[7][0][5] = -3; LinePoints[7][0][6] = -4; LinePoints[7][0][7] = -4; LinePoints[7][0][8] = -5; LinePoints[7][0][9] = -5; LinePoints[7][0][10] = -6; LinePoints[7][0][11] = -6; LinePoints[7][0][12] = -7; LinePoints[7][0][13] = -7; LinePoints[7][0][14] = -8; LinePoints[7][0][15] = -8;
	LinePoints[7][1][0] = -1; LinePoints[7][1][1] = 0; LinePoints[7][1][2] = -2; LinePoints[7][1][3] = -1; LinePoints[7][1][4] = -3; LinePoints[7][1][5] = -2; LinePoints[7][1][6] = -4; LinePoints[7][1][7] = -3; LinePoints[7][1][8] = -5; LinePoints[7][1][9] = -4; LinePoints[7][1][10] = -6; LinePoints[7][1][11] = -5; LinePoints[7][1][12] = -7; LinePoints[7][1][13] = -6; LinePoints[7][1][14] = -8; LinePoints[7][1][15] = -7;
	LinePoints[7][2][0] = -1; LinePoints[7][2][1] = 0; LinePoints[7][2][2] = -2; LinePoints[7][2][3] = -1; LinePoints[7][2][4] = -3; LinePoints[7][2][5] = -2; LinePoints[7][2][6] = -4; LinePoints[7][2][7] = -3; LinePoints[7][2][8] = -5; LinePoints[7][2][9] = -3; LinePoints[7][2][10] = -6; LinePoints[7][2][11] = -4; LinePoints[7][2][12] = -7; LinePoints[7][2][13] = -5; LinePoints[7][2][14] = -8; LinePoints[7][2][15] = -6;
	LinePoints[7][3][0] = -1; LinePoints[7][3][1] = 0; LinePoints[7][3][2] = -2; LinePoints[7][3][3] = -1; LinePoints[7][3][4] = -3; LinePoints[7][3][5] = -2; LinePoints[7][3][6] = -4; LinePoints[7][3][7] = -2; LinePoints[7][3][8] = -5; LinePoints[7][3][9] = -3; LinePoints[7][3][10] = -6; LinePoints[7][3][11] = -4; LinePoints[7][3][12] = -7; LinePoints[7][3][13] = -4; LinePoints[7][3][14] = -8; LinePoints[7][3][15] = -5;
	LinePoints[7][4][0] = -1; LinePoints[7][4][1] = 0; LinePoints[7][4][2] = -2; LinePoints[7][4][3] = -1; LinePoints[7][4][4] = -3; LinePoints[7][4][5] = -1; LinePoints[7][4][6] = -4; LinePoints[7][4][7] = -2; LinePoints[7][4][8] = -5; LinePoints[7][4][9] = -2; LinePoints[7][4][10] = -6; LinePoints[7][4][11] = -3; LinePoints[7][4][12] = -7; LinePoints[7][4][13] = -3; LinePoints[7][4][14] = -8; LinePoints[7][4][15] = -4;
	LinePoints[7][5][0] = -1; LinePoints[7][5][1] = 0; LinePoints[7][5][2] = -2; LinePoints[7][5][3] = 0; LinePoints[7][5][4] = -3; LinePoints[7][5][5] = -1; LinePoints[7][5][6] = -4; LinePoints[7][5][7] = -1; LinePoints[7][5][8] = -5; LinePoints[7][5][9] = -2; LinePoints[7][5][10] = -6; LinePoints[7][5][11] = -2; LinePoints[7][5][12] = -7; LinePoints[7][5][13] = -3; LinePoints[7][5][14] = -8; LinePoints[7][5][15] = -3;
	LinePoints[7][6][0] = -1; LinePoints[7][6][1] = 0; LinePoints[7][6][2] = -2; LinePoints[7][6][3] = 0; LinePoints[7][6][4] = -3; LinePoints[7][6][5] = -1; LinePoints[7][6][6] = -4; LinePoints[7][6][7] = -1; LinePoints[7][6][8] = -5; LinePoints[7][6][9] = -1; LinePoints[7][6][10] = -6; LinePoints[7][6][11] = -2; LinePoints[7][6][12] = -7; LinePoints[7][6][13] = -2; LinePoints[7][6][14] = -8; LinePoints[7][6][15] = -2;
	LinePoints[7][7][0] = -1; LinePoints[7][7][1] = 0; LinePoints[7][7][2] = -2; LinePoints[7][7][3] = 0; LinePoints[7][7][4] = -3; LinePoints[7][7][5] = 0; LinePoints[7][7][6] = -4; LinePoints[7][7][7] = 0; LinePoints[7][7][8] = -5; LinePoints[7][7][9] = -1; LinePoints[7][7][10] = -6; LinePoints[7][7][11] = -1; LinePoints[7][7][12] = -7; LinePoints[7][7][13] = -1; LinePoints[7][7][14] = -8; LinePoints[7][7][15] = -1;
	LinePoints[7][8][0] = -1; LinePoints[7][8][1] = 0; LinePoints[7][8][2] = -2; LinePoints[7][8][3] = 0; LinePoints[7][8][4] = -3; LinePoints[7][8][5] = 0; LinePoints[7][8][6] = -4; LinePoints[7][8][7] = 0; LinePoints[7][8][8] = -5; LinePoints[7][8][9] = 0; LinePoints[7][8][10] = -6; LinePoints[7][8][11] = 0; LinePoints[7][8][12] = -7; LinePoints[7][8][13] = 0; LinePoints[7][8][14] = -8; LinePoints[7][8][15] = 0;
	LinePoints[7][9][0] = -1; LinePoints[7][9][1] = 0; LinePoints[7][9][2] = -2; LinePoints[7][9][3] = 0; LinePoints[7][9][4] = -3; LinePoints[7][9][5] = 0; LinePoints[7][9][6] = -4; LinePoints[7][9][7] = 0; LinePoints[7][9][8] = -5; LinePoints[7][9][9] = 0; LinePoints[7][9][10] = -6; LinePoints[7][9][11] = 0; LinePoints[7][9][12] = -7; LinePoints[7][9][13] = 0; LinePoints[7][9][14] = -8; LinePoints[7][9][15] = 0;
	LinePoints[7][10][0] = -1; LinePoints[7][10][1] = 0; LinePoints[7][10][2] = -2; LinePoints[7][10][3] = 0; LinePoints[7][10][4] = -3; LinePoints[7][10][5] = 0; LinePoints[7][10][6] = -4; LinePoints[7][10][7] = 0; LinePoints[7][10][8] = -5; LinePoints[7][10][9] = 0; LinePoints[7][10][10] = -6; LinePoints[7][10][11] = 0; LinePoints[7][10][12] = -7; LinePoints[7][10][13] = 0; LinePoints[7][10][14] = -8; LinePoints[7][10][15] = 0;
	LinePoints[7][11][0] = -1; LinePoints[7][11][1] = 0; LinePoints[7][11][2] = -2; LinePoints[7][11][3] = 0; LinePoints[7][11][4] = -3; LinePoints[7][11][5] = 0; LinePoints[7][11][6] = -4; LinePoints[7][11][7] = 0; LinePoints[7][11][8] = -5; LinePoints[7][11][9] = 1; LinePoints[7][11][10] = -6; LinePoints[7][11][11] = 1; LinePoints[7][11][12] = -7; LinePoints[7][11][13] = 1; LinePoints[7][11][14] = -8; LinePoints[7][11][15] = 1;
	LinePoints[7][12][0] = -1; LinePoints[7][12][1] = 0; LinePoints[7][12][2] = -2; LinePoints[7][12][3] = 0; LinePoints[7][12][4] = -3; LinePoints[7][12][5] = 1; LinePoints[7][12][6] = -4; LinePoints[7][12][7] = 1; LinePoints[7][12][8] = -5; LinePoints[7][12][9] = 1; LinePoints[7][12][10] = -6; LinePoints[7][12][11] = 2; LinePoints[7][12][12] = -7; LinePoints[7][12][13] = 2; LinePoints[7][12][14] = -8; LinePoints[7][12][15] = 2;
	LinePoints[7][13][0] = -1; LinePoints[7][13][1] = 0; LinePoints[7][13][2] = -2; LinePoints[7][13][3] = 0; LinePoints[7][13][4] = -3; LinePoints[7][13][5] = 1; LinePoints[7][13][6] = -4; LinePoints[7][13][7] = 1; LinePoints[7][13][8] = -5; LinePoints[7][13][9] = 2; LinePoints[7][13][10] = -6; LinePoints[7][13][11] = 2; LinePoints[7][13][12] = -7; LinePoints[7][13][13] = 3; LinePoints[7][13][14] = -8; LinePoints[7][13][15] = 3;
	LinePoints[7][14][0] = -1; LinePoints[7][14][1] = 0; LinePoints[7][14][2] = -2; LinePoints[7][14][3] = 1; LinePoints[7][14][4] = -3; LinePoints[7][14][5] = 1; LinePoints[7][14][6] = -4; LinePoints[7][14][7] = 2; LinePoints[7][14][8] = -5; LinePoints[7][14][9] = 2; LinePoints[7][14][10] = -6; LinePoints[7][14][11] = 3; LinePoints[7][14][12] = -7; LinePoints[7][14][13] = 3; LinePoints[7][14][14] = -8; LinePoints[7][14][15] = 4;
	LinePoints[7][15][0] = -1; LinePoints[7][15][1] = 0; LinePoints[7][15][2] = -2; LinePoints[7][15][3] = 1; LinePoints[7][15][4] = -3; LinePoints[7][15][5] = 2; LinePoints[7][15][6] = -4; LinePoints[7][15][7] = 2; LinePoints[7][15][8] = -5; LinePoints[7][15][9] = 3; LinePoints[7][15][10] = -6; LinePoints[7][15][11] = 4; LinePoints[7][15][12] = -7; LinePoints[7][15][13] = 4; LinePoints[7][15][14] = -8; LinePoints[7][15][15] = 5;
	LinePoints[7][16][0] = -1; LinePoints[7][16][1] = 0; LinePoints[7][16][2] = -2; LinePoints[7][16][3] = 1; LinePoints[7][16][4] = -3; LinePoints[7][16][5] = 2; LinePoints[7][16][6] = -4; LinePoints[7][16][7] = 3; LinePoints[7][16][8] = -5; LinePoints[7][16][9] = 3; LinePoints[7][16][10] = -6; LinePoints[7][16][11] = 4; LinePoints[7][16][12] = -7; LinePoints[7][16][13] = 5; LinePoints[7][16][14] = -8; LinePoints[7][16][15] = 6;
	LinePoints[7][17][0] = -1; LinePoints[7][17][1] = 0; LinePoints[7][17][2] = -2; LinePoints[7][17][3] = 1; LinePoints[7][17][4] = -3; LinePoints[7][17][5] = 2; LinePoints[7][17][6] = -4; LinePoints[7][17][7] = 3; LinePoints[7][17][8] = -5; LinePoints[7][17][9] = 4; LinePoints[7][17][10] = -6; LinePoints[7][17][11] = 5; LinePoints[7][17][12] = -7; LinePoints[7][17][13] = 6; LinePoints[7][17][14] = -8; LinePoints[7][17][15] = 7;
	LinePoints[7][18][0] = -1; LinePoints[7][18][1] = 1; LinePoints[7][18][2] = -2; LinePoints[7][18][3] = 2; LinePoints[7][18][4] = -3; LinePoints[7][18][5] = 3; LinePoints[7][18][6] = -4; LinePoints[7][18][7] = 4; LinePoints[7][18][8] = -5; LinePoints[7][18][9] = 5; LinePoints[7][18][10] = -6; LinePoints[7][18][11] = 6; LinePoints[7][18][12] = -7; LinePoints[7][18][13] = 7; LinePoints[7][18][14] = -8; LinePoints[7][18][15] = 8;
	LinePoints[7][19][0] = 0; LinePoints[7][19][1] = -1; LinePoints[7][19][2] = -1; LinePoints[7][19][3] = -2; LinePoints[7][19][4] = -2; LinePoints[7][19][5] = -3; LinePoints[7][19][6] = -3; LinePoints[7][19][7] = -4; LinePoints[7][19][8] = -4; LinePoints[7][19][9] = -5; LinePoints[7][19][10] = -5; LinePoints[7][19][11] = -6; LinePoints[7][19][12] = -6; LinePoints[7][19][13] = -7; LinePoints[7][19][14] = -7; LinePoints[7][19][15] = -8;
	LinePoints[7][20][0] = 0; LinePoints[7][20][1] = 1; LinePoints[7][20][2] = -1; LinePoints[7][20][3] = 2; LinePoints[7][20][4] = -2; LinePoints[7][20][5] = 3; LinePoints[7][20][6] = -3; LinePoints[7][20][7] = 4; LinePoints[7][20][8] = -4; LinePoints[7][20][9] = 5; LinePoints[7][20][10] = -5; LinePoints[7][20][11] = 6; LinePoints[7][20][12] = -6; LinePoints[7][20][13] = 7; LinePoints[7][20][14] = -7; LinePoints[7][20][15] = 8;
	LinePoints[7][21][0] = 0; LinePoints[7][21][1] = -1; LinePoints[7][21][2] = -1; LinePoints[7][21][3] = -2; LinePoints[7][21][4] = -2; LinePoints[7][21][5] = -3; LinePoints[7][21][6] = -3; LinePoints[7][21][7] = -4; LinePoints[7][21][8] = -3; LinePoints[7][21][9] = -5; LinePoints[7][21][10] = -4; LinePoints[7][21][11] = -6; LinePoints[7][21][12] = -5; LinePoints[7][21][13] = -7; LinePoints[7][21][14] = -6; LinePoints[7][21][15] = -8;
	LinePoints[7][22][0] = 0; LinePoints[7][22][1] = 1; LinePoints[7][22][2] = -1; LinePoints[7][22][3] = 2; LinePoints[7][22][4] = -2; LinePoints[7][22][5] = 3; LinePoints[7][22][6] = -3; LinePoints[7][22][7] = 4; LinePoints[7][22][8] = -3; LinePoints[7][22][9] = 5; LinePoints[7][22][10] = -4; LinePoints[7][22][11] = 6; LinePoints[7][22][12] = -5; LinePoints[7][22][13] = 7; LinePoints[7][22][14] = -6; LinePoints[7][22][15] = 8;
	LinePoints[7][23][0] = 0; LinePoints[7][23][1] = -1; LinePoints[7][23][2] = -1; LinePoints[7][23][3] = -2; LinePoints[7][23][4] = -2; LinePoints[7][23][5] = -3; LinePoints[7][23][6] = -2; LinePoints[7][23][7] = -4; LinePoints[7][23][8] = -3; LinePoints[7][23][9] = -5; LinePoints[7][23][10] = -4; LinePoints[7][23][11] = -6; LinePoints[7][23][12] = -4; LinePoints[7][23][13] = -7; LinePoints[7][23][14] = -5; LinePoints[7][23][15] = -8;
	LinePoints[7][24][0] = 0; LinePoints[7][24][1] = 1; LinePoints[7][24][2] = -1; LinePoints[7][24][3] = 2; LinePoints[7][24][4] = -2; LinePoints[7][24][5] = 3; LinePoints[7][24][6] = -2; LinePoints[7][24][7] = 4; LinePoints[7][24][8] = -3; LinePoints[7][24][9] = 5; LinePoints[7][24][10] = -4; LinePoints[7][24][11] = 6; LinePoints[7][24][12] = -4; LinePoints[7][24][13] = 7; LinePoints[7][24][14] = -5; LinePoints[7][24][15] = 8;
	LinePoints[7][25][0] = 0; LinePoints[7][25][1] = -1; LinePoints[7][25][2] = -1; LinePoints[7][25][3] = -2; LinePoints[7][25][4] = -1; LinePoints[7][25][5] = -3; LinePoints[7][25][6] = -2; LinePoints[7][25][7] = -4; LinePoints[7][25][8] = -2; LinePoints[7][25][9] = -5; LinePoints[7][25][10] = -3; LinePoints[7][25][11] = -6; LinePoints[7][25][12] = -3; LinePoints[7][25][13] = -7; LinePoints[7][25][14] = -4; LinePoints[7][25][15] = -8;
	LinePoints[7][26][0] = 0; LinePoints[7][26][1] = 1; LinePoints[7][26][2] = -1; LinePoints[7][26][3] = 2; LinePoints[7][26][4] = -1; LinePoints[7][26][5] = 3; LinePoints[7][26][6] = -2; LinePoints[7][26][7] = 4; LinePoints[7][26][8] = -2; LinePoints[7][26][9] = 5; LinePoints[7][26][10] = -3; LinePoints[7][26][11] = 6; LinePoints[7][26][12] = -3; LinePoints[7][26][13] = 7; LinePoints[7][26][14] = -4; LinePoints[7][26][15] = 8;
	LinePoints[7][27][0] = 0; LinePoints[7][27][1] = -1; LinePoints[7][27][2] = 0; LinePoints[7][27][3] = -2; LinePoints[7][27][4] = -1; LinePoints[7][27][5] = -3; LinePoints[7][27][6] = -1; LinePoints[7][27][7] = -4; LinePoints[7][27][8] = -2; LinePoints[7][27][9] = -5; LinePoints[7][27][10] = -2; LinePoints[7][27][11] = -6; LinePoints[7][27][12] = -3; LinePoints[7][27][13] = -7; LinePoints[7][27][14] = -3; LinePoints[7][27][15] = -8;
	LinePoints[7][28][0] = 0; LinePoints[7][28][1] = 1; LinePoints[7][28][2] = 0; LinePoints[7][28][3] = 2; LinePoints[7][28][4] = -1; LinePoints[7][28][5] = 3; LinePoints[7][28][6] = -1; LinePoints[7][28][7] = 4; LinePoints[7][28][8] = -2; LinePoints[7][28][9] = 5; LinePoints[7][28][10] = -2; LinePoints[7][28][11] = 6; LinePoints[7][28][12] = -3; LinePoints[7][28][13] = 7; LinePoints[7][28][14] = -3; LinePoints[7][28][15] = 8;
	LinePoints[7][29][0] = 0; LinePoints[7][29][1] = -1; LinePoints[7][29][2] = 0; LinePoints[7][29][3] = -2; LinePoints[7][29][4] = -1; LinePoints[7][29][5] = -3; LinePoints[7][29][6] = -1; LinePoints[7][29][7] = -4; LinePoints[7][29][8] = -1; LinePoints[7][29][9] = -5; LinePoints[7][29][10] = -2; LinePoints[7][29][11] = -6; LinePoints[7][29][12] = -2; LinePoints[7][29][13] = -7; LinePoints[7][29][14] = -2; LinePoints[7][29][15] = -8;
	LinePoints[7][30][0] = 0; LinePoints[7][30][1] = 1; LinePoints[7][30][2] = 0; LinePoints[7][30][3] = 2; LinePoints[7][30][4] = -1; LinePoints[7][30][5] = 3; LinePoints[7][30][6] = -1; LinePoints[7][30][7] = 4; LinePoints[7][30][8] = -1; LinePoints[7][30][9] = 5; LinePoints[7][30][10] = -2; LinePoints[7][30][11] = 6; LinePoints[7][30][12] = -2; LinePoints[7][30][13] = 7; LinePoints[7][30][14] = -2; LinePoints[7][30][15] = 8;
	LinePoints[7][31][0] = 0; LinePoints[7][31][1] = -1; LinePoints[7][31][2] = 0; LinePoints[7][31][3] = -2; LinePoints[7][31][4] = 0; LinePoints[7][31][5] = -3; LinePoints[7][31][6] = 0; LinePoints[7][31][7] = -4; LinePoints[7][31][8] = -1; LinePoints[7][31][9] = -5; LinePoints[7][31][10] = -1; LinePoints[7][31][11] = -6; LinePoints[7][31][12] = -1; LinePoints[7][31][13] = -7; LinePoints[7][31][14] = -1; LinePoints[7][31][15] = -8;
	LinePoints[7][32][0] = 0; LinePoints[7][32][1] = 1; LinePoints[7][32][2] = 0; LinePoints[7][32][3] = 2; LinePoints[7][32][4] = 0; LinePoints[7][32][5] = 3; LinePoints[7][32][6] = 0; LinePoints[7][32][7] = 4; LinePoints[7][32][8] = -1; LinePoints[7][32][9] = 5; LinePoints[7][32][10] = -1; LinePoints[7][32][11] = 6; LinePoints[7][32][12] = -1; LinePoints[7][32][13] = 7; LinePoints[7][32][14] = -1; LinePoints[7][32][15] = 8;
	LinePoints[7][33][0] = 0; LinePoints[7][33][1] = -1; LinePoints[7][33][2] = 0; LinePoints[7][33][3] = -2; LinePoints[7][33][4] = 0; LinePoints[7][33][5] = -3; LinePoints[7][33][6] = 0; LinePoints[7][33][7] = -4; LinePoints[7][33][8] = 0; LinePoints[7][33][9] = -5; LinePoints[7][33][10] = 0; LinePoints[7][33][11] = -6; LinePoints[7][33][12] = 0; LinePoints[7][33][13] = -7; LinePoints[7][33][14] = 0; LinePoints[7][33][15] = -8;
	LinePoints[7][34][0] = 0; LinePoints[7][34][1] = 1; LinePoints[7][34][2] = 0; LinePoints[7][34][3] = 2; LinePoints[7][34][4] = 0; LinePoints[7][34][5] = 3; LinePoints[7][34][6] = 0; LinePoints[7][34][7] = 4; LinePoints[7][34][8] = 0; LinePoints[7][34][9] = 5; LinePoints[7][34][10] = 0; LinePoints[7][34][11] = 6; LinePoints[7][34][12] = 0; LinePoints[7][34][13] = 7; LinePoints[7][34][14] = 0; LinePoints[7][34][15] = 8;
	LinePoints[7][35][0] = 0; LinePoints[7][35][1] = -1; LinePoints[7][35][2] = 0; LinePoints[7][35][3] = -2; LinePoints[7][35][4] = 0; LinePoints[7][35][5] = -3; LinePoints[7][35][6] = 0; LinePoints[7][35][7] = -4; LinePoints[7][35][8] = 0; LinePoints[7][35][9] = -5; LinePoints[7][35][10] = 0; LinePoints[7][35][11] = -6; LinePoints[7][35][12] = 0; LinePoints[7][35][13] = -7; LinePoints[7][35][14] = 0; LinePoints[7][35][15] = -8;
	LinePoints[7][36][0] = 0; LinePoints[7][36][1] = 1; LinePoints[7][36][2] = 0; LinePoints[7][36][3] = 2; LinePoints[7][36][4] = 0; LinePoints[7][36][5] = 3; LinePoints[7][36][6] = 0; LinePoints[7][36][7] = 4; LinePoints[7][36][8] = 0; LinePoints[7][36][9] = 5; LinePoints[7][36][10] = 0; LinePoints[7][36][11] = 6; LinePoints[7][36][12] = 0; LinePoints[7][36][13] = 7; LinePoints[7][36][14] = 0; LinePoints[7][36][15] = 8;
	LinePoints[7][37][0] = 0; LinePoints[7][37][1] = -1; LinePoints[7][37][2] = 0; LinePoints[7][37][3] = -2; LinePoints[7][37][4] = 0; LinePoints[7][37][5] = -3; LinePoints[7][37][6] = 0; LinePoints[7][37][7] = -4; LinePoints[7][37][8] = 0; LinePoints[7][37][9] = -5; LinePoints[7][37][10] = 0; LinePoints[7][37][11] = -6; LinePoints[7][37][12] = 0; LinePoints[7][37][13] = -7; LinePoints[7][37][14] = 0; LinePoints[7][37][15] = -8;
	LinePoints[7][38][0] = 0; LinePoints[7][38][1] = 1; LinePoints[7][38][2] = 0; LinePoints[7][38][3] = 2; LinePoints[7][38][4] = 0; LinePoints[7][38][5] = 3; LinePoints[7][38][6] = 0; LinePoints[7][38][7] = 4; LinePoints[7][38][8] = 0; LinePoints[7][38][9] = 5; LinePoints[7][38][10] = 0; LinePoints[7][38][11] = 6; LinePoints[7][38][12] = 0; LinePoints[7][38][13] = 7; LinePoints[7][38][14] = 0; LinePoints[7][38][15] = 8;
	LinePoints[7][39][0] = 0; LinePoints[7][39][1] = -1; LinePoints[7][39][2] = 0; LinePoints[7][39][3] = -2; LinePoints[7][39][4] = 0; LinePoints[7][39][5] = -3; LinePoints[7][39][6] = 0; LinePoints[7][39][7] = -4; LinePoints[7][39][8] = 1; LinePoints[7][39][9] = -5; LinePoints[7][39][10] = 1; LinePoints[7][39][11] = -6; LinePoints[7][39][12] = 1; LinePoints[7][39][13] = -7; LinePoints[7][39][14] = 1; LinePoints[7][39][15] = -8;
	LinePoints[7][40][0] = 0; LinePoints[7][40][1] = 1; LinePoints[7][40][2] = 0; LinePoints[7][40][3] = 2; LinePoints[7][40][4] = 0; LinePoints[7][40][5] = 3; LinePoints[7][40][6] = 0; LinePoints[7][40][7] = 4; LinePoints[7][40][8] = 1; LinePoints[7][40][9] = 5; LinePoints[7][40][10] = 1; LinePoints[7][40][11] = 6; LinePoints[7][40][12] = 1; LinePoints[7][40][13] = 7; LinePoints[7][40][14] = 1; LinePoints[7][40][15] = 8;
	LinePoints[7][41][0] = 0; LinePoints[7][41][1] = -1; LinePoints[7][41][2] = 0; LinePoints[7][41][3] = -2; LinePoints[7][41][4] = 1; LinePoints[7][41][5] = -3; LinePoints[7][41][6] = 1; LinePoints[7][41][7] = -4; LinePoints[7][41][8] = 1; LinePoints[7][41][9] = -5; LinePoints[7][41][10] = 2; LinePoints[7][41][11] = -6; LinePoints[7][41][12] = 2; LinePoints[7][41][13] = -7; LinePoints[7][41][14] = 2; LinePoints[7][41][15] = -8;
	LinePoints[7][42][0] = 0; LinePoints[7][42][1] = 1; LinePoints[7][42][2] = 0; LinePoints[7][42][3] = 2; LinePoints[7][42][4] = 1; LinePoints[7][42][5] = 3; LinePoints[7][42][6] = 1; LinePoints[7][42][7] = 4; LinePoints[7][42][8] = 1; LinePoints[7][42][9] = 5; LinePoints[7][42][10] = 2; LinePoints[7][42][11] = 6; LinePoints[7][42][12] = 2; LinePoints[7][42][13] = 7; LinePoints[7][42][14] = 2; LinePoints[7][42][15] = 8;
	LinePoints[7][43][0] = 0; LinePoints[7][43][1] = -1; LinePoints[7][43][2] = 0; LinePoints[7][43][3] = -2; LinePoints[7][43][4] = 1; LinePoints[7][43][5] = -3; LinePoints[7][43][6] = 1; LinePoints[7][43][7] = -4; LinePoints[7][43][8] = 2; LinePoints[7][43][9] = -5; LinePoints[7][43][10] = 2; LinePoints[7][43][11] = -6; LinePoints[7][43][12] = 3; LinePoints[7][43][13] = -7; LinePoints[7][43][14] = 3; LinePoints[7][43][15] = -8;
	LinePoints[7][44][0] = 0; LinePoints[7][44][1] = 1; LinePoints[7][44][2] = 0; LinePoints[7][44][3] = 2; LinePoints[7][44][4] = 1; LinePoints[7][44][5] = 3; LinePoints[7][44][6] = 1; LinePoints[7][44][7] = 4; LinePoints[7][44][8] = 2; LinePoints[7][44][9] = 5; LinePoints[7][44][10] = 2; LinePoints[7][44][11] = 6; LinePoints[7][44][12] = 3; LinePoints[7][44][13] = 7; LinePoints[7][44][14] = 3; LinePoints[7][44][15] = 8;
	LinePoints[7][45][0] = 0; LinePoints[7][45][1] = -1; LinePoints[7][45][2] = 1; LinePoints[7][45][3] = -2; LinePoints[7][45][4] = 1; LinePoints[7][45][5] = -3; LinePoints[7][45][6] = 2; LinePoints[7][45][7] = -4; LinePoints[7][45][8] = 2; LinePoints[7][45][9] = -5; LinePoints[7][45][10] = 3; LinePoints[7][45][11] = -6; LinePoints[7][45][12] = 3; LinePoints[7][45][13] = -7; LinePoints[7][45][14] = 4; LinePoints[7][45][15] = -8;
	LinePoints[7][46][0] = 0; LinePoints[7][46][1] = 1; LinePoints[7][46][2] = 1; LinePoints[7][46][3] = 2; LinePoints[7][46][4] = 1; LinePoints[7][46][5] = 3; LinePoints[7][46][6] = 2; LinePoints[7][46][7] = 4; LinePoints[7][46][8] = 2; LinePoints[7][46][9] = 5; LinePoints[7][46][10] = 3; LinePoints[7][46][11] = 6; LinePoints[7][46][12] = 3; LinePoints[7][46][13] = 7; LinePoints[7][46][14] = 4; LinePoints[7][46][15] = 8;
	LinePoints[7][47][0] = 0; LinePoints[7][47][1] = -1; LinePoints[7][47][2] = 1; LinePoints[7][47][3] = -2; LinePoints[7][47][4] = 2; LinePoints[7][47][5] = -3; LinePoints[7][47][6] = 2; LinePoints[7][47][7] = -4; LinePoints[7][47][8] = 3; LinePoints[7][47][9] = -5; LinePoints[7][47][10] = 4; LinePoints[7][47][11] = -6; LinePoints[7][47][12] = 4; LinePoints[7][47][13] = -7; LinePoints[7][47][14] = 5; LinePoints[7][47][15] = -8;
	LinePoints[7][48][0] = 0; LinePoints[7][48][1] = 1; LinePoints[7][48][2] = 1; LinePoints[7][48][3] = 2; LinePoints[7][48][4] = 2; LinePoints[7][48][5] = 3; LinePoints[7][48][6] = 2; LinePoints[7][48][7] = 4; LinePoints[7][48][8] = 3; LinePoints[7][48][9] = 5; LinePoints[7][48][10] = 4; LinePoints[7][48][11] = 6; LinePoints[7][48][12] = 4; LinePoints[7][48][13] = 7; LinePoints[7][48][14] = 5; LinePoints[7][48][15] = 8;
	LinePoints[7][49][0] = 0; LinePoints[7][49][1] = -1; LinePoints[7][49][2] = 1; LinePoints[7][49][3] = -2; LinePoints[7][49][4] = 2; LinePoints[7][49][5] = -3; LinePoints[7][49][6] = 3; LinePoints[7][49][7] = -4; LinePoints[7][49][8] = 3; LinePoints[7][49][9] = -5; LinePoints[7][49][10] = 4; LinePoints[7][49][11] = -6; LinePoints[7][49][12] = 5; LinePoints[7][49][13] = -7; LinePoints[7][49][14] = 6; LinePoints[7][49][15] = -8;
	LinePoints[7][50][0] = 0; LinePoints[7][50][1] = 1; LinePoints[7][50][2] = 1; LinePoints[7][50][3] = 2; LinePoints[7][50][4] = 2; LinePoints[7][50][5] = 3; LinePoints[7][50][6] = 3; LinePoints[7][50][7] = 4; LinePoints[7][50][8] = 3; LinePoints[7][50][9] = 5; LinePoints[7][50][10] = 4; LinePoints[7][50][11] = 6; LinePoints[7][50][12] = 5; LinePoints[7][50][13] = 7; LinePoints[7][50][14] = 6; LinePoints[7][50][15] = 8;
	LinePoints[7][51][0] = 0; LinePoints[7][51][1] = -1; LinePoints[7][51][2] = 1; LinePoints[7][51][3] = -2; LinePoints[7][51][4] = 2; LinePoints[7][51][5] = -3; LinePoints[7][51][6] = 3; LinePoints[7][51][7] = -4; LinePoints[7][51][8] = 4; LinePoints[7][51][9] = -5; LinePoints[7][51][10] = 5; LinePoints[7][51][11] = -6; LinePoints[7][51][12] = 6; LinePoints[7][51][13] = -7; LinePoints[7][51][14] = 7; LinePoints[7][51][15] = -8;
	LinePoints[7][52][0] = 0; LinePoints[7][52][1] = 1; LinePoints[7][52][2] = 1; LinePoints[7][52][3] = 2; LinePoints[7][52][4] = 2; LinePoints[7][52][5] = 3; LinePoints[7][52][6] = 3; LinePoints[7][52][7] = 4; LinePoints[7][52][8] = 4; LinePoints[7][52][9] = 5; LinePoints[7][52][10] = 5; LinePoints[7][52][11] = 6; LinePoints[7][52][12] = 6; LinePoints[7][52][13] = 7; LinePoints[7][52][14] = 7; LinePoints[7][52][15] = 8;
	LinePoints[7][53][0] = 1; LinePoints[7][53][1] = -1; LinePoints[7][53][2] = 2; LinePoints[7][53][3] = -2; LinePoints[7][53][4] = 3; LinePoints[7][53][5] = -3; LinePoints[7][53][6] = 4; LinePoints[7][53][7] = -4; LinePoints[7][53][8] = 5; LinePoints[7][53][9] = -5; LinePoints[7][53][10] = 6; LinePoints[7][53][11] = -6; LinePoints[7][53][12] = 7; LinePoints[7][53][13] = -7; LinePoints[7][53][14] = 8; LinePoints[7][53][15] = -8;
	LinePoints[7][54][0] = 1; LinePoints[7][54][1] = 0; LinePoints[7][54][2] = 2; LinePoints[7][54][3] = -1; LinePoints[7][54][4] = 3; LinePoints[7][54][5] = -2; LinePoints[7][54][6] = 4; LinePoints[7][54][7] = -3; LinePoints[7][54][8] = 5; LinePoints[7][54][9] = -4; LinePoints[7][54][10] = 6; LinePoints[7][54][11] = -5; LinePoints[7][54][12] = 7; LinePoints[7][54][13] = -6; LinePoints[7][54][14] = 8; LinePoints[7][54][15] = -7;
	LinePoints[7][55][0] = 1; LinePoints[7][55][1] = 0; LinePoints[7][55][2] = 2; LinePoints[7][55][3] = -1; LinePoints[7][55][4] = 3; LinePoints[7][55][5] = -2; LinePoints[7][55][6] = 4; LinePoints[7][55][7] = -3; LinePoints[7][55][8] = 5; LinePoints[7][55][9] = -3; LinePoints[7][55][10] = 6; LinePoints[7][55][11] = -4; LinePoints[7][55][12] = 7; LinePoints[7][55][13] = -5; LinePoints[7][55][14] = 8; LinePoints[7][55][15] = -6;
	LinePoints[7][56][0] = 1; LinePoints[7][56][1] = 0; LinePoints[7][56][2] = 2; LinePoints[7][56][3] = -1; LinePoints[7][56][4] = 3; LinePoints[7][56][5] = -2; LinePoints[7][56][6] = 4; LinePoints[7][56][7] = -2; LinePoints[7][56][8] = 5; LinePoints[7][56][9] = -3; LinePoints[7][56][10] = 6; LinePoints[7][56][11] = -4; LinePoints[7][56][12] = 7; LinePoints[7][56][13] = -4; LinePoints[7][56][14] = 8; LinePoints[7][56][15] = -5;
	LinePoints[7][57][0] = 1; LinePoints[7][57][1] = 0; LinePoints[7][57][2] = 2; LinePoints[7][57][3] = -1; LinePoints[7][57][4] = 3; LinePoints[7][57][5] = -1; LinePoints[7][57][6] = 4; LinePoints[7][57][7] = -2; LinePoints[7][57][8] = 5; LinePoints[7][57][9] = -2; LinePoints[7][57][10] = 6; LinePoints[7][57][11] = -3; LinePoints[7][57][12] = 7; LinePoints[7][57][13] = -3; LinePoints[7][57][14] = 8; LinePoints[7][57][15] = -4;
	LinePoints[7][58][0] = 1; LinePoints[7][58][1] = 0; LinePoints[7][58][2] = 2; LinePoints[7][58][3] = 0; LinePoints[7][58][4] = 3; LinePoints[7][58][5] = -1; LinePoints[7][58][6] = 4; LinePoints[7][58][7] = -1; LinePoints[7][58][8] = 5; LinePoints[7][58][9] = -2; LinePoints[7][58][10] = 6; LinePoints[7][58][11] = -2; LinePoints[7][58][12] = 7; LinePoints[7][58][13] = -3; LinePoints[7][58][14] = 8; LinePoints[7][58][15] = -3;
	LinePoints[7][59][0] = 1; LinePoints[7][59][1] = 0; LinePoints[7][59][2] = 2; LinePoints[7][59][3] = 0; LinePoints[7][59][4] = 3; LinePoints[7][59][5] = -1; LinePoints[7][59][6] = 4; LinePoints[7][59][7] = -1; LinePoints[7][59][8] = 5; LinePoints[7][59][9] = -1; LinePoints[7][59][10] = 6; LinePoints[7][59][11] = -2; LinePoints[7][59][12] = 7; LinePoints[7][59][13] = -2; LinePoints[7][59][14] = 8; LinePoints[7][59][15] = -2;
	LinePoints[7][60][0] = 1; LinePoints[7][60][1] = 0; LinePoints[7][60][2] = 2; LinePoints[7][60][3] = 0; LinePoints[7][60][4] = 3; LinePoints[7][60][5] = 0; LinePoints[7][60][6] = 4; LinePoints[7][60][7] = 0; LinePoints[7][60][8] = 5; LinePoints[7][60][9] = -1; LinePoints[7][60][10] = 6; LinePoints[7][60][11] = -1; LinePoints[7][60][12] = 7; LinePoints[7][60][13] = -1; LinePoints[7][60][14] = 8; LinePoints[7][60][15] = -1;
	LinePoints[7][61][0] = 1; LinePoints[7][61][1] = 0; LinePoints[7][61][2] = 2; LinePoints[7][61][3] = 0; LinePoints[7][61][4] = 3; LinePoints[7][61][5] = 0; LinePoints[7][61][6] = 4; LinePoints[7][61][7] = 0; LinePoints[7][61][8] = 5; LinePoints[7][61][9] = 0; LinePoints[7][61][10] = 6; LinePoints[7][61][11] = 0; LinePoints[7][61][12] = 7; LinePoints[7][61][13] = 0; LinePoints[7][61][14] = 8; LinePoints[7][61][15] = 0;
	LinePoints[7][62][0] = 1; LinePoints[7][62][1] = 0; LinePoints[7][62][2] = 2; LinePoints[7][62][3] = 0; LinePoints[7][62][4] = 3; LinePoints[7][62][5] = 0; LinePoints[7][62][6] = 4; LinePoints[7][62][7] = 0; LinePoints[7][62][8] = 5; LinePoints[7][62][9] = 0; LinePoints[7][62][10] = 6; LinePoints[7][62][11] = 0; LinePoints[7][62][12] = 7; LinePoints[7][62][13] = 0; LinePoints[7][62][14] = 8; LinePoints[7][62][15] = 0;
	LinePoints[7][63][0] = 1; LinePoints[7][63][1] = 0; LinePoints[7][63][2] = 2; LinePoints[7][63][3] = 0; LinePoints[7][63][4] = 3; LinePoints[7][63][5] = 0; LinePoints[7][63][6] = 4; LinePoints[7][63][7] = 0; LinePoints[7][63][8] = 5; LinePoints[7][63][9] = 0; LinePoints[7][63][10] = 6; LinePoints[7][63][11] = 0; LinePoints[7][63][12] = 7; LinePoints[7][63][13] = 0; LinePoints[7][63][14] = 8; LinePoints[7][63][15] = 0;
	LinePoints[7][64][0] = 1; LinePoints[7][64][1] = 0; LinePoints[7][64][2] = 2; LinePoints[7][64][3] = 0; LinePoints[7][64][4] = 3; LinePoints[7][64][5] = 0; LinePoints[7][64][6] = 4; LinePoints[7][64][7] = 0; LinePoints[7][64][8] = 5; LinePoints[7][64][9] = 1; LinePoints[7][64][10] = 6; LinePoints[7][64][11] = 1; LinePoints[7][64][12] = 7; LinePoints[7][64][13] = 1; LinePoints[7][64][14] = 8; LinePoints[7][64][15] = 1;
	LinePoints[7][65][0] = 1; LinePoints[7][65][1] = 0; LinePoints[7][65][2] = 2; LinePoints[7][65][3] = 0; LinePoints[7][65][4] = 3; LinePoints[7][65][5] = 1; LinePoints[7][65][6] = 4; LinePoints[7][65][7] = 1; LinePoints[7][65][8] = 5; LinePoints[7][65][9] = 1; LinePoints[7][65][10] = 6; LinePoints[7][65][11] = 2; LinePoints[7][65][12] = 7; LinePoints[7][65][13] = 2; LinePoints[7][65][14] = 8; LinePoints[7][65][15] = 2;
	LinePoints[7][66][0] = 1; LinePoints[7][66][1] = 0; LinePoints[7][66][2] = 2; LinePoints[7][66][3] = 0; LinePoints[7][66][4] = 3; LinePoints[7][66][5] = 1; LinePoints[7][66][6] = 4; LinePoints[7][66][7] = 1; LinePoints[7][66][8] = 5; LinePoints[7][66][9] = 2; LinePoints[7][66][10] = 6; LinePoints[7][66][11] = 2; LinePoints[7][66][12] = 7; LinePoints[7][66][13] = 3; LinePoints[7][66][14] = 8; LinePoints[7][66][15] = 3;
	LinePoints[7][67][0] = 1; LinePoints[7][67][1] = 0; LinePoints[7][67][2] = 2; LinePoints[7][67][3] = 1; LinePoints[7][67][4] = 3; LinePoints[7][67][5] = 1; LinePoints[7][67][6] = 4; LinePoints[7][67][7] = 2; LinePoints[7][67][8] = 5; LinePoints[7][67][9] = 2; LinePoints[7][67][10] = 6; LinePoints[7][67][11] = 3; LinePoints[7][67][12] = 7; LinePoints[7][67][13] = 3; LinePoints[7][67][14] = 8; LinePoints[7][67][15] = 4;
	LinePoints[7][68][0] = 1; LinePoints[7][68][1] = 0; LinePoints[7][68][2] = 2; LinePoints[7][68][3] = 1; LinePoints[7][68][4] = 3; LinePoints[7][68][5] = 2; LinePoints[7][68][6] = 4; LinePoints[7][68][7] = 2; LinePoints[7][68][8] = 5; LinePoints[7][68][9] = 3; LinePoints[7][68][10] = 6; LinePoints[7][68][11] = 4; LinePoints[7][68][12] = 7; LinePoints[7][68][13] = 4; LinePoints[7][68][14] = 8; LinePoints[7][68][15] = 5;
	LinePoints[7][69][0] = 1; LinePoints[7][69][1] = 0; LinePoints[7][69][2] = 2; LinePoints[7][69][3] = 1; LinePoints[7][69][4] = 3; LinePoints[7][69][5] = 2; LinePoints[7][69][6] = 4; LinePoints[7][69][7] = 3; LinePoints[7][69][8] = 5; LinePoints[7][69][9] = 3; LinePoints[7][69][10] = 6; LinePoints[7][69][11] = 4; LinePoints[7][69][12] = 7; LinePoints[7][69][13] = 5; LinePoints[7][69][14] = 8; LinePoints[7][69][15] = 6;
	LinePoints[7][70][0] = 1; LinePoints[7][70][1] = 0; LinePoints[7][70][2] = 2; LinePoints[7][70][3] = 1; LinePoints[7][70][4] = 3; LinePoints[7][70][5] = 2; LinePoints[7][70][6] = 4; LinePoints[7][70][7] = 3; LinePoints[7][70][8] = 5; LinePoints[7][70][9] = 4; LinePoints[7][70][10] = 6; LinePoints[7][70][11] = 5; LinePoints[7][70][12] = 7; LinePoints[7][70][13] = 6; LinePoints[7][70][14] = 8; LinePoints[7][70][15] = 7;
	LinePoints[7][71][0] = 1; LinePoints[7][71][1] = 1; LinePoints[7][71][2] = 2; LinePoints[7][71][3] = 2; LinePoints[7][71][4] = 3; LinePoints[7][71][5] = 3; LinePoints[7][71][6] = 4; LinePoints[7][71][7] = 4; LinePoints[7][71][8] = 5; LinePoints[7][71][9] = 5; LinePoints[7][71][10] = 6; LinePoints[7][71][11] = 6; LinePoints[7][71][12] = 7; LinePoints[7][71][13] = 7; LinePoints[7][71][14] = 8; LinePoints[7][71][15] = 8;
}