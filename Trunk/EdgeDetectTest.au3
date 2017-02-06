#include "MouseOnEvent.au3"
#RequireAdmin

;HotKeySet("{ESC}", "RestoreMouse")
HotKeySet("9", "TakeScreenshotsWithMouse")
HotKeySet("=", "EndScript")

global $dllhandle = DllOpen ( "Debug/ImageSearchDLL.dll" )

Global $NrOfMouseClicks
global $MouseCoords[2][2]

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
	MsgBox(64, "", "Took initial screeenshot of area : [" & $MouseCoords[0][0] & "-" & $MouseCoords[0][1] & "][" & $MouseCoords[1][0] & "-" & $MouseCoords[1][1] & "]")
;	$result = DllCall( $dllhandle, "NONE", "TakeScreenshot", "int", $x1, "int", $y1, "int", $x2, "int", $y2 );
;	$result = DllCall( $dllhandle, "NONE", "SaveScreenshot" )
;	$result = DllCall( $dllhandle,"NONE","ApplyColorBitmask","int", 0x00F0F0F0)
;	return;
	
	$result = DllCall( $dllhandle, "NONE", "TakeScreenshot", "int", $x1, "int", $y1, "int", $x2, "int", $y2 )
	$result = DllCall( $dllhandle, "NONE", "SaveScreenshot" )
	$result = DllCall( $dllhandle, "NONE", "EdgeDetectRobertCross3Channels" )
	$result = DllCall( $dllhandle, "NONE", "SaveScreenshot" )		
	$result = DllCall( $dllhandle, "NONE", "ConvertToGrayScaleMaxChannel" )
	$result = DllCall( $dllhandle, "NONE", "SaveScreenshot" )		
	$result = DllCall( $dllhandle, "NONE", "EdgeBleed" )
	$result = DllCall( $dllhandle, "NONE", "SaveScreenshot" )		

	$result = DllCall( $dllhandle, "NONE", "TakeScreenshot", "int", $x1, "int", $y1, "int", $x2, "int", $y2 )
	$result = DllCall( $dllhandle, "NONE", "ConvertToGrayScale" )
	$result = DllCall( $dllhandle, "NONE", "SaveScreenshot" )		
	$result = DllCall( $dllhandle, "NONE", "EdgeDetectRobertCross1Channel" )
	$result = DllCall( $dllhandle, "NONE", "SaveScreenshot" )		

	$result = DllCall( $dllhandle, "NONE", "TakeScreenshot", "int", $x1, "int", $y1, "int", $x2, "int", $y2 )
	$result = DllCall( $dllhandle, "NONE", "EdgeDetectSobel3Channels" )
	$result = DllCall( $dllhandle, "NONE", "SaveScreenshot" )		
	$result = DllCall( $dllhandle, "NONE", "ConvertToGrayScaleMaxChannel" )
	$result = DllCall( $dllhandle, "NONE", "SaveScreenshot" )		
	$result = DllCall( $dllhandle, "NONE", "EdgeBleed" )
	$result = DllCall( $dllhandle, "NONE", "SaveScreenshot" )		

	$result = DllCall( $dllhandle, "NONE", "TakeScreenshot", "int", $x1, "int", $y1, "int", $x2, "int", $y2 )
	$result = DllCall( $dllhandle, "NONE", "EdgeDetectRobertCross3Channels" )
	$result = DllCall( $dllhandle, "NONE", "ConvertToGrayScaleMaxChannel" )
	$result = DllCall( $dllhandle, "NONE", "EdgeKeepLocalMaximaMaximaOnly", "int", 2 )
	$result = DllCall( $dllhandle, "NONE", "SaveScreenshot" )		

	$result = DllCall( $dllhandle, "NONE", "TakeScreenshot", "int", $x1, "int", $y1, "int", $x2, "int", $y2 )
	$result = DllCall( $dllhandle, "NONE", "SaveScreenshot" )		
	$result = DllCall( $dllhandle, "NONE", "TakeScreenshot", "int", $x1, "int", $y1, "int", $x2, "int", $y2 )
	$result = DllCall( $dllhandle, "NONE", "EdgeDetectRobertCross3Channels" )
	$result = DllCall( $dllhandle, "NONE", "ConvertToGrayScaleMaxChannel" )
	$result = DllCall( $dllhandle, "NONE", "EdgeKeepLocalMaximaMaximaOnly", "int", 2 )
	$result = DllCall( $dllhandle, "NONE", "EdgeCopyOriginalForEdges" )
	$result = DllCall( $dllhandle, "NONE", "SaveScreenshot" )		
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
