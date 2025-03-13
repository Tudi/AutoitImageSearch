// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

// Windows Header Files:
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <olectl.h> // for OleLoadPicture()
#include <Gdiplus.h> // Used by LoadPicture().
#include <windef.h>
#include <windows.h>
#include <winuser.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <shellapi.h>

#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <assert.h>
#include <immintrin.h>
#include <conio.h>
#include <memory.h>
#include <tchar.h>
#include <atlimage.h>
#include <math.h>
#include <set>
#include <map>
#include <list>
#include <vector>

#ifdef LIBRARY_EXPORTS
#    define LIBRARY_API __declspec(dllexport)
#else
//#    define LIBRARY_API __declspec(dllimport)
#    define LIBRARY_API 
#endif

#define DEFAULT_STR_BUFFER_SIZE 1024

#define TRANSPARENT_COLOR			(0x00FFFFFF)
#define REMOVE_ALPHA_CHANNEL_MASK	(0x00FFFFFF)

#include "Tools/Allocator.h"
#include "PictureCache.h"
#include "Tools/util.h"
#include "resampling.h"
#include "InputProcess.h"
#include "debug.h"
#include "SaveImages.h"
#include "ImageFilters.h"
#include "MotionEstimation.h"
#include "Tools/Tools.h"
#include "PiramidSearch.h"
#include "SimilarSearch.h"
#include "OCR.h"
#include "UniquePixelPairSearch.h"
#include "EdgeDetect.h"
#include "SplitChannelSearch.h"
#include "ImageSearch.h"
#include "OCR_LuckyCase.h"
#include "PixelProcess.h"
#include "GradientRemove.h"
#include "EdgeFilter.h"
#include <algorithm>
#include <time.h>
#include "ProjectionDistanceMap.h"
#include "DrawLine.h"
#include "ImageSearchPreSAD.h"
#include "ImageSAD.h"
#include "ImageHash.h"
#include "ImageSATD.h"

#ifdef _CONSOLE
    #include "Tools\CommandProcessor.h"
	#include "Benchmarks/Benchmarks.h"
	#include "Testing/EdgeFilterTest.h"
	#include "Testing/SATDTest.h"
#endif

#define RGB_GET_R(Color) ( Color & 0xFF )
#define RGB_GET_G(Color) ( (Color >> 8 ) & 0xFF)
#define RGB_GET_B(Color) ( (Color >> 16 )& 0xFF)
#define RGB_GET_A(Color) ( (Color >> 24 )& 0xFF)
#define STATIC_BGR_RGB(Color) (( RGB_GET_R( Color ) << 16 ) | ( RGB_GET_G( Color ) << 8 ) | ( RGB_GET_B( Color ) ) )


// TODO: reference additional headers your program requires here
