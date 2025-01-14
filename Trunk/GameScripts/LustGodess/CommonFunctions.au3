#include <AutoItConstants.au3>
#include <date.au3>
#include <WinAPISys.au3>

Opt("MustDeclareVars", 1)

global $dllhandle = null

Func ResetDebugLog($sFileName)
   Local $hFilehandle = FileOpen($sFileName, $FO_OVERWRITE)
   FileClose($hFilehandle)
endfunc

Func _WriteDebugLog($TextToLog, $sFileName = "")
	if( $sFileName == "") then 
		$sFileName = "DebugLog.txt"
	endif
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

func InitScreenshotDllIfRequired()
	if $dllhandle == null then
		$dllhandle = DllOpen( "ImageSearchDLL.dll" )
	endif
endfunc

func GetWindowRelativPos()
	local $ret[2]
	$ret[0] = 0
	$ret[1] = 0
	return $ret
endfunc

func TakeScreenshotAtMousePos($width, $height, $ColorMask = 0x00F0F0F0)
	InitScreenshotDllIfRequired()
	local $mpos = MouseGetPos()
	$result = DllCall( $dllhandle,"NONE","TakeScreenshot","int",$mpos[0],"int",$mpos[1],"int",$mpos[0] + $width,"int",$mpos[1] + $height)
	if $ColorMask <> 0 then 
		$result = DllCall( $dllhandle,"NONE", "ApplyColorBitmask", "int", 0x00F0F0F0)
	endif
	$result = DllCall( $dllhandle,"NONE","SaveScreenshot")
endfunc

func CopyArray($FromArray, ByRef $ToArray)
	For $i = 0 To UBound($FromArray) - 1
		$ToArray[$i] = $FromArray[$i]
	Next
endfunc

global $PrevMousePos[2], $PrevPrevMousePos[2]

;HotKeySet("Q", "TakeScreenshotCyclePositions")
;HotKeySet("W", "TakeScreenshotAtMousePosPreRecorded")
func TakeScreenshotCyclePositions()
	local $mousePosNow = MouseGetPos()
	CopyArray($PrevMousePos, $PrevPrevMousePos)
	CopyArray($mousePosNow, $PrevMousePos)
endfunc
func TakeScreenshotAtMousePosPreRecorded()
	InitScreenshotDllIfRequired()
	local $result = DllCall( $dllhandle,"NONE","TakeScreenshot","int",$PrevPrevMousePos[0],"int",$PrevPrevMousePos[1],"int",$PrevMousePos[0],"int",$PrevMousePos[1])
	local $ColorMask = 0x00F0F0F0
	if $ColorMask <> 0 then 
		$result = DllCall( $dllhandle,"NONE", "ApplyColorBitmask", "int", $ColorMask)
	endif
	$result = DllCall( $dllhandle,"NONE","SaveScreenshot")
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

Func IsColorAtPos($x,$y,$colorToFind,$SearchRadius,$tolerance)
    Local $xStart = $x-$SearchRadius
    Local $xEnd = $x+$SearchRadius
    Local $yStart = $y-$SearchRadius
    Local $yEnd = $y+$SearchRadius
    Local $pos1 = PixelSearch($xStart, $yStart, $xEnd, $yEnd, $colorToFind, $tolerance)
    If Not @error Then
		global $pos = $pos1
        return 1
    EndIf	
	return 0
Endfunc

; used for loading screens
func WaitImageAppear( $ImageName, $X = -1, $Y = -1, $Sleep = 500, $Timout = 3000 )
	InitScreenshotDllIfRequired()
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
	InitScreenshotDllIfRequired()
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

func ClickButtonIfAvailable( $ImageName, $X = -1, $Y = -1, $SearchRadius = 0, $SleepBetweenRetrys = 500, $retryCount = 1000, $AcceptableDiffPCT = 0)
	InitScreenshotDllIfRequired()
	if( $x == -1 ) then
		GetCoordFromImageFileName( $ImageName, $x, $y )
	endif
	while( $retryCount > 0 )
		_WriteDebugLog("new search name=" & $ImageName & " x=" & $x & " y=" & $y & " SearchRadius=" & $SearchRadius & " SleepBetweenRetrys=" & $SleepBetweenRetrys & " retryCount=" & $retryCount & " AcceptableSAD=" & $AcceptableSAD)
		local $startStamp = _WinAPI_GetTickCount()
		local $Pos = ImageIsAt($ImageName, $X, $Y, $SearchRadius, $AcceptableDiffPCT)
		local $searchDuration = _WinAPI_GetTickCount() - $startStamp
		_WriteDebugLog("found at " & $Pos[0] & " " & $Pos[1] & " SAD " & $Pos[2] & " searching took " & $searchDuration & " ms")
		; if SAD is small enough, we consider it as found
		if( $Pos[0] > 0 ) then
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

; when the window is partially under another window
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

Func GetCoordFromImageFileName( $ImgName, ByRef $x, ByRef $y, ByRef $width, ByRef $height, $AbsoluteCoord = 0 )
	local $array = StringSplit($ImgName,"_")
	local $resCount = $array[0]
	$width=Int(Number($array[$resCount-1]))
	$height=Int(Number($array[$resCount-0]))
	$x=Int(Number($array[$resCount-3]))
	$y=Int(Number($array[$resCount-2]))
	if( $AbsoluteCoord <> 0 ) then
		Local $aPos = GetWindowRelativPos()
		$x=$aPos[0] + $x
		$y=$apos[1] + $y
	endif
endfunc

Func ImageIsAt( $ImgName, $x = -1, $y = -1, $Radius = 2, $AcceptableSAD = 0)
	InitScreenshotDllIfRequired()
	; in case the image file name in a common sense format we can extract the coordinate of it
	local $width, $height
	if( $x == -1 ) then
		GetCoordFromImageFileName( $ImgName, $x, $y, $width, $height )
	endif
	global $dllhandle
	local $start_x = $x - $Radius
	local $end_x = $x + $Radius
	local $start_y = $y - $Radius
	local $end_y = $y + $Radius
	
	if $start_x < 0 then $start_x = 0
	if $start_y < 0 then $start_y = 0
	if $end_x > @DesktopWidth then $end_x = @DesktopWidth
	if $end_y > @DesktopHeight then $end_y = @DesktopHeight

	local $result = DllCall( $dllhandle, "NONE", "TakeScreenshot", "int", -1, "int", -1, "int", -1, "int", -1)
	$result = DllCall( $dllhandle, "NONE", "ApplyColorBitmask", "int", 0x00F0F0F0) ; remove small aberations due to color merge
	$result = DllCall( $dllhandle, "str", "ImageSearch_SAD_Region", "str", $ImgName, "int", $x - $Radius, "int", $y - $Radius, "int", $end_x, "int", $end_y, "int", 0)
	local $widthTruncated = int($width / 8) * 8 ; SAD only works on multiple of 8 pixels on width
	local $res = SearchResultToVectSingleRes( $result )
	return $res
endfunc

func SearchResultToVectSingleRes( $result )
	local $array = StringSplit($result[0],"|")
	local $resCount = Number( $array[1] )
	;MsgBox( 64, "", "res count " & $resCount & " and res " & $result[0] & " split count " & UBound($array) )
	local $ret[8]
	$ret[0]=-1 ; x
	$ret[1]=-1 ; y
	$ret[2]=-1 ; SAD
	$ret[3]=-1 ; SAD / Pixel - because sad does not use exact width
	$ret[4]=-1 ; avg color diff
	$ret[5]=-1 ; color diff count
	$ret[6]=100 ; color diff pct
	$ret[7]=-1 ; Hash Diff if requested
	if( $resCount >= 1 and UBound($array) >= 7 ) then
		$ret[0]=Int(Number($array[2]))
		$ret[1]=Int(Number($array[3]))
		$ret[2]=Int(Number($array[4]))	; SAD
		$ret[3]=Int(Number($array[5]))	; SAD / Pixel
		$ret[4]=Int(Number($array[6]))	; avg color diff
		$ret[5]=Int(Number($array[7]))	; color diff count
		$ret[6]=Int(Number($array[8]))	; color diff pct
		$ret[7]=Int(Number($array[9]))	; Hash diff pct
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
	InitScreenshotDllIfRequired()
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
		Sleep( 100 ) ; 10 FPS. Screenshot taking default max FPS is 5 !
		$AntiInfiniteLoop = $AntiInfiniteLoop - 100
		DllCall( $dllhandle, "NONE", "TakeScreenshot", "int", $x2 - $Radius, "int", $y2 - $Radius, "int", $x2 + $Radius, "int", $y2 + $Radius)
		DllCall( $dllhandle, "NONE", "ApplyColorBitmask", "int", 0x00F0F0F0)
		$result = DllCall( $dllhandle, "NONE", "IsAnythingChanced", "int", 0, "int", 0, "int", 0, "int", 0)
	wend
endfunc

;Func OnAutoItExit()     
;	DllClose($dllhandle)
;EndFunc ;==>OnAutoItExit
