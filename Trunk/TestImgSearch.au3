$dllhandle = DllOpen ( "Debug/ImageSearchDLL.dll" )

;taking a chunk out of the screen to test if img search is working
;$result = DllCall( $dllhandle,"NONE","TakeScreenshot","int",40,"int",40,"int",80,"int",80)
;$result = DllCall( $dllhandle,"NONE","SaveScreenshot")

$result = DllCall( $dllhandle,"NONE","TakeScreenshot","int",0,"int",0,"int",800,"int",800)
;$result = DllCall( $dllhandle,"NONE","SaveScreenshot")
$SkipSearchOnColor = 0x00FEFEFE
$colorTolerance = 10
$ColorToleranceFaultsAccepted = 1
$ExitAfterNMatchesFound=1
$result = DllCall( $dllhandle,"str","ImageSearchOnScreenshot","str","tosearch.bmp","int",$SkipSearchOnColor,"int",$colorTolerance,"int",$ColorToleranceFaultsAccepted,"int",$ExitAfterNMatchesFound)
$colorTolerance = 0
$result = DllCall( $dllhandle,"str","ImageSearchOnScreenshot","str","tosearch.bmp","int",$SkipSearchOnColor,"int",$colorTolerance,"int",$ColorToleranceFaultsAccepted,"int",$ExitAfterNMatchesFound)
$result = DllCall( $dllhandle,"str","ImageSearchOnScreenshot","str","tosearchTrans.bmp","int",$SkipSearchOnColor,"int",$colorTolerance,"int",$ColorToleranceFaultsAccepted,"int",$ExitAfterNMatchesFound)
;send( $result );
send( $result[0] );
;$array = StringSplit($result[0],"|")
;$x=Int(Number($array[2]))
;$y=Int(Number($array[3]))
;$result = DllCall( $dllhandle,"str","SaveScreenshot")

DllClose ( $dllhandle )

