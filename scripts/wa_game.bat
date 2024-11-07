@echo off

set EXE=client/bin/wa_game.exe
set OUTPUT=client/log.txt

%EXE% > %OUTPUT% 2>&1
