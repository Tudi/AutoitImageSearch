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
		MouseClick("left",851,929,1)
		Sleep(1000)
		if $FuncIsRunning <> 1 then ExitLoop
		
		MouseClick("left",540,249,1)
		Sleep(1000)
		if $FuncIsRunning <> 1 then ExitLoop
		
		MouseClick("left",637,417,1)
		Sleep(1000)
		if $FuncIsRunning <> 1 then ExitLoop
		
		; wait for game to load
		MouseClick("left",654,588,1)
		Sleep(6000)
		if $FuncIsRunning <> 1 then ExitLoop
		
		; spawn hero
		Send("u")
		Sleep(1000)
		MouseClick("left",628,516,1)
		Sleep(1000)
		if $FuncIsRunning <> 1 then ExitLoop
		
		; spawn sniper
		Send("z")
		Sleep(1000)
		MouseClick("left",1468,495,1)
		Sleep(1000)
		MouseClick("left",1468,495,1)
		Sleep(1000)
		if $FuncIsRunning <> 1 then ExitLoop
		
		; start game
		Sleep(1000)
		Send("{SPACE}")
		Sleep(1000)
		Send("{SPACE}")
		Sleep(1000)

		Sleep(10000)
		if $FuncIsRunning <> 1 then ExitLoop
		Send("/")
		
		Sleep(50000)
		if $FuncIsRunning <> 1 then ExitLoop
		Send("/")
		
		Sleep(15000)
		if $FuncIsRunning <> 1 then ExitLoop
		Send(",")

		Sleep(40000)
		if $FuncIsRunning <> 1 then ExitLoop
		Send(",")
		
		Sleep(60000)
		if $FuncIsRunning <> 1 then ExitLoop
		Send("/")
		
		Sleep(60000)
		if $FuncIsRunning <> 1 then ExitLoop
		Send("/")

		; wait for the game to end
		Sleep(75000)
		if $FuncIsRunning <> 1 then ExitLoop
		
		MouseClick("left",976,904,1)
		Sleep(1000)
		MouseClick("left",695,854,1)
		Sleep(3000)
		
		$Cycles = $Cycles + 1
	wend
endfunc