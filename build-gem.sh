#!/usr/bin/env bash
cd "$(dirname "$0")"

if [ -e "build_ruby" ]
then
  rm -rf "build_ruby"
fi
cp -r ruby "build_ruby"
cp README.md COPYING "build_ruby"
export VERSION=$(< VERSION)
install -D "build_mac/loopless" "build_ruby/darwin/loopless"
install -D "build_linux/loopless" "build_ruby/linux/loopless"
cd "build_ruby"
gem build loopless.gemspec
