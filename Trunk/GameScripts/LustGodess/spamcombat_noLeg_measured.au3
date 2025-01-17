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
Const $SCREEN_PVP_FIGHTING = 1, $SCREEN_START_PVP_FIGHT = 0, $SCREEN_UNKNOWN = 3
global $g_VisibleScreenType = $SCREEN_UNKNOWN
global $GamesDetected = 0
global $GameCountAtPrevLegendary = 0
global $LegendariesSkipped = 0
global $GamesCounterPrev = -1
global $InfiniLoopStampAtStart = _WinAPI_GetTickCount() / 1000
global $InfiniLoopStampAtEnd = 0

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
Global $inputSpamLimit = GUICtrlCreateInput("", $margin + $labelWidth + $spacing, $margin, $inputWidth, $ComponentHeight)
$ComponentsAdded = $ComponentsAdded + 1

;GUICtrlCreateLabel("Win limit :", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth, $ComponentHeight)
;Global $inputWinLimit = GUICtrlCreateInput("", $margin + $labelWidth + $spacing, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $inputWidth, $ComponentHeight)
;$ComponentsAdded = $ComponentsAdded + 1

;GUICtrlCreateLabel("Games Prev :", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth, $ComponentHeight)
;Global $inputGamesBeforeRestart = GUICtrlCreateInput("", $margin + $labelWidth + $spacing, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $inputWidth, $ComponentHeight)
;$ComponentsAdded = $ComponentsAdded + 1

;GUICtrlCreateLabel("Games played:", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth, $ComponentHeight)
;Global $lblGamesPlayed = GUICtrlCreateLabel("", $margin + $labelWidth + $spacing, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $inputWidth, $ComponentHeight)
;Global $lblGamesPlayed = GUICtrlCreateLabel("", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth + $inputWidth, $ComponentHeight)
;$ComponentsAdded = $ComponentsAdded + 1

Global $lblGamesPlayed2 = GUICtrlCreateLabel("", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth + $inputWidth, $ComponentHeight)
$ComponentsAdded = $ComponentsAdded + 1

Global $hCheckboxSpamCombat = GUICtrlCreateCheckbox("Spam Combat", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth + $inputWidth, $ComponentHeight)
$ComponentsAdded = $ComponentsAdded + 1

Global $hCheckboxOpenLegendary = GUICtrlCreateCheckbox("OpenLeg", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth + $inputWidth, $ComponentHeight)
$ComponentsAdded = $ComponentsAdded + 1

Global $hCheckboxPauseLegendary = GUICtrlCreateCheckbox("PauseLeg", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth + $inputWidth, $ComponentHeight)
$ComponentsAdded = $ComponentsAdded + 1

Global $hCheckboxSkipLegendary = GUICtrlCreateCheckbox("SkipLeg", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth + $inputWidth, $ComponentHeight)
GUICtrlSetState($hCheckboxSkipLegendary, $GUI_CHECKED)
$ComponentsAdded = $ComponentsAdded + 1

GUICtrlCreateLabel("G4L limit :", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth, $ComponentHeight)
Global $inputGamesForLegChance = GUICtrlCreateInput("100", $margin + $labelWidth + $spacing, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $inputWidth, $ComponentHeight)
$ComponentsAdded = $ComponentsAdded + 1

GUICtrlCreateLabel("Max rank :", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth, $ComponentHeight)
Global $inputMinRankAllowed = GUICtrlCreateInput("19", $margin + $labelWidth + $spacing, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $inputWidth, $ComponentHeight)
$ComponentsAdded = $ComponentsAdded + 1

GUICtrlCreateLabel("Min rank always :", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth, $ComponentHeight)
Global $inputFixedRankUsed = GUICtrlCreateInput("0", $margin + $labelWidth + $spacing, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $inputWidth, $ComponentHeight)
$ComponentsAdded = $ComponentsAdded + 1

;GUICtrlCreateLabel("Rank:", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth, $ComponentHeight)
;Local $lblPVPRank = GUICtrlCreateLabel("", $margin + $labelWidth + $spacing, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $inputWidth, $ComponentHeight)
;$ComponentsAdded = $ComponentsAdded + 1

Global $hCheckboxSpamUntilLegChance = GUICtrlCreateCheckbox("Spam4Leg", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth + $inputWidth, $ComponentHeight)
$ComponentsAdded = $ComponentsAdded + 1
GUICtrlSetState($hCheckboxSpamUntilLegChance, $GUI_CHECKED)

;avoid accidentally storing low level legendary. Has a 9 hour chest slot cooldown
Global $hCheckboxNoChestsWhileSpamUntilLeg = GUICtrlCreateCheckbox("NoChestWhileSpam", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth + $inputWidth, $ComponentHeight)
$ComponentsAdded = $ComponentsAdded + 1
GUICtrlSetState($hCheckboxNoChestsWhileSpamUntilLeg, $GUI_CHECKED)

GUICtrlCreateLabel("Playtime(min):", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth, $ComponentHeight)
Global $lblTimeDiff = GUICtrlCreateLabel("", $margin + $labelWidth + $spacing, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $inputWidth, $ComponentHeight)
$ComponentsAdded = $ComponentsAdded + 1

Global $btnSpam20 = GUICtrlCreateButton("Spam 15", $margin, $margin + $ComponentsAdded * ($margin + $ComponentHeight), $labelWidth + $inputWidth + $spacing, $ComponentHeight)
$ComponentsAdded = $ComponentsAdded + 1

Global $btnSpam25 = GUICtrlCreateButton("Spam 20", $margin, $margin + $ComponentsAdded * ($margin + $ComponentHeight), $labelWidth + $inputWidth + $spacing, $ComponentHeight)
$ComponentsAdded = $ComponentsAdded + 1

Global $btnSpam30 = GUICtrlCreateButton("Spam 30", $margin, $margin + $ComponentsAdded * ($margin + $ComponentHeight), $labelWidth + $inputWidth + $spacing, $ComponentHeight)
$ComponentsAdded = $ComponentsAdded + 1

Local $lblDbgOut = GUICtrlCreateLabel("Debug", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth + $inputWidth, $ComponentHeight)
$ComponentsAdded = $ComponentsAdded + 1

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
	HandleGUIMessages()
WEnd

; sadly this would need to be run in a separate thread than ForeverLoopFunc
Func HandleGUIMessages()
    local $msg = GUIGetMsg()

    If $msg = $GUI_EVENT_CLOSE Then 
		$BotIsRunning = 0
		$FuncIsRunning = 0
		Exit
	endif
	
    ; Check if the "Spam 15" button was clicked
    If $msg = $btnSpam20 Then
        SpamButtonHandler(15)
    ElseIf $msg = $btnSpam25 Then
        SpamButtonHandler(20)
    ElseIf $msg = $btnSpam30 Then
        SpamButtonHandler(30)
    EndIf	
endfunc

exit

Func SpamButtonHandler($SpamXGames)
	GUICtrlSetData($inputSpamLimit, $GamesDetected + $SpamXGames)
	GUICtrlSetState($hCheckboxSpamCombat, $GUI_CHECKED)
EndFunc

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
	If ( $FuncIsRunning <> 1 or $BotIsRunning <> 1 ) then 
		return
	endif
    ; Set the color to search for
    Local $colorsToFind[2]
	$colorsToFind[0] = 0x76B9ED
	$colorsToFind[1] = 0xDDDFFF
	For $i = 0 To UBound($colorsToFind) - 1
		Local $colorToFind = $colorsToFind[$i]
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
			Sleep(3000)
			MouseClick("left", $pos1[0], $pos1[1])
			Sleep(1000)
			return
		EndIf
	Next
EndFunc

Func OpenWinstreakChests()
	If ( $FuncIsRunning <> 1 or $BotIsRunning <> 1 ) then 
		return
	endif
    ; If the color is found, click on it
    If IsColorAtPos(1585,653,0xFFD802,$ColorSearchRadius,$AcceptedColorTolerance) Then
        MouseClick("left", 1542, 525)
		Sleep(3000)
        MouseClick("left", 1703, 899)
		Sleep(1000)
    EndIf
EndFunc

Func Fight()
	If ( $FuncIsRunning <> 1 or $BotIsRunning <> 1 ) then 
		return
	endif
	Global $GamesStarted, $pos
    If IsColorAtPos(1088, 535, 0xFF4444, $ColorSearchRadius, $AcceptedColorTolerance) Then
		$GamesStarted = $GamesStarted + 1
        MouseClick("left", $pos[0], $pos[1])
		Sleep(2000)
        MouseClick("left", $pos[0], $pos[1]) ; same click second time because of random animations. Might be useless if first one triggered loading screen
		Sleep(3000)
    EndIf
EndFunc

Func Fight2()
	If ( $FuncIsRunning <> 1 or $BotIsRunning <> 1 ) then 
		return
	endif
	Global $GamesStarted, $pos
    If IsColorAtPos(1092,536,0xBF2D03,$ColorSearchRadius,$AcceptedColorTolerance) Then
		$GamesStarted = $GamesStarted + 1
        MouseClick("left", $pos[0], $pos[1])
		Sleep(2000)
        MouseClick("left", $pos[0], $pos[1]) ; same click second time because of random animations. Might be useless if first one triggered loading screen
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
	If ( $FuncIsRunning <> 1 or $BotIsRunning <> 1 ) then 
		return
	endif
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
	If ( $FuncIsRunning <> 1 or $BotIsRunning <> 1 ) then 
		return
	endif
	Global $GamesWon, $pos
    If IsColorAtPos(1129,235,0xFFEBA2,$ColorSearchRadius,$AcceptedColorTolerance) Then
		global $g_LastSeenRank = 0 ; make sure we refresh the rank before we infinite spam "dropgames"
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
	If ( $FuncIsRunning <> 1 or $BotIsRunning <> 1 ) then 
		return
	endif
	Global $GamesLost, $pos
    If IsColorAtPos(1106,192,0xFF4342,$ColorSearchRadius,$AcceptedColorTolerance) Then
		global $g_LastSeenRank = 0 ; make sure we refresh the rank before we infinite spam "dropgames"
		$GamesLost = $GamesLost + 1
        MouseClick("left", $pos[0], $pos[1])
		Sleep(3000)
    EndIf
EndFunc

Func ClearRandomClickContent()
	If ( $FuncIsRunning <> 1 or $BotIsRunning <> 1 ) then 
		return
	endif
    ; Check if the click interval has passed since the last click
    If TimerDiff($lastClickTimer) >= $clickInterval Then
		MouseClick("left", 1634, 203)
        $lastClickTimer = TimerInit()
    EndIf	
EndFunc

func CountIngameStates()
	global $g_VisibleScreenType, $GamesDetected 
	; if previously we were out of the game, check if we are in the game right now
	if $g_VisibleScreenType == $SCREEN_START_PVP_FIGHT or $g_VisibleScreenType == $SCREEN_UNKNOWN then
		If IsColorAtPos(1707, 277, 0xB41A1B, 2, 4) or IsColorAtPos(1707, 277, 0xA10E18, 2, 4) Then ; red color
			$g_VisibleScreenType = $SCREEN_PVP_FIGHTING
			GUICtrlSetBkColor($lblGamesPlayed2, 0x00FF00)
			$GamesDetected = $GamesDetected + 1
			SaveVariables()
		EndIf
	EndIf	
	; if previously we were in game, check if we are out of the game right now
	if $g_VisibleScreenType == $SCREEN_PVP_FIGHTING or $g_VisibleScreenType == $SCREEN_UNKNOWN then
		If IsColorAtPos(1707, 277, 0x28283F, 2, 4) Then ; some gray color
			$g_VisibleScreenType = $SCREEN_START_PVP_FIGHT
			GUICtrlSetBkColor($lblGamesPlayed2, 0xFFBEBE)
		EndIf
	endif
	; some loading screen or something
	Local $ConfirmedIngame = 0
	Local $ConfirmedOutGame = 0
	if $g_VisibleScreenType == $SCREEN_START_PVP_FIGHT and IsColorAtPos(1707, 277, 0x28283F, 2, 4) then
		$ConfirmedOutGame = 1
	endif
	if $g_VisibleScreenType == $SCREEN_PVP_FIGHTING and (IsColorAtPos(1707, 277, 0xB41A1B, 2, 4) or IsColorAtPos(1707, 277, 0xA10E18, 2, 4) ) then
		$ConfirmedIngame = 1
	endif	
	if $ConfirmedIngame == 0 and $ConfirmedOutGame == 0 then
		$g_VisibleScreenType = $SCREEN_UNKNOWN
		GUICtrlSetBkColor($lblGamesPlayed2, 0xBEBEFF)
	endif
endfunc

func UpdateUiGameCounter($bForceUpdate = False)
	global $GamesDetected, $GamesCounterPrev, $InfiniLoopStampAtEnd, $InfiniLoopStampAtStart
;	if $GamesDetected <> $GamesCounterPrev or $bForceUpdate == True then
		$GamesCounterPrev = $GamesDetected

		; time played since reset. Shown in minutes
		$InfiniLoopStampAtEnd = _WinAPI_GetTickCount() / 1000
		local $TotalSecondsPassedRunning = $SecondsPlayed + ($InfiniLoopStampAtEnd-$InfiniLoopStampAtStart)
		local $hoursPassed = int($TotalSecondsPassedRunning/(60*60))
		local $minutesPassed = Mod(int($TotalSecondsPassedRunning/60),60)
		GUICtrlSetData($lblTimeDiff, $hoursPassed & " : " & $minutesPassed)

		; Total games played so far
		;local $playedString = "GP " & $GamesCounter & " : " & $GamesStarted & " /(" & $GamesWon & "+" & $GamesLost  & ")"
		;local $playedString = $GamesStarted & "=" & ($GamesWon + $GamesLost)
		;GUICtrlSetData($lblGamesPlayed, $playedString)
		local $GamesSinceLegendary = $GamesDetected - $GameCountAtPrevLegendary
		;$GamesSinceLegendary = $GamesSinceLegendary + getInputNumericValue($inputGamesBeforeRestart, 0)
		local $playedString2 = "G " & $GamesDetected & " GSL " & $GamesSinceLegendary & " LS " & $LegendariesSkipped
		GUICtrlSetData($lblGamesPlayed2, $playedString2)
;	endif
endfunc

global $g_LastSeenRank = 0
global $g_AppliedColorMaskToCachedImages = 0
func GuessCurrentPVPRank()
	; if script got interrupted, skip executing this function. Happens due to sleeps
	If ( $FuncIsRunning <> 1 or $BotIsRunning <> 1 ) then 
		return
	endif
	global $g_LastSeenRank, $g_AppliedColorMaskToCachedImages, $g_VisibleScreenType
	if $g_LastSeenRank <> 0 then
		return
	endif
	if $g_VisibleScreenType == $SCREEN_START_PVP_FIGHT and IsColorAtPos(1707, 277, 0x28283F, 2, 4) == 1 then
		Local $acceptableDiffPCT = 15
		Local $acceptableSADPerPixel = 8
		Local $imageRanks[12][2]
		$imageRanks[0][0] = "L10_0826_0181_0283_0109.bmp"
		$imageRanks[0][1] = 10
		$imageRanks[1][0] = "L11_0822_0167_0266_0122.bmp"
		$imageRanks[1][1] = 11
		$imageRanks[2][0] = "L12_0822_0171_0267_0117.bmp"
		$imageRanks[2][1] = 12
		$imageRanks[3][0] = "L13_0823_0171_0265_0120.bmp"
		$imageRanks[3][1] = 13
		$imageRanks[4][0] = "L14_0821_0174_0266_0115.bmp"
		$imageRanks[4][1] = 14
		$imageRanks[5][0] = "L15_0829_0169_0260_0122.bmp"
		$imageRanks[5][1] = 15
		$imageRanks[6][0] = "L16_0825_0162_0250_0133.bmp"
		$imageRanks[6][1] = 16
		$imageRanks[7][0] = "L17_0825_0162_0250_0133.bmp"
		$imageRanks[7][1] = 17
		$imageRanks[8][0] = "L18_0825_0162_0250_0133.bmp"
		$imageRanks[8][1] = 18
		$imageRanks[9][0] = "L19_0825_0162_0250_0133.bmp"
		$imageRanks[9][1] = 19
		$imageRanks[10][0] = "L20_0825_0162_0250_0133.bmp"
		$imageRanks[10][1] = 20
		$imageRanks[11][0] = "L21_0825_0162_0250_0133.bmp"
		$imageRanks[11][1] = 21
		
		if $g_AppliedColorMaskToCachedImages == 0 then
			$g_AppliedColorMaskToCachedImages = 1
			SetScreenshotMaxFPS(2)
			For $i = 0 To UBound($imageRanks) - 1
				ApplyColorMaskOnCachedImage($imageRanks[$i][0])
			Next
		endif

		;Local $hTimer = TimerInit()
		$g_LastSeenRank = 0
		local $bestSadPP = 100000
		For $i = 0 To UBound($imageRanks) - 1
			Local $searchRet = ImageIsAt($imageRanks[$i][0])
;			If $searchRet[0] > 0 and $searchRet[5] < $acceptableDiffPCT Then
			If $searchRet[0] > 0 and $searchRet[3] < $acceptableSADPerPixel Then
;				$acceptableDiffPCT = $searchRet[5]
				$acceptableSADPerPixel = $searchRet[3]
				$g_LastSeenRank = $imageRanks[$i][1]
			EndIf
			If $searchRet[0] > 0 and $searchRet[3] < $bestSadPP Then
				$bestSadPP = $searchRet[3]
			EndIf
		Next
		;GUICtrlSetData($lblPVPRank, $g_LastSeenRank)
		
		;Local $fDiff = TimerDiff($hTimer)
		;GUICtrlSetData($lblDbgOut, $g_LastSeenRank & '!' & $searchRet[2] & '!' & $searchRet[3] & '!' & $searchRet[4] & '!' & $searchRet[5])
		;GUICtrlSetData($lblDbgOut, $g_LastSeenRank & "ETA " & $fDiff & " ms")
		;GUICtrlSetData($lblDbgOut, $g_LastSeenRank)
		GUICtrlSetData($lblDbgOut, $g_LastSeenRank & " SADPP=" & $acceptableSADPerPixel & " best " & $bestSadPP)
	else 
		GUICtrlSetData($lblDbgOut, $g_VisibleScreenType & " seepvp = " & IsColorAtPos(1707, 277, 0x28283F, 2, 4))
	endif
endfunc

func DropGame()
	MouseClick("left", 1684, 247) ; open settings
	Sleep(500)
	MouseClick("left", 1489, 420) ; surrender button
	Sleep(500)
	MouseClick("left", 1086, 642) ; do you really want to concede ?
	Sleep(5000)
	MouseClick("left", 1129, 235) ; click defeat to go back to PVP screen
	Sleep(3000)	
	global $g_LastSeenRank = 0 ; make sure we refresh the rank before we infinite spam "dropgames"
endfunc

func CheckDropGameASP()
	If ( $FuncIsRunning <> 1 or $BotIsRunning <> 1 ) then 
		return
	endif
	; are we ingame ?
	global $g_LastSeenRank, $g_VisibleScreenType
	if $g_VisibleScreenType == $SCREEN_PVP_FIGHTING and $g_LastSeenRank <> 0 then
			local $GamesSinceLegendary = $GamesDetected - $GameCountAtPrevLegendary
			local $OptimalGameCount = getInputNumericValue($inputGamesForLegChance, 100)
			if $GamesSinceLegendary < $OptimalGameCount Then
				; are we ingame ? check if we are allowed to drop a game
				local $CurrentProgressInSpam = $GamesSinceLegendary / $OptimalGameCount
				GUICtrlSetData($lblDbgOut, $g_LastSeenRank & ' spampct ' & $CurrentProgressInSpam)
				; 0.3 is probably 30->50 games. After that we try to climb the ladder with 30% winrate until best rank
				; ! need to measure how many games we need to climb back to best rank. Probably a lot
				if $g_LastSeenRank < getInputNumericValue($inputMinRankAllowed, 20) and $CurrentProgressInSpam < 0.66 then
					DropGame()
				endif
			endif
	else
		GUICtrlSetData($lblDbgOut, $g_LastSeenRank & ' ! ' & $g_VisibleScreenType)
	endif
endfunc

func CheckDropGameAlwaysToMaintainRank()
	If ( $FuncIsRunning <> 1 or $BotIsRunning <> 1 ) then 
		return
	endif
	; are we ingame ?
	global $g_LastSeenRank, $g_VisibleScreenType
	if $g_LastSeenRank <> 0 then
			local $MaintainSpecificRank = getInputNumericValue($inputFixedRankUsed, 0)
			if $MaintainSpecificRank > 0 and $MaintainSpecificRank > $g_LastSeenRank then
				GUICtrlSetData($lblDbgOut, $g_LastSeenRank & ' < ' & $MaintainSpecificRank)
				if $g_VisibleScreenType == $SCREEN_PVP_FIGHTING then
					DropGame()
				else
					Fight()
				endif
			endif
	endif
endfunc

func ShouldSpamFightsForLegendary()
	If GUICtrlRead($hCheckboxSpamUntilLegChance) = $GUI_CHECKED then
		local $GamesSinceLegendary = $GamesDetected - $GameCountAtPrevLegendary
		local $OptimalGameCount = getInputNumericValue($inputGamesForLegChance, 100)
		if $GamesSinceLegendary < $OptimalGameCount Then
			; if we can start a fight, fight.
			return 1
		endif
	EndIf
	return 0
endfunc

func ForeverLoopFunc()
	$FuncIsRunning = 1 - $FuncIsRunning

	if $FuncIsRunning == 1 then
		GUICtrlSetBkColor($lblTimeDiff, 0x00FF00)
	endif
	
	global $InfiniLoopStampAtStart = _WinAPI_GetTickCount() / 1000
	local $Cycles = 0
	global $g_VisibleScreenType, $FuncIsRunning, $BotIsRunning
	while( $FuncIsRunning == 1 and $BotIsRunning == 1 )	
		;GUICtrlSetData($lblDbgOut, Hex( PixelGetColor(1707, 277), 6 ) & " -- " & $g_VisibleScreenType)

		CountIngameStates()
		CheckDropGameAlwaysToMaintainRank()
		
		; things to do while out of fight screen
		if $g_VisibleScreenType == $SCREEN_START_PVP_FIGHT then
			GuessCurrentPVPRank() ; internally will handle states based on ingame or outofgame screens
			
			; do not open chests until we are in a very low rank ?
			if ShouldSpamFightsForLegendary() == 0 or GUICtrlRead($hCheckboxNoChestsWhileSpamUntilLeg) = $GUI_UNCHECKED then
				OpenWinstreakChests() ; only do these at low rank
				OpenChests()
			endif
			
			; normal fight always in case we have chest slot open
			Fight()
			
			; spam fights while generating statistics about leg distances
			if ShouldSpamFightsForLegendary() == 1 then
				; if we can start a fight, fight.
				Fight2()
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
		endif
		if $g_VisibleScreenType == $SCREEN_PVP_FIGHTING then
			; drop games if too high league
			If GUICtrlRead($hCheckboxSpamUntilLegChance) = $GUI_CHECKED then
				local $GamesSinceLegendary = $GamesDetected - $GameCountAtPrevLegendary
				local $OptimalGameCount = getInputNumericValue($inputGamesForLegChance, 100)
				if $GamesSinceLegendary < $OptimalGameCount Then
					; are we ingame ? check if we are allowed to drop a game
					CheckDropGameASP()
				endif
			EndIf		
		endif
		if $g_VisibleScreenType == $SCREEN_UNKNOWN then
			; done fighting ? pick rewards
			ClickFightWon()
			ClickFightLost()
		endif
		
		; random click that maybe closes the window
		ClearRandomClickContent()

		UpdateUiGameCounter()
		
		$Cycles = $Cycles + 1

		; Small delay to prevent high CPU usage
		If ( $FuncIsRunning == 1 and $BotIsRunning == 1 ) then 
			Sleep(500)
		endif
	wend

	global $InfiniLoopStampAtEnd = _WinAPI_GetTickCount() / 1000
	global $SecondsPlayed = $SecondsPlayed + ($InfiniLoopStampAtEnd-$InfiniLoopStampAtStart)

	GUICtrlSetBkColor($lblTimeDiff, 0xFF0000)
	
endfunc

; Why it actually works : becase short game duration ( relative to long game) skews the probablity curve.
; Counting starts after getting a legendary. Count every game, both wins and losses. If at any point you get a legendary, you can restart the process.
; This process highly depends on 'time loop', number of chest slots, computer speed, fastest way to end a game.
; There is about 25% chance you can get 1 legendary chests in X hours with this method. But high chance to get at least 1 leg
; Method 1:
;	- drop games until you drop to a rank where you can oneshot enemy comander. For me this is League 19 as comanders tend to have 170 health and my ghost does 200 dmg )
;	- win 2 games, drop one game
;	- repeat the process until you done 100 games. If I do this right, I can reach 100 games in 100*42/60=70 minutes
;	- the next 50 games do not drop games. Keep lowering your rank as much as possible. For me, this takes about 50*120/60=100 minutes
;	- if arrived here and still did not get a LEGO, keep playing game only when you can store the reward chest