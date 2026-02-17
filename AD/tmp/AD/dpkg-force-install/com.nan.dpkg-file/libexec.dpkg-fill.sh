#!/bin/sh -


if [ -f '/etc/profile' ]; then
	. /etc/profile
	_root=''
elif [ -f '/var/jb/etc/profile' ]; then
	. /var/jb/etc/profile
	_root='/var/jb'
else
	echo 'Where the fuck "profile"?' 1>&2
	exit 1
fi
dpkg-fill &
echo "$!" >"${0}.fpid"
launchctl unload "${_root}/Library/LaunchDaemons/com.nan.dpkg-fill.plist"
exit 0
