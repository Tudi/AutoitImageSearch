#RequireAdmin

#include "CommonFunctions.au3"

Opt("MustDeclareVars", 1)
HotKeySet("o", "ExitBot")
HotKeySet("p", "TempFunc")
HotKeySet("[", "ScreenshotCleanRegion")

global $BotIsRunning = 1
global $FuncIsRunning = 0
global $isFailedMap = 0

global $dllhandle = DllOpen ( "ImageSearchDLL.dll" )

ResetDebugLog("log.txt")

; set up ImageSearchDll so it performs ok for our case
; !! images need to be BMP. Try to make them as small as possible. Preferably width and height to be dividable by 8. Internal format is A8R8G8B8 though Alpha is not used
DllCall( $dllhandle, "NONE", "SetScreehotFPSLimit", "float", 0.5) ; because search takes more than 1 second

while( $BotIsRunning == 1)
	Sleep(1000)
wend

func ExitBot()
	global $BotIsRunning = 0
endfunc

DllClose ( $dllhandle )

exit

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

func SpawnHeroAt($x, $y)
	if $FuncIsRunning <> 1 or $BotIsRunning <> 1 or $isFailedMap <> 0 then return
	; spawn hero
	Send("u")
	Sleep(200)
	if $FuncIsRunning <> 1 or $BotIsRunning <> 1 or $isFailedMap <> 0 then return
	MouseClick("left",$x,$y,1)
	Sleep(200)
endfunc

func SpawnNinjaAtLoc($x,$y)
	if $FuncIsRunning <> 1 or $BotIsRunning <> 1 or $isFailedMap <> 0 then return
	_WriteDebugLog("log.txt", "Sawning ninja at loc x=" & $x & " y=" & $y)
	; spawn 
	Send("d")
	Sleep(1000)
	if $FuncIsRunning <> 1 or $BotIsRunning <> 1 or $isFailedMap <> 0 then return
	MouseClick("left", $x, $y, 1)
	Sleep(1000)
	if $FuncIsRunning <> 1 or $BotIsRunning <> 1 or $isFailedMap <> 0 then return
	; select it
	MouseClick("left", $x, $y, 1)
	Sleep(1000)
	if $FuncIsRunning <> 1 or $BotIsRunning <> 1 or $isFailedMap <> 0 then return
endfunc

func TryReselectNinja($x, $y)
	if $FuncIsRunning <> 1 or $BotIsRunning <> 1 or $isFailedMap <> 0 then return 0
	_WriteDebugLog("log.txt", "reselecting ninja at loc x=" & $x & " y=" & $y)
	local $isSelected = ClickButtonIfAvailable("Images/ninja_isSelected.bmp", 0, 0, 2000, 2000, 1, 180000 )
	if $isSelected == 0 then
		MouseClick("left", $x, $y, 1)
		Sleep(1000)
		if $FuncIsRunning <> 1 or $BotIsRunning <> 1 or $isFailedMap <> 0 then return 0
		$isSelected = ClickButtonIfAvailable("Images/ninja_isSelected.bmp", 0, 0, 2000, 2000, 1, 180000 )
	endif
	return $isSelected
endfunc

func isFailedRestart()
	if $FuncIsRunning <> 1 or $BotIsRunning <> 1 or $isFailedMap <> 0 then return
	$isFailedMap = ClickButtonIfAvailable("Images/restart.bmp", 0, 0, 2000, 2000, 1, 40000 )
	if $isFailedMap == 1 then
		_WriteDebugLog("log.txt", "Fail button has been detected")
;		MouseClick("left",855,815,1)
		Sleep(2000)
		MouseClick("left",1142,720,1)
		Sleep(2000)
	endif
endfunc

func UpgradeSelectedNinja()
	if $FuncIsRunning <> 1 or $BotIsRunning <> 1 or $isFailedMap <> 0 then return
	
	local $DefaultActionSleep = 500
	_WriteDebugLog("log.txt", "Started upgrading selected ninja")
	
	; avoid mouse hovering over capture area
	MouseMove(0,0,0)
	local $madeClickOnImage = 0
	while( $madeClickOnImage == 0)	
		isFailedRestart()
		if $FuncIsRunning <> 1 or $BotIsRunning <> 1 or $isFailedMap <> 0 then return
		$madeClickOnImage = ClickButtonIfAvailable("Images/ninja_1xx.bmp", 0, 0, 2000, 2000, 1, 40000 )
		Sleep($DefaultActionSleep)
	wend
	
	MouseMove(0,0,0)
	$madeClickOnImage = 0
	while( $madeClickOnImage == 0)	
		isFailedRestart()
		if $FuncIsRunning <> 1 or $BotIsRunning <> 1 or $isFailedMap <> 0 then return
		$madeClickOnImage = ClickButtonIfAvailable("Images/ninja_xx1.bmp", 0, 0, 2000, 2000, 1, 40000 )
		Sleep($DefaultActionSleep)
	wend	

	MouseMove(0,0,0)
	$madeClickOnImage = 0
	while( $madeClickOnImage == 0)	
		isFailedRestart()
		if $FuncIsRunning <> 1 or $BotIsRunning <> 1 or $isFailedMap <> 0 then return
		$madeClickOnImage = ClickButtonIfAvailable("Images/ninja_2xx.bmp", 0, 0, 2000, 2000, 1, 40000 )
		Sleep($DefaultActionSleep)
	wend	

	MouseMove(0,0,0)
	$madeClickOnImage = 0
	while( $madeClickOnImage == 0)	
		isFailedRestart()
		if $FuncIsRunning <> 1 or $BotIsRunning <> 1 or $isFailedMap <> 0 then return
		$madeClickOnImage = ClickButtonIfAvailable("Images/ninja_xx2.bmp", 0, 0, 2000, 2000, 1, 180000 )
		Sleep($DefaultActionSleep)
	wend	

	MouseMove(0,0,0)
	$madeClickOnImage = 0
	while( $madeClickOnImage == 0)	
		isFailedRestart()
		if $FuncIsRunning <> 1 or $BotIsRunning <> 1 or $isFailedMap <> 0 then return
		$madeClickOnImage = ClickButtonIfAvailable("Images/ninja_xx3.bmp", 0, 0, 2000, 2000, 1, 180000 )
		Sleep($DefaultActionSleep)
	wend
endfunc

func WaitRestartOnFail()
	global $FuncIsRunning, $BotIsRunning
	local $DefaultActionSleep = 1000
	; avoid mouse hovering over capture area
	MouseMove(0,0,0)
	while( $FuncIsRunning == 1 and $BotIsRunning == 1 and $isFailedMap == 0)	
		isFailedRestart()
		Sleep($DefaultActionSleep)
	wend
endfunc

func WaitMoneyForNinja()
	if $FuncIsRunning <> 1 or $BotIsRunning <> 1 or $isFailedMap <> 0 then return
	local $DefaultActionSleep = 1000
	_WriteDebugLog("log.txt", "Started waiting for money for ninja")
	; avoid mouse hovering over capture area
	MouseMove(0,0,0)
	local $madeClickOnImage = 0
	while( $madeClickOnImage == 0)	
		isFailedRestart()
		if $FuncIsRunning <> 1 or $BotIsRunning <> 1 or $isFailedMap <> 0 then return
		$madeClickOnImage = ClickButtonIfAvailable("Images/ninja_hasmoney.bmp", 0, 0, 2000, 2000, 1, 40000 )
		Sleep($DefaultActionSleep)
	wend
endfunc

func AddNinjaToMap($x, $y)
	if $FuncIsRunning <> 1 or $BotIsRunning <> 1 or $isFailedMap <> 0 then return
	WaitMoneyForNinja()
	if $FuncIsRunning <> 1 or $BotIsRunning <> 1 or $isFailedMap <> 0 then return
	SpawnNinjaAtLoc($x, $y)
	; if for some reason we failed to spawn or select this ninja, we can't upgrade him
	if TryReselectNinja($x, $y) == 0 then return
	if $FuncIsRunning <> 1 or $BotIsRunning <> 1 or $isFailedMap <> 0 then return
	UpgradeSelectedNinja()
endfunc

func RestartMap()
	if $FuncIsRunning <> 1 or $BotIsRunning <> 1 or $isFailedMap <> 0 then return
	Send("{ESC}")
	Sleep(1000)
	if $FuncIsRunning <> 1 or $BotIsRunning <> 1 or $isFailedMap <> 0 then return
	MouseClick("left",1056,828,1)
	Sleep(1000)
	if $FuncIsRunning <> 1 or $BotIsRunning <> 1 or $isFailedMap <> 0 then return
	MouseClick("left",1144,722,1)
	Sleep(2000)
endfunc

func Map_2_2_Pool()
	SpawnHeroAt(327,213)
	AddNinjaToMap(1281,255)
	AddNinjaToMap(1350,193)
	AddNinjaToMap(1467,192)
	AddNinjaToMap(1282,193)
	AddNinjaToMap(1350,256)
	AddNinjaToMap(1475,307)
	AddNinjaToMap(1473,249)
	AddNinjaToMap(1357,76)
	AddNinjaToMap(1280,75)
	AddNinjaToMap(1203,76)
	AddNinjaToMap(1467,364)
	AddNinjaToMap(1130,74)
	AddNinjaToMap(1471,423)
	AddNinjaToMap(1063,70)
	AddNinjaToMap(996,72)
	AddNinjaToMap(927,75)
	AddNinjaToMap(860,73)
	AddNinjaToMap(790,69)
	AddNinjaToMap(719,63)
	AddNinjaToMap(996,72)
endfunc

func Map_3_2_PatakAtlosan()
	SpawnHeroAt(1176,121)
	AddNinjaToMap(706,359)
	AddNinjaToMap(631,422)
	AddNinjaToMap(608,481)
	AddNinjaToMap(594,544)
	AddNinjaToMap(591,609)
	AddNinjaToMap(696,742)
	AddNinjaToMap(756,764)
	AddNinjaToMap(822,784)
	AddNinjaToMap(888,778)
	AddNinjaToMap(967,756)
	AddNinjaToMap(602,296)
	AddNinjaToMap(550,337)
	AddNinjaToMap(509,381)
	AddNinjaToMap(485,435)
	AddNinjaToMap(469,490)
	AddNinjaToMap(464,549)
	AddNinjaToMap(468,607)
	AddNinjaToMap(477,665)
	AddNinjaToMap(497,720)
endfunc

func Map_2_5_Cukorkas()
	SpawnHeroAt(380,930)
	AddNinjaToMap(1299,806)
	AddNinjaToMap(1228,793)
	AddNinjaToMap(1264,857)
	AddNinjaToMap(1179,830)
	AddNinjaToMap(389,516)
	AddNinjaToMap(456,519)
	AddNinjaToMap(418,567)
	AddNinjaToMap(523,524)
	AddNinjaToMap(334,548)
	AddNinjaToMap(1165,771)
	AddNinjaToMap(1210,682)
	AddNinjaToMap(501,410)
	AddNinjaToMap(581,493)
	AddNinjaToMap(1105,798)
	AddNinjaToMap(1109,736)
	AddNinjaToMap(1042,729)
	AddNinjaToMap(561,372)
	AddNinjaToMap(1152,655)
	AddNinjaToMap(1088,609)
endfunc

func Map_1_2_Fatorzs()
	SpawnHeroAt(518,256)
	AddNinjaToMap(535,386)
	AddNinjaToMap(526,317)
	AddNinjaToMap(485,435)
	AddNinjaToMap(471,364)
	AddNinjaToMap(708,355)
	AddNinjaToMap(710,414)
	AddNinjaToMap(563,523)
	AddNinjaToMap(762,315)
	AddNinjaToMap(406,375)
	AddNinjaToMap(1231,566)
	AddNinjaToMap(1296,578)
	AddNinjaToMap(1361,597)
	AddNinjaToMap(1289,670)
	AddNinjaToMap(1324,472)
	AddNinjaToMap(1387,498)
	AddNinjaToMap(1418,625)
	AddNinjaToMap(1451,514)
	AddNinjaToMap(1424,685)
	AddNinjaToMap(1258,721)
endfunc

func TempFunc()
	$FuncIsRunning = 1 - $FuncIsRunning
	local $Cycles = 0
	while( $FuncIsRunning == 1 and $BotIsRunning == 1)
		_WriteDebugLog("log.txt", "Starting full cycle=" & $Cycles)
		
		$isFailedMap = 0
		
		; start game
		Send("{SPACE}")
		Sleep(200)
		Send("{SPACE}")
		Sleep(200)
		
		;Map_2_2_Pool()
		;Map_3_2_PatakAtlosan()
		;Map_2_5_Cukorkas()
		Map_1_2_Fatorzs()
		
		if $isFailedMap == 0 then RestartMap()
		;if $isFailedMap == 0 then WaitRestartOnFail()
		
		$Cycles = $Cycles + 1
	wend
endfunc

func ScreenshotCleanRegion()
	global $dllhandle
	; take screenshot of kingdom view
	DllCall( $dllhandle, "NONE", "TakeScreenshot", "int", 0, "int", 0, "int", 2000, "int", 2000)
	DllCall( $dllhandle,"NONE","SaveScreenshot")
endfunc