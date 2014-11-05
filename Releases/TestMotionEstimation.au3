#include <FileConstants.au3>

$DebugFile = FileOpen( "auDebug.txt", $FO_OVERWRITE )

$dllhandle = DllOpen ( "Debug/ImageSearchDLL.dll" )
	
$FoundMotion = 0;
while( $FoundMotion == 0 )
	Sleep( 1000 )
	DllCall( $dllhandle,"NONE","TakeScreenshot","int",0,"int",0,"int",800,"int",800 )
	$result = DllCall( $dllhandle, "INT", "GenerateDiffMap" )
    FileWrite( $DebugFile, "result sad now :" & String( $result[0] ) & @CRLF )
	$FoundMotion = $result[0]
wend
DllCall( $dllhandle, "NONE", "SaveDiffMap" )
DllCall( $dllhandle, "NONE", "SaveScreenshotDiffMask","int",0 )
; errode removes small change regions and will make existing ones more "round". Errode is good for us because diffmask precision is a 4x4 pixel
DllCall( $dllhandle, "NONE", "ErrodeDiffMap","int",5 )
DllCall( $dllhandle, "NONE", "SaveDiffMap" )
DllCall( $dllhandle, "NONE", "SaveScreenshotDiffMask","int",0 )

DllClose ( $dllhandle )

FileClose($DebugFile)
