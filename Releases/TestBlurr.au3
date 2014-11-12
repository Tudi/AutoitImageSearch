global $MB_SYSTEMMODAL = 4096

$dllhandle = DllOpen ( "Debug/ImageSearchDLL.dll" )

DllCall( $dllhandle,"NONE","TakeScreenshot","int",0,"int",0,"int",800,"int",800)
DllCall( $dllhandle,"NONE","SaveScreenshot")
DllCall( $dllhandle,"NONE","BlurrImage","int",1)
DllCall( $dllhandle,"NONE","SaveScreenshot")

DllCall( $dllhandle,"NONE","TakeScreenshot","int",0,"int",0,"int",800,"int",800)
DllCall( $dllhandle,"NONE","BlurrImage","int",2)
DllCall( $dllhandle,"NONE","SaveScreenshot")

DllClose ( $dllhandle )

