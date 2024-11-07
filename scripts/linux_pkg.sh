#!/usr/bin/env bash

VERSION=$(cat VERSION)

BUILD_DIR=build
STAG_NAME=wa_game_linux$VERSION
STAG_DIR=$BUILD_DIR/$STAG_NAME
SHADERS_DIR=$STAG_DIR/client/src/shaders
CLIENT_PATH=$BUILD_DIR/client/wa_game
SERVER_PATH=$BUILD_DIR/server/server
BIN_DIR=$STAG_DIR/bin

OUT_TAR=wa_game_linux$VERSION.tar

ninja -C $BUILD_DIR
mkdir -p $SHADERS_DIR $STAG_DIR/lib $BIN_DIR

cp -rv res $STAG_DIR
cp -v client/src/shaders/* $SHADERS_DIR
./scripts/copylibs.py $CLIENT_PATH $STAG_DIR/lib
strip $SERVER_PATH $CLIENT_PATH
cp -v $CLIENT_PATH $SERVER_PATH $BIN_DIR
cp -v scripts/wa_game.sh $STAG_DIR

cd $BUILD_DIR && tar -cvf $OUT_TAR $STAG_NAME

zstd $OUT_TAR
xz -z -k $OUT_TAR
gzip -k $OUT_TAR
