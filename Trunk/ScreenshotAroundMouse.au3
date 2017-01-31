#include "MouseOnEvent.au3"

HotKeySet("9", "TakeScreenshotsWithMouse")
HotKeySet("=", "EndScript")

global $dllhandle = DllOpen ( "Debug/ImageSearchDLL.dll" )

func TakeScreenshotsWithMouse()
	global $dllhandle
	$x1 = MouseGetPos( 0 ) ; X
	$y1 = MouseGetPos( 1 ) ; Y

	$result = DllCall( $dllhandle, "NONE", "TakeScreenshot", "int", $x1 - 7, "int", $y1 - 7, "int", $x1 + 8, "int", $y1 + 8 )
	$result = DllCall( $dllhandle, "NONE", "SaveScreenshot" )

	$result = DllCall( $dllhandle, "NONE", "TakeScreenshot", "int", $x1 - 8, "int", $y1 - 8, "int", $x1 + 8, "int", $y1 + 8 )
	$result = DllCall( $dllhandle, "NONE", "SaveScreenshot" )
endfunc

func EndScript()
	global $dllhandle
	DllClose ( $dllhandle )
	exit
endfunc

while(1)
	Sleep(1000)
wend

DllClose ( $dllhandle )
