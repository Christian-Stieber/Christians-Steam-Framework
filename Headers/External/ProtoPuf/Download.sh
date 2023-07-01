#! /bin/bash

TAG="v2.2.1"

DOWNLOAD_DIR="/tmp/ProtoPufDownload"

rm -rf "$DOWNLOAD_DIR"
mkdir "$DOWNLOAD_DIR"

git clone --depth 1 --branch "$TAG" --config "advice.detachedHead=false" https://github.com/PragmaTwice/protopuf.git "$DOWNLOAD_DIR"
rm -f *.h
mv "$DOWNLOAD_DIR/include/protopuf"/*.h .
mv "$DOWNLOAD_DIR/LICENSE" .
rm -rf "$DOWNLOAD_DIR"
