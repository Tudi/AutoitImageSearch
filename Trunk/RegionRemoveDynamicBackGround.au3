#include "MouseOnEvent.au3"

;HotKeySet("{ESC}", "RestoreMouse")
HotKeySet("9", "TakeScreenshotsWithMouse")
HotKeySet("-", "SaveImage")
HotKeySet("=", "EndScript")

HotKeySet("5", "TestCanFindImage")

global $dllhandle = DllOpen ( "Debug/ImageSearchDLL.dll" )

global $IsInitialScreenshot = 1
Global $NrOfMouseClicks
global $MouseCoords[2][2]
global $ToggleLoop = 0
global $InitialWidth, $InitialHeight

; select with mouse a region to take screenshots
; calling this function multiple times will record the same area. This will be used to remove blinking or changing parts of the image
func TakeScreenshotsWithMouse()
	$NrOfMouseClicks = 0
	
	; Disable mouse clicks so we can select a region on the target window without clicking anything
	_MouseSetOnEvent($MOUSE_PRIMARYDOWN_EVENT, "_MousePrimaryDown_Event")
	
	; wait until we make the 2 clicks
	while( $NrOfMouseClicks < 2 )
		Sleep(100)
	wend
		
	; restore mouse movement
	_MouseSetOnEvent($MOUSE_PRIMARYDOWN_EVENT)
	
	; take the screenshot
	local $x1 = $MouseCoords[0][0]
	local $y1 = $MouseCoords[0][1]
	local $x2 = $MouseCoords[1][0]
	local $y2 = $MouseCoords[1][1]
	if($IsInitialScreenshot == 0) then
		if($MouseCoords[1][0] - $MouseCoords[0][0] >= $InitialWidth or $MouseCoords[1][1] - $MouseCoords[0][1] >= $InitialHeight) then
			$result = DllCall( $dllhandle, "NONE", "TakeScreenshotRemoveDynamicPixelsAutoPosition", "int", $x1, "int", $y1, "int", $x2, "int", $y2 )
			MsgBox(64, "", "Took refining screeenshot of area : [" & $MouseCoords[0][0] & "-" & $MouseCoords[0][1] & "][" & $MouseCoords[1][0] & "-" & $MouseCoords[1][1] & "]" & $result)
		else
			MsgBox(64, "", "You need to select a larger area so we can search initial image in it")
		endif
	endif
	if($IsInitialScreenshot == 1) then
		$result = DllCall( $dllhandle, "NONE", "TakeScreenshotDetectUniquePixelsInitial", "int", $x1, "int", $y1, "int", $x2, "int", $y2 )
		MsgBox(64, "", "Took initial screeenshot of area : [" & $MouseCoords[0][0] & "-" & $MouseCoords[0][1] & "][" & $MouseCoords[1][0] & "-" & $MouseCoords[1][1] & "]")

		$InitialWidth = $x2 - $x1
		$InitialHeight = $y2 - $y1
		$IsInitialScreenshot = 0
	endif
endfunc

func _MousePrimaryDown_Event()
	if( $NrOfMouseClicks >= 2 ) then
		return
	endif
	$MouseCoords[$NrOfMouseClicks][0] = MouseGetPos( 0 ) ; X
	$MouseCoords[$NrOfMouseClicks][1] = MouseGetPos( 1 ) ; Y
;	MsgBox(64, "", "click pos is " & $NrOfMouseClicks & "-" & $MouseCoords[$NrOfMouseClicks][0] & "-" & $MouseCoords[$NrOfMouseClicks][1], 5)
	$NrOfMouseClicks = $NrOfMouseClicks + 1

	; block the click. No need to change window in the background
	Return $MOE_BLOCKDEFPROC
endfunc

func SaveImage()
	global $dllhandle
	$result = DllCall( $dllhandle, "NONE", "SaveScreenshotUniqueFeatures" )
	MsgBox(64, "", "Saved screenshot current state" )
endfunc

func RestoreMouse()
	_MouseSetOnEvent($MOUSE_PRIMARYDOWN_EVENT)
endfunc

func EndScript()
	global $dllhandle
	DllClose ( $dllhandle )
	exit
endfunc

while(1)
	Sleep(1000)
wend

; this is a one time test. You would need to make your own test image ...
func TestCanFindImage()
	$ColorToleranceFaultsAccepted = 10
	$ExitAfterNMatchesFound = 1
	if($IsInitialScreenshot>0) then
		$result = DllCall( $dllhandle,"NONE","TakeScreenshot","int",0,"int",0,"int",1800,"int",1800)
	else
		$result = DllCall( $dllhandle,"NONE","TakeScreenshot","int",$MouseCoords[0][0] - 10,"int",$MouseCoords[0][1] - 10,"int",$MouseCoords[1][0] + 10,"int",$MouseCoords[1][1] + 10)
	endif
	$result = DllCall( $dllhandle,"str","ImageSearchOnScreenshotUniqueFeatures","str","temp.bmp","int",$ColorToleranceFaultsAccepted,"int",$ExitAfterNMatchesFound)
	$result = DllCall( $dllhandle,"NONE","SaveScreenshot")
	HandleResult( $result )
endfunc

func HandleResult( $result )
	$array = StringSplit($result[0],"|")
	$resCount = Number( $array[1] )
	MsgBox( $MB_SYSTEMMODAL, "", "res count " & $resCount )
	if( $resCount > 0 ) then
		$x=Int(Number($array[2]))
		$y=Int(Number($array[3]))
		MouseMove( $x, $y );
		MsgBox( $MB_SYSTEMMODAL, "", "found at " & $x & " " & $y )
	endif
endfunc