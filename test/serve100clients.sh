#!/usr/bin/env sh

prog="$1"
sh -c "sh -c 'sleep 5; echo stop; exit 0' | $prog --cli" &
for i in $(seq 1 100); do
    mcstatus 127.0.0.1 status > /dev/null &
done

echo "done";
