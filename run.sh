#!/bin/sh

"$DEVKITPPC/bin/wiiload" build/bin/main.elf
sleep 2
netcat ${WIILOAD#tcp:} 16784
