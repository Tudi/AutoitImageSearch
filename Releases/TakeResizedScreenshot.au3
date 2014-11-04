#include <ImageSearch.au3>

$dllhandle = DllOpen ( "ImageSearchDLL/Release/ImageSearchDLL.dll" )

$result = DllCall( $dllhandle,"str","TakeScreenshot","int",0,"int",0,"int",800,"int",800)
$result = DllCall( $dllhandle,"str","SaveScreenshot")
$result = DllCall( $dllhandle,"str","BlurrImage","int",1)
$result = DllCall( $dllhandle,"str","SaveScreenshot")
$result = DllCall( $dllhandle,"str","ResizeScreenshot","int",320,"int",200)
$result = DllCall( $dllhandle,"str","SaveScreenshot")

DllClose ( $dllhandle )

