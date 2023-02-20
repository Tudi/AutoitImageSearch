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
; Always check the speed of the things you do !
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
	Sleep(1000)
	if $FuncIsRunning <> 1 or $BotIsRunning <> 1 or $isFailedMap <> 0 then return
	MouseClick("left",$x,$y,1)
	Sleep(1000)
endfunc

func SpawnUnitAtLoc( $unit, $x,$y)
	if $FuncIsRunning <> 1 or $BotIsRunning <> 1 or $isFailedMap <> 0 then return
	_WriteDebugLog("log.txt", "Sawning sniper 024 at loc x=" & $x & " y=" & $y)
	; spawn 
	Send($unit)
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

func TryReselectUnit( $unit_img, $x, $y)
	if $FuncIsRunning <> 1 or $BotIsRunning <> 1 or $isFailedMap <> 0 then return 0
	_WriteDebugLog("log.txt", "reselecting unit at loc x=" & $x & " y=" & $y)
	local $isSelected = ClickButtonIfAvailable($unit_img, 0, 0, 2000, 2000, 1, 180000 )
	if $isSelected == 0 then
		MouseClick("left", $x, $y, 1)
		Sleep(1000)
		if $FuncIsRunning <> 1 or $BotIsRunning <> 1 or $isFailedMap <> 0 then return 0
		$isSelected = ClickButtonIfAvailable($unit_img, 0, 0, 2000, 2000, 1, 180000 )
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

func UpgradeSelectedUnit($unitName, $upgradeList)
	if $FuncIsRunning <> 1 or $BotIsRunning <> 1 or $isFailedMap <> 0 then return
	
	local $DefaultActionSleep = 1000
	_WriteDebugLog("log.txt", "Started upgrading selected ")
	
	local $UpgradeIndex = 0;
	for $UpgradeIndex = 0 to 7-1
		if $upgradeList[$UpgradeIndex] == '' then return
		; avoid mouse hovering over capture area
		MouseMove(0,0,0)
		local $madeClickOnImage = 0
		while( $madeClickOnImage == 0)	
			isFailedRestart()
			if $FuncIsRunning <> 1 or $BotIsRunning <> 1 or $isFailedMap <> 0 then return
			$madeClickOnImage = ClickButtonIfAvailable("Images/"& $unitName & "_" & $upgradeList[$UpgradeIndex] & ".bmp", 0, 0, 2000, 2000, 1, 150000 )
			Sleep($DefaultActionSleep)
		wend
	Next 
endfunc

func WaitMoneyForUnit($unitIcon)
	if $FuncIsRunning <> 1 or $BotIsRunning <> 1 or $isFailedMap <> 0 then return
	local $DefaultActionSleep = 1000
	_WriteDebugLog("log.txt", "Started waiting for money for " & $unitIcon)
	; avoid mouse hovering over capture area
	MouseMove(0,0,0)
	local $madeClickOnImage = 0
	while( $madeClickOnImage == 0)	
		isFailedRestart()
		if $FuncIsRunning <> 1 or $BotIsRunning <> 1 or $isFailedMap <> 0 then return
		$madeClickOnImage = ClickButtonIfAvailable($unitIcon, 0, 0, 2000, 2000, 1, 40000 )
		Sleep($DefaultActionSleep)
	wend
endfunc

func AddUnitWithUpgradesToMap( $unitName, $unitHotkey, $x, $y, $upgrades)
	if $FuncIsRunning <> 1 or $BotIsRunning <> 1 or $isFailedMap <> 0 then return
	WaitMoneyForUnit("Images/" & $unitName & "_hasmoney.bmp")
	if $FuncIsRunning <> 1 or $BotIsRunning <> 1 or $isFailedMap <> 0 then return
	SpawnUnitAtLoc( $unitHotkey, $x, $y)
	; if for some reason we failed to spawn or select this ninja, we can't upgrade him
	if TryReselectUnit( "Images/" & $unitName & "_isSelected.bmp", $x, $y) == 0 then return
	if $FuncIsRunning <> 1 or $BotIsRunning <> 1 or $isFailedMap <> 0 then return
	UpgradeSelectedUnit( $unitName, $upgrades)
endfunc

func Map_1()
	SpawnHeroAt(544,390)

	; start game
	Send("{SPACE}")
	Sleep(500)
	Send("{SPACE}")
	Sleep(500)
		
	Local $sniperx24[7] = ["xx1","xx2","x1x","x2x","xx3","xx4",""]
	AddUnitWithUpgradesToMap("sniper", "z", 206,844, $sniperx24)
	Local $sniper2x3[7] = ["xx1","xx2","1xx","2xx",""]
	AddUnitWithUpgradesToMap("sniper", "z", 561,897, $sniper2x3)
	;Sleep(15000)
	WaitMoneyForUnit("Images/dyssey_next.bmp")
endfunc

func Map_2()
	SpawnHeroAt(1181,121)

	; start game
	Send("{SPACE}")
	Sleep(500)
	Send("{SPACE}")
	Sleep(500)
		
	Local $sniperx24[7] = ["xx1","xx2","x1x","x2x","xx3","xx4",""]
	AddUnitWithUpgradesToMap("sniper", "z", 221,885, $sniperx24)
	Local $sniper2x4[7] = ["xx1","xx2","1xx","2xx","xx3","xx4",""]
	AddUnitWithUpgradesToMap("sniper", "z", 169,780, $sniper2x4)
;	Sleep(40000)
	WaitMoneyForUnit("Images/dyssey_next.bmp")
endfunc

func Map_3()
	SpawnHeroAt(349,220)

	; start game
	Send("{SPACE}")
	Sleep(500)
	Send("{SPACE}")
	Sleep(500)
		
	Local $sniperx24[7] = ["xx1","xx2","x1x","x2x","xx3","xx4",""]
	AddUnitWithUpgradesToMap("sniper", "z", 1292,749, $sniperx24)
	Local $sniper2x4[7] = ["xx1","xx2","1xx","2xx","xx3","xx4",""]
	AddUnitWithUpgradesToMap("sniper", "z", 1186,757, $sniper2x4)
;	Local $ninja2x3[7] = ["x1x","x2x","xx1","xx2","xx3",""]
;	AddUnitWithUpgradesToMap("ninja", "d", 838,787, $ninja2x3)
;	Sleep(115000)
	WaitMoneyForUnit("Images/dyssey_next.bmp")
endfunc

func RestartMap()
endfunc

func TempFunc()
	$FuncIsRunning = 1 - $FuncIsRunning
	local $Cycles = 0
	while( $FuncIsRunning == 1 and $BotIsRunning == 1)
		_WriteDebugLog("log.txt", "Starting full cycle=" & $Cycles)
		
		$isFailedMap = 0
		
		; restart the odyssey
		MouseClick("left",1763,952,1)
		Sleep(1000)
		
		; start odysey
		MouseClick("left",1763,952,1)
		Sleep(1000)
		
		; use selected troop and wait for game to start
		MouseClick("left",1763,952,1)
		Sleep(9000)
		
		Map_1()
		; continue
		;MouseClick("left",831,806,1)
		Sleep(2000)

		; continue to next island
		MouseClick("left",1763,952,1)
		Sleep(9000)
		Map_2()
		; continue
		;MouseClick("left",831,806,1)
		Sleep(2000)

		; continue to next island
		MouseClick("left",1763,952,1)
		Sleep(9000)
		Map_3()
		;MouseClick("left",831,806,1)
		Sleep(2000)
		
		; gather "rewards"
		MouseClick("left",962,837,1)
		Sleep(1500)
		MouseClick("left",962,837,1)
		Sleep(1000)
		MouseClick("left",962,837,1)
		Sleep(1000)
		MouseClick("left",962,837,1)
		Sleep(1000)
		MouseClick("left",962,837,1)
		Sleep(1000)
		MouseClick("left",962,837,1)
		Sleep(1000)
		
		;if $isFailedMap == 0 then RestartMap()
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