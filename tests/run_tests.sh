#!/bin/sh

LX=../lx
FAIL=0

for t in $(find . -name "*.lx"); do
    out="${t%.lx}.out"
    printf "TEST %-40s " "$t"

    if [ ! -f "$out" ]; then
        echo "SKIP (no .out)"
        continue
    fi

    res=$($LX "$t" 2>&1)

    if [ "$res" = "$(cat "$out")" ]; then
        echo "OK"
    else
        echo "FAIL"
        echo "---- expected ----"
        cat "$out"
        echo "---- got ----"
        echo "$res"
        FAIL=1
    fi
done

exit $FAIL
