Set objShell = CreateObject("WScript.Shell")

' Base command
cmd = "cmd /c ""client\bin\wa_game.exe"

' Loop through each argument passed to the VBScript and add it to the command
If WScript.Arguments.Count > 0 Then
    For i = 0 To WScript.Arguments.Count - 1
        cmd = cmd & " " & WScript.Arguments(i)
    Next
End If

' Append the redirection for stdout and stderr to the log file
cmd = cmd & " > client\log.txt 2>&1"""

objShell.Run cmd, 0, True
