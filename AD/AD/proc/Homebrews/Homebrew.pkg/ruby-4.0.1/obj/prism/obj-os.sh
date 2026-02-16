#! /bin/sh -

set -x # debug

jb="/var/jb"
include="$jb/include"

cc \
  -I$include/ruby/aarch64-apple-ios \
  -I$include \
  -I$include/ruby \
  *.c \
  -c

  # -I../include/ruby \
  # -I../include \