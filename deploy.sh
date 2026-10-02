#!/bin/bash
VITA_IP=192.168.137.19
NAME=moving_box
cmake --build build && \
curl.exe -T "$(wslpath -w build/$NAME.vpk)" "ftp://$VITA_IP:1337/ux0:/data/$NAME.vpk"
