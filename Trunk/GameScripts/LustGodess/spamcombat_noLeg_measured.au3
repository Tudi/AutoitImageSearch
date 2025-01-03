#include <GUIConstantsEx.au3>
#include <Date.au3>
#include <WinAPISys.au3>
#Include "CommonFunctions.au3"

Opt("MustDeclareVars", 1)
HotKeySet("O", "ExitBot")
HotKeySet("P", "ForeverLoopFunc")
HotKeySet("I", "ResetStats")

;only used while developing
HotKeySet("Q", "TakeScreenshotCyclePositions")
HotKeySet("W", "TakeScreenshotAtMousePosPreRecorded")

global $BotIsRunning = 1
global $FuncIsRunning = 0
Global $clickInterval = 20000
Global $lastClickTimer = TimerInit()
Global $GamesStarted = 0
Global $GamesWon = 0
Global $GamesLost = 0
Global $StopAfterXPlayedGames = 0
global $SecondsPlayed = 0
global $AcceptedColorTolerance = 3
global $ColorSearchRadius = 1
global $pos ; last pixel pos 
; generate statistics of number of games played between 2 legendary chests
global $CheckIngamePrevState = 1
global $GamesDetected = 0
global $GameCountAtPrevLegendary = 0
global $LegendariesSkipped = 0
global $GamesCounterPrev = -1
local $InfiniLoopStampAtStart = _WinAPI_GetTickCount() / 1000
local $InfiniLoopStampAtEnd = 0

func ExitBot()
	global $BotIsRunning = 0
endfunc

Func ResetStats()
	Global $GamesWon = 0
	Global $GamesLost = 0
	Global $GamesStarted = 0
	global $SecondsPlayed = 0
EndFunc

; Define GUI margins and spacing
Local $margin = 3
Local $spacing = 3

; Create the GUI (initial size will be adjusted later)
Local $hGUI = GUICreate("auto combat", 400, 300)

; Create labels and input controls
Local $labelWidth = 80
Local $inputWidth = 50
Local $ComponentHeight = 15
Local $ComponentsAdded = 0

GUICtrlCreateLabel("Spam limit :", $margin, $margin, $labelWidth, $ComponentHeight)
Local $inputSpamLimit = GUICtrlCreateInput("", $margin + $labelWidth + $spacing, $margin, $inputWidth, $ComponentHeight)
$ComponentsAdded = $ComponentsAdded + 1

;GUICtrlCreateLabel("Win limit :", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth, $ComponentHeight)
;Local $inputWinLimit = GUICtrlCreateInput("", $margin + $labelWidth + $spacing, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $inputWidth, $ComponentHeight)
;$ComponentsAdded = $ComponentsAdded + 1

;GUICtrlCreateLabel("Games Prev :", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth, $ComponentHeight)
;Local $inputGamesBeforeRestart = GUICtrlCreateInput("", $margin + $labelWidth + $spacing, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $inputWidth, $ComponentHeight)
;$ComponentsAdded = $ComponentsAdded + 1

;GUICtrlCreateLabel("Games played:", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth, $ComponentHeight)
;Local $lblGamesPlayed = GUICtrlCreateLabel("", $margin + $labelWidth + $spacing, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $inputWidth, $ComponentHeight)
;Local $lblGamesPlayed = GUICtrlCreateLabel("", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth + $inputWidth, $ComponentHeight)
;$ComponentsAdded = $ComponentsAdded + 1

Local $lblGamesPlayed2 = GUICtrlCreateLabel("", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth + $inputWidth, $ComponentHeight)
$ComponentsAdded = $ComponentsAdded + 1

Global $hCheckboxSpamCombat = GUICtrlCreateCheckbox("Spam Combat", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth, $ComponentHeight)
$ComponentsAdded = $ComponentsAdded + 1

Global $hCheckboxOpenLegendary = GUICtrlCreateCheckbox("OpenLeg", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth, $ComponentHeight)
$ComponentsAdded = $ComponentsAdded + 1

Global $hCheckboxPauseLegendary = GUICtrlCreateCheckbox("PauseLeg", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth, $ComponentHeight)
$ComponentsAdded = $ComponentsAdded + 1

Global $hCheckboxSkipLegendary = GUICtrlCreateCheckbox("SkipLeg", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth, $ComponentHeight)
GUICtrlSetState($hCheckboxSkipLegendary, $GUI_CHECKED)
$ComponentsAdded = $ComponentsAdded + 1

GUICtrlCreateLabel("G4L limit :", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth, $ComponentHeight)
Local $inputGamesForLegChance = GUICtrlCreateInput("100", $margin + $labelWidth + $spacing, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $inputWidth, $ComponentHeight)
$ComponentsAdded = $ComponentsAdded + 1

Global $hCheckboxSpamUntilLegChance = GUICtrlCreateCheckbox("Spam4Leg", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth, $ComponentHeight)
$ComponentsAdded = $ComponentsAdded + 1
GUICtrlSetState($hCheckboxSpamUntilLegChance, $GUI_CHECKED)

GUICtrlCreateLabel("Playtime(min):", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth, $ComponentHeight)
Local $lblTimeDiff = GUICtrlCreateLabel("", $margin + $labelWidth + $spacing, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $inputWidth, $ComponentHeight)
$ComponentsAdded = $ComponentsAdded + 1

GUICtrlCreateLabel("Rank:", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth, $ComponentHeight)
Local $lblPVPRank = GUICtrlCreateLabel("", $margin + $labelWidth + $spacing, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $inputWidth, $ComponentHeight)
$ComponentsAdded = $ComponentsAdded + 1

;Local $lblDbgOut = GUICtrlCreateLabel("Debug", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth + $inputWidth, $ComponentHeight)
;$ComponentsAdded = $ComponentsAdded + 1

; Dynamically calculate the GUI size based on the components
Local $guiWidth = $margin * 2 + $labelWidth + $spacing + $inputWidth
Local $guiHeight = $margin * 2 + $ComponentsAdded * ( $margin + $ComponentHeight) + 25; Total height includes all components and margin

; Calculate the top-right corner position
Local $posX = @DesktopWidth - $guiWidth - 20
Local $posY = 40 ; Top edge of the screen

; Resize and move the GUI to the top-right corner
WinMove($hGUI, "", $posX, $posY, $guiWidth, $guiHeight)

; Show the GUI
GUISetState(@SW_SHOW, $hGUI)
WinSetOnTop($hGUI, "", 1)

; load saved variables
LoadVariables()
CountIngameStates()
UpdateUiGameCounter(True)
CalculateAverageFromFile("legDistances.txt")

; Event Loop
While ( $BotIsRunning == 1)
    local $msg = GUIGetMsg()

    If $msg = $GUI_EVENT_CLOSE Then 
		$BotIsRunning = 0
		$FuncIsRunning = 0
		Exit
	endif
WEnd

exit

; save/load saved variables 
Func SaveVariables()
    IniWrite("variables.ini", "Variables", "GamesDetected", $GamesDetected)
    IniWrite("variables.ini", "Variables", "GameCountAtPrevLegendary", $GameCountAtPrevLegendary)
    IniWrite("variables.ini", "Variables", "LegendariesSkipped", $LegendariesSkipped)
EndFunc

Func LoadVariables()
    Global $GamesDetected = IniRead("variables.ini", "Variables", "GamesDetected", 0)
    Global $GameCountAtPrevLegendary = IniRead("variables.ini", "Variables", "GameCountAtPrevLegendary", 0)
    Global $LegendariesSkipped = IniRead("variables.ini", "Variables", "LegendariesSkipped", 0)
EndFunc

Func CalculateAverageFromFile($filePath)
    ; Open the file in read mode
    Local $fileHandle = FileOpen($filePath, 0)
    If $fileHandle = -1 Then
        Return -1
    EndIf

    ; Read the entire file
    Local $fileContent = FileRead($fileHandle)
    FileClose($fileHandle)

    ; Remove line breaks and normalize the content
    $fileContent = StringReplace($fileContent, @CRLF, ",")
    $fileContent = StringReplace($fileContent, @LF, ",")
    $fileContent = StringReplace($fileContent, @CR, ",")

    ; Split the content by comma
    Local $values = StringSplit($fileContent, ",", 2)
    If @error Then
        Return -1
    EndIf

    ; Calculate sum and count valid numbers
    Local $sum = 0
    Local $count = 0

    For $i = 0 To UBound($values) - 1
        Local $num = Number($values[$i])
        If $values[$i] <> "" And IsNumber($num) Then
            $sum += $num
            $count += 1
        EndIf
    Next

    ; Check if there are any numbers to calculate the average
    If $count = 0 Then
        Return 0
    EndIf

    ; Calculate and return the average
    Local $average = (int)($sum / $count)

	GUICtrlSetData($inputGamesForLegChance, $average)
	
    Return $average
EndFunc

func getInputNumericValue($InputName, $DefaultValue = 999999)
	; Get number of calls from input
	local $InputVal = GUICtrlRead($InputName)
	If Not StringIsInt($InputVal) Or $InputVal <= 0 Then
		$InputVal = $DefaultValue
	EndIf
	return $InputVal
endfunc

Func OpenChests()
    ; Set the color to search for
    Local $colorToFind = 0x76B9ED
    ; Define the search area
    Local $xStart = 233
    Local $xEnd = 1683
    Local $yStart = 872
    Local $yEnd = 892
	Local $tolerance = 10

    ; Search for the pixel color in the defined area
    Local $pos1 = PixelSearch($xStart, $yStart, $xEnd, $yEnd, $colorToFind, $tolerance)

    ; If the color is found, click on it
    If Not @error Then
        MouseClick("left", $pos1[0], $pos1[1])
		Sleep(5000)
        MouseClick("left", $pos1[0], $pos1[1])
		Sleep(1000)
    EndIf
EndFunc

Func OpenChests2()
    ; Set the color to search for
    Local $colorToFind = 0xDDDFFF
    ; Define the search area
    Local $xStart = 233
    Local $xEnd = 1683
    Local $yStart = 872
    Local $yEnd = 892
	Local $tolerance = 10

    ; Search for the pixel color in the defined area
    Local $pos1 = PixelSearch($xStart, $yStart, $xEnd, $yEnd, $colorToFind, $tolerance)

    ; If the color is found, click on it
    If Not @error Then
        MouseClick("left", $pos1[0], $pos1[1])
		Sleep(5000)
        MouseClick("left", $pos1[0], $pos1[1])
		Sleep(1000)
    EndIf
EndFunc

Func OpenWinstreakChests()
    ; If the color is found, click on it
    If IsColorAtPos(1585,653,0xFFD802,$ColorSearchRadius,$AcceptedColorTolerance) Then
        MouseClick("left", 1542, 525)
		Sleep(5000)
        MouseClick("left", 1703, 899)
		Sleep(1000)
    EndIf
EndFunc

Func Fight()
	Global $GamesStarted, $pos
    If IsColorAtPos(1088, 535, 0xFF4444, $ColorSearchRadius, $AcceptedColorTolerance) Then
		$GamesStarted = $GamesStarted + 1
        MouseClick("left", $pos[0], $pos[1])
		Sleep(2000)
        MouseClick("left", $pos[0], $pos[1])
		Sleep(3000)
    EndIf
EndFunc

Func Fight2()
	Global $GamesStarted, $pos
    If IsColorAtPos(1092,536,0xBF2D03,$ColorSearchRadius,$AcceptedColorTolerance) Then
		$GamesStarted = $GamesStarted + 1
        MouseClick("left", $pos[0], $pos[1])
		Sleep(2000)
        MouseClick("left", $pos[0], $pos[1])
		Sleep(3000)
    EndIf
EndFunc

Func AddLegendaryGameFreqStat()
;	local $GamesBeforeAppRestart = getInputNumericValue($inputGamesBeforeRestart, 0)
;	if $GameCountAtPrevLegendary <> 0 or $GamesBeforeAppRestart <> 0 then
;		GUICtrlSetData($inputGamesBeforeRestart, 0)
		local $GamesSinceLegendary = $GamesDetected - $GameCountAtPrevLegendary
		;$GamesSinceLegendary = $GamesSinceLegendary + $GamesBeforeAppRestart
		if $GamesSinceLegendary > 0 then
			Local $hFile = FileOpen("legDistances.txt", 1) ; Mode 1 = Append
			If $hFile <> -1 Then
				FileWrite($hFile, $GamesSinceLegendary & ",")
				FileClose($hFile)		
			EndIf
		Endif
;	endif
	$GameCountAtPrevLegendary = $GamesDetected
	; just so that I don't freak out that the counter is not working
	UpdateUiGameCounter(True)
EndFunc

Func HandleLegendaryWinReward()
	; is this a legendary reward ?
    If IsColorAtPos(854,575,0xC56001,$ColorSearchRadius,$AcceptedColorTolerance) Then
		AddLegendaryGameFreqStat()
		
		If GUICtrlRead($hCheckboxPauseLegendary) = $GUI_CHECKED Then
			$FuncIsRunning = 0
			return 1
		EndIf
		
		If GUICtrlRead($hCheckboxOpenLegendary) = $GUI_CHECKED Then
			MouseClick("left", 929, 768)
			Sleep(2000)
			MouseClick("left", 929, 768);
			Sleep(1000)
			MouseClick("left", 1634, 203)
			Sleep(1000)
			return 1
		EndIf
		
		If GUICtrlRead($hCheckboxSkipLegendary) = $GUI_CHECKED Then
			MouseClick("left", 1128, 224)
			Sleep(3000)
			MouseClick("left", 1079, 570)
			Sleep(3000)
			$LegendariesSkipped = $LegendariesSkipped + 1
			return 1
		EndIf
		
	EndIf
	return 0
Endfunc

Func ClickFightWon()	
	Global $GamesWon, $pos
    If IsColorAtPos(1129,235,0xFFEBA2,$ColorSearchRadius,$AcceptedColorTolerance) Then
		Sleep(750) ; seems like the chest appears slightly later than the victory letters
		$GamesWon = $GamesWon + 1
		; no need to click on yellow letter if we opened the check ?
		if HandleLegendaryWinReward() == 1 then
			return
		endif
		; click the 'y' to make the screen go away
        MouseClick("left", $pos[0], $pos[1])
		Sleep(3000)
    EndIf
EndFunc

Func ClickFightLost()
	Global $GamesLost, $pos
    If IsColorAtPos(1106,192,0xFF4342,$ColorSearchRadius,$AcceptedColorTolerance) Then
		$GamesLost = $GamesLost + 1
        MouseClick("left", $pos[0], $pos[1])
		Sleep(3000)
    EndIf
EndFunc

Func ClearRandomClickContent()
    ; Check if the click interval has passed since the last click
    If TimerDiff($lastClickTimer) >= $clickInterval Then
		MouseClick("left", 1634, 203)
        $lastClickTimer = TimerInit()
    EndIf	
EndFunc

func CountIngameStates()
	; if previously we were ingame, check if we are out of a game right now
	if $CheckIngamePrevState == 0 then
		If IsColorAtPos(1707, 277, 0xB41A1B, 2, 4) or IsColorAtPos(1707, 277, 0xA10E18, 2, 4) Then ; red color
			$CheckIngamePrevState = 1 - $CheckIngamePrevState
			GUICtrlSetBkColor($lblGamesPlayed2, 0x00FF00)
			$GamesDetected = $GamesDetected + 1
			SaveVariables()
		EndIf
	; if previously we were out of game, check if we are in a game right now
	elseif $CheckIngamePrevState == 1 then
		If IsColorAtPos(1707, 277, 0x28283F, 2, 4) Then ; some gray color
			$CheckIngamePrevState = 1 - $CheckIngamePrevState
			GUICtrlSetBkColor($lblGamesPlayed2, 0xFF7F7F)
		EndIf
	endif
endfunc

func UpdateUiGameCounter($bForceUpdate = False)
	global $GamesDetected, $GamesCounterPrev
	if $GamesDetected <> $GamesCounterPrev or $bForceUpdate == True then
		$GamesCounterPrev = $GamesDetected

		; time played since reset. Shown in minutes
		$InfiniLoopStampAtEnd = _WinAPI_GetTickCount() / 1000
		GUICtrlSetData($lblTimeDiff, ($SecondsPlayed + ($InfiniLoopStampAtEnd-$InfiniLoopStampAtStart)/60))

		; Total games played so far
		;local $playedString = "GP " & $GamesCounter & " : " & $GamesStarted & " /(" & $GamesWon & "+" & $GamesLost  & ")"
		;local $playedString = $GamesStarted & "=" & ($GamesWon + $GamesLost)
		;GUICtrlSetData($lblGamesPlayed, $playedString)
		local $GamesSinceLegendary = $GamesDetected - $GameCountAtPrevLegendary
		;$GamesSinceLegendary = $GamesSinceLegendary + getInputNumericValue($inputGamesBeforeRestart, 0)
		local $playedString2 = "G " & $GamesDetected & " GSL " & $GamesSinceLegendary & " LS " & $LegendariesSkipped
		GUICtrlSetData($lblGamesPlayed2, $playedString2)
	endif
endfunc

global $g_LastSeenRank = 0
func GuessCurrentPVPRank()
	global $g_LastSeenRank
	if $CheckIngamePrevState == 0 or 1 then
		local $acceptableSADNumber = 200
		local $searchRet = ImageIsAt("rank_11_0189_0023_0038_0020.bmp")
		if $searchRet[2] < $acceptableSADNumber and $searchRet[2] > -1 then
			$g_LastSeenRank = 11
			GUICtrlSetData($lblPVPRank, $g_LastSeenRank & '  ' & $searchRet[2])
		endif
	endif
endfunc

func ForeverLoopFunc()
	$FuncIsRunning = 1 - $FuncIsRunning

	if $FuncIsRunning == 1 then
		GUICtrlSetBkColor($lblTimeDiff, 0x00FF00)
	endif
	
	local $Cycles = 0
	while( $FuncIsRunning == 1 and $BotIsRunning == 1 )

		; Small delay to prevent high CPU usage
		Sleep(500)

		; How many games have we plaed so far. Are we still allowed to spam games ?
		Local $GamesCounter = $GamesWon + $GamesLost
		if $GamesStarted < $GamesCounter then
			$GamesCounter = $GamesStarted
		endif
			
		;GUICtrlSetData($lblDbgOut, Hex( PixelGetColor(1707, 277), 6 ) & " -- " & $CheckIngamePrevState)

		GuessCurrentPVPRank()
			
		for $i=0 to 2
			If ( $FuncIsRunning == 1 and $BotIsRunning == 1 ) then 
				CountIngameStates()
				OpenWinstreakChests()
				OpenChests()
				OpenChests2()
				Sleep(250)
			EndIf
		Next
		
		; normal fight always in case we have chest slot open
		Fight()
		
		; spam fights while generating statistics about leg distances
		If GUICtrlRead($hCheckboxSpamUntilLegChance) = $GUI_CHECKED then
			local $GamesSinceLegendary = $GamesDetected - $GameCountAtPrevLegendary
			;$GamesSinceLegendary = $GamesSinceLegendary + getInputNumericValue($inputGamesBeforeRestart, 0)
			local $OptimalGameCount = getInputNumericValue($inputGamesForLegChance,100)
			if $GamesSinceLegendary < $OptimalGameCount Then
				Fight2()
			endif
		EndIf
		; spam fights in case we want to finish heliquests
		If GUICtrlRead($hCheckboxSpamCombat) = $GUI_CHECKED then
			; there is the quest win X games
;			if getInputNumericValue($inputSpamLimit,999999) > $GamesDetected or getInputNumericValue($inputWinLimit,999999) > $GamesWon Then
			if getInputNumericValue($inputSpamLimit,999999) > $GamesDetected Then
				Fight2()
			endif
			; Clear the checkmark of the checkbox
;			if getInputNumericValue($inputSpamLimit,999999) <= $GamesDetected or getInputNumericValue($inputWinLimit,999999) <= $GamesWon Then
			if getInputNumericValue($inputSpamLimit,999999) <= $GamesDetected Then
				GUICtrlSetState($hCheckboxSpamCombat, $GUI_UNCHECKED)
			endif
		endif
		ClickFightWon()
		ClickFightLost()
		ClearRandomClickContent()

		UpdateUiGameCounter()
		
		$Cycles = $Cycles + 1
	wend

	$InfiniLoopStampAtEnd = _WinAPI_GetTickCount() / 1000
	$SecondsPlayed = $SecondsPlayed + ($InfiniLoopStampAtEnd-$InfiniLoopStampAtStart)/60

	GUICtrlSetBkColor($lblTimeDiff, 0xFF0000)
	
endfunc