#!/usr/bin/env bash

BUILD_DIR=build-win32
STAG_DIR=$BUILD_DIR/wa_game
SHADERS_DIR=$STAG_DIR/client/src/shaders
PTHREAD_PATH=/usr/x86_64-w64-mingw32/bin/libwinpthread-1.dll
EXE_PATH=$BUILD_DIR/client/wa_game.exe

ninja -C $BUILD_DIR
mkdir -p $SHADERS_DIR

cp -rv res $STAG_DIR
cp -v client/src/shaders/* $SHADERS_DIR
strip $EXE_PATH
cp -v $EXE_PATH $PTHREAD_PATH $STAG_DIR

touch $STAG_DIR/wa_game_win32.tar.zst

cd $STAG_DIR && tar --exclude wa_game_win32.tar.zst --zstd -cvf wa_game_win32.tar.zst .

