#!/usr/bin/env bash

BUILD_DIR=build
STAG_DIR=$BUILD_DIR/wa_game
SHADERS_DIR=$STAG_DIR/client/src/shaders
CLIENT_PATH=$BUILD_DIR/client/wa_game
SERVER_PATH=$BUILD_DIR/server/server
OUT_NAME=wa_game_linux.tar.zst

ninja -C $BUILD_DIR
mkdir -p $SHADERS_DIR $STAG_DIR/lib

cp -rv res $STAG_DIR
cp -v client/src/shaders/* $SHADERS_DIR
./scripts/copylibs.py $CLIENT_PATH $STAG_DIR/lib
strip $SERVER_PATH $CLIENT_PATH
cp -v $CLIENT_PATH $SERVER_PATH $STAG_DIR

cd $BUILD_DIR && tar --zstd -cvf $OUT_NAME wa_game

