#!/bin/bash

if [ ! -e /usr/bin/crossbuild ]
then
  echo 1>&2 "Must be run from crossbuild container!"
  exit 1
fi

if [ "$UID" -eq 0 ]
then
  groupadd -f -g "$BUILD_GID" build_user
  useradd -u "$BUILD_UID" -g "$BUILD_GID" build_user
  exec su build_user -c "cd $PWD; $0"
fi

PATH=/usr/osxcross/bin:$PATH

if [ "$MAC" != 0 ]
then
  echo "---- BUILDING MAC -----"

  CROSS_TRIPLE=x86_64-apple-darwin14 ./do-build.sh build_mac
fi

if [ "$LINUX" != 0 ]
then
  echo "---- BUILDING LINUX -----"

  CROSS_TRIPLE=x86_64-linux-gnu ./do-build.sh build_linux
fi
