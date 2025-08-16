#include <GUIConstantsEx.au3>
#include <Date.au3>
#include <WinAPISys.au3>
#Include "CommonFunctions.au3"

Opt("MustDeclareVars", 1)
HotKeySet("I", "ExitBot")
HotKeySet("P", "ForeverLoopFunc")
HotKeySet("A", "PasteClipboard")

;only used while developing
HotKeySet("Q", "TakeScreenshotCyclePositions")
HotKeySet("W", "TakeScreenshotAtMousePosPreRecorded")

Global $g_BotIsRunning = 1
Global $g_FuncIsRunning = 0
Global $g_ClickIntervalUnkScreen = 15000 ; pvp rankup/down or huge loading screens
Global $g_lastClickTimer = _WinAPI_GetTickCount()
Global $g_lastPVPScreenTimeStamp = _WinAPI_GetTickCount()
Global $g_GamesWon = 0
Global $g_GamesWonRecordedAt = 0
Global $AcceptedColorTolerance = 3
Global $ColorSearchRadius = 1
Global $pos ; last pixel pos . Cancer code detected
Const $SCREEN_PVP_FIGHTING = 1, $SCREEN_START_PVP_FIGHT = 0, $SCREEN_UNKNOWN = 3
Const $CRATE_BLUE = 1, $CRATE_BLUE2 = 2, $CRATE_PURPLE = 3, $CRATE_GOLD = 4
Const $CrateCountBlue = 56, $CrateCountBlue2 = 10, $CrateCountPurple = 3, $CrateCountGold = 1
Const $gc_CrateCycleLen = 70
Global $g_VisibleScreenType = $SCREEN_UNKNOWN
Global $g_VisibleScreenTypeNoReset = $SCREEN_UNKNOWN
Global $g_GamesDetected = 0
Global $g_GameCountAtPrevLegendary = 0
Global $g_WinCountAtPrevLegendary = 0
Global $g_LegendariesSkipped = 0
Global $g_LegendariesOpened = 0
Global $g_GamesCounterPrev = -1
Global $g_imageRanks[21][2]
Global $g_imageRewards[6][2]
Global $g_imageBlueCrate[2][2]
Global $g_AppliedColorMaskToCachedImages = 0
Global $g_image1EmeryxCost = ""
Global $g_imagePVPStartFight = ""
Global $g_imagePVPIngame1 = ""
Global $g_imagePVPIngame2 = ""
Global $g_imageVictorySreen = ""
Global $g_imageDefeatSreen = ""
Global $g_imageLeagueSreen = ""
Global $g_imageGameCrashed = ""
Global $g_imagePageReload = ""
Global $g_imageGameloaded = ""
Global $g_imageGameloaded2 = ""
Global $g_imageClosePopup1 = ""
Global $g_imageClosePopup2 = ""
Global $g_imageClosePopup3 = ""
Global $g_imagePVPButton = ""
;Global $g_WinHistory[8]
Global $g_CrateHistory[500]
Global $g_CrateCycleIndex = -1000 ; index where the next cycle starts in our history array
Global $g_FoundLegThisCycle = 0
Global $g_LastSeenGoldCrateCanBeStored = 1
Global $g_MouseSpeed = 10
Global $g_FightIsForChest = 0
Global $g_LastSeenRank = 0
Global $g_LastSeenRankNoReset = 0
Global $g_LastSeenRankMin = 33
global $g_StampLastLegObtained = 0
global $g_StampLastLegObtainedAndStored = 0
global $g_ChestsObtainedSinceLastLeg[33][6]	; note that this also stores if chest could be stored or not 0-blue, 1-bluenostore
global $g_RanksSeenSinceLastLegendary[33] ; also counts rank switches
global $g_ChestsOpenedSinceLastLeg = 0

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
	Global $g_BotIsRunning = 0
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
Global $inputOpenLegendaryIfLower = GUICtrlCreateInput("1", $margin + $labelWidth + $spacing, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $inputWidth, $ComponentHeight)
GUICtrlSetTip($lblTempForTooltip, "If a legendary can't be stored, should we open it. Probably happens while spamming games")
GUICtrlSetTip($inputOpenLegendaryIfLower, "If a legendary can't be stored, should we open it. Probably happens while spamming games")
;Global $hCheckboxOpenLegendary = GUICtrlCreateCheckbox("OpenLegIfLowerLegue", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth + $inputWidth, $ComponentHeight)
;GUICtrlSetTip($hCheckboxOpenLegendary, "If a legendary can't be stored, should we open it. Probably happens while spamming games")
$ComponentsAdded = $ComponentsAdded + 1

Global $lblTempForTooltip = GUICtrlCreateLabel("OpenLegCount :", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth, $ComponentHeight)
Global $inputOpenLegendaryCount = GUICtrlCreateInput("0", $margin + $labelWidth + $spacing, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $inputWidth, $ComponentHeight)
GUICtrlSetTip($lblTempForTooltip, "If a legendary can't be stored, should we open it. Needed for 3 girl event")
GUICtrlSetTip($inputOpenLegendaryCount, "If a legendary can't be stored, should we open it. Needed for 3 girl event")
$ComponentsAdded = $ComponentsAdded + 1


Global $hCheckboxPauseLegendary = GUICtrlCreateCheckbox("PauseOnLeg", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth + $inputWidth, $ComponentHeight)
GUICtrlSetTip($hCheckboxPauseLegendary, "If a legendary can't be stored, should we pause game? Probably happens while quests that require to open X legendaries. Kinda unused feature")
$ComponentsAdded = $ComponentsAdded + 1

Global $hCheckboxSkipLegendary = GUICtrlCreateCheckbox("SkipLeg", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth + $inputWidth, $ComponentHeight)
GUICtrlSetTip($hCheckboxSkipLegendary, "If a legendary can't be stored, should we skip it ? Resets CCI lego spam hunt")
GUICtrlSetState($hCheckboxSkipLegendary, $GUI_CHECKED)
$ComponentsAdded = $ComponentsAdded + 1

Global $lblTempForTooltip = GUICtrlCreateLabel("MaxRankToDrop:", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth, $ComponentHeight)
Global $inputMinRankAllowed = GUICtrlCreateInput("1", $margin + $labelWidth + $spacing, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $inputWidth, $ComponentHeight)
GUICtrlSetTip($inputMinRankAllowed, "For fast game spam, we will drop games until we reach this rank(worst case). Helps us do games faster")
GUICtrlSetTip($lblTempForTooltip, "For fast game spam, we will drop games until we reach this rank(worst case). Helps us do games faster")
$ComponentsAdded = $ComponentsAdded + 1

Global $lblTempForTooltip = GUICtrlCreateLabel("ClimbToRank :", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth, $ComponentHeight)
Global $inputTargetRankForCCI = GUICtrlCreateInput("1", $margin + $labelWidth + $spacing, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $inputWidth, $ComponentHeight)
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
Global $inputSpamCombatUntilCCINoLeg = GUICtrlCreateInput("35", $margin + $labelWidth + $spacing, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $inputWidth, $ComponentHeight)
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
Global $inputSingleGameDurationSec = GUICtrlCreateInput("180", $margin + $labelWidth + $spacing, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $inputWidth, $ComponentHeight)
GUICtrlSetTip($lblTempForTooltip, "How many minutes does a game take ? Highly depends on your rank, team. Used to estimate time remaining until CCI target")
GUICtrlSetTip($inputSingleGameDurationSec, "How many minutes does a game take ? Highly depends on your rank, team. Used to estimate time remaining until CCI target")
$ComponentsAdded = $ComponentsAdded + 1

Global $lblTempForTooltip = GUICtrlCreateLabel("NoChestIfRemSec :", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth, $ComponentHeight)
Global $inputPauseOpensIfSpamSecRemains = GUICtrlCreateInput("3600", $margin + $labelWidth + $spacing, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $inputWidth, $ComponentHeight)
GUICtrlSetTip($lblTempForTooltip, "Pause opening chests if we are close to our CCI target")
GUICtrlSetTip($inputPauseOpensIfSpamSecRemains, "Pause opening chests if we are close to our CCI target")
$ComponentsAdded = $ComponentsAdded + 1

Local $lblTimeEstimates = GUICtrlCreateLabel("TimeEstimates", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth + $inputWidth, $ComponentHeight)
GUICtrlSetTip($lblTimeEstimates, "ETA for : time spam avg case / time spam remaining currently + max chest open time / max chest open time remaining currently")
$ComponentsAdded = $ComponentsAdded + 1

Global $lblTempForTooltip = GUICtrlCreateLabel("MaintainRank :", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth, $ComponentHeight)
Global $inputFixedRankUsed = GUICtrlCreateInput("0", $margin + $labelWidth + $spacing, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $inputWidth, $ComponentHeight)
GUICtrlSetTip($lblTempForTooltip, "Try to reach this specific Rank. Used for CAM")
GUICtrlSetTip($inputFixedRankUsed, "Try to reach this specific Rank. Used for CAM")
$ComponentsAdded = $ComponentsAdded + 1

Local $lblDbgOut = GUICtrlCreateLabel("Debug", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth + $inputWidth, $ComponentHeight)
$ComponentsAdded = $ComponentsAdded + 1
Local $lblDbgOut2 = GUICtrlCreateLabel("Debug 2", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth + $inputWidth, $ComponentHeight)
$ComponentsAdded = $ComponentsAdded + 1
;Local $lblDbgOut3 = GUICtrlCreateLabel("Debug screen", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth + $inputWidth, $ComponentHeight)
;$ComponentsAdded = $ComponentsAdded + 1
;Local $lblDbgOut4 = GUICtrlCreateLabel("Debug screen", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth + $inputWidth, $ComponentHeight)
;$ComponentsAdded = $ComponentsAdded + 1
;Local $lblDbgOut5 = GUICtrlCreateLabel("Debug screen", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth + $inputWidth, $ComponentHeight)
;$ComponentsAdded = $ComponentsAdded + 1
;Local $lblDbgOut3 = GUICtrlCreateLabel("MinutesUntilSpamCCITareget", $margin, $margin + $ComponentsAdded * ( $margin + $ComponentHeight), $labelWidth + $inputWidth, $ComponentHeight)
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

ResetDebugLog()
;_WriteDebugLog(GetWinsRequiredAdvanceLeagues(16,12)) ; 35 steps to get from a spammy situation to a decent reward rank

; load saved variables
LoadVariables()
UpdateUiGameCounter(True)
LoadCrateHistoryFromFile("GameRewards.txt")
CalculateCrateCycleIdex()
UpdateUICrateCycleIndex()
UpdateUITimeEstimates()
InitImageSearchDLLImages()
GuessIngameVisibleScreen()
DisableFileExistsChecks()

for $i=0 to 33 - 1
	for $j=0 to 6 - 1
		$g_ChestsObtainedSinceLastLeg[$i][$j] = 0
	Next
Next

for $i=0 to UBound($g_RanksSeenSinceLastLegendary) - 1
	$g_RanksSeenSinceLastLegendary[$i] = 0
Next

; Event Loop
While ( $g_BotIsRunning == 1)
	HandleGUIMessages()
WEnd

; sadly this would need to be run in a separate thread than ForeverLoopFunc
Func HandleGUIMessages()
    Local $msg = GUIGetMsg()

    If $msg = $GUI_EVENT_CLOSE Then 
		$g_BotIsRunning = 0
		$g_FuncIsRunning = 0
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
	GUICtrlSetData($inputSpamLimit, $g_GamesDetected + $SpamXGames)
EndFunc

; save/load saved variables 
Func SaveVariables()
    IniWrite("variables.ini", "Variables", "GamesDetected", $g_GamesDetected)
    IniWrite("variables.ini", "Variables", "GameCountAtPrevLegendary", $g_GameCountAtPrevLegendary)
    IniWrite("variables.ini", "Variables", "LegendariesSkipped", $g_LegendariesSkipped)
    IniWrite("variables.ini", "Variables", "LegendariesOpened", $g_LegendariesOpened)	
    IniWrite("variables.ini", "Variables", "GamesWon", $g_GamesWon)
    IniWrite("variables.ini", "Variables", "WinCountAtPrevLegendary", $g_WinCountAtPrevLegendary)
    IniWrite("variables.ini", "Variables", "StampLastLegObtained", $g_StampLastLegObtained)
    IniWrite("variables.ini", "Variables", "StampLastLegObtainedAndStored", $g_StampLastLegObtainedAndStored)
EndFunc

Func LoadVariables()
    Global $g_GamesDetected = IniRead("variables.ini", "Variables", "GamesDetected", 0)
    Global $g_GameCountAtPrevLegendary = IniRead("variables.ini", "Variables", "GameCountAtPrevLegendary", 0)
    Global $g_LegendariesSkipped = IniRead("variables.ini", "Variables", "LegendariesSkipped", 0)
    Global $g_LegendariesOpened = IniRead("variables.ini", "Variables", "LegendariesOpened", 0)
    Global $g_GamesWon = IniRead("variables.ini", "Variables", "GamesWon", 0)
    Global $g_WinCountAtPrevLegendary = IniRead("variables.ini", "Variables", "WinCountAtPrevLegendary", 0)
    Global $g_StampLastLegObtained = IniRead("variables.ini", "Variables", "StampLastLegObtained", 0)
    Global $g_StampLastLegObtainedAndStored = IniRead("variables.ini", "Variables", "StampLastLegObtainedAndStored", 0)
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
	$g_lastClickTimer = _WinAPI_GetTickCount()	; else it might close chest open window or similar. This timer is for the random content clear click
EndFunc

Func LoadCrateHistoryFromFile($filePath)

	For $i=0 to UBound($g_CrateHistory) - 1
		$g_CrateHistory[$i] = 0
	Next
	
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
	Local $CheckIndices[7]
	$CheckIndices[0] = 0
	$CheckIndices[1] = 1
	$CheckIndices[2] = -1
	$CheckIndices[3] = 2
	$CheckIndices[4] = -2
	$CheckIndices[5] = 3
	$CheckIndices[6] = -3
	Local $HistoryValuesFound = 0
	For $i = 0 to UBound($g_CrateHistory) - 1
		If $g_CrateHistory[$i] == 0 Then
			ExitLoop
		EndIf
		$HistoryValuesFound = $HistoryValuesFound + 1
	Next
	GUICtrlSetData($lblCrateCycleCrates, "Recalculating CCI on " & $HistoryValuesFound)
	_WriteDebugLog("Recalculating CCI on " & $HistoryValuesFound, 1)
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
		while $NextCycleStart < $HistoryValuesFound - $gc_CrateCycleLen - 1
			Local $BestCycleMatchDeviation = $gc_CrateCycleLen
			Local $CorrectedNextCycleStart = $NextCycleStart
			; there are times when a reward crate does not get recorded, or badly recorded
			; do a best of X searches and pick the best one
			;for $MiniCycleStart = $NextCycleStart - $AcceptableHistoryErrorCount to $NextCycleStart + $AcceptableHistoryErrorCount
			; maybe we added extra crates or forgot to note down crates
			for $i = 0 to $AcceptableHistoryErrorCount * 2 + 1 - 1
				Local $MiniCycleStart = $NextCycleStart + $CheckIndices[$i]
				Local $CycleMatchDeviation = 0
				Local $isGood = IsGoodIndexForCycleStart($MiniCycleStart, $MiniCycleStart + $gc_CrateCycleLen, $CycleMatchDeviation, $AcceptableHistoryErrorCount)
				if $CycleMatchDeviation < $BestCycleMatchDeviation then 
					$CorrectedNextCycleStart = $MiniCycleStart
					$BestCycleMatchDeviation = $CycleMatchDeviation
				EndIf
				If $BestCycleMatchDeviation == 0 or $NextCycleStart == $CycleStart Then
					ExitLoop
				EndIf
			Next
			IF $BestCycleMatchDeviation <= $AcceptableHistoryErrorCount then
				$NextCycleStart = $CorrectedNextCycleStart ; maybe we added extra crates or forgot to note down crates
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
		if $CyclesTested > 0 and $SumCycleDeviations / $CyclesTested < $BestCycleDeviation and $CyclesGood >= 1 then
			_WriteDebugLog("Improved crate index " & $CycleStart & " from " & $BestCycleDeviation & " to " & $SumCycleDeviations & @CRLF,0)
			$BestCycleDeviation = $SumCycleDeviations / $CyclesTested
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
	If ( $g_FuncIsRunning <> 1 or $g_BotIsRunning <> 1 ) then 
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
			$g_ChestsOpenedSinceLastLeg = $g_ChestsOpenedSinceLastLeg + 1
			MyMouseClick("left", $pos1[0], $pos1[1])
			Sleep(3000)
			MyMouseClick("left", 1634, 203)
			Sleep(1000)
			return
		EndIf
	Next
EndFunc

Func OpenWinstreakChests()
	If ( $g_FuncIsRunning <> 1 or $g_BotIsRunning <> 1 ) then 
		return
	EndIf
    ; If the color is found, click on it
    If IsColorAtPos(1585,653,0xFFD802,$ColorSearchRadius,$AcceptedColorTolerance) Then
        MyMouseClick("left", 1542, 525)
		Sleep(3000)
		MyMouseClick("left", 1634, 203) ; close the reward screen by clicking in the upper right corner
		Sleep(1000)
    EndIf
EndFunc

Func Fight()
	If ( $g_FuncIsRunning <> 1 or $g_BotIsRunning <> 1 ) then 
		return
	EndIf
    If IsColorAtPos(1088, 535, 0xFF4444, $ColorSearchRadius, $AcceptedColorTolerance) Then
		$g_FightIsForChest = 1
        MyMouseClick("left", $pos[0], $pos[1])
		Sleep(2000)
    EndIf
EndFunc

Func Fight2()
	If ( $g_FuncIsRunning <> 1 or $g_BotIsRunning <> 1 ) then 
		return
	EndIf
    If IsColorAtPos(1092,536,0xBF2D03,$ColorSearchRadius,$AcceptedColorTolerance) Then
        MyMouseClick("left", $pos[0], $pos[1])
		Sleep(2000)
    EndIf
EndFunc

Func AddLegendaryGameFreqStat()
	$g_GameCountAtPrevLegendary = $g_GamesDetected
	$g_WinCountAtPrevLegendary = $g_GamesWon
		
	; just so that I don't freak out that the counter is not working
	UpdateUiGameCounter(True)
EndFunc

Func HandleLegendaryWinReward()
	If ( $g_FuncIsRunning <> 1 or $g_BotIsRunning <> 1 ) then 
		return
	EndIf
	; is this a legendary reward ?
    If IsColorAtPos(854,575,0xC56001,$ColorSearchRadius,$AcceptedColorTolerance) Then
		AddLegendaryGameFreqStat()
		
		If $g_LastSeenGoldCrateCanBeStored == 1 then
			$g_LegendariesOpened = $g_LegendariesOpened + 1
			MyMouseClick("left", 1634, 203) ; click somewhere to simply close the victory screen
			;GUICtrlSetData($lblDbgOut5, "HandleLegendaryWinReward")
			Sleep(3000)
			return 1
		EndIf
		
		If GUICtrlRead($hCheckboxPauseLegendary) = $GUI_CHECKED Then
			$g_FuncIsRunning = 0
			return 1
		EndIf
				
		Local $OpenLegChestIfLowLeague = getInputNumericValue($inputOpenLegendaryIfLower, 0)
		Local $OpenLegCount = getInputNumericValue($inputOpenLegendaryCount, 0)
;		If GUICtrlRead($hCheckboxOpenLegendary) = $GUI_CHECKED Then
		If $OpenLegChestIfLowLeague <> 0 and $OpenLegChestIfLowLeague > $g_LastSeenRankNoReset and $g_LastSeenRankNoReset <> 0 and $OpenLegCount > 0 Then
			$g_LegendariesOpened = $g_LegendariesOpened + 1
			GUICtrlSetData($inputOpenLegendaryCount, $OpenLegCount - 1)
			MyMouseClick("left", 929, 768)
			Sleep(2000)
			MyMouseClick("left", 929, 768);
			Sleep(1000)
			MyMouseClick("left", 1634, 203)
			;GUICtrlSetData($lblDbgOut5, "HandleLegendaryWinReward2")
			Sleep(1000)
			return 1
		EndIf
		
		If GUICtrlRead($hCheckboxSkipLegendary) = $GUI_CHECKED Then
			MyMouseClick("left", 1128, 224)
			Sleep(3000)
			MyMouseClick("left", 1079, 570)
			Sleep(3000)
			$g_LegendariesSkipped = $g_LegendariesSkipped + 1
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

	$g_ChestsObtainedSinceLastLeg[$g_LastSeenRankNoReset][$bestRewardImageIndex] = $g_ChestsObtainedSinceLastLeg[$g_LastSeenRankNoReset][$bestRewardImageIndex] + 1
	
	; there is small blue crate and large blue crate
	if $bestRewardType == $CRATE_BLUE or $bestRewardType == $CRATE_BLUE2 Then
		; click the crate to show details
		Sleep(500) ; wait for crate details animation. This seems to bug out from time to time and not open crate details ?
        MyMouseClick("left", 846 + 96/2, 576 + 96/2)
		Sleep(1500) ; wait for crate details animation. This seems to bug out from time to time and not open crate details ?
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
			Sleep(1500) ; sometimes there seems to be latency here and chest type is a black image ?
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
		;GUICtrlSetData($lblDbgOut5, "RecordVictoryRewards")
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
	UpdateUICrateCycleIndex()
	
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
		$g_StampLastLegObtained = _WinAPI_GetTickCount()

		Local $MinuesSpentSinceLastStoredLeg = int((_WinAPI_GetTickCount() - $g_StampLastLegObtainedAndStored)/1000/60)		
		if $g_StampLastLegObtainedAndStored == 0 then
			$MinuesSpentSinceLastStoredLeg = 0
		EndIf
		if $g_LastSeenGoldCrateCanBeStored then
			$g_StampLastLegObtainedAndStored = _WinAPI_GetTickCount()
		EndIf
		
		Local $WinsSinceLegendary = $g_GamesWon - $g_WinCountAtPrevLegendary
		Local $RemainingCrates = GUICtrlRead($lblCrateCycleCrates)
		Local $MoreStatsStr = ""
		$MoreStatsStr = $MoreStatsStr & ",MinutesSinceL=" & $MinuesSpentSinceLastLeg
		$MoreStatsStr = $MoreStatsStr & ",MinutesSinceStoredL=" & $MinuesSpentSinceLastStoredLeg
		$MoreStatsStr = $MoreStatsStr & ",CCI=" & $g_CrateCycleIndex
		$MoreStatsStr = $MoreStatsStr & ",CCITarget=" & getInputNumericValue($inputSpamCombatUntilCCINoLeg, 100)
		$MoreStatsStr = $MoreStatsStr & ",CanStore=" & $g_LastSeenGoldCrateCanBeStored
		$MoreStatsStr = $MoreStatsStr & ",League=" & $g_LastSeenRank
		$MoreStatsStr = $MoreStatsStr & ",WinsSinceL=" & $WinsSinceLegendary
		$MoreStatsStr = $MoreStatsStr & ",CratesLeftThisCCI=" & $RemainingCrates

		$MoreStatsStr = $MoreStatsStr & ",CratesOpenedSinceL=" & $g_ChestsOpenedSinceLastLeg

		$MoreStatsStr = $MoreStatsStr & ",ChestsObtainedRankSinceL(B,B2,P)="
		for $i=0 to 33 - 1
			for $j=0 to 6 - 3
				if $g_ChestsObtainedSinceLastLeg[$i][$j] > 0 Then
					$MoreStatsStr = $MoreStatsStr & $i & "-" & $j & "(" & $g_ChestsObtainedSinceLastLeg[$i][$j] & ");"
				EndIf
			Next
			$g_ChestsObtainedSinceLastLeg[$i][$j] = 0
		Next
		
		$MoreStatsStr = $MoreStatsStr & ",RanksSeenSinceL="
		for $i=0 to UBound($g_RanksSeenSinceLastLegendary) - 1
			If $g_RanksSeenSinceLastLegendary[$i] > 0 Then
				$MoreStatsStr = $MoreStatsStr & $i & "(" & $g_RanksSeenSinceLastLegendary[$i] & ");"
			EndIf
			$g_RanksSeenSinceLastLegendary[$i] = 0
		Next

		Local $hFile = FileOpen("legCCIs2.txt", 1) ; Mode 1 = Append
		If $hFile <> -1 Then
			FileWrite($hFile, (@WDAY - 1) & "," & @HOUR & ":" & @MIN & $MoreStatsStr & "," & @CRLF)
			FileClose($hFile)		
		EndIf
		$g_ChestsOpenedSinceLastLeg = 0;
		;Local $hFile = FileOpen(GetStringWithDayOfWeek(), 1) ; Mode 1 = Append
		;If $hFile <> -1 Then
		;	FileWrite($hFile, $g_CrateCycleIndex & ",")
		;	FileClose($hFile)		
		;EndIf	
	Endif	
	
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
	If ( $g_FuncIsRunning <> 1 or $g_BotIsRunning <> 1 ) then 
		return
	EndIf
	Local $canSeeVictoryImage = 0
	; innacurate and might go out of date with game updates
	If IsColorAtPos(1129,235,0xFFEBA2,$ColorSearchRadius,$AcceptedColorTolerance) then
		$canSeeVictoryImage = 1
	EndIf
	If $canSeeVictoryImage == 0 Then
		Local $searchRet = ImageIsAtRegion($g_imageVictorySreen)
		If $searchRet[0] > 0 and $searchRet[3] <= 4 then
			$canSeeVictoryImage = 1
		EndIf
	EndIf
    If $canSeeVictoryImage == 1 Then
		$g_FightIsForChest = 0 ; pause spam drop games if we can store a new chest
		Sleep(1000) ; seems like the chest appears slightly later than the victory letters
		
		; should not happen. Somehow we are recording the win condition more than once ?
		; we can still see the victory screen even though we already tried to click on the 'y' once
		if $g_GamesWonRecordedAt == $g_GamesDetected then
			MyMouseClick("left", 1634, 203)
			;GUICtrlSetData($lblDbgOut5, "ClickFightWon")
			$g_LastSeenRank = 0 ; make sure we refresh the rank before we infinite spam "dropgames"
			return
		EndIf
		
		$g_GamesWon = $g_GamesWon + 1
		$g_GamesWonRecordedAt = $g_GamesDetected

		RecordVictoryRewards()
		UpdateUITimeEstimates()

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
	If ( $g_FuncIsRunning <> 1 or $g_BotIsRunning <> 1 ) then 
		return
	EndIf
	Local $canSeeDefeatImage = 0
	; innacurate and might go out of date with game updates
    If IsColorAtPos(1106,192,0xFF4342,$ColorSearchRadius,$AcceptedColorTolerance) Then
		$canSeeDefeatImage = 1
	EndIf
	If $canSeeDefeatImage == 0 Then
		Local $searchRet = ImageIsAtRegion($g_imageDefeatSreen)
		If $searchRet[0] > 0 and $searchRet[3] <= 4 then
			$canSeeDefeatImage = 1
		EndIf
	EndIf
    If $canSeeDefeatImage == 1 Then
		UpdateUITimeEstimates()
		$g_LastSeenRank = 0 ; make sure we refresh the rank before we infinite spam "dropgames"
		$g_FightIsForChest = 0 ; pause spam drop games if we can store a new chest
        MyMouseClick("left", $pos[0], $pos[1])
		Sleep(3000)
    EndIf
EndFunc

Func ClearRandomClickContent()
	If ( $g_FuncIsRunning <> 1 or $g_BotIsRunning <> 1 ) then 
		return
	EndIf
	; we can recognize this screen. Only click on unrecognized screens
	if $g_VisibleScreenType <> $SCREEN_UNKNOWN then
		$g_lastClickTimer = _WinAPI_GetTickCount()
		return
	endif	
    ; Check if the click interval has passed since the last click
    If $g_lastClickTimer + $g_ClickIntervalUnkScreen < _WinAPI_GetTickCount() Then
		MyMouseClick("left", 1634, 203)
		;GUICtrlSetData($lblDbgOut5, "ClearRandomClickContent")
        $g_lastClickTimer = _WinAPI_GetTickCount()
    EndIf	
EndFunc

Func GuessIngameVisibleScreen()
	Local $IngameScreen = 0
	Local $StartGameScreen = 0
	
	If $StartGameScreen == 0 then
		Local $searchRet = ImageIsAtRegion($g_imagePVPStartFight)
		If $searchRet[0] > 0 and $searchRet[3] <= 4 then
			$StartGameScreen = 1
		EndIF
		;GUICtrlSetData($lblDbgOut4, "startpvp spp " & $searchRet[3])
	EndIf
	If $StartGameScreen == 0 then
		Local $searchRet = ImageIsAtRegion($g_imagePVPIngame1,-1,-1,-1,-1, 1)
		If $searchRet[0] > 0 and $searchRet[3] <= 4 then
			$IngameScreen = 1
		EndIF
		Local $searchRetSADPP = $searchRet[3]
		Local $searchRetAvgColorDiffPP = $searchRet[4] ; 66
		Local $searchRetAvgColorDiffCount = $searchRet[5] ; 3713
		Local $searchRetAvgColorDiffPCT = $searchRet[6]
		Local $searchRetHashMatchPCT = $searchRet[7]
		;GUICtrlSetData($lblDbgOut3, "ingame spp " & $searchRet[3])
		;GUICtrlSetData($lblDbgOut3, $searchRet[0] & " , " & $searchRet[1] & " spp " & $searchRetSADPP & " cd " & $searchRetAvgColorDiffPP & " cdc " & $searchRetAvgColorDiffCount & " " & $searchRetAvgColorDiffPCT  & " " & $searchRetHashMatchPCT)
;		if $searchRetSADPP > 4 and $searchRetSADPP < 25 then
;			TakeScreenshotRegionAndSaveit(192,149,192+14,149+543)
;		Endif
	EndIf
	If $StartGameScreen == 0 and $IngameScreen == 0 then
		Local $searchRet = ImageIsAtRegion($g_imagePVPIngame2,-1,-1,-1,-1, 1)
		If $searchRet[0] > 0 and $searchRet[3] <= 4 then
			$IngameScreen = 1
		EndIF
		Local $searchRetSADPP = $searchRet[3]
		Local $searchRetAvgColorDiffPP = $searchRet[4] ; 66
		Local $searchRetAvgColorDiffCount = $searchRet[5] ; 3713
		Local $searchRetAvgColorDiffPCT = $searchRet[6]
		Local $searchRetHashMatchPCT = $searchRet[7]
		;GUICtrlSetData($lblDbgOut3, "ingame spp " & $searchRet[3])
		;GUICtrlSetData($lblDbgOut3, $searchRet[0] & " , " & $searchRet[1] & " spp " & $searchRetSADPP & " cd " & $searchRetAvgColorDiffPP & " cdc " & $searchRetAvgColorDiffCount & " " & $searchRetAvgColorDiffPCT  & " " & $searchRetHashMatchPCT)
	EndIf

	; if previously we were out of the game, check if we are in the game right now
	if $g_VisibleScreenType == $SCREEN_START_PVP_FIGHT or $g_VisibleScreenType == $SCREEN_UNKNOWN then
		;If IsColorAtPos(1707, 277, 0xB41A1B, 2, 4) or IsColorAtPos(1707, 277, 0xA10E18, 2, 4) Then ; red color
		If $IngameScreen == 1 then
			; cur screen is fighting and last recognized screen is PVP fight, we made a full circle
			if $g_VisibleScreenTypeNoReset == $SCREEN_START_PVP_FIGHT or $g_VisibleScreenTypeNoReset == $SCREEN_UNKNOWN then
				$g_GamesDetected = $g_GamesDetected + 1
				SaveVariables()
				$g_VisibleScreenTypeNoReset = $SCREEN_PVP_FIGHTING
			EndIf
			$g_VisibleScreenType = $SCREEN_PVP_FIGHTING ; from unknown to known
			GUICtrlSetBkColor($lblGamesPlayed2, 0x00FF00)
		EndIf
	EndIf
	; if previously we were in game, check if we are out of the game right now
	if $g_VisibleScreenType == $SCREEN_PVP_FIGHTING or $g_VisibleScreenType == $SCREEN_UNKNOWN then
		;If IsColorAtPos(1707, 277, 0x28283F, 2, 4) Then ; some gray color
		If $StartGameScreen == 1 then
			if $g_VisibleScreenTypeNoReset == $SCREEN_PVP_FIGHTING or $g_VisibleScreenTypeNoReset == $SCREEN_UNKNOWN then
				$g_VisibleScreenTypeNoReset = $SCREEN_START_PVP_FIGHT
			EndIf	
			$g_VisibleScreenType = $SCREEN_START_PVP_FIGHT
			GUICtrlSetBkColor($lblGamesPlayed2, 0xFFBEBE)
		EndIf
	EndIf
	; debugging how many times we loose the guessed screen. It is expected to happen for every loading screen
	if $StartGameScreen == 0 and $IngameScreen == 0 then
		$g_VisibleScreenType = $SCREEN_UNKNOWN
		GUICtrlSetBkColor($lblGamesPlayed2, 0xBEBEFF)
	EndIf
EndFunc

Func UpdateUiGameCounter($bForceUpdate = False)
	; Total games played so far
	Local $GamesSinceLegendary = $g_GamesDetected - $g_GameCountAtPrevLegendary
	Local $WinsSinceLegendary = $g_GamesWon - $g_WinCountAtPrevLegendary
	Local $playedString2 = $g_GamesDetected & " G " & $GamesSinceLegendary & " W " & $WinsSinceLegendary & " L " & $g_LegendariesOpened & " S " & $g_LegendariesSkipped
	GUICtrlSetData($lblGamesPlayed2, $playedString2)
EndFunc

Func InitImageSearchDLLImages()
	Global $g_AppliedColorMaskToCachedImages

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
	$g_imageRanks[12][0] = "L09_0826_0172_0283_0111.bmp"
	$g_imageRanks[12][1] = 9
	$g_imageRanks[13][0] = "L08_0816_0177_0275_0113.bmp"
	$g_imageRanks[13][1] = 8
	$g_imageRanks[14][0] = "L07_0822_0172_0276_0152.bmp"
	$g_imageRanks[14][1] = 7
	$g_imageRanks[15][0] = "L06_0820_0182_0278_0108.bmp"
	$g_imageRanks[15][1] = 6
	$g_imageRanks[16][0] = "L05_0819_0172_0281_0112.bmp"
	$g_imageRanks[16][1] = 5
	$g_imageRanks[17][0] = "L04_0823_0200_0270_0087.bmp"
	$g_imageRanks[17][1] = 4
	$g_imageRanks[18][0] = "L03_0822_0173_0273_0114.bmp"
	$g_imageRanks[18][1] = 3
	$g_imageRanks[19][0] = "L02_0822_0177_0269_0109.bmp"
	$g_imageRanks[19][1] = 2
	$g_imageRanks[20][0] = "L01_0839_0287_0219_0049.bmp"
	$g_imageRanks[20][1] = 1

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

	$g_image1EmeryxCost = "1EmeryxCost_0833_0826_0071_0033.bmp"
	ApplyColorMaskOnCachedImage($g_image1EmeryxCost)
	ImageIsAtRegion($g_image1EmeryxCost, 249, 820, 249+1405, 820+20) ; so that screenshot will contain this region also

	$g_imagePVPStartFight = "PVPStart_0198_0088_0306_0070.bmp"
	ApplyColorMaskOnCachedImage($g_imagePVPStartFight)
	ImageIsAtRegion($g_imagePVPStartFight)

	$g_imagePVPIngame1 = "PVPIngame1_0192_0149_0014_0543.bmp"
	ApplyColorMaskOnCachedImage($g_imagePVPIngame1)
	ImageIsAtRegion($g_imagePVPIngame1)

	$g_imagePVPIngame2 = "PVPIngame2_0192_0149_0014_0543.bmp"
	ApplyColorMaskOnCachedImage($g_imagePVPIngame2)
	ImageIsAtRegion($g_imagePVPIngame2)

	$g_imageVictorySreen = "PVP_Victory_0803_0198_0316_0065.bmp"
	ApplyColorMaskOnCachedImage($g_imageVictorySreen)
	ImageIsAtRegion($g_imageVictorySreen)

	$g_imageDefeatSreen = "PVP_Defeat_0821_0198_0302_0062.bmp"
	ApplyColorMaskOnCachedImage($g_imageDefeatSreen)
	ImageIsAtRegion($g_imageDefeatSreen)

	$g_imageLeagueSreen = "LeagueClose_0206_0089_0332_0048.bmp"
	ApplyColorMaskOnCachedImage($g_imageLeagueSreen)
	ImageIsAtRegion($g_imageLeagueSreen)

	$g_imageGameCrashed = "game_crashed_0791_0138_0181_0033.bmp"
	ApplyColorMaskOnCachedImage($g_imageGameCrashed)
	ImageIsAtRegion($g_imageGameCrashed)

	$g_imagePageReload = "page_reload_0081_0047_0029_0030.bmp"
	ApplyColorMaskOnCachedImage($g_imagePageReload)
	ImageIsAtRegion($g_imagePageReload)

	$g_imageGameloaded = "game_loaded_0918_0825_0080_0042.bmp"
	ApplyColorMaskOnCachedImage($g_imageGameloaded)
	ImageIsAtRegion($g_imageGameloaded)

	$g_imageGameloaded2 = "game_loaded_0821_0760_0276_0053.bmp"
	ApplyColorMaskOnCachedImage($g_imageGameloaded2)
	ImageIsAtRegion($g_imageGameloaded2)

	$g_imageClosePopup1 = "close_popup1_1561_0190_0047_0050.bmp"
	ApplyColorMaskOnCachedImage($g_imageClosePopup1)
	ImageIsAtRegion($g_imageClosePopup1)

	$g_imageClosePopup2 = "close_popup2_1619_0130_0053_0045.bmp"
	ApplyColorMaskOnCachedImage($g_imageClosePopup2)
	ImageIsAtRegion($g_imageClosePopup2)

	$g_imageClosePopup3 = "close_popup3_0205_0089_0068_0039.bmp"
	ApplyColorMaskOnCachedImage($g_imageClosePopup3)
	ImageIsAtRegion($g_imageClosePopup3)

	$g_imagePVPButton = "PVPButton_1481_0807_0165_0077.bmp"
	ApplyColorMaskOnCachedImage($g_imagePVPButton)
	ImageIsAtRegion($g_imagePVPButton)
EndFunc

Func GuessCurrentPVPRank()
	; if script got interrupted, skip executing this function. Happens due to sleeps
	If ( $g_FuncIsRunning <> 1 or $g_BotIsRunning <> 1 ) then 
		return
	EndIf
	; this should get resetted after each fight won/lost
	if $g_LastSeenRank <> 0 then
		return
	EndIf
	
	if $g_VisibleScreenType == $SCREEN_START_PVP_FIGHT then
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

		; for legendary statistics
		If $g_LastSeenRank <> $g_LastSeenRankNoReset And $g_LastSeenRank <> 0 Then
			$g_RanksSeenSinceLastLegendary[$g_LastSeenRank] = $g_RanksSeenSinceLastLegendary[$g_LastSeenRank] + 1
		EndIf
		
		; because we have auto open options
		If $g_LastSeenRank <> 0 Then
			$g_LastSeenRankNoReset = $g_LastSeenRank
			$g_lastPVPScreenTimeStamp = _WinAPI_GetTickCount()
		EndIF
		If $g_LastSeenRankNoReset < $g_LastSeenRankMin Then
			$g_LastSeenRankMin = $g_LastSeenRankNoReset
		EndIf
		
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
	If ( $g_FuncIsRunning <> 1 or $g_BotIsRunning <> 1 ) then 
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
	Local $WinsSinceLegendary = $g_GamesWon - $g_WinCountAtPrevLegendary
	Local $WinsRemainToSpam = $WinsUntilStopSpam - $WinsSinceLegendary
	if $WinsRemainToSpam > $WinsRequiredUntilTargerRank then
		GUICtrlSetData($lblDbgOut, $g_LastSeenRank & ' ' & $WinsRemainToSpam & ' > ' & $WinsRequiredUntilTargerRank)
		DropGame()
		return
	EndIf
	
	; if we know crate cycle index, we can skip a counple of games for sure
	Local $WinsRemainUntilStopSpam = GetSpammedWinCountRemainingForCCI()
	if $WinsRemainUntilStopSpam > $WinsRequiredUntilTargerRank then
		GUICtrlSetData($lblDbgOut2, "CCI drop : " & $WinsRemainUntilStopSpam & ' > ' & $WinsRequiredUntilTargerRank)
		DropGame()
		return
	endif
EndFunc

Func ShouldSpamFightsForLegendary()
	If GUICtrlRead($hCheckboxSpamUntilLegChance) = $GUI_CHECKED then
		; in case we loose track of game rewards, we could fall back to the good old no logic spam X
		Local $WinsSinceLegendary = $g_GamesWon - $g_WinCountAtPrevLegendary
		Local $WinsToSPamUntilStopSpam = getInputNumericValue($inputSpamWinsForLegChance, 100)
		if $WinsSinceLegendary < $WinsToSPamUntilStopSpam Then
			return 1
		EndIf
		; if we know crate index, we know with a higher precision when to stop spamming
		Local $WinsRemainUntilStopSpam = GetSpammedWinCountRemainingForCCI()
		if $WinsRemainUntilStopSpam > 0 then
			return 1
		endif
	EndIf
	return 0
EndFunc

Func OpenChestsWith1Emeryx()
	If ( $g_FuncIsRunning <> 1 or $g_BotIsRunning <> 1 ) then 
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
		;GUICtrlSetData($lblDbgOut5, "OpenChestsWith1Emeryx")
		return 1
	else
		GUICtrlSetData($lblDbgOut2, $searchRet[0] & " , " & $searchRet[1] & " spp " & $searchRetSADPP & " cd" & $searchRetAvgColorDiffPP & " cdc" & $searchRetAvgColorDiffCount & " " & $searchRetAvgColorDiffPCT  & " " & $searchRetHashMatchPCT)
	EndIf
	
	return 0
EndFunc

; out of blue moon once a week this screen gets clicked. Maybe some dust falls on my mouse pad
Func ExitAccidentalLegueScreen()
	Local $searchRet = ImageIsAtRegion($g_imageLeagueSreen)
	Local $searchRetSADPP = $searchRet[3], $searchRetAvgColorDiffPP = $searchRet[4], $searchRetAvgColorDiffCount = $searchRet[5], $searchRetAvgColorDiffPCT = $searchRet[6], $searchRetHashMatchPCT = $searchRet[7]
	If $searchRet[0] > 0 and $searchRetSADPP <= 4 then
        MyMouseClick("left", 245, 120)
		Sleep(1000)
    EndIf
EndFunc

Func ReloadCrashedGame()
	Global $g_VisibleScreenType, $g_FuncIsRunning, $g_BotIsRunning
	Local $timeSincePVPScreen = _WinAPI_GetTickCount() - $g_lastPVPScreenTimeStamp
	Local $searchRet = ImageIsAtRegion($g_imageGameCrashed)
	Local $searchRetSADPP = $searchRet[3], $searchRetAvgColorDiffPP = $searchRet[4], $searchRetAvgColorDiffCount = $searchRet[5], $searchRetAvgColorDiffPCT = $searchRet[6], $searchRetHashMatchPCT = $searchRet[7]
	If ($searchRet[0] > 0 and $searchRetSADPP <= 4) or ($timeSincePVPScreen > 10 * 60 * 1000) then
		$g_lastPVPScreenTimeStamp = _WinAPI_GetTickCount()
		GUICtrlSetData($lblDbgOut2, "gc x: " & $searchRet[1] & "y=" & $searchRet[2] & ",sad=" & $searchRetSADPP & "cd=" & $searchRetAvgColorDiffPP & " " & $searchRetAvgColorDiffCount & " " & $searchRetAvgColorDiffPCT  & " " & $searchRetHashMatchPCT)
		; click the page reload
        MyMouseClick("left", 95, 60)
		Sleep(1000)
		; wait at least 30 seconds - maybe 15 is enough
		Local $GameLoaded = 0
		while($GameLoaded == 0 and $g_FuncIsRunning == 1 and $g_BotIsRunning == 1)
			Sleep(15*1000)
			Local $searchRet = ImageIsAtRegion($g_imageGameloaded)
			Local $searchRetSADPP = $searchRet[3], $searchRetAvgColorDiffPP = $searchRet[4], $searchRetAvgColorDiffCount = $searchRet[5], $searchRetAvgColorDiffPCT = $searchRet[6], $searchRetHashMatchPCT = $searchRet[7]
			GUICtrlSetData($lblDbgOut2, "gl x: " & $searchRet[0] & "y=" & $searchRet[1] & ",sad=" & $searchRetSADPP & "cd=" & $searchRetAvgColorDiffPP & " " & $searchRetAvgColorDiffCount & " " & $searchRetAvgColorDiffPCT  & " " & $searchRetHashMatchPCT)
			If $searchRet[0] > 0 and $searchRetSADPP <= 4 then
				$GameLoaded = 1
				MyMouseClick("left", $searchRet[0] + 10, $searchRet[1] + 10)
				ExitLoop
			EndIf
			$searchRet = ImageIsAtRegion($g_imageGameloaded2)
			$searchRetSADPP = $searchRet[3]
			If $searchRet[0] > 0 and $searchRetSADPP <= 4 then
				$GameLoaded = 1
				MyMouseClick("left", $searchRet[0] + 10, $searchRet[1] + 10)
				ExitLoop
			EndIf
		wend
		Sleep(2*1000)
		Local $ClosedAllPopupsOrClickedPVP = 0
		Local $timeStampStartedClicking = _WinAPI_GetTickCount()
		while($ClosedAllPopupsOrClickedPVP == 0 and $g_FuncIsRunning == 1 and $g_BotIsRunning == 1 and _WinAPI_GetTickCount() - $timeStampStartedClicking < 2 * 60 * 1000)
			; close splash popups by force clicking the PVP button
;			Local $popupImages[3] = [$g_imageClosePopup1, $g_imageClosePopup2, $g_imagePVPButton]
;			For $i = 0 To UBound($popupImages) - 1
;				$searchRet = ImageIsAtRegion($popupImages[$i])
;				GUICtrlSetData($lblDbgOut2, "pp x: " & $searchRet[0] & "y=" & $searchRet[1] & ",sad=" & $searchRetSADPP & "cd=" & $searchRetAvgColorDiffPP & " " & $searchRetAvgColorDiffCount & " " & $searchRetAvgColorDiffPCT  & " " & $searchRetHashMatchPCT)
;				If $searchRet[0] > 0 And $searchRet[3] <= 4 Then
;					MyMouseClick("left", 1654, 875)
;					Sleep(1000)
;				EndIf
;			Next
;			; close accidentally opened "scene" screen
;			Local $popupImages[1] = [$g_imageClosePopup3]
;			For $i = 0 To UBound($popupImages) - 1
;				$searchRet = ImageIsAtRegion($popupImages[$i])
;				GUICtrlSetData($lblDbgOut2, "pp x: " & $searchRet[0] & "y=" & $searchRet[1] & ",sad=" & $searchRetSADPP & "cd=" & $searchRetAvgColorDiffPP & " " & $searchRetAvgColorDiffCount & " " & $searchRetAvgColorDiffPCT  & " " & $searchRetHashMatchPCT)
;				If $searchRet[0] > 0 And $searchRet[3] <= 4 Then
;					MyMouseClick("left", $searchRet[0] + 10, $searchRet[1] + 10)
;					Sleep(1000)
;				EndIf
;			Next
			; try to click the PVP figh button
			MyMouseClick("left", 1654, 875)
			Sleep(2000)
			; if we can see the PVP screen, we can exit this loop
			GuessIngameVisibleScreen()
			if $g_VisibleScreenType == $SCREEN_START_PVP_FIGHT then
				$ClosedAllPopupsOrClickedPVP = 1
			EndIf
		wend
    EndIf
EndFunc

Func GetSpammedWinCountRemainingForCCI()
	Local $WinsRemainUntilStopSpam = 0
	; when you did not gather enough data to know cci. Spam some games to know your cci
	If $g_CrateCycleIndex < 0 then
		return 0
	EndIf
	if $g_FoundLegThisCycle == 1 then 
		$WinsRemainUntilStopSpam = getInputNumericValue($inputSpamCombatUntilCCINoLeg, 100) + ( $gc_CrateCycleLen - $g_CrateCycleIndex ) ; have to reach next cycle. add remaining games to be played
	else 
		$WinsRemainUntilStopSpam = getInputNumericValue($inputSpamCombatUntilCCINoLeg, 100) - $g_CrateCycleIndex
	EndIf
	return $WinsRemainUntilStopSpam
EndFunc

Func GetEstimatedTimeUntilSpamTarget()
	Local $EstimatedSecPerGame = getInputNumericValue($inputSingleGameDurationSec, 120)
	Local $RemainingWinsForCycle = GetSpammedWinCountRemainingForCCI()
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
	If GetSpammedWinCountRemainingForCCI() > 0 and $SecondsRemainUntilSpamTarget < getInputNumericValue($inputPauseOpensIfSpamSecRemains, 0) then
		; GUICtrlSetData($lblDbgOut3, "wins: " & GetSpammedWinCountRemainingForCCI() & " m: " & int($SecondsRemainUntilSpamTarget/60) & " < " & int(getInputNumericValue($inputPauseOpensIfSpamSecRemains, 0) / 60))
		return 0
	EndIf
	; no idea which case we failed to test
	return 1
EndFunc

Func SpamGameForHeliQuest()
	If ( $g_FuncIsRunning <> 1 or $g_BotIsRunning <> 1 ) then 
		return
	EndIf
	; are we ingame ?
	if $g_VisibleScreenType <> $SCREEN_START_PVP_FIGHT then
		return
	EndIf
	; there is the quest win X games
	if getInputNumericValue($inputSpamLimit, 0) > $g_GamesDetected Then
		Fight2()
	Else
		GUICtrlSetData($inputSpamLimit, "")
	EndIf
EndFunc

Func UpdateUITimeEstimates()
	Local $AvgGamesToSpam = getInputNumericValue($inputSpamCombatUntilCCINoLeg, 0)
	Local $AvgTimeForSpam = $AvgGamesToSpam * getInputNumericValue($inputSingleGameDurationSec, 0)
	Local $ChanceForFullSpam = getInputNumericValue($inputSpamCombatUntilCCINoLeg, 0) / $gc_CrateCycleLen
	Local $ChanceCompensatedTimeForSpam = $AvgTimeForSpam * ( 1 + $ChanceForFullSpam )
	
	Local $GamesRemainToSpam = GetSpammedWinCountRemainingForCCI()
	Local $SpamTimeRemain = $GamesRemainToSpam * getInputNumericValue($inputSingleGameDurationSec, 0)
	
	Local $AvgChestOpenTime = ($CrateCountBlue * 2 * 60 + $CrateCountBlue2 * 4 * 60 + $CrateCountPurple * 5 * 60 + $CrateCountGold * 10 * 60) / ($CrateCountBlue+$CrateCountBlue2+$CrateCountPurple+$CrateCountGold)
	Local $HardcodedChestSpeedup = 1 - 0.22
	Local $HardcodedChestSlots = 8
	Local $AvgChestsToOpen = $gc_CrateCycleLen - getInputNumericValue($inputSpamCombatUntilCCINoLeg, 0)
	Local $AvgChestsToOpenTime = $AvgChestsToOpen * $AvgChestOpenTime * $HardcodedChestSpeedup / $HardcodedChestSlots
	Local $RemainingChestsToOpen = $gc_CrateCycleLen - $g_CrateCycleIndex
	Local $RemainingChestsToOpenTime = $RemainingChestsToOpen * $AvgChestOpenTime * $HardcodedChestSpeedup / $HardcodedChestSlots
	GUICtrlSetData($lblTimeEstimates, "ST " & Intf($ChanceCompensatedTimeForSpam/60/60,1) & "/" & Intf($SpamTimeRemain/60/60,1) & " PT " & Intf($AvgChestsToOpenTime/60,1) & "/" & Intf($RemainingChestsToOpenTime/60,1))
EndFunc

func CheckDropGameAlwaysToMaintainRank()
	If ( $g_FuncIsRunning <> 1 or $g_BotIsRunning <> 1 ) then 
		return
	endif
	if $g_LastSeenRank <> 0 then
			local $MaintainSpecificRank = getInputNumericValue($inputFixedRankUsed, 0)
			if $MaintainSpecificRank > 0 and $MaintainSpecificRank > $g_LastSeenRank then
				GUICtrlSetData($lblDbgOut, "should drop: " & $g_LastSeenRank & ' < ' & $MaintainSpecificRank)
				if $g_VisibleScreenType == $SCREEN_PVP_FIGHTING then
					DropGame()
				else
					Fight()
				endif
			endif
	endif
endfunc

Func ForeverLoopFunc()
	Global $g_VisibleScreenType, $g_FuncIsRunning, $g_BotIsRunning

	$g_FuncIsRunning = 1 - $g_FuncIsRunning

	if $g_FuncIsRunning == 1 then
		GUICtrlSetBkColor($lblCrateCycleCrates, 0x00FF00)
	EndIf	
	while( $g_FuncIsRunning == 1 and $g_BotIsRunning == 1 )	
		; else you can't click on buttons
		HandleGUIMessages()
		
		;GUICtrlSetData($lblDbgOut, Hex( PixelGetColor(1707, 277), 6 ) & " -- " & $g_VisibleScreenType)

		;most functions depend on these 2 functions. make sure info is available in the loop
		GuessIngameVisibleScreen()
		if $g_VisibleScreenType == $SCREEN_START_PVP_FIGHT then
			GuessCurrentPVPRank() ; internally will handle states based on ingame or outofgame screens
		EndIf
		
		ExitAccidentalLegueScreen()
		ReloadCrashedGame()
		CheckDropGameAlwaysToMaintainRank()
		
		; things to do while out of fight screen
		if $g_VisibleScreenType == $SCREEN_START_PVP_FIGHT then
			
			; probably just for some quests. Open multiple chests without fighting because we might miss some
			while OpenChestsWith1Emeryx() == 1
			WEnd
			
			; do not open chests until we are in a very low rank ?
			if AllowedToOpenChests() == 1 or $g_LastSeenRankNoReset - 2 <= $g_LastSeenRankMin then 
				OpenWinstreakChests() ; only do these at low rank
			EndIf
			if AllowedToOpenChests() == 1 then 
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
		If ( $g_FuncIsRunning == 1 and $g_BotIsRunning == 1 ) then 
			Sleep(500)
		EndIf
	wend

	GUICtrlSetBkColor($lblCrateCycleCrates, 0xFF7777)
	
EndFunc

Func PasteClipboard()
    Local $clipboardText = ClipGet()

    If $clipboardText = "" Then
        MsgBox(0, "Error", "Clipboard is empty!")
        Return
    EndIf

    For $i = 1 To StringLen($clipboardText)
        Local $char = StringMid($clipboardText, $i, 1)

        ; Send character with escaping for special characters
        Switch $char
            Case "!"
                Send("{!}")
            Case "+"
                Send("{+}")
            Case "^"
                Send("{^}")
            Case "~"
                Send("{~}")
            Case "{"
                Send("{{}")
            Case "}"
                Send("{}}")
            Case "["
                Send("{[}")
            Case "]"
                Send("{]}")
            Case "("
                Send("{(}")
            Case ")"
                Send("{)}")
            Case @CR, @LF
                Send("{ENTER}")
			Case "I"
				Send("i") ; avoid triggering hotkey
			Case "P"
				Send("p") ; avoid triggering hotkey
			Case "A"
				Send("a") ; avoid triggering hotkey
			Case "Q"
				Send("q") ; avoid triggering hotkey
			Case "W"
				Send("w") ; avoid triggering hotkey
            Case Else
                Send($char)
        EndSwitch

        Sleep(50) ; simulate natural typing speed
    Next
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