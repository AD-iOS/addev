#!/bin/sh
code=(
    adcreate.cc
    adcreate
    admk
)
build="obj"
rm -rf ./$build
mkdir -p "$build"

if test -d ./$build; then
    mkdir -p obj
fi

if test -x $(which clang) && test -x $( \which clang++); then
    CC=$(which clang);
    CXX=$(which clang++);
elif test -x $(which gcc) && test -x $(which g++); then
    CC=$(which gcc);
    CXX=$(which g++);
else
    echo "[Error]: No C/C++ compiler found"
    exit 1
fi

SDK="/var/theos/sdks/iPhoneOS17.5.sdk"

$CXX -lc -lc++ \
  -isystem "$SDK" \
  -I/var/jb/usr/include \
  ${code[0]} \
  -o "$build/${code[1]}" \
  > "$build/build_err.log" 2>&1


ln -s ${code[1]} $build/${code[2]}