#! /bin/sh

renewtime='2026-1-12'
me=`echo "$0" | sed -e 's,.*/,,'`

usage="\
usage: $0 [options]

options:
  -h, --help         print this help, then exit
  -v, --version      print version number, then exit"

version="\
AD-dev build.guess ($renewtime)
version 0.1
"

help="
try '$me --help | -h' for more information
"

while test $# -gt 0; do
  case $1 in
   --version | -v )
      echo "$version";
      exit;
      ;;
   --help | -h )
      echo "$usage";
      exit;
      ;;
      shift;
      break;
      ;;
   -* | * )
      echo "$me: invalid option $1$help" >&2;
      break;
      exit 1;
      ;;
  esac
done

if test -f /etc/os-release; then
  . /etc/os-release
  echo "$NAME $VERSION"
else


echo "System: $(detect_os)"
echo "Architecture: $(uname -m)"
echo "Kernel: $(uname -r)"
