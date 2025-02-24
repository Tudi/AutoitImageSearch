#include <GUIConstantsEx.au3>
#include <Date.au3>
#include <WinAPISys.au3>
#Include "CommonFunctions.au3"

Opt("MustDeclareVars", 1)
HotKeySet("I", "ExitBot")
HotKeySet("P", "ForeverLoopFunc")

;only used while developing
HotKeySet("Q", "TakeScreenshotCyclePositions")
HotKeySet("W", "TakeScreenshotAtMousePosPreRecorded")

Global $BotIsRunning = 1
Global $FuncIsRunning = 0
Global $g_ClickIntervalUnkScreen = 10000
Global $lastClickTimer = TimerInit()
Global $GamesWon = 0
Global $GamesWonRecordedAt = 0
Global $AcceptedColorTolerance = 3
Global $ColorSearchRadius = 1
Global $pos ; last pixel pos . Cancer code detected
Const $SCREEN_PVP_FIGHTING = 1, $SCREEN_START_PVP_FIGHT = 0, $SCREEN_UNKNOWN = 3
Const $CRATE_BLUE = 1, $CRATE_BLUE2 = 2, $CRATE_PURPLE = 3, $CRATE_GOLD = 4
Const $CrateCountBlue = 56, $CrateCountBlue2 = 10, $CrateCountPurple = 3, $CrateCountGold = 1
Const $gc_CrateCycleLen = 70
Global $g_VisibleScreenType = $SCREEN_UNKNOWN
Global $GamesDetected = 0
Global $GameCountAtPrevLegendary = 0
Global $WinCountAtPrevLegendary = 0
Global $LegendariesSkipped = 0
Global $GamesCounterPrev = -1
Global $g_imageRanks[13][2]
Global $g_imageRewards[6][2]
Global $g_imageBlueCrate[2][2]
Global $g_AppliedColorMaskToCachedImages = 0
Global $g_image1EmeryxCost = "1EmeryxCost_0833_0826_0071_0033.bmp"
;Global $g_WinHistory[8]
Global $g_CrateHistory[800]
Global $g_CrateCycleIndex = -1000 ; index where the next cycle starts in our history array
Global $g_FoundLegThisCycle = 0
Global $g_LastSeenGoldCrateCanBeStored = 1
Global $g_MouseSpeed = 10
Global $g_FightIsForChest = 0
Global $g_LastSeenRank = 0
global $g_StampLastLegObtained = 0

Global $g_LeagueAdvanceInfo[30][2]
$g_LeagueAdvanceInfo[0][0] = 95000 ; points min to reach this rank
$g_LeagueAdvanceInfo[0][1] = 1 ; points gained with a win ( double negative for loss )
$g_LeagueAdvanceInfo[1][0] = 15000 ; points min to reach this rank
$g_LeagueAdvanceInfo[1][1] = 1 ; points gained with a win ( double negative for loss )
$g_LeagueAdvanceInfo[2][0] = 14100 ; points min to reach this rank
$g_LeagueAdvanceInfo[2][1] = 80 ; points gained with a win ( double negative for loss )
$g_LeagueAdvanceInfo[3][0] = 13200 ; points min to reach this rank
$g_LeagueAdvanceInfo[3][1] = 75 ; points gained with a win ( double negative for loss )
$g_LeagueAdvanceInfo[4][0] = 12200 ; points min to reach this rank
$g_LeagueAdvanceInfo[4][1] = 75 ; points gained with a win ( double negative for loss )
$g_LeagueAdvanceInfo[5][0] = 11200 ; points min to reach this rank
$g_LeagueAdvanceInfo[5][1] = 75 ; points gained with a win ( double negative for loss )
$g_LeagueAdvanceInfo[6][0] = 9900 ; points min to reach this rank
$g_LeagueAdvanceInfo[6][1] = 75 ; points gained with a win ( double negative for loss )
$g_LeagueAdvanceInfo[7][0] = 8600 ; points min to reach this rank
$g_LeagueAdvanceInfo[7][1] = 70 ; points gained with a win ( double negative for loss )
$g_LeagueAdvanceInfo[8][0] = 7200 ; points min to reach this rank
$g_LeagueAdvanceInfo[8][1] = 60 ; points gained with a win ( double negative for loss )
$g_LeagueAdvanceInfo[9][0] = 5900 ; points min to reach this rank
$g_LeagueAdvanceInfo[9][1] = 60 ; points gained with a win ( double negative for loss )
$g_LeagueAdvanceInfo[10][0] = 4800 ; points min to reach this rank
$g_LeagueAdvanceInfo[10][1] = 55 ; points gained with a win ( double negative for loss )
$g_LeagueAdvanceInfo[11][0] = 4100 ; points min to reach this rank
$g_LeagueAdvanceInfo[11][1] = 50 ; points gained with a win ( double negative for loss )
$g_LeagueAdvanceInfo[12][0] = 3650 ; points min to reach this rank
$g_LeagueAdvanceInfo[12][1] = 50 ; points gained with a win ( double negative for loss )
$g_LeagueAdvanceInfo[13][0] = 3250 ; points min to reach this rank
$g_LeagueAdvanceInfo[13][1] = 45 ; points gained with a win ( double negative for loss )
$g_LeagueAdvanceInfo[14][0] = 2850 ; points min to reach this rank
$g_LeagueAdvanceInfo[14][1] = 45 ; points gained with a win ( double negative for loss )
$g_LeagueAdvanceInfo[15][0] = 2500 ; points min to reach this rank
$g_LeagueAdvanceInfo[15][1] = 40 ; points gained with a win ( double negative for loss )
$g_LeagueAdvanceInfo[16][0] = 2150 ; points min to reach this rank
$g_LeagueAdvanceInfo[16][1] = 40 ; points gained with a win ( double negative for loss )
$g_LeagueAdvanceInfo[17][0] = 1850 ; points min to reach this rank
$g_LeagueAdvanceInfo[17][1] = 35 ; points gained with a win ( double negative for loss )
$g_LeagueAdvanceInfo[18][0] = 1550 ; points min to reach this rank
$g_LeagueAdvanceInfo[18][1] = 35 ; points gained with a win ( double negative for loss )
$g_LeagueAdvanceInfo[19][0] = 1250 ; points min to reach this rank
$g_LeagueAdvanceInfo[19][1] = 30 ; points gained with a win ( double negative for loss )
	
Func ExitBot()
	Global $BotIsRunning = 0
EndFunc

; Define GUI margins and spacing
Local $margin = 3
Local $spacing = 3

; Create the GUI (initial size will be adjusted later)
Local $hGUI = GUICreate("auto combat", 450, 300)

; Create labels and input controls
Local $labelWidth = 110
Local $inputWidth = 50
Local $ComponentHeight = 15
Local $ComponentsAdded = 0

Global $lblGamesPlayed2 = GUICtrlCreateLabel("", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth + $inputWidth, $ComponentHeight)
GUICtrlSetTip($lblGamesPlayed2, "Game counters since running the script. Ex total, games since last legendary, wins since last legenday, legendaries seen.")
$ComponentsAdded = $ComponentsAdded + 1

Global $lblTempForTooltip = GUICtrlCreateLabel("Spam limit :", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth, $ComponentHeight)
Global $inputSpamLimit = GUICtrlCreateInput("", $margin + $labelWidth + $spacing, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $inputWidth, $ComponentHeight)
GUICtrlSetTip($lblTempForTooltip, "Spam games until we reach this game counter. Used for Heliquests.")
GUICtrlSetTip($inputSpamLimit, "Spam games until we reach this game counter. Used for Heliquests.")
$ComponentsAdded = $ComponentsAdded + 1

Global $btnSpam0 = GUICtrlCreateButton("Stop Spam", $margin, $margin + $ComponentsAdded * ($margin + $ComponentHeight), $labelWidth + $inputWidth + $spacing, $ComponentHeight)
GUICtrlSetTip($btnSpam0, "Spam games for Heli quests.")
$ComponentsAdded = $ComponentsAdded + 1

Global $btnSpam5 = GUICtrlCreateButton("Spam 5", $margin, $margin + $ComponentsAdded * ($margin + $ComponentHeight), $labelWidth + $inputWidth + $spacing, $ComponentHeight)
GUICtrlSetTip($btnSpam5, "Spam games for Heli quests.")
$ComponentsAdded = $ComponentsAdded + 1

Global $btnSpam10 = GUICtrlCreateButton("Spam 10", $margin, $margin + $ComponentsAdded * ($margin + $ComponentHeight), $labelWidth + $inputWidth + $spacing, $ComponentHeight)
GUICtrlSetTip($btnSpam10, "Spam games for Heli quests.")
$ComponentsAdded = $ComponentsAdded + 1

Global $btnSpam15 = GUICtrlCreateButton("Spam 15", $margin, $margin + $ComponentsAdded * ($margin + $ComponentHeight), $labelWidth + $inputWidth + $spacing, $ComponentHeight)
GUICtrlSetTip($btnSpam15, "Spam games for Heli quests.")
$ComponentsAdded = $ComponentsAdded + 1

Global $btnSpam20 = GUICtrlCreateButton("Spam 20", $margin, $margin + $ComponentsAdded * ($margin + $ComponentHeight), $labelWidth + $inputWidth + $spacing, $ComponentHeight)
GUICtrlSetTip($btnSpam20, "Spam games for Heli quests.")
$ComponentsAdded = $ComponentsAdded + 1

Global $btnSpam25 = GUICtrlCreateButton("Spam 25", $margin, $margin + $ComponentsAdded * ($margin + $ComponentHeight), $labelWidth + $inputWidth + $spacing, $ComponentHeight)
GUICtrlSetTip($btnSpam25, "Spam games for Heli quests.")
$ComponentsAdded = $ComponentsAdded + 1

Global $btnSpam30 = GUICtrlCreateButton("Spam 30", $margin, $margin + $ComponentsAdded * ($margin + $ComponentHeight), $labelWidth + $inputWidth + $spacing, $ComponentHeight)
GUICtrlSetTip($btnSpam30, "Spam games for Heli quests.")
$ComponentsAdded = $ComponentsAdded + 1

Global $lblTempForTooltip = GUICtrlCreateLabel("1Emeryx :", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth, $ComponentHeight)
Global $inputOpen1emeryxChests = GUICtrlCreateInput("0", $margin + $labelWidth + $spacing, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $inputWidth, $ComponentHeight)
GUICtrlSetTip($lblTempForTooltip, "In case a chest speedup costs 1 Emeryx, open it. Used for quests that require chest speedups. Value is for number of chests to open")
GUICtrlSetTip($inputOpen1emeryxChests, "In case a chest speedup costs 1 Emeryx, open it. Used for quests that require chest speedups. Value is for number of chests to open")
$ComponentsAdded = $ComponentsAdded + 1

Global $lblCrateCycleCrates = GUICtrlCreateLabel("cycle crates", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth + $inputWidth, $ComponentHeight)
GUICtrlSetTip($lblCrateCycleCrates, "Remaining crates this cycle and your crate cycle index")
$ComponentsAdded = $ComponentsAdded + 1

Global $lblTempForTooltip = GUICtrlCreateLabel("OpenLegIfLowerLegue :", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth, $ComponentHeight)
Global $inputOpenLegendaryIfLower = GUICtrlCreateInput("10", $margin + $labelWidth + $spacing, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $inputWidth, $ComponentHeight)
GUICtrlSetTip($lblTempForTooltip, "If a legendary can't be stored, should we open it. Probably happens while spamming games")
GUICtrlSetTip($inputOpenLegendaryIfLower, "If a legendary can't be stored, should we open it. Probably happens while spamming games")
;Global $hCheckboxOpenLegendary = GUICtrlCreateCheckbox("OpenLegIfLowerLegue", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth + $inputWidth, $ComponentHeight)
;GUICtrlSetTip($hCheckboxOpenLegendary, "If a legendary can't be stored, should we open it. Probably happens while spamming games")
$ComponentsAdded = $ComponentsAdded + 1

Global $hCheckboxPauseLegendary = GUICtrlCreateCheckbox("PauseOnLeg", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth + $inputWidth, $ComponentHeight)
GUICtrlSetTip($hCheckboxPauseLegendary, "If a legendary can't be stored, should we pause game? Probably happens while quests that require to open X legendaries. Kinda unused feature")
$ComponentsAdded = $ComponentsAdded + 1

Global $hCheckboxSkipLegendary = GUICtrlCreateCheckbox("SkipLeg", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth + $inputWidth, $ComponentHeight)
GUICtrlSetTip($hCheckboxSkipLegendary, "If a legendary can't be stored, should we skip it ? Resets CCI lego spam hunt")
GUICtrlSetState($hCheckboxSkipLegendary, $GUI_CHECKED)
$ComponentsAdded = $ComponentsAdded + 1

Global $lblTempForTooltip = GUICtrlCreateLabel("MaxRankToDrop:", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth, $ComponentHeight)
Global $inputMinRankAllowed = GUICtrlCreateInput("19", $margin + $labelWidth + $spacing, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $inputWidth, $ComponentHeight)
GUICtrlSetTip($inputMinRankAllowed, "For fast game spam, we will drop games until we reach this rank(worst case). Helps us do games faster")
GUICtrlSetTip($lblTempForTooltip, "For fast game spam, we will drop games until we reach this rank(worst case). Helps us do games faster")
$ComponentsAdded = $ComponentsAdded + 1

Global $lblTempForTooltip = GUICtrlCreateLabel("ClimbToRank :", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth, $ComponentHeight)
Global $inputTargetRankForCCI = GUICtrlCreateInput("9", $margin + $labelWidth + $spacing, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $inputWidth, $ComponentHeight)
GUICtrlSetTip($lblTempForTooltip, "Climb ranks and try to reach this rank before we pause spamming games. Assumes we do not loose any games")
GUICtrlSetTip($inputTargetRankForCCI, "Climb ranks and try to reach this rank before we pause spamming games. Assumes we do not loose any games")
$ComponentsAdded = $ComponentsAdded + 1

Global $lblTempForTooltip = GUICtrlCreateLabel("ExtraWins :", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth, $ComponentHeight)
Global $inputWinOffsetForTarget = GUICtrlCreateInput("10", $margin + $labelWidth + $spacing, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $inputWidth, $ComponentHeight)
GUICtrlSetTip($lblTempForTooltip, "Climbing the ranks to target rank does not mean we will always win. This value might help with win/loss ratio guessing")
GUICtrlSetTip($inputWinOffsetForTarget, "Climbing the ranks to target rank does not mean we will always win. This value might help with win/loss ratio guessing")
$ComponentsAdded = $ComponentsAdded + 1

Global $lblTempForTooltip = GUICtrlCreateLabel("SpamWins4Leg :", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth, $ComponentHeight)
Global $inputSpamWinsForLegChance = GUICtrlCreateInput("1", $margin + $labelWidth + $spacing, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $inputWidth, $ComponentHeight)
GUICtrlSetTip($lblTempForTooltip, "Number of wins to obtain once we seen a LEGO. Used to initialize CCI hystory data")
GUICtrlSetTip($inputSpamWinsForLegChance, "Number of wins to obtain once we seen a LEGO. Used to initialize CCI hystory data")
$ComponentsAdded = $ComponentsAdded + 1

Global $hCheckboxSpamUntilLegChance = GUICtrlCreateCheckbox("Spam4Leg", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth + $inputWidth, $ComponentHeight)
GUICtrlSetTip($hCheckboxSpamUntilLegChance, "If current CCI is smaller than target CCI, spam games")
$ComponentsAdded = $ComponentsAdded + 1
GUICtrlSetState($hCheckboxSpamUntilLegChance, $GUI_CHECKED)

Global $lblTempForTooltip = GUICtrlCreateLabel("SpamUntilCCI :", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth, $ComponentHeight)
Global $inputSpamCombatUntilCCINoLeg = GUICtrlCreateInput("55", $margin + $labelWidth + $spacing, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $inputWidth, $ComponentHeight)
GUICtrlSetTip($lblTempForTooltip, "Spam games until we reach this CCI and we still did not open a LEGO")
GUICtrlSetTip($inputSpamCombatUntilCCINoLeg, "Spam games until we reach this CCI and we still did not open a LEGO")
$ComponentsAdded = $ComponentsAdded + 1

;avoid accidentally storing low level legendary. Has a 9 hour chest slot cooldown
Global $hCheckboxNoChestsWhileSpamUntilLeg = GUICtrlCreateCheckbox("NoChestWhileSpam4Leg", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth + $inputWidth, $ComponentHeight)
;GUICtrlSetState($hCheckboxNoChestsWhileSpamUntilLeg, $GUI_CHECKED)
GUICtrlSetTip($hCheckboxNoChestsWhileSpamUntilLeg, "Do not open chests while spamming for CCI target. Helps push CCI further without loosing a leg")
$ComponentsAdded = $ComponentsAdded + 1

Global $hCheckboxNoChestOpens = GUICtrlCreateCheckbox("NoChestOpens", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth + $inputWidth, $ComponentHeight)
GUICtrlSetTip($hCheckboxNoChestOpens, "Do not open chests. Ever. Used while waiting for 3 girl event")
$ComponentsAdded = $ComponentsAdded + 1

Global $lblTempForTooltip = GUICtrlCreateLabel("Seconds/Game :", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth, $ComponentHeight)
Global $inputSingleGameDurationSec = GUICtrlCreateInput("120", $margin + $labelWidth + $spacing, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $inputWidth, $ComponentHeight)
GUICtrlSetTip($lblTempForTooltip, "How many minutes does a game take ? Highly depends on your rank, team. Used to estimate time remaining until CCI target")
GUICtrlSetTip($inputSingleGameDurationSec, "How many minutes does a game take ? Highly depends on your rank, team. Used to estimate time remaining until CCI target")
$ComponentsAdded = $ComponentsAdded + 1

Global $lblTempForTooltip = GUICtrlCreateLabel("NoChestIfRemSec :", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth, $ComponentHeight)
Global $inputPauseOpensIfSpamSecRemains = GUICtrlCreateInput("3600", $margin + $labelWidth + $spacing, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $inputWidth, $ComponentHeight)
GUICtrlSetTip($lblTempForTooltip, "Pause opening chests if we are close to our CCI target")
GUICtrlSetTip($inputPauseOpensIfSpamSecRemains, "Pause opening chests if we are close to our CCI target")
$ComponentsAdded = $ComponentsAdded + 1

Local $lblDbgOut = GUICtrlCreateLabel("Debug", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth + $inputWidth, $ComponentHeight)
$ComponentsAdded = $ComponentsAdded + 1
Local $lblDbgOut2 = GUICtrlCreateLabel("Debug 2", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth + $inputWidth, $ComponentHeight)
$ComponentsAdded = $ComponentsAdded + 1
Local $lblDbgOut3 = GUICtrlCreateLabel("MinutesUntilSpamCCITareget", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth + $inputWidth, $ComponentHeight)
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

ResetDebugLog()
;_WriteDebugLog(GetWinsRequiredAdvanceLeagues(16,12)) ; 35 steps to get from a spammy situation to a decent reward rank

; load saved variables
LoadVariables()
GuessIngameVisibleScreen()
UpdateUiGameCounter(True)
LoadCrateHistoryFromFile("GameRewards.txt")
CalculateCrateCycleIdex()
UpdateUICrateCycleIndex()
InitImageSearchDLLImages()

; Event Loop
While ( $BotIsRunning == 1)
	HandleGUIMessages()
WEnd

; sadly this would need to be run in a separate thread than ForeverLoopFunc
Func HandleGUIMessages()
    Local $msg = GUIGetMsg()

    If $msg = $GUI_EVENT_CLOSE Then 
		$BotIsRunning = 0
		$FuncIsRunning = 0
		Exit
	EndIf
	
    ; Check if the "Spam 15" button was clicked
    If $msg = $btnSpam0 Then
        SpamButtonHandler(-1)
    ElseIf $msg = $btnSpam5 Then
        SpamButtonHandler(5)
    ElseIf $msg = $btnSpam10 Then
        SpamButtonHandler(10)
    ElseIf $msg = $btnSpam15 Then
        SpamButtonHandler(15)
    ElseIf $msg = $btnSpam20 Then
        SpamButtonHandler(20)
    ElseIf $msg = $btnSpam25 Then
        SpamButtonHandler(25)
    ElseIf $msg = $btnSpam30 Then
        SpamButtonHandler(30)
    EndIf	
EndFunc

exit

Func SpamButtonHandler($SpamXGames)
	GUICtrlSetData($inputSpamLimit, $GamesDetected + $SpamXGames)
EndFunc

; save/load saved variables 
Func SaveVariables()
    IniWrite("variables.ini", "Variables", "GamesDetected", $GamesDetected)
    IniWrite("variables.ini", "Variables", "GameCountAtPrevLegendary", $GameCountAtPrevLegendary)
    IniWrite("variables.ini", "Variables", "LegendariesSkipped", $LegendariesSkipped)
    IniWrite("variables.ini", "Variables", "GamesWon", $GamesWon)
    IniWrite("variables.ini", "Variables", "WinCountAtPrevLegendary", $WinCountAtPrevLegendary)
EndFunc

Func LoadVariables()
    Global $GamesDetected = IniRead("variables.ini", "Variables", "GamesDetected", 0)
    Global $GameCountAtPrevLegendary = IniRead("variables.ini", "Variables", "GameCountAtPrevLegendary", 0)
    Global $LegendariesSkipped = IniRead("variables.ini", "Variables", "LegendariesSkipped", 0)
    Global $GamesWon = IniRead("variables.ini", "Variables", "GamesWon", 0)
    Global $WinCountAtPrevLegendary = IniRead("variables.ini", "Variables", "WinCountAtPrevLegendary", 0)
EndFunc

Func MyMouseClick($button, $x, $y, $speed = $g_MouseSpeed )
	If WinActive("[CLASS:MozillaWindowClass]") = 0 Then
		WinActivate("[CLASS:MozillaWindowClass]")
		Sleep( 250 ) ; maybe it takes some time for the window to activate
	EndIf
	If $speed <> 10 then 
		MouseClick($button, $x, $y, $speed)
	Else
		MouseClick($button, $x, $y) ; maybe default speed is not 10 ? This insignificant line does make a difference for me
	Endif
	$lastClickTimer = TimerInit()	; else it might close chest open window or similar. This timer is for the random content clear click
EndFunc

Func LoadCrateHistoryFromFile($filePath)
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
	$fileContent = StringReplace($fileContent, ",,", ",")
	$fileContent = StringReplace($fileContent, ",,", ",")

    ; Split the content by comma
    Local $values = StringSplit($fileContent, ",", 2)
    If @error Then
        Return -1
    EndIf

	Local $HistoryStartIndex = 0
	if UBound($values) > UBound($g_CrateHistory) then
		$HistoryStartIndex = UBound($values) - UBound($g_CrateHistory)
	EndIf
	
    For $i = $HistoryStartIndex To UBound($values) - 1
        Local $num = Number($values[$i])
        If $values[$i] <> "" And IsNumber($num) Then
			CycleHistoryArray($g_CrateHistory, $num)
        EndIf
    Next
EndFunc

Func IsGoodIndexForCycleStart($CycleStart, $CycleEnd, ByRef $out_Deviation, $AcceptableHistoryErrorCount = 2)
	if $CycleStart < 0 or $CycleEnd >= UBound($g_CrateHistory) then 
		$out_Deviation = $CycleEnd - $CycleStart
		return 0
	endif
	Local $CycleFoundCrateCounts[5]
	$CycleFoundCrateCounts[$CRATE_BLUE] = 0
	$CycleFoundCrateCounts[$CRATE_BLUE2] = 0
	$CycleFoundCrateCounts[$CRATE_PURPLE] = 0
	$CycleFoundCrateCounts[$CRATE_GOLD] = 0
	for $CrateIndex = $CycleStart to $CycleEnd - 1
		Local $CrateTypeAtHisIndex = $g_CrateHistory[$CrateIndex]
		; missing or bad data in history
		if $CrateTypeAtHisIndex == 0 then
			return 0
		EndIf
		if $CrateTypeAtHisIndex <= $CRATE_GOLD then
			$CycleFoundCrateCounts[$CrateTypeAtHisIndex] = $CycleFoundCrateCounts[$CrateTypeAtHisIndex] + 1
		EndIf
	Next
	Local $CurrDeviation = 0
	$CurrDeviation = $CurrDeviation + abs($CrateCountBlue - $CycleFoundCrateCounts[$CRATE_BLUE])
	$CurrDeviation = $CurrDeviation + abs($CrateCountBlue2 - $CycleFoundCrateCounts[$CRATE_BLUE2])
	$CurrDeviation = $CurrDeviation + abs($CrateCountPurple - $CycleFoundCrateCounts[$CRATE_PURPLE])
	$out_Deviation = $CurrDeviation
	if $CycleFoundCrateCounts[$CRATE_GOLD] <> 1 then 
		$out_Deviation = $CycleEnd - $CycleStart
	EndIf
	if $CurrDeviation <= $AcceptableHistoryErrorCount and $CycleFoundCrateCounts[$CRATE_GOLD] == 1 then
		return 1
	EndIf
	return 0
EndFunc

; you should have multiple 70 times crates in crate history for proper detection
; this function calculates your current crate index 
; updates UI if you already got a LEG this cycle and your index. If you already got a crate this cycle, spam until next cycle
Func CalculateCrateCycleIdex()
	Local $AcceptableHistoryErrorCount = 3
	GUICtrlSetData($lblCrateCycleCrates, "Recalculating based on history data")
	; visually debug crate hystory content. Seems like after 255 values are missing
	;for $CycleStart = 0 to UBound($g_CrateHistory) - $gc_CrateCycleLen - 1
	;	_WriteDebugLog($g_CrateHistory[$CycleStart], 0)
	;Next
	;_WriteDebugLog(@CRLF,0)
	Local $BestCycleDeviation = 10000
	Local $BestCycleDeviationCycleStart = 0
	for $CycleStart = 0 to $gc_CrateCycleLen - 1
		_WriteDebugLog($CycleStart, 0)
		; this index is good to be considered a good cycle start index, but is it good for the previous cycles ?
		Local $CyclesTested = 0
		Local $CyclesGood = 0
		Local $NextCycleStart = $CycleStart
		Local $SumCycleDeviations = 0
		while $NextCycleStart < UBound($g_CrateHistory) - $gc_CrateCycleLen - 1
			Local $BestCycleMatchDeviation = $gc_CrateCycleLen
			Local $CorrectedNextCycleStart = $NextCycleStart
			; there are times when a reward crate does not get recorded, or badly recorded
			; do a best of X searches and pick the best one
			if $NextCycleStart == $CycleStart then
				Local $isGood = IsGoodIndexForCycleStart($NextCycleStart, $NextCycleStart + $gc_CrateCycleLen, $BestCycleMatchDeviation, $AcceptableHistoryErrorCount)
			Else
				for $MiniCycleStart = $NextCycleStart - $AcceptableHistoryErrorCount to $NextCycleStart + $AcceptableHistoryErrorCount
					Local $CycleMatchDeviation = 0
					Local $isGood = IsGoodIndexForCycleStart($MiniCycleStart, $MiniCycleStart + $gc_CrateCycleLen, $CycleMatchDeviation, $AcceptableHistoryErrorCount)
					if $CycleMatchDeviation < $BestCycleMatchDeviation then 
						$CorrectedNextCycleStart = $MiniCycleStart
						$BestCycleMatchDeviation = $CycleMatchDeviation
					EndIf
				Next
			Endif
			IF $BestCycleMatchDeviation <= $AcceptableHistoryErrorCount then
				$NextCycleStart = $CorrectedNextCycleStart
				$CyclesGood = $CyclesGood + 1
				_WriteDebugLog($NextCycleStart & '=1' & '(' & $BestCycleMatchDeviation & ')', 0)
			Else
				_WriteDebugLog($NextCycleStart & '=0' & '(' & $BestCycleMatchDeviation & ')', 0)
				; if the first cycle can't be considered good, no need to search for additional ones
				if $NextCycleStart < $gc_CrateCycleLen and $BestCycleMatchDeviation >= $AcceptableHistoryErrorCount then
					ExitLoop
				endif
			endif
			$SumCycleDeviations = $SumCycleDeviations + $BestCycleMatchDeviation
			$CyclesTested = $CyclesTested + 1
			$NextCycleStart = $NextCycleStart + $gc_CrateCycleLen
		WEnd
		if $SumCycleDeviations < $BestCycleDeviation and $CyclesGood > 3 then
			_WriteDebugLog("Improved crate index " & $CycleStart & " from " & $BestCycleDeviation & " to " & $SumCycleDeviations & @CRLF,0)
			$BestCycleDeviation = $SumCycleDeviations
			$BestCycleDeviationCycleStart = $CycleStart
		EndIf
		; can't get better than this
		if $BestCycleDeviation == 0 then
			ExitLoop
		EndIf
		_WriteDebugLog(@CRLF,0)
	Next
	if $BestCycleDeviation < 10000 then
		_WriteDebugLog("Found good crate index " & $BestCycleDeviationCycleStart & @CRLF, 0)
		$g_CrateCycleIndex = $BestCycleDeviationCycleStart
	EndIf
EndFunc

Func getInputNumericValue($InputName, $DefaultValue = 999999)
    Local $InputVal = GUICtrlRead($InputName)
    If Not StringIsFloat($InputVal) And Not StringIsInt($InputVal) Or Number($InputVal) <= 0 Then
        return $DefaultValue
    EndIf    
    return $InputVal
EndFunc

Func OpenChests()
	If ( $FuncIsRunning <> 1 or $BotIsRunning <> 1 ) then 
		return
	EndIf
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
			MyMouseClick("left", $pos1[0], $pos1[1])
			Sleep(3000)
			MyMouseClick("left", $pos1[0], $pos1[1])
			Sleep(1000)
			return
		EndIf
	Next
EndFunc

Func OpenWinstreakChests()
	If ( $FuncIsRunning <> 1 or $BotIsRunning <> 1 ) then 
		return
	EndIf
    ; If the color is found, click on it
    If IsColorAtPos(1585,653,0xFFD802,$ColorSearchRadius,$AcceptedColorTolerance) Then
        MyMouseClick("left", 1542, 525)
		Sleep(3000)
        MyMouseClick("left", 1703, 899)
		Sleep(1000)
    EndIf
EndFunc

Func Fight()
	If ( $FuncIsRunning <> 1 or $BotIsRunning <> 1 ) then 
		return
	EndIf
    If IsColorAtPos(1088, 535, 0xFF4444, $ColorSearchRadius, $AcceptedColorTolerance) Then
		$g_FightIsForChest = 1
        MyMouseClick("left", $pos[0], $pos[1])
		Sleep(2000)
    EndIf
EndFunc

Func Fight2()
	If ( $FuncIsRunning <> 1 or $BotIsRunning <> 1 ) then 
		return
	EndIf
    If IsColorAtPos(1092,536,0xBF2D03,$ColorSearchRadius,$AcceptedColorTolerance) Then
        MyMouseClick("left", $pos[0], $pos[1])
		Sleep(2000)
    EndIf
EndFunc

Func AddLegendaryGameFreqStat()
	$GameCountAtPrevLegendary = $GamesDetected
	$WinCountAtPrevLegendary = $GamesWon
		
	; just so that I don't freak out that the counter is not working
	UpdateUiGameCounter(True)
EndFunc

Func HandleLegendaryWinReward()
	If ( $FuncIsRunning <> 1 or $BotIsRunning <> 1 ) then 
		return
	EndIf
	; is this a legendary reward ?
    If IsColorAtPos(854,575,0xC56001,$ColorSearchRadius,$AcceptedColorTolerance) Then
		AddLegendaryGameFreqStat()
		
		If $g_LastSeenGoldCrateCanBeStored == 1 then
			MyMouseClick("left", 1634, 203) ; click somewhere to simply close the victory screen
			Sleep(3000)
			return 1
		EndIf
		
		If GUICtrlRead($hCheckboxPauseLegendary) = $GUI_CHECKED Then
			$FuncIsRunning = 0
			return 1
		EndIf
				
		Local $OpenLegChestIfLowLeague = getInputNumericValue($inputOpenLegendaryIfLower, 0)
;		If GUICtrlRead($hCheckboxOpenLegendary) = $GUI_CHECKED Then
		If $OpenLegChestIfLowLeague <> 0 and $OpenLegChestIfLowLeague > $g_LastSeenRank and $g_LastSeenRank <> 0 Then
			MyMouseClick("left", 929, 768)
			Sleep(2000)
			MyMouseClick("left", 929, 768);
			Sleep(1000)
			MyMouseClick("left", 1634, 203)
			Sleep(1000)
			return 1
		EndIf
		
		If GUICtrlRead($hCheckboxSkipLegendary) = $GUI_CHECKED Then
			MyMouseClick("left", 1128, 224)
			Sleep(3000)
			MyMouseClick("left", 1079, 570)
			Sleep(3000)
			$LegendariesSkipped = $LegendariesSkipped + 1
			return 1
		EndIf	
	EndIf
	
	return 0
EndFunc

Func UpdateUICrateCycleIndex()
	; count which crate appeared how many times this cycle
	Local $CycleFoundCrateCounts[5]
	$CycleFoundCrateCounts[$CRATE_BLUE] = 0
	$CycleFoundCrateCounts[$CRATE_BLUE2] = 0
	$CycleFoundCrateCounts[$CRATE_PURPLE] = 0
	$CycleFoundCrateCounts[$CRATE_GOLD] = 0
	for $CrateIndex = 0 to $g_CrateCycleIndex - 1
		Local $CrateAtThisIndex = $g_CrateHistory[$CrateIndex]
		if $CrateAtThisIndex <= $CRATE_GOLD then
			$CycleFoundCrateCounts[$CrateAtThisIndex] = $CycleFoundCrateCounts[$CrateAtThisIndex] + 1
		EndIf
	Next
	; generate UI info based on this cycle we are in
	Local $lblTXTCrates = ""
	for $CrateTypeIndex = 1 to UBound($CycleFoundCrateCounts) - 1
		Local $CratesToBeFoundThistype = 1
		if $CrateTypeIndex == $CRATE_BLUE then
			$CratesToBeFoundThistype = $CrateCountBlue - $CycleFoundCrateCounts[$CrateTypeIndex]
			$lblTXTCrates = $lblTXTCrates & "B=" & $CratesToBeFoundThistype & ' '
		ElseIf $CrateTypeIndex == $CRATE_BLUE2 then
			$CratesToBeFoundThistype = $CrateCountBlue2 - $CycleFoundCrateCounts[$CrateTypeIndex]
			$lblTXTCrates = $lblTXTCrates & "B2=" & $CratesToBeFoundThistype & ' '
		ElseIf $CrateTypeIndex == $CRATE_PURPLE then
			$CratesToBeFoundThistype = $CrateCountPurple - $CycleFoundCrateCounts[$CrateTypeIndex]
			$lblTXTCrates = $lblTXTCrates & "P=" & $CratesToBeFoundThistype & ' '
		ElseIf $CrateTypeIndex == $CRATE_GOLD then
			$CratesToBeFoundThistype = $CrateCountGold - $CycleFoundCrateCounts[$CrateTypeIndex]
			$g_FoundLegThisCycle = $CycleFoundCrateCounts[$CrateTypeIndex]
			$lblTXTCrates = $lblTXTCrates & "L=" & $CratesToBeFoundThistype & ' '
		Endif
	Next	
	$lblTXTCrates = $lblTXTCrates & "CCI " & $g_CrateCycleIndex	
	GUICtrlSetData($lblCrateCycleCrates, $lblTXTCrates)	
EndFunc

Func GetStringWithDayOfWeek($prefix = "legcci", $suffix=".txt")
    ; Array of weekday names
    Local $aDays[7] = ["Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"]
    
    ; Get the current day of the week (0 = Sunday, 6 = Saturday)
    Local $iDayOfWeek = @WDAY - 1

    ; Construct the string
    Local $sResult = $prefix & "_" & $aDays[$iDayOfWeek] & $suffix
    
    Return $sResult
EndFunc

Func RecordVictoryRewards()
	Local $acceptableSADPerPixel = 4
	Local $bestSadPP = 100000
	Local $bestRewardType = -1
	Local $bestRewardImageIndex = 0
	For $i = 0 To UBound($g_imageRewards) - 1
		Local $searchRet = ImageIsAtRadius($g_imageRewards[$i][0])
		If $searchRet[0] > 0 and $searchRet[3] < $acceptableSADPerPixel and $searchRet[3] < $bestSadPP Then
			$bestSadPP = $searchRet[3]
			$bestRewardType = $g_imageRewards[$i][1]
			$bestRewardImageIndex = $i
			if $bestSadPP == 0 then
				ExitLoop 
			EndIf
		EndIf
	Next
	
	; there is small blue crate and large blue crate
	if $bestRewardType == $CRATE_BLUE Then
		; click the crate to show details
        MyMouseClick("left", 846 + 96/2, 576 + 96/2)
		Sleep(1000) ; wait for crate details animation. This seems to bug out from time to time and not open crate details ?
		Local $acceptableSADPerPixel = 8
		Local $bestRewardType2 = -1
		Local $bestSadPP = 100000
		Local $bestSadPPDebug = 100000
		For $i=0 To 1
			For $i = 0 To UBound($g_imageBlueCrate) - 1
				Local $searchRet = ImageIsAtRadius($g_imageBlueCrate[$i][0])
				If $searchRet[0] > 0 and $searchRet[3] < $acceptableSADPerPixel and $searchRet[3] < $bestSadPP Then
					$bestSadPP = $searchRet[3]
					$bestRewardType2 = $g_imageBlueCrate[$i][1]
					if $bestSadPP == 0 then
						ExitLoop 
					EndIf
				EndIf
				If $searchRet[0] > 0 and $searchRet[3] < $bestSadPPDebug Then
					$bestSadPPDebug = $searchRet[3]
				EndIf
			Next
			if $bestRewardType2 <> -1 then
				ExitLoop
			EndIf
			Sleep(1000) ; sometimes there seems to be latency here and chest type is a black image ?
		Next
		if $bestRewardType2 == -1 then
			GUICtrlSetData($lblDbgOut2, "blue crate bspp: " & $bestSadPPDebug)
			Sleep(1000)
			TakeScreenshotRegionAndSaveit(478,265,478+263,265+33)
		else
			$bestRewardType = $bestRewardType2
		EndIf
		; close crate screen
		MyMouseClick("left", 1634, 203)
		Sleep(1000)		
	EndIf
	
	; do we want to open an unstorable gold crate ? These test will be made later on
	$g_LastSeenGoldCrateCanBeStored = -1
	if $bestRewardType == $CRATE_GOLD then
		if $bestRewardImageIndex == 4 then
			$g_LastSeenGoldCrateCanBeStored = 1
		ElseIf $bestRewardImageIndex == 5 then
			$g_LastSeenGoldCrateCanBeStored = 0
		EndIf
	EndIf

	; victory screen on 3 girl event for example will not show a crate and reward type will remain -1
	if $bestRewardType <> -1 then
		GUICtrlSetData($lblDbgOut2, "last crate type: " & $bestRewardType)
		CycleHistoryArray($g_CrateHistory, $bestRewardType) ; store even bad rewards
		$g_CrateCycleIndex =  $g_CrateCycleIndex + 1
		if $g_CrateCycleIndex >= $gc_CrateCycleLen then 
			$g_CrateCycleIndex = 0
		EndIf
	EndIf
	
	; this seems to desync. Not exactly sure why. Probably not counting g_CrateCycleIndex correctly ?
	if $bestRewardType == $CRATE_GOLD or $g_CrateCycleIndex == 0 then
		CalculateCrateCycleIdex()
	EndIf
	
	; have a feeling some days are better than others
	if $bestRewardType == $CRATE_GOLD then
		Local $hFile = FileOpen("legCCIs.txt", 1) ; Mode 1 = Append
		If $hFile <> -1 Then
			FileWrite($hFile, $g_CrateCycleIndex & ",")
			FileClose($hFile)		
		EndIf
		Local $MinuesSpentSinceLastLeg = int((_WinAPI_GetTickCount() - $g_StampLastLegObtained)/1000/60)
		if $g_StampLastLegObtained == 0 then
			$MinuesSpentSinceLastLeg = 0
		EndIf
		$g_StampLastLegObtained = _WinAPI_GetTickCount() / 1000
		Local $hFile = FileOpen("legCCIs2.txt", 1) ; Mode 1 = Append
		If $hFile <> -1 Then
			FileWrite($hFile, (@WDAY - 1) & "," & @HOUR & ":" & @MIN & ",CCI=" & $g_CrateCycleIndex & ",Minutes=" & $MinuesSpentSinceLastLeg & ",CanStore=" & $g_LastSeenGoldCrateCanBeStored & ",League=" & $g_LastSeenRank  & "," & @CRLF)
			FileClose($hFile)		
		EndIf
		;Local $hFile = FileOpen(GetStringWithDayOfWeek(), 1) ; Mode 1 = Append
		;If $hFile <> -1 Then
		;	FileWrite($hFile, $g_CrateCycleIndex & ",")
		;	FileClose($hFile)		
		;EndIf	
	Endif	
	UpdateUICrateCycleIndex()
	
	; write it to a file
	if $bestRewardType <> -1 then
		Local $hFile = FileOpen("GameRewards.txt", 1) ; Mode 1 = Append
		If $hFile <> -1 Then
			FileWrite($hFile, $bestRewardType & "," & @CRLF)
			FileClose($hFile)		
		EndIf
	else
		Sleep(1000)
		TakeScreenshotRegionAndSaveit(846,576,846+96,576+96)
	EndIf
EndFunc

Func ClickFightWon()	
	If ( $FuncIsRunning <> 1 or $BotIsRunning <> 1 ) then 
		return
	EndIf
    If IsColorAtPos(1129,235,0xFFEBA2,$ColorSearchRadius,$AcceptedColorTolerance) Then
		$g_FightIsForChest = 0 ; pause spam drop games if we can store a new chest
		Sleep(1000) ; seems like the chest appears slightly later than the victory letters
		
		; should not happen. Somehow we are recording the win condition more than once ?
		; we can still see the victory screen even though we already tried to click on the 'y' once
		if $GamesWonRecordedAt == $GamesDetected then
			MyMouseClick("left", 1634, 203)
			$g_LastSeenRank = 0 ; make sure we refresh the rank before we infinite spam "dropgames"
			return
		EndIf
		
		$GamesWon = $GamesWon + 1
		$GamesWonRecordedAt = $GamesDetected

		RecordVictoryRewards()

		; no need to click on yellow letter if we opened the check ?
		if HandleLegendaryWinReward() == 1 then
			$g_LastSeenRank = 0 ; make sure we refresh the rank before we infinite spam "dropgames"
			return
		EndIf
		
		; click the 'y' to make the screen go away
        MyMouseClick("left", $pos[0], $pos[1])
		Sleep(3000)
		$g_LastSeenRank = 0 ; make sure we refresh the rank before we infinite spam "dropgames"
    EndIf
EndFunc

Func ClickFightLost()
	If ( $FuncIsRunning <> 1 or $BotIsRunning <> 1 ) then 
		return
	EndIf
    If IsColorAtPos(1106,192,0xFF4342,$ColorSearchRadius,$AcceptedColorTolerance) Then
		$g_LastSeenRank = 0 ; make sure we refresh the rank before we infinite spam "dropgames"
		$g_FightIsForChest = 0 ; pause spam drop games if we can store a new chest
        MyMouseClick("left", $pos[0], $pos[1])
		Sleep(3000)
    EndIf
EndFunc

Func ClearRandomClickContent()
	If ( $FuncIsRunning <> 1 or $BotIsRunning <> 1 ) then 
		return
	EndIf
	; we can recognize this screen. Only click on unrecognized screens
	if $g_VisibleScreenType <> $SCREEN_UNKNOWN then
		$lastClickTimer = TimerInit()
		return
	endif	
    ; Check if the click interval has passed since the last click
    If TimerDiff($lastClickTimer) >= $g_ClickIntervalUnkScreen Then
		MyMouseClick("left", 1634, 203)
        $lastClickTimer = TimerInit()
    EndIf	
EndFunc

Func GuessIngameVisibleScreen()
	Global $g_VisibleScreenType, $GamesDetected 
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
	EndIf
	; some loading screen or something
	Local $ConfirmedIngame = 0
	Local $ConfirmedOutGame = 0
	if $g_VisibleScreenType == $SCREEN_START_PVP_FIGHT and IsColorAtPos(1707, 277, 0x28283F, 2, 4) then
		$ConfirmedOutGame = 1
	EndIf
	if $g_VisibleScreenType == $SCREEN_PVP_FIGHTING and (IsColorAtPos(1707, 277, 0xB41A1B, 2, 4) or IsColorAtPos(1707, 277, 0xA10E18, 2, 4) ) then
		$ConfirmedIngame = 1
	EndIf	
	if $ConfirmedIngame == 0 and $ConfirmedOutGame == 0 then
		$g_VisibleScreenType = $SCREEN_UNKNOWN
		GUICtrlSetBkColor($lblGamesPlayed2, 0xBEBEFF)
	EndIf
EndFunc

Func UpdateUiGameCounter($bForceUpdate = False)
	; Total games played so far
	Local $GamesSinceLegendary = $GamesDetected - $GameCountAtPrevLegendary
	Local $WinsSinceLegendary = $GamesWon - $WinCountAtPrevLegendary
	Local $playedString2 = $GamesDetected & " G " & $GamesSinceLegendary & " W " & $WinsSinceLegendary & " L " & $LegendariesSkipped
	GUICtrlSetData($lblGamesPlayed2, $playedString2)
EndFunc

Func InitImageSearchDLLImages()
	Global $g_AppliedColorMaskToCachedImages, $g_image1EmeryxCost

	if $g_AppliedColorMaskToCachedImages == 1 then
		return
	EndIf
	$g_AppliedColorMaskToCachedImages = 1

	$g_imageRanks[0][0] = "L10_0826_0181_0283_0109.bmp"
	$g_imageRanks[0][1] = 10
	$g_imageRanks[1][0] = "L11_0822_0167_0266_0122.bmp"
	$g_imageRanks[1][1] = 11
	$g_imageRanks[2][0] = "L12_0822_0171_0267_0117.bmp"
	$g_imageRanks[2][1] = 12
	$g_imageRanks[3][0] = "L13_0823_0171_0265_0120.bmp"
	$g_imageRanks[3][1] = 13
	$g_imageRanks[4][0] = "L14_0821_0174_0266_0115.bmp"
	$g_imageRanks[4][1] = 14
	$g_imageRanks[5][0] = "L15_0829_0169_0260_0122.bmp"
	$g_imageRanks[5][1] = 15
	$g_imageRanks[6][0] = "L16_0825_0162_0250_0133.bmp"
	$g_imageRanks[6][1] = 16
	$g_imageRanks[7][0] = "L17_0825_0162_0250_0133.bmp"
	$g_imageRanks[7][1] = 17
	$g_imageRanks[8][0] = "L18_0825_0162_0250_0133.bmp"
	$g_imageRanks[8][1] = 18
	$g_imageRanks[9][0] = "L19_0825_0162_0250_0133.bmp"
	$g_imageRanks[9][1] = 19
	$g_imageRanks[10][0] = "L20_0825_0162_0250_0133.bmp"
	$g_imageRanks[10][1] = 20
	$g_imageRanks[11][0] = "L21_0825_0162_0250_0133.bmp"
	$g_imageRanks[11][1] = 21
	$g_imageRanks[12][0] = "L09__0826_0172_0283_0111.bmp"
	$g_imageRanks[12][1] = 9

	$g_imageRewards[0][0] = "Victory_BlueChest_0846_0576_0096_0096.bmp"
	$g_imageRewards[0][1] = $CRATE_BLUE
	$g_imageRewards[1][0] = "Victory_BlueNoStoreChest_0846_0576_0096_0096.bmp"
	$g_imageRewards[1][1] = $CRATE_BLUE
	$g_imageRewards[2][0] = "Victory_PurpleChest_0846_0576_0096_0096.bmp"
	$g_imageRewards[2][1] = $CRATE_PURPLE
	$g_imageRewards[3][0] = "Victory_PurpleNoStoreChest_0846_0576_0096_0096.bmp"
	$g_imageRewards[3][1] = $CRATE_PURPLE
	$g_imageRewards[4][0] = "Victory_GoldChest_0846_0576_0096_0096.bmp"
	$g_imageRewards[4][1] = $CRATE_GOLD
	$g_imageRewards[5][0] = "Victory_GoldNoStoreChest_0846_0576_0096_0096.bmp"
	$g_imageRewards[5][1] = $CRATE_GOLD

	$g_imageBlueCrate[0][0] = "BlueChest1_0478_0265_0263_0033.bmp"
	$g_imageBlueCrate[0][1] = $CRATE_BLUE
	$g_imageBlueCrate[1][0] = "BlueChest2_0478_0265_0263_0033.bmp"
	$g_imageBlueCrate[1][1] = $CRATE_BLUE2
	
	SetScreenshotMaxFPS(2)
	For $i = 0 To UBound($g_imageRanks) - 1
		ApplyColorMaskOnCachedImage($g_imageRanks[$i][0])
		ImageIsAtRadius($g_imageRanks[$i][0])
	Next
	For $i = 0 To UBound($g_imageRewards) - 1
		ApplyColorMaskOnCachedImage($g_imageRewards[$i][0])
		ImageIsAtRadius($g_imageRewards[$i][0])
	Next
	For $i = 0 To UBound($g_imageBlueCrate) - 1
		ApplyColorMaskOnCachedImage($g_imageBlueCrate[$i][0])
		ImageIsAtRadius($g_imageBlueCrate[$i][0])
	Next

	ApplyColorMaskOnCachedImage($g_image1EmeryxCost)
	ImageIsAtRegion($g_image1EmeryxCost, 249, 820, 249+1405, 820+20) ; so that screenshot will contain this region also
EndFunc

Global $g_LastSeenRank = 0
Func GuessCurrentPVPRank()
	; if script got interrupted, skip executing this function. Happens due to sleeps
	If ( $FuncIsRunning <> 1 or $BotIsRunning <> 1 ) then 
		return
	EndIf
	Global $g_LastSeenRank, $g_AppliedColorMaskToCachedImages, $g_VisibleScreenType
	if $g_LastSeenRank <> 0 then
		return
	EndIf
	
	if $g_VisibleScreenType == $SCREEN_START_PVP_FIGHT and IsColorAtPos(1707, 277, 0x28283F, 2, 4) == 1 then
		Local $acceptableDiffPCT = 15
		Local $acceptableSADPerPixel = 8

		;Local $hTimer = TimerInit()
		$g_LastSeenRank = 0
		Local $bestSadPP = $acceptableSADPerPixel
		For $i = 0 To UBound($g_imageRanks) - 1
			Local $searchRet = ImageIsAtRadius($g_imageRanks[$i][0])
			If $searchRet[0] > 0 and $searchRet[3] < $acceptableSADPerPixel Then
				$acceptableSADPerPixel = $searchRet[3]
				$g_LastSeenRank = $g_imageRanks[$i][1]
			EndIf
			If $searchRet[0] > 0 and $searchRet[3] < $bestSadPP Then
				$bestSadPP = $searchRet[3]
			EndIf
		Next

		GUICtrlSetData($lblDbgOut, $g_LastSeenRank & " SADPP=" & $acceptableSADPerPixel & " best " & $bestSadPP & " ")
	else 
		GUICtrlSetData($lblDbgOut, $g_VisibleScreenType & " seepvp = " & IsColorAtPos(1707, 277, 0x28283F, 2, 4))
	EndIf
EndFunc

Func DropGame()
	MyMouseClick("left", 1684, 247) ; open settings
	Sleep(500)
	MyMouseClick("left", 1489, 420) ; surrender button
	Sleep(500)
	MyMouseClick("left", 1086, 642) ; do you really want to concede ?
	Sleep(5000)
	MyMouseClick("left", 1129, 235) ; click defeat to go back to PVP screen
	Sleep(3000)	
	Global $g_LastSeenRank = 0 ; make sure we refresh the rank before we infinite spam "dropgames"
EndFunc

Func GetWinsRequiredAdvanceLeagues($HighLegueValue, $LowLeagueValue)
	Local $ret = 0
	$HighLegueValue = int($HighLegueValue)
	$LowLeagueValue = int($LowLeagueValue)
	If $HighLegueValue < $LowLeagueValue then
		return 0
	EndIf
	if $HighLegueValue >= 19 then
		$ret = 7 * (19 - $HighLegueValue)
		$HighLegueValue = 19
	endif
	If $LowLeagueValue < 1 then
		$ret = 11
		$LowLeagueValue = 1
	endif
	For $RankNow = $HighLegueValue to $LowLeagueValue + 1 step -1
		Local $PointsRequiredNextRank = $g_LeagueAdvanceInfo[$RankNow - 1][0] - $g_LeagueAdvanceInfo[$RankNow][0]
		Local $PointsGainedWithWin = $g_LeagueAdvanceInfo[$RankNow][1]
		Local $WinsRequiredThisRank = $PointsRequiredNextRank / $PointsGainedWithWin
		;_WriteDebugLog( $RankNow & "  PointsRequiredNextRank " & $PointsRequiredNextRank & "  PointsGainedWithWin " & $PointsGainedWithWin & "  WinsRequiredThisRank " & $WinsRequiredThisRank & "  " )
		$ret = $ret + $WinsRequiredThisRank
	Next
		
	return int($ret)
EndFunc

Func CheckDropGameASP()
	If ( $FuncIsRunning <> 1 or $BotIsRunning <> 1 ) then 
		return
	EndIf
	If GUICtrlRead($hCheckboxSpamUntilLegChance) <> $GUI_CHECKED then
		return
	Endif
	If $g_FightIsForChest <> 0 then ; get chest while we are still at a decent rank
		return
	EndIf
	if $g_VisibleScreenType <> $SCREEN_PVP_FIGHTING then
		return
	EndIf
	if $g_LastSeenRank == 0 or $g_LastSeenRank >= getInputNumericValue($inputMinRankAllowed, 20) then
		return
	EndIf
	
	;Local $WinsRequiredPerRank = 10
	;Local $WinsRequiredUntilTargerRank = Abs($g_LastSeenRank - getInputNumericValue( $inputTargetRankForCCI, 10 )) * $WinsRequiredPerRank ; this is dependent on the lowest rank you are willing to drop
	;Local $WinsRequiredUntilTargerRank = GetWinsRequiredAdvanceLeagues($g_LastSeenRank - 1, getInputNumericValue( $inputTargetRankForCCI, 10 ))
	Local $WinsRequiredUntilTargerRank = GetWinsRequiredAdvanceLeagues($g_LastSeenRank, getInputNumericValue( $inputTargetRankForCCI, 10 )) + getInputNumericValue($inputWinOffsetForTarget, 0)

	; in case we loose track of counting rewards, can fall back to dumb approach : win X games since last legendary
	Local $WinsUntilStopSpam = getInputNumericValue($inputSpamWinsForLegChance, 100)
	Local $WinsSinceLegendary = $GamesWon - $WinCountAtPrevLegendary
	Local $WinsRemainToSpam = $WinsUntilStopSpam - $WinsSinceLegendary
	if $WinsRemainToSpam > $WinsRequiredUntilTargerRank then
		GUICtrlSetData($lblDbgOut, $g_LastSeenRank & ' ' & $WinsRemainToSpam & ' > ' & $WinsRequiredUntilTargerRank)
		DropGame()
		return
	EndIf
	
	; if we know crate cycle index, we can skip a counple of games for sure
	Local $WinsRemainUntilStopSpam = GetSpammedWinCountRemaining()
	if $WinsRemainUntilStopSpam > $WinsRequiredUntilTargerRank then
		GUICtrlSetData($lblDbgOut2, "CCI drop : " & $WinsRemainUntilStopSpam & ' > ' & $WinsRequiredUntilTargerRank)
		DropGame()
		return
	endif
EndFunc

Func ShouldSpamFightsForLegendary()
	If GUICtrlRead($hCheckboxSpamUntilLegChance) = $GUI_CHECKED then
		; in case we loose track of game rewards, we could fall back to the good old no logic spam X
		Local $WinsSinceLegendary = $GamesWon - $WinCountAtPrevLegendary
		Local $WinsToSPamUntilStopSpam = getInputNumericValue($inputSpamWinsForLegChance, 100)
		if $WinsSinceLegendary < $WinsToSPamUntilStopSpam Then
			return 1
		EndIf
		; if we know crate index, we know with a higher precision when to stop spamming
		Local $WinsRemainUntilStopSpam = GetSpammedWinCountRemaining()
		if $WinsRemainUntilStopSpam > 0 then
			return 1
		endif
	EndIf
	return 0
EndFunc

Func OpenChestsWith1Emeryx()
	If ( $FuncIsRunning <> 1 or $BotIsRunning <> 1 ) then 
		return 0
	EndIf
	; are we ingame ?
	Global $g_VisibleScreenType
	if $g_VisibleScreenType <> $SCREEN_START_PVP_FIGHT then
		return 0
	EndIf
	Local $ChestsRemainingToOpen = getInputNumericValue($inputOpen1emeryxChests, 0)
	if $ChestsRemainingToOpen <= 0 then
		return 0
	EndIf
	
	Local $searchRet = ImageIsAtRegion($g_image1EmeryxCost, 249, 820, 249+1405, 820+20, 1 + 2)
	Local $searchRetSADPP = $searchRet[3]
	Local $searchRetAvgColorDiffPP = $searchRet[4]
	Local $searchRetAvgColorDiffCount = $searchRet[5]
	Local $searchRetAvgColorDiffPCT = $searchRet[6]
	Local $searchRetHashMatchPCT = $searchRet[7]
;	If $searchRet[0] > 0 and $searchRetSADPP <= 10 and $searchRetAvgColorDiffPP <= 30 and $searchRetHashMatchPCT >= 8 then
	If $searchRet[0] > 0 and $searchRetSADPP <= 4 then
		GUICtrlSetData($inputOpen1emeryxChests, $ChestsRemainingToOpen - 1)
		MyMouseClick("left", $searchRet[0], $searchRet[1] - 20)
		Sleep(2000) ; wait for chest show animation : the pay with emery button to appear
		MyMouseClick("left", 958, 767) ; open by spending emeryx
		Sleep(3000) ; waiting for chest open animation : see merc
		MyMouseClick("left", 1634, 203) ; close the opened "chest"
		Sleep(2000) ; allow screen to fully refresh so we can 
		GUICtrlSetData($lblDbgOut2, "O: " & $searchRet[0] & "," & $searchRet[1] & " " & $searchRetSADPP & " " & $searchRetAvgColorDiffPP & " " & $searchRetAvgColorDiffCount & " " & $searchRetAvgColorDiffPCT  & " " & $searchRetHashMatchPCT)
		return 1
	else
		GUICtrlSetData($lblDbgOut2, $searchRet[0] & " , " & $searchRet[1] & " spp " & $searchRetSADPP & " cd" & $searchRetAvgColorDiffPP & " cdc" & $searchRetAvgColorDiffCount & " " & $searchRetAvgColorDiffPCT  & " " & $searchRetHashMatchPCT)
	EndIf
	
	return 0
EndFunc

; out of blue moon once a week this screen gets clicked. Maybe some dust falls on my mouse pad
Func ExitAccidentalLegueScreen()
    If IsColorAtPos(200,601,0xFDD200,$ColorSearchRadius,$AcceptedColorTolerance) and IsColorAtPos(200,907,0x010207,$ColorSearchRadius,$AcceptedColorTolerance) Then
        MyMouseClick("left", 245, 120)
		Sleep(1000)
    EndIf
EndFunc

Func GetSpammedWinCountRemaining()
	Local $WinsRemainUntilStopSpam = 0
	if $g_FoundLegThisCycle == 1 then 
		$WinsRemainUntilStopSpam = getInputNumericValue($inputSpamCombatUntilCCINoLeg, 100) + ( $gc_CrateCycleLen - $g_CrateCycleIndex ) ; have to reach next cycle. add remaining games to be played
	else 
		$WinsRemainUntilStopSpam = getInputNumericValue($inputSpamCombatUntilCCINoLeg, 100) - $g_CrateCycleIndex
	EndIf
	return $WinsRemainUntilStopSpam
EndFunc

Func GetEstimatedTimeUntilSpamTarget()
	Local $EstimatedSecPerGame = getInputNumericValue($inputSingleGameDurationSec, 120)
	Local $RemainingWinsForCycle = GetSpammedWinCountRemaining()
	Local $EstiatedTimeUntilSpamEnds = $RemainingWinsForCycle * $EstimatedSecPerGame
	return Int($EstiatedTimeUntilSpamEnds)
EndFunc

Func AllowedToOpenChests()
	; chests exist only on this screen
	if $g_VisibleScreenType <> $SCREEN_START_PVP_FIGHT then
		return 0
	EndIf
	; probably waiting for 3 girl event to start
	if GUICtrlRead($hCheckboxNoChestOpens) == $GUI_CHECKED then
		return 0
	EndIf
	; just idling ingame. No special script running... we collect chests for skin stones
	if ShouldSpamFightsForLegendary() == 0 then
		return 1
	EndIf
	; we are spamming games for leg chests, and checkbox says we should not open chests
	If GUICtrlRead($hCheckboxNoChestsWhileSpamUntilLeg) == $GUI_CHECKED then
		return 0
	EndIf
	; there is so much time left that just open chests, we will have a second chance later on to open the same slot a second time
	Local $SecondsRemainUntilSpamTarget = GetEstimatedTimeUntilSpamTarget()
	If GetSpammedWinCountRemaining() > 0 and SecondsRemainUntilSpamTarget < getInputNumericValue($inputPauseOpensIfSpamSecRemains, 0) then
		GUICtrlSetData($lblDbgOut3, "wcr: " & GetSpammedWinCountRemaining() & " m: " & int(SecondsRemainUntilSpamTarget/60) & " < " & getInputNumericValue($inputPauseOpensIfSpamSecRemains, 0))
		return 0
	EndIf
	; no idea which case we failed to test
	return 1
EndFunc

Func SpamGameForHeliQuest()
	If ( $FuncIsRunning <> 1 or $BotIsRunning <> 1 ) then 
		return
	EndIf
	; are we ingame ?
	if $g_VisibleScreenType <> $SCREEN_START_PVP_FIGHT then
		return
	EndIf
	; there is the quest win X games
	if getInputNumericValue($inputSpamLimit, 0) > $GamesDetected Then
		Fight2()
	Else
		GUICtrlSetData($inputSpamLimit, "")
	EndIf
EndFunc

Func ForeverLoopFunc()
	Global $g_VisibleScreenType, $FuncIsRunning, $BotIsRunning

	$FuncIsRunning = 1 - $FuncIsRunning

	if $FuncIsRunning == 1 then
		GUICtrlSetBkColor($lblCrateCycleCrates, 0x00FF00)
	EndIf	
	while( $FuncIsRunning == 1 and $BotIsRunning == 1 )	
		;GUICtrlSetData($lblDbgOut, Hex( PixelGetColor(1707, 277), 6 ) & " -- " & $g_VisibleScreenType)

		ExitAccidentalLegueScreen()
		GuessIngameVisibleScreen()
		;CheckDropGameAlwaysToMaintainRank()
		
		; things to do while out of fight screen
		if $g_VisibleScreenType == $SCREEN_START_PVP_FIGHT then
			GuessCurrentPVPRank() ; internally will handle states based on ingame or outofgame screens
			
			; probably just for some quests. Open multiple chests without fighting because we might miss some
			while OpenChestsWith1Emeryx() == 1
			WEnd
			
			; do not open chests until we are in a very low rank ?
			if AllowedToOpenChests() == 1 then 
				OpenWinstreakChests() ; only do these at low rank
				OpenChests()
			EndIf
			
			; normal fight always in case we have chest slot open
			Fight()
			
			; spam fights while generating statistics about leg distances
			if ShouldSpamFightsForLegendary() == 1 then
				; if we can start a fight, fight.
				Fight2()
			EndIf
			
			SpamGameForHeliQuest();
		EndIf
		if $g_VisibleScreenType == $SCREEN_PVP_FIGHTING then
			CheckDropGameASP()
		EndIf
		if $g_VisibleScreenType == $SCREEN_UNKNOWN then
			; done fighting ? pick rewards
			ClickFightWon()
			ClickFightLost()
		EndIf
		
		; random click that maybe closes the window
		ClearRandomClickContent()

		UpdateUiGameCounter()

		; else you can't click on buttons
		HandleGUIMessages()
		
		; Small delay to prevent high CPU usage
		If ( $FuncIsRunning == 1 and $BotIsRunning == 1 ) then 
			Sleep(500)
		EndIf
	wend

	GUICtrlSetBkColor($lblCrateCycleCrates, 0xFF7777)
	
EndFunc

; Why it actually works : because short game duration ( relative to long game) skews the probability curve. AKA Multiple times faster to loose a legendary chest with 1/200 probability compared to a 75% chance to slowly wait for a legendary
; Counting starts after getting a legendary. Count every game, both wins and losses. If at any point you get a legendary, you can restart the process.
; This process highly depends on 'time loop', number of chest slots, computer speed, fastest way to end a game.
; If you push for 150 games, there is about 25% chance you can get 1 legendary chests in 2+X hours with this method ( worst case 1 / day). I managed 5 leg chests / day multiple times
; 150 games spammed : There is a 75% chance you had to restart the process. Maybe even 3+ times. There is a 100% chance you will get a legendary crate the next 50 games. Takes me worst case 12 hours
; 100 games spammed : There is a 50% chance you had to restart the process. Maybe even 2+ times. There is a 100% chance you will get a legendary crate the next 100 games. Takes me worst case 24 hours
; Method:
;	- drop games until you drop to a rank where you can oneshot enemy commander. For me this is League 19 as commanders tend to have 170 health and my ghost does 200 dmg 
;	- stay on L19 ( win 2 games, drop 1 game ) for 66 games ( 44 wins, 22 losses). If I do this right, I can reach 66 games in 66 * 38/60=42 minutes
;	- the next 44 games do not drop games(44 wins). Try win as much as possible. I can lower my rank to L12. For me, this takes about 44 * 120/60=88 minutes
;   - you decide : can keep chests slots with unopened chests and only open them now. This pushes me another 8 games. At this point I'm if at 118 games(22 losses, 96 wins) in 2 hours of spamming games. Worst case will get a legendary in the next day ( 140 wins max )
;	- if arrived here and still did not get a LEGO, keep playing game only when you can store the reward chest 

; theories if we should keep blue chests unopened while spamming games
; if 140 crates remaing : 2*(56 * 2 + 10 * 4 + 3 * 5 + 12) = 2*179 / 8 = 44 hours / leg = 34.8 hous with timeloop
; if 70 crates remaining avg case : 56 * 2 + 10 * 4 + 3 * 5 + 12 = 179 / 8 = 22 hours / leg = 17.4 hous with timeloop
; if 20 crates remaining + worst case : 3 * 5 + 10 * 4 + 7 * 2 = 69 / 8 = 8.6 * 0.78 = 6.7 hours / leg with timeloop and also like 64 skin stones
; Not opening blue chests for 9 hours means : 8*(9/2)*300=10800 blues .. 54 legendaries lost
; Not opening blue chests for 18 hours means : 8*(9/2)*300=10800 blues .. 108 legendaries lost