#! /bin/sh

dpkgarch=$(dpkg --print-architecture)

if [ "$dpkgarch" = "iphoneos-arm" ] || [ "$dpkgarch" = "iphoneos-arm64" ]; then
    rootfs=""
else
    rootfs="/rootfs"
fi

cd "/var/mobile/AD-dev/AD"

rm -rfv ./tmp/* && mkdir -p tmp

if [ -d AD ]; then
    mv AD tmp/
fi

cp -aR "/private/preboot/7C7922343CC8461523468BB696C09C1D8FA1EA773FC2B0757CB650C46F0A18BE8D23D2CB9002DF0C5C8E3312893793FC/dopamine-wpn2YR/procursus/Users/AD/.opt/Dev/AD-dev/Library/AD" "/var/mobile/AD-dev/AD/"

chown -R mobile:AD "/var/mobile/AD-dev/AD"
