#!/bin/bash

cd bin/ssv-mls160a
hmac256 --binary "mls160a-update" mls160a-slot0.$1.riot.bin > mls160a-slot0.$1.riot.bin.hash
hmac256 --binary "mls160a-update" mls160a-slot1.$1.riot.bin > mls160a-slot1.$1.riot.bin.hash

cp mls160a-slot?.$1.riot.bin* ../..
