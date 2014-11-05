$dllhandle = DllOpen ( "Debug/ImageSearchDLL.dll" )

;taking a chunk out of the screen to test if img search is working
;$result = DllCall( $dllhandle,"NONE","TakeScreenshot","int",40,"int",40,"int",80,"int",80)
;$result = DllCall( $dllhandle,"NONE","SaveScreenshot")

$result = DllCall( $dllhandle,"NONE","TakeScreenshot","int",0,"int",0,"int",800,"int",800)
; save screenshot that we will use later as masking
;$result = DllCall( $dllhandle,"NONE","SaveScreenshot")
$SkipSearchOnColor = 0x00FEFEFE
$colorTolerance = 10
$ColorToleranceFaultsAccepted = 1
$ExitAfterNMatchesFound=1
$result = DllCall( $dllhandle,"str","ImageSearchOnScreenshotMasked","str","tosearch.bmp","str","OverlayAsMask.bmp","int",$SkipSearchOnColor,"int",$colorTolerance,"int",$ColorToleranceFaultsAccepted,"int",$ExitAfterNMatchesFound)
Send( $result[0] );
$colorTolerance = 0
$result = DllCall( $dllhandle,"str","ImageSearchOnScreenshotMasked","str","tosearch.bmp","str","OverlayAsMask.bmp","int",$SkipSearchOnColor,"int",$colorTolerance,"int",$ColorToleranceFaultsAccepted,"int",$ExitAfterNMatchesFound)
Send( $result[0] );
$result = DllCall( $dllhandle,"str","ImageSearchOnScreenshotMasked","str","tosearchTrans.bmp","str","OverlayAsMask.bmp","int",$SkipSearchOnColor,"int",$colorTolerance,"int",$ColorToleranceFaultsAccepted,"int",$ExitAfterNMatchesFound)
Send( $result[0] );
;$result = DllCall( $dllhandle,"str","SaveScreenshot")

DllClose ( $dllhandle )

