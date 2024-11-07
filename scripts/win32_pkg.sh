#!/usr/bin/env bash

BUILD_DIR=build-win32
STAG_DIR=$BUILD_DIR/wa_game
SHADERS_DIR=$STAG_DIR/client/src/shaders
CLIENT_BIN_DIR=$STAG_DIR/client/bin

PTHREAD_PATH=/usr/x86_64-w64-mingw32/bin/libwinpthread-1.dll
LIBSSP=/usr/x86_64-w64-mingw32/bin/libssp-0.dll
LIBZSTD=/usr/x86_64-w64-mingw32/bin/libzstd.dll

OUTPUT_ZIP=wa_game_win32.zip
OUTPUT_TAR=wa_game_win32.tar

EXE_PATH=$BUILD_DIR/client/wa_game.exe
SCRIPT_PATH=scripts/wa_game.bat

ninja -C $BUILD_DIR
mkdir -p $SHADERS_DIR $CLIENT_BIN_DIR

cp -rv res $STAG_DIR
rm $STAG_DIR/res/lastinfo
cp -v client/src/shaders/* $SHADERS_DIR
strip $EXE_PATH
cp -v $EXE_PATH $PTHREAD_PATH $LIBSSP $LIBZSTD $CLIENT_BIN_DIR
cp -v $SCRIPT_PATH $STAG_DIR

touch $STAG_DIR/$OUTPUT_TAR

cd $STAG_DIR && tar --exclude $OUTPUT_TAR -cvf $OUTPUT_TAR . && mv $OUTPUT_TAR ..
cd ..

zstd $OUTPUT_TAR
xz -z -k $OUTPUT_TAR
gzip -k $OUTPUT_TAR
zip -r $OUTPUT_ZIP wa_game

