echo "APPLICATION: $APPLICATION"
echo


if [ ! -f $HMAC ]; then
	apt-get update
	apt-get install libgcrypt20-dev
	cp /usr/bin/hmac256 $HMAC
fi
[ -x $HMAC ] || chmod +x $HMAC


for i in $(ls -1 ./bin/$BOARD/$APPLICATION-slot?.*.riot.bin); do
	$HMAC --binary "mls160a-update" ${i} > ${i}.hash
	echo $HMAC' --binary "mls160a-update" '${i}' > '${i}'.hash'
done

cp ./bin/$BOARD/$APPLICATION-slot?.*.riot.bin* .

exit
