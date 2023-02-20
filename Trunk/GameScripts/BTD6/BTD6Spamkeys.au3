Opt("MustDeclareVars", 1)
HotKeySet("o", "ExitBot")
HotKeySet("p", "TempFunc")

global $BotIsRunning = 1
global $FuncIsRunning = 0

while( $BotIsRunning == 1)
	Sleep(1000)
wend

func ExitBot()
	global $BotIsRunning = 0
endfunc

exit

func TempFunc()
	$FuncIsRunning = 1 - $FuncIsRunning
	local $Cycles = 0
	while( $FuncIsRunning == 1)
;		_WinWaitActivate("BloonsTD6","")
		Send("1")
		Sleep(1000)		
		Send("2")
		Sleep(1000)
		Send("3")
		Sleep(1000)
		Send("4")
		Sleep(1000)
		Send("5")
		Sleep(1000)
		Send("6")
		Sleep(1000)
;		Send("7")
;		Sleep(1000)
;		Send("8")
;		Sleep(1000)
		Send("9")
		Sleep(1000)
		Send("0")
		Sleep(1000)
;		Send("-")
;		Sleep(1000)
;		Send("=")
;		Sleep(1000)
		
		if( mod($Cycles,3) == 0 )then
			Send("8")
			Sleep(1000)
			MouseClick("left")
		endif
		
		$Cycles = $Cycles + 1
	wend
endfunc