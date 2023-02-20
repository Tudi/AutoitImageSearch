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
	local $loop = 0
	while( $FuncIsRunning == 1)
;		_WinWaitActivate("BloonsTD6","")
	
		if( mod($loop, 10) == 0) then
			Send("s")
			Sleep(1000)
			MouseClick("left")
			Sleep(1000)
			Send("3")
			Sleep(1000)
			MouseClick("left")
			Sleep(1000)
		endif
		
		Send("1")
		Sleep(1000)		
		Send("2")
		Sleep(1000)
		Send("4")
		Sleep(1000)
		Send("5")
		Sleep(1000)
		Send("6")
		Sleep(1000)
		Send("7")
		Sleep(1000)
		Send("0")
		Sleep(1000)
		Send("-")
		Sleep(1000)
		$loop = $loop + 1
	wend
endfunc