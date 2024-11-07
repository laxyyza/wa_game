#!/usr/bin/env bash

OUT=client/log.txt
EXE=bin/wa_game

exec $EXE $@ &> $OUT
