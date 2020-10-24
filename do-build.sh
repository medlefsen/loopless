#!/bin/bash

if [ -z "$1" ]
then
  exit 1
fi

build_dir="$1"

if [ ! -e "$build_dir" ]
then
  mkdir "$build_dir"
fi

cd "$build_dir"

if [ "$CROSS_TRIPLE" == x86_64-apple-darwin14 ]
then
  export CC=x86_64-apple-darwin14-cc
  CMAKE_COMMAND=x86_64-apple-darwin14-cmake
else
  CMAKE_COMMAND=cmake
fi

CMAKE_ARGS=(-D CMAKE_CMD="$CMAKE_COMMAND")

if [ "$CROSS_TRIPLE" != x86_64-linux-gnu ]
then
  CMAKE_ARGS+=(-D CONFIGURE_ARGS="--host;$CROSS_TRIPLE" )
fi

${CMAKE_COMMAND} "${CMAKE_ARGS[@]}" ..
make VERBOSE=1