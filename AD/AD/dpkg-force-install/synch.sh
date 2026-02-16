#! /bin/zsh

cd "/var/mobile/Containers/Shared/AppGroup/C516F168-0B8E-4DAF-8CE5-371FB33E9270/File Provider Storage/xcode"

uuid=$(uuid -v4)

tmp(){
  mkdir -p tmp
  mkdir "./$uuid"
  mv dpkg-force-install "./$uuid/"
  mv "./$uuid" tmp/
   
}

main_synch(){
  if [ ! -d tmp ]; then
    mkdir tmp
  fi
  tmp
}

main_synch

cp -R "/var/jb/Users/AD/.opt/Dev/AD-dev/Library/AD/dpkg-force-install" "/var/mobile/Containers/Shared/AppGroup/C516F168-0B8E-4DAF-8CE5-371FB33E9270/File Provider Storage/xcode/"

chown -R mobile:AD "/var/mobile/Containers/Shared/AppGroup/C516F168-0B8E-4DAF-8CE5-371FB33E9270/File Provider Storage/xcode/dpkg-force-install"
