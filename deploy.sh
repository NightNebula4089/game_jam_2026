#!/bin/bash
VITA_IP=192.168.137.76
NAME=test_game
cmake -B build
cmake --build build && \
curl.exe -T "$(wslpath -w build/$NAME.vpk)" "ftp://$VITA_IP:1337/ux0:/data/$NAME.vpk"
