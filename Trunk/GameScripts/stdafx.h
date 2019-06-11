#pragma once
#include "../stdafx.h"

//#define COMPILE_LORDS_SCRIPTS

#if defined( _CONSOLE ) && defined(COMPILE_LORDS_SCRIPTS)
	#include "Testing/Lords.h"
	#include "GameScripts/LordsMobile.h"
#endif