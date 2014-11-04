$dllhandle = DllOpen ( "Release/ImageSearchDLL.dll" )

;taking a chunk out of the screen to test if img search is working
;$result = DllCall( $dllhandle,"str","TakeScreenshot","int",40,"int",40,"int",80,"int",80)
;$result = DllCall( $dllhandle,"str","SaveScreenshot")

$result = DllCall( $dllhandle,"str","TakeScreenshot","int",0,"int",0,"int",800,"int",800)
;$result = DllCall( $dllhandle,"str","SaveScreenshot")
$SkipSearchOnColor = 0x0F00000
$colorTolerance = 10
$ColorToleranceFaultsAccepted = 1
$ExitAfterNMatchesFound=1
;$result = DllCall( $dllhandle,"str","ImageSearchOnScreenshot","str","tosearch.bmp","int",$SkipSearchOnColor,"int",$colorTolerance,"int",$ColorToleranceFaultsAccepted,"int",$ExitAfterNMatchesFound)
$colorTolerance = 0
$result = DllCall( $dllhandle,"str","ImageSearchOnScreenshot","str","tosearch.bmp","int",$SkipSearchOnColor,"int",$colorTolerance,"int",$ColorToleranceFaultsAccepted,"int",$ExitAfterNMatchesFound)
;send( $result );
send( $result[0] );
;$array = StringSplit($result[0],"|")
;$x=Int(Number($array[2]))
;$y=Int(Number($array[3]))
;$result = DllCall( $dllhandle,"str","SaveScreenshot")

DllClose ( $dllhandle )

