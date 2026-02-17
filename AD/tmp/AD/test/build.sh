#!/bin/sh
code=(
    test.cc
    test1
    test.c
    test_c
)
build="obj"
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


$CC -lc -lc++ \
  -isystem $theos_sdk \
  -I$jb/usr/include \
  ${code[2]} \
  -o "$build/${code[3]}" \
  > "$build/c_build_err.log" 2>&1