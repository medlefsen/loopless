#!/bin/bash

cd "$(dirname "$0")"
echo $PWD

sudo docker run -it --rm -v "$PWD":/workdir -e MAC="$MAC" -e LINUX="$LINUX" -e BUILD_UID="$(id -u)" -e BUILD_GID="$(id -g)" medlefsen/crossbuild ./crossbuild.sh
