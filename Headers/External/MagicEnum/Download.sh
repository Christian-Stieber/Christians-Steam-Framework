#! /bin/sh

TAG="v0.8.2"

rm -rf /tmp/MagicEnumDownload
mkdir /tmp/MagicEnumDownload
git clone --depth 1 --branch "$TAG" --config "advice.detachedHead=false" https://github.com/Neargye/magic_enum.git /tmp/MagicEnumDownload
mv /tmp/MagicEnumDownload/include/*.hpp .
mv /tmp/MagicEnumDownload/LICENSE .
rm -rf /tmp/MagicEnumDownload
