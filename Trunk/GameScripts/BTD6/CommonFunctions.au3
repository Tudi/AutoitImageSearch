#include <AutoItConstants.au3>
#include <date.au3>
#include <WinAPISys.au3>

Func ResetDebugLog($sFileName)
   Local $hFilehandle = FileOpen($sFileName, $FO_OVERWRITE)
   FileClose($hFilehandle)
endfunc

Func _WriteDebugLog($sFileName, $TextToLog)

   local $sWriteOption
   If FileExists($sFileName) Then
      $sWriteOption = $FO_APPEND
   Else
      $sWriteOption = $FO_OVERWRITE
   EndIf

   Local $hFilehandle = FileOpen($sFileName, $sWriteOption)
   Local $sLogEntry = _Now() & " " & $TextToLog & @CRLF
   FileWrite($hFilehandle, $sLogEntry)
   FileClose($hFilehandle)
	;MsgBox( 64, "", "found at " & $Pos[0] & " " & $Pos[1] & " SAD " & $Pos[2])
EndFunc

func GetKoPlayer()
	Local $hWnd = WinGetHandle("[Title:KOPLAYER 1.4.1052")
	if( @error ) then
		$hWnd = WinGetHandle("[CLASS:Qt5QWindowIcon]")
	endif
	return $hWnd
endfunc

func GetWindowRelativPos()
;	Local $hWnd = GetKoPlayer()
;	Local $aPos = WinGetPos( $hWnd ) ; x, y, w, h
	Local $bPos[5]
	$bPos[0] = 0;
	$bPos[1] = 0;
	$bPos[2] = 0;
	$bPos[3] = 0;
;	$bPos[0] = $aPos[0] + 2; there is a left pannel that can change it's size. Seems like it pinches off the content 2 pixels
;	$bPos[1] = $aPos[1] + 38 ; content of the player starts below the menu bar
;	$bPos[2] = $aPos[2] - 63 ; Borders
;	$bPos[3] = $aPos[3] - 38 - 2 - 1; More borders
;	$bPos[4] = $hWnd
	return $bPos
endfunc

func ReduceColorPrecision( $color, $Mask = 0 )
	if( $Mask == 0 ) then
		$Mask = 0x00F0F0F0
	endif
	return BitAND( $color, $Mask )
endfunc

; just in case our positioning is not perfect, Pixel getcolor should still work okish
func IsPixelAroundPos( $x, $y, $Color, $Mask = 0, $Radius = 0, $RelativeCords = 0 )
	if( $Radius == 0 ) then
		$Radius = 2
	endif
	if( $Mask == 0 ) then
		$Mask = 0x00F0F0F0
	endif
	if( $RelativeCords <> 0) then
		Local $aPos = GetWindowRelativPos()
		$x = $x + $aPos[0];
		$y = $y + $aPos[1];
	endif
	$Color = ReduceColorPrecision( $Color, $Mask )
	for $y2 = $y - $Radius to $y + $Radius
		for $x2 = $x - $Radius to $x + $Radius
			local $col = PixelGetColor( $x2, $y2 )
			$col = ReduceColorPrecision( $col, $Mask )
;			MouseMove( $x2, $y2 )
;			FileWriteLine ( "PixelsAroundMouse.txt", Hex($Color) & "!=" & Hex($col) & " Mask " & Hex($Mask) & " rad " & $Radius )
			if( $col == $Color ) then 
;				FileWriteLine ( "PixelsAroundMouse.txt", "Matched" )
				return 1
			endif
		next
	next
	return 0
endfunc

; used for loading screens
func WaitImageAppear( $ImageName, $X = -1, $Y = -1, $Sleep = 500, $Timout = 3000 )
	if( $x == -1 ) then
		GetCoordFromImageFileName( $ImageName, $x, $y, 0 )
	endif
	local $Pos = ImageIsAt($ImageName, $X, $Y)
	;MsgBox( 64, "", "found at " & $Pos[0] & " " & $Pos[1] & " SAD " & $Pos[2])
	while( $Pos[2] > 32 * 32 * 3 * 10 and $Timout > 0 )
		Sleep( $Sleep ) ; wait for the window to refresh
		$Pos = ImageIsAt($ImageName, $X, $Y)
		$Timout = $Timout - $Sleep
	wend
endfunc

; used for loading screens
func WaitImageDisappear( $ImageName, $X = -1, $Y = -1, $Sleep = 500, $Timout = 3000 )
	if( $x == -1 ) then
		GetCoordFromImageFileName( $ImageName, $x, $y, 0 )
	endif
	local $Pos = ImageIsAt($ImageName, $X, $Y)
	;MsgBox( 64, "", "found at " & $Pos[0] & " " & $Pos[1] & " SAD " & $Pos[2])
	while( $Pos[2] <= 32 * 32 * 3 * 10 and $Timout > 0 )
		Sleep( $Sleep ) ; wait for the window to refresh
		$Pos = ImageIsAt($ImageName, $X, $Y)
		$Timout = $Timout - $Sleep
	wend
endfunc

func ClickButtonIfAvailable( $ImageName, $X = -1, $Y = -1, $SearchRadius = 0, $SleepBetweenRetrys = 500, $retryCount = 1000, $AcceptableSAD = 0)
	if( $x == -1 ) then
		GetCoordFromImageFileName( $ImageName, $x, $y )
	endif
	while( $retryCount > 0 )
		_WriteDebugLog("log.txt", "new search name=" & $ImageName & " x=" & $x & " y=" & $y & " SearchRadius=" & $SearchRadius & " SleepBetweenRetrys=" & $SleepBetweenRetrys & " retryCount=" & $retryCount & " AcceptableSAD=" & $AcceptableSAD)
		local $startStamp = _WinAPI_GetTickCount()
		local $Pos = ImageIsAt($ImageName, $X, $Y, $SearchRadius, $AcceptableSAD)
		local $searchDuration = _WinAPI_GetTickCount() - $startStamp
		_WriteDebugLog("log.txt", "found at " & $Pos[0] & " " & $Pos[1] & " SAD " & $Pos[2] & " searching took " & $searchDuration & " ms")
		; if SAD is small enough, we consider it as found
		if( $Pos[0] > 0 and $Pos[2] <= $AcceptableSAD ) then
			MouseClick( $MOUSE_CLICK_LEFT, $Pos[0] + 16, $Pos[1] + 16, 1, 1 )
			return 1
		endif
		$retryCount = $retryCount - 1
		if $retryCount > 0 then
			Sleep( $SleepBetweenRetrys ) ; wait for the window to refresh
		endif
	wend
	return 0
endfunc

Func _MouseClickPlus($Window, $Button = "left", $X = "", $Y = "", $Clicks = 1)
    MsgBox(1, "", "112333")
    Local $MK_LBUTTON = 0x0001
    Local $WM_LBUTTONDOWN = 0x0201
    Local $WM_LBUTTONUP = 0x0202

    Local $MK_RBUTTON = 0x0002
    Local $WM_RBUTTONDOWN = 0x0204
    Local $WM_RBUTTONUP = 0x0205

    Local $WM_MOUSEMOVE = 0x0200

    Local $i = 0

    Select
        Case $Button = "left"
            $Button = $MK_LBUTTON
            $ButtonDown = $WM_LBUTTONDOWN
            $ButtonUp = $WM_LBUTTONUP
        Case $Button = "right"
            $Button = $MK_RBUTTON
            $ButtonDown = $WM_RBUTTONDOWN
            $ButtonUp = $WM_RBUTTONUP
    EndSelect

    If $X = "" Or $Y = "" Then
        $MouseCoord = MouseGetPos()
        $X = $MouseCoord[0]
        $Y = $MouseCoord[1]
    EndIf

    For $i = 1 To $Clicks
        DllCall("user32.dll", "int", "SendMessage", _
                "hwnd", WinGetHandle($Window), _
                "int", $WM_MOUSEMOVE, _
                "int", 0, _
                "long", _MakeLong($X, $Y))

        DllCall("user32.dll", "int", "SendMessage", _
                "hwnd", WinGetHandle($Window), _
                "int", $ButtonDown, _
                "int", $Button, _
                "long", _MakeLong($X, $Y))
		
		Sleep( 100 )
		
        DllCall("user32.dll", "int", "SendMessage", _
                "hwnd", WinGetHandle($Window), _
                "int", $ButtonUp, _
                "int", $Button, _
                "long", _MakeLong($X, $Y))
    Next
EndFunc  ;==>_MouseClickPlus

Func _MakeLong($LoWord, $HiWord)
    Return BitOR($HiWord * 0x10000, BitAND($LoWord, 0xFFFF))
EndFunc  ;==>_MakeLong

Func GetCoordFromImageFileName( $ImgName, ByRef $x, ByRef $y, $AbsoluteCoord = 0 )
	local $array = StringSplit($ImgName,"_")
	local $resCount = $array[0]
	$x=Int(Number($array[$resCount-1]))
	$y=Int(Number($array[$resCount-0]))
	if( $AbsoluteCoord <> 0 ) then
		Local $aPos = GetWindowRelativPos()
		$x=$aPos[0] + $x
		$y=$apos[1] + $y
	endif
endfunc

Func ImageIsAt( $ImgName, $x = -1, $y = -1, $Radius = 0, $AcceptableSAD = 0)
	; in case the image file name in a common sense format we can extract the coordinate of it
	if( $x == -1 ) then
		GetCoordFromImageFileName( $ImgName, $x, $y )
	endif
	global $dllhandle
	Local $aPos = GetWindowRelativPos()
	local $x2 = $x + $aPos[0]
	local $y2 = $y + $aPos[1]
	local $start_x = $x2 - $Radius
	local $start_y = $y2 - $Radius
	local $end_x = $x2 + $Radius
	local $end_y = $y2 + $Radius
	
	if $start_x < 0 then $start_x = 0
	if $start_y < 0 then $start_y = 0
	if $end_x > @DesktopWidth then $end_x = @DesktopWidth
	if $end_y > @DesktopHeight then $end_y = @DesktopHeight

	local $result = DllCall( $dllhandle, "NONE", "TakeScreenshot", "int", $start_x, "int", $start_y, "int", $end_x, "int", $end_y)
;	$result = DllCall( $dllhandle, "NONE", "ApplyColorBitmask", "int", 0x00F0F0F0) ; remove small aberations due to color merge
;	$result = DllCall( $dllhandle, "str", "ImageSearch_SAD", "str", $ImgName)
	$result = DllCall( $dllhandle, "str", "ImageSearch_SAD_Limit", "str", $ImgName, "int", $AcceptableSAD)
	; put back previous screenshot. Maybe we were parsing it
;	DllCall( $dllhandle, "str", "CycleScreenshots")
	local $res = SearchResultToVectSingleRes( $result )
	return $res
endfunc

func SearchResultToVectSingleRes( $result )
	local $array = StringSplit($result[0],"|")
	local $resCount = Number( $array[1] )
	;MsgBox( 64, "", "res count " & $resCount )
	local $ret[3]
	$ret[0]=-1
	$ret[1]=-1
	$ret[2]=-1
	if( $resCount > 0 ) then
		$ret[0]=Int(Number($array[2]))
		$ret[1]=Int(Number($array[3]))
		$ret[2]=Int(Number($array[4]))	; SAD
		;MouseMove( $ret[0], $ret[1] );
		;MsgBox( 64, "", "found at " & $ret[0] & " " & $ret[1] & " SAD " & $ret[2])
	endif
	return $ret
endfunc

func SearchResultToVectMultiRes( $result )
	local $array = StringSplit($result[0],"|")
	local $resCount = Number( $array[1] )
	;MsgBox( 64, "", "res count " & $resCount )
	local $ret[3]
	$ret[0]=-1
	$ret[1]=-1
	$ret[2]=2147483646
    For $i = 0 To $resCount
		$ret[$i * 3 + 0]=Int(Number($array[$i * 3 + 2]))
		$ret[$i * 3 + 1]=Int(Number($array[$i * 3 + 3]))
		$ret[$i * 3 + 2]=Int(Number($array[$i * 3 + 4]))	; SAD
		;MouseMove( $ret[0], $ret[1] );
		;MsgBox( 64, "", "found at " & $ret[0] & " " & $ret[1] & " SAD " & $ret[2])
	next
	return $ret
endfunc

func FlipNumber( $Nr )
	Local $ret = 0
	while($Nr > 0)
		local $Digit = mod($Nr,10)
		$ret = $ret * 10 + $Digit
		$Nr = (int)( $Nr / 10 )
	wend
	return $ret
endfunc

func CountDigits( $Nr )
	Local $ret = 0
	while($Nr > 0)
		$ret = $ret + 1
		$Nr = (int)( $Nr / 10 )
	wend
	return $ret
endfunc

func GetNthDigit( $Nr, $Index )
	$Index = $Index - 1
	for $i = 0 to $Index step 1
		$Nr = (int)( $Nr / 10 )
	next
	local $Digit = mod($Nr,10)
	return $Digit
endfunc

; about 12 coord units / screen
func DragScreenToRight()
	Local $MarginUndragged = 2	; maybe around 50 pixels
	Local $DragLatency = 0 ; might require even 100 to fully drag the screen
	Local $aPos = GetWindowRelativPos()
	; this should drag about  tiles if used with speed 9
	MouseMove( $aPos[0] + $aPos[2] - $MarginUndragged, $aPos[1] + $aPos[3] / 2, 0 )
	MouseDown($MOUSE_CLICK_LEFT)
	MouseMove( $aPos[0] - $DragLatency + $MarginUndragged , $aPos[1] + $aPos[3] / 2, 9 )
	MouseUp($MOUSE_CLICK_LEFT)
	
	local $SecondDragSize = 150
	MouseMove( $aPos[0] + $aPos[2] - $MarginUndragged, $aPos[1] + $aPos[3] / 2, 0 )
	MouseDown($MOUSE_CLICK_LEFT)
	MouseMove( $aPos[0] + $aPos[2] - $SecondDragSize , $aPos[1] + $aPos[3] / 2, 9 )
	MouseUp($MOUSE_CLICK_LEFT)	
	
	; make sure latency does not affect our search
	WaitScreenFinishLoading()
endfunc

; it may take variable time for the screen to finish loading
; there should be a spinning circle in the lower left corner. Or maybe the screen will fade from black to colored ...
; if the circle goes away, we can presume the game loaded the screen
func WaitScreenFinishLoading()
 ; repeat taking screenshots until there is no change between the screens
 ; there is a chance that snow wil make our screenshot change all the time
	sleep(1000)
	return;
	global $dllhandle
	Local $Radius = 16
	Local $AntiInfiniteLoop = 10000 ; timeout the function after this amount of mseconds
	Local $aPos = GetWindowRelativPos()
	local $x2 = 11 + $aPos[0]
	local $y2 = 12 + $aPos[1]
	local $result = DllCall( $dllhandle, "NONE", "TakeScreenshot", "int", $x2 - $Radius, "int", $y2 - $Radius, "int", $x2 + $Radius, "int", $y2 + $Radius)
	DllCall( $dllhandle, "NONE", "ApplyColorBitmask", "int", 0x00F0F0F0)
	$result[ = "1|0|0"
	while ( $result[0] == '1' and $AntiInfiniteLoop > 0 )
		Sleep( 100 )
		$AntiInfiniteLoop = $AntiInfiniteLoop - 100
		DllCall( $dllhandle, "NONE", "TakeScreenshot", "int", $x2 - $Radius, "int", $y2 - $Radius, "int", $x2 + $Radius, "int", $y2 + $Radius)
		DllCall( $dllhandle, "NONE", "ApplyColorBitmask", "int", 0x00F0F0F0)
		$result = DllCall( $dllhandle, "NONE", "IsAnythingChanced", "int", 0, "int", 0, "int", 0, "int", 0)
	wend
endfunc
