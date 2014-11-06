#ifndef _SIMILAR_SEARCH_H_
#define _SIMILAR_SEARCH_H_

//Search is not accurate! Can not distinguish rotation and it is very tolerant to texture movement
//it compares 2 images in 3 comparisons minimum
//imagine if both images would suffer a NxN blurr before search
//does not support transparency
//works best if images we search have the same size ( screenshot is only built once )
//what it does : shrinks so image is replicated in every pixel.

#define REMINDER_TO_FINISH_IMPLEMENTING_MULTI_BLOCKS	1
//you can find a simple texture in so many places :(
//#define ADD_COLOR_LOCALIZATION_4x4	1
//#define ADD_COLOR_LOCALIZATION_DIAG2	1
//#define ADD_COLOR_LOCALIZATION_DIAG3	1
//#define ADD_COLOR_LOCALIZATION_HOR3	1
//#define ADD_COLOR_LOCALIZATION_HOR4	1		//only half of the info and can overflow !
//#define ADD_COLOR_LOCALIZATION_DIAG3_1	1		//every 3x row will skip 2 pixels
//#define ADD_COLOR_LOCALIZATION_HOR4_DIV4	1	//half of the information is lost
//#define ADD_COLOR_LOCALIZATION_DIAG3RGB	1
//#define ADD_COLOR_LOCALIZATION_DIAG5_OVERF	1
//#define ADD_COLOR_LOCALIZATION_DIAG6_OVERF	1
//#define ADD_COLOR_LOCALIZATION_XOR_ROW	1	//this does not make any sense. Leaving it for laughs
//#define ADD_COLOR_LOCALIZATION_ADDBUG	1	//this does not make any sense. Leaving it for laughs
#define ADD_COLOR_LOCALIZATION_MULBUGRGB	1	//this does not make any sense. Leaving it for laughs

class SimilarSearch
{
public:
	SimilarSearch();
	~SimilarSearch();
	void BuildFromImg( LPCOLORREF Pixels, int pWidth, int pHeight, int pStride );
	int Width, Height;
	int	*R,*G,*B;
#ifndef IMPLEMENTING_MULTI_BLOCKS
	int BlockWidth,BlockHeight;
#endif
};

int GetNextBestMatch( SimilarSearch *SearchIn, SimilarSearch *SearchFor, int &retx, int &rety );
char * WINAPI SearchSimilarOnScreenshot( char *aImageFile );

extern LIBRARY_API int SimilarSearchGroupingSizeX;
extern LIBRARY_API int SimilarSearchGroupingSizeY;

#endif