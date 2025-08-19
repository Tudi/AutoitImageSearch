#ifndef _SIMILAR_SEARCH_H_
#define _SIMILAR_SEARCH_H_

//Search is not accurate! Can not distinguish rotation and it is very tolerant to texture movement
//it compares 2 images in 3 comparisons minimum
//imagine if both images would suffer a NxN blurr before search
//does not support transparency
//works best if images we search have the same size ( screenshot is only built once )
//what it does : shrinks so image is replicated in every pixel.

#define REMINDER_TO_FINISH_IMPLEMENTING_MULTI_BLOCKS	1
//#define SS_SUM_RGB
//you can find a simple texture in so many places :(
//#define ADD_COLOR_LOCALIZATION_SIMPLESUM	1
//#define ADD_COLOR_LOCALIZATION_4x4	1
//#define ADD_COLOR_LOCALIZATION_DIAG2	1
//#define ADD_COLOR_LOCALIZATION_DIAG3	1
//#define ADD_COLOR_LOCALIZATION_HOR3	1
//#define ADD_COLOR_LOCALIZATION_HOR4	1		//only half of the info and can overflow !
//#define ADD_COLOR_LOCALIZATION_DIAG3_1	1	//every 3x row will skip 2 pixels
//#define ADD_COLOR_LOCALIZATION_HOR4_DIV4	1	//half of the information is lost
//#define ADD_COLOR_LOCALIZATION_DIAG3RGB	1
//#define ADD_COLOR_LOCALIZATION_DIAG5_OVERF	1
//#define ADD_COLOR_LOCALIZATION_DIAG6_OVERF	1
//#define ADD_COLOR_LOCALIZATION_XOR_ROW	1	//this does not make any sense. Leaving it for laughs
//#define ADD_COLOR_LOCALIZATION_ADDBUG	1	//this does not make any sense. Leaving it for laughs
//#define ADD_COLOR_LOCALIZATION_MULBUGRGB	1	//this does not make any sense. Links color combinations for pixels making similarity test fail. Leaving it for laughs
//#define ADD_COLOR_LOCALIZATION_2MULBUGRGB	1	//this does not make any sense. Links color combinations for pixels making similarity test fail. Leaving it for laughs

class SimilarSearch
{
public:
	SimilarSearch();
	~SimilarSearch();
	void BuildFromImg( LPCOLORREF Pixels, int pWidth, int pHeight, int pStride, bool bIsCache );
	int Width, Height, Stride;
	int	*R,*G,*B;
	int SearchType;
	int SearchDownScale;
#ifndef IMPLEMENTING_MULTI_BLOCKS
	int BlockWidth,BlockHeight;
#endif
};

uint64_t GetNextBestMatch( SimilarSearch *SearchIn, SimilarSearch *SearchFor, int &retx, int &rety );
char * WINAPI SearchSimilarOnScreenshot(const char *aImageFile );
void WINAPI SetupSimilarSearch( int MaxImageSize, int DownScale, int SearchType, int OnlyAtDiffMask );

enum SS_SEARCH_TYPES
{
	SS_SEARCH_TYPE_START = 0,
	SS_SEARCH_TYPE_BUGGED_LINKED_PIXELS = 1,
	SS_SEARCH_TYPE_LINKED_SUMMED_PIXELS2 = 2,
	SS_SEARCH_TYPE_LINKED_SUMMED_PIXELS = 3,
	SS_SEARCH_TYPE_SUMMED_PIXELS = 4,
	SS_SEARCH_TYPE_END,
};

extern LIBRARY_API int SimilarSearchGroupingSizeX;
extern LIBRARY_API int SimilarSearchGroupingSizeY;
extern LIBRARY_API int SimilarSearchResizeStep;
extern LIBRARY_API int SimilarSearchSearchType;
extern LIBRARY_API int SimilarSearchOnlySearchOnDiffMask;

#endif