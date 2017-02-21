// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER				// Allow use of features specific to Windows XP or later.
#define WINVER 0x0501		// Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif						

#ifndef _WIN32_WINDOWS		// Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE			// Allow use of features specific to IE 6.0 or later.
#define _WIN32_IE 0x0600	// Change this to the appropriate value to target other versions of IE.
#endif

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
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
#include <tmmintrin.h>
#include <conio.h>
#include <memory.h>
#include <tchar.h>
#include <atlimage.h>
#include <math.h>
#include <set>

#ifdef LIBRARY_EXPORTS
#    define LIBRARY_API __declspec(dllexport)
#else
//#    define LIBRARY_API __declspec(dllimport)
#    define LIBRARY_API 
#endif

#define DEFAULT_STR_BUFFER_SIZE 1024

#define SSE_ALIGNMENT			16
#define SSE_BYTE_COUNT			16
#define SSE_PADDING				32	//12 bytes would be enough

#define TRANSPARENT_COLOR			(0x00FFFFFF)
#define REMOVE_ALPHA_CHANNEL_MASK	(0x00FFFFFF)

#include "PictureCache.h"
#include "util.h"
#include "resampling.h"
#include "InputProcess.h"
#include "debug.h"
#include "SaveImages.h"
#include "ImageFilters.h"
#include "MotionEstimation.h"
#include "Tools.h"
#include "PiramidSearch.h"
#include "SimilarSearch.h"
#include "OCR.h"
#include "UniquePixelPairSearch.h"
#include "EdgeDetect.h"
#include "SplitChannelSearch.h"
#include "ImageSearch.h"
#include "OCR_LuckyCase.h"
#include "PixelProcess.h"
#include <map>
#include <algorithm>
#include <vector>
#include <time.h>

#ifdef _CONSOLE
	#include "Benchmarks/Benchmarks.h"
	#include "Testing/Lords.h"
	#include "GameScripts/LordsMobile.h"
#endif

#define RGB_GET_R(Color) ( Color & 0xFF )
#define RGB_GET_G(Color) ( (Color >> 8 ) & 0xFF)
#define RGB_GET_B(Color) ( (Color >> 16 )& 0xFF)
#define STATIC_BGR_RGB(Color) (( RGB_GET_R( Color ) << 16 ) | ( RGB_GET_G( Color ) << 8 ) | ( RGB_GET_B( Color ) ) )


// TODO: reference additional headers your program requires here
