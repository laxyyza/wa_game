#!/usr/bin/env bash

VERSION=$(cat VERSION)

BUILD_DIR=build-win32
if [[ $# -gt 0 ]]; then
	BUILD_DIR=$1
fi

STAG_NAME=wa_game_$VERSION
STAG_DIR=$BUILD_DIR/$STAG_NAME
SHADERS_DIR=$STAG_DIR/client/src/shaders
CLIENT_BIN_DIR=$STAG_DIR/client/bin

PTHREAD_PATH=/usr/x86_64-w64-mingw32/bin/libwinpthread-1.dll
LIBSSP=/usr/x86_64-w64-mingw32/bin/libssp-0.dll
LIBZSTD=/usr/x86_64-w64-mingw32/bin/libzstd.dll

OUTPUT_ZIP=wa_game_win32_$VERSION.zip
OUTPUT_TAR=wa_game_win32_$VERSION.tar

EXE_PATH=$BUILD_DIR/client/wa_game.exe
SCRIPT_PATH=scripts/wa_game.vbs

rm -v $STAG_DIR/*

ninja -C $BUILD_DIR
mkdir -p $SHADERS_DIR $CLIENT_BIN_DIR

rm -v $BUILD_DIR/${OUTPUT_TAR}* $BUILD_DIR/${OUTPUT_ZIP}

cp -rv res $STAG_DIR
rm -v $STAG_DIR/res/lastinfo
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
zip -r $OUTPUT_ZIP $STAG_NAME
