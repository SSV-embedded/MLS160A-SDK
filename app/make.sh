#!/bin/bash

RIOTDIR=/data/riotbuild/riotbase
WORKDIR=/data/riotbuild/riotbase/examples/app
HMAC=/data/riotbuild/riotbase/hmac256

BOARD=ssv-mls160a
APPLICATION=$(sed -ne 's/APPLICATION[ ]*=[ ]*\(.*\)$/\1/p' /data/riotbuild/riotbase/examples/app/Makefile)

echo "PWD:         $PWD"
echo "RIOTDIR:     $RIOTDIR"
echo "WORKDIR:     $WORKDIR"
echo "HMAC:        $HMAC"
echo "BOARD:       $BOARD"
echo "APPLICATION: $APPLICATION"
echo
echo

echo "Check and install hmac256 utility"
if [ ! -f $HMAC ]; then
	apt-get update
	apt-get -y install libgcrypt20-dev
	cp /usr/bin/hmac256 $HMAC
fi
[ -x $HMAC ] || chmod +x $HMAC

cd $WORKDIR
echo "Build '$APPLICATION' application"
make all RIOTBASE=$RIOTDIR
make riotboot RIOTBASE=$RIOTDIR

echo "Create firmware HASH"
for i in $(ls -1 ./bin/$BOARD/$APPLICATION-slot?.*.riot.bin); do
	$HMAC --binary "mls160a-update" ${i} > ${i}.hash
	echo $HMAC' --binary "mls160a-update" '${i}' > '${i}'.hash'
done

cp ./bin/$BOARD/$APPLICATION-slot?.*.riot.bin* .
