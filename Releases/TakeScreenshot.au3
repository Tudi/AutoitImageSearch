$dllhandle = DllOpen ( "Release/ImageSearchDLL.dll" )

$result = DllCall( $dllhandle,"str","TakeScreenshot","int",0,"int",0,"int",2000,"int",2000)
$result = DllCall( $dllhandle,"str","SaveScreenshot")
;$result = DllCall( $dllhandle,"str","ResizeScreenshot","int",320,"int",200)
;$result = DllCall( $dllhandle,"str","SaveScreenshot")

DllClose ( $dllhandle )

