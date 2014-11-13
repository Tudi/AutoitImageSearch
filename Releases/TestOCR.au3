;Testing it here : 123.123,123,.123
;In my editor the above text is green. If i place my mouse before "1" and push "[", it will write out 12312

Opt('MustDeclareVars', 1)
global $MB_SYSTEMMODAL = 4096

global $IsRunning = 1
HotKeySet("=", "Terminate")
Func Terminate()
    $IsRunning = 0
EndFunc  

HotKeySet("[", "Read6CharsAtMouse")

global $dllhandle = DllOpen ( "Debug/ImageSearchDLL.dll" )

DllCall( $dllhandle,"str", "RegisterOCRFont","str", "OCR_1_green.bmp", "int", Asc( '1' ) )
DllCall( $dllhandle,"str", "RegisterOCRFont","str", "OCR_2_green.bmp", "int", Asc( '2' ) )
DllCall( $dllhandle,"str", "RegisterOCRFont","str", "OCR_3_green.bmp", "int", Asc( '3' ) )

While $IsRunning == 1
    Sleep(100)
WEnd

DllClose ( $dllhandle )

Func Read6CharsAtMouse()
	local $MousePos = MouseGetPos()
	local $result
	
	DllCall( $dllhandle,"str","TakeScreenshot", "int", $MousePos[0], "int", $MousePos[1] - 8, "int", $MousePos[0] + 6 * 8 + 5, "int", $MousePos[1] + 8 + 5)
	DllCall( $dllhandle,"NONE","SaveScreenshot")
	;MsgBox( $MB_SYSTEMMODAL, "", " searchbox is " & $MousePos[0] & "," & $MousePos[1] - 8 & "," & $MousePos[0] + 6 * 8 + 5 & "," & $MousePos[1] + 8 + 5)
	$result = DllCall( $dllhandle,"str","ReadTextFromScreenshot", "int", $MousePos[0], "int", $MousePos[1] - 8, "int", $MousePos[0] + 6 * 8 + 5, "int", $MousePos[1] + 8 + 5)
	MsgBox( $MB_SYSTEMMODAL, "", " result is " & $result[0])
EndFunc
