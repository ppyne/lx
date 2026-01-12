#!/bin/sh

LX=../lx
FAIL=0
CONFIG_H=../config.h

enabled() {
    key=$1
    if [ ! -f "$CONFIG_H" ]; then
        echo 1
        return
    fi
    awk -v k="$key" '$1=="#define" && $2==k {print $3; found=1} END{if(!found) print 1}' "$CONFIG_H"
}

for t in $(find . -name "*.lx"); do
    out="${t%.lx}.out"
    printf "TEST %-40s " "$t"

    case "$t" in
        ./core/include.lx) ext_key="LX_ENABLE_INCLUDE" ;;
        ./ext/files.lx|./ext/pwd.lx) ext_key="LX_ENABLE_FS" ;;
        ./ext/json.lx) ext_key="LX_ENABLE_JSON" ;;
        ./ext/serializer.lx) ext_key="LX_ENABLE_SERIALIZER" ;;
        ./ext/hex.lx) ext_key="LX_ENABLE_HEX" ;;
        ./ext/blake2b.lx) ext_key="LX_ENABLE_BLAKE2B" ;;
        ./ext/time.lx) ext_key="LX_ENABLE_TIME" ;;
        ./ext/env.lx) ext_key="LX_ENABLE_ENV" ;;
        ./ext/utf8.lx) ext_key="LX_ENABLE_UTF8" ;;
        ./ext/sqlite.lx) ext_key="LX_ENABLE_SQLITE" ;;
        ./ext/aead.lx) ext_key="LX_ENABLE_AEAD" ;;
        *) ext_key="" ;;
    esac

    if [ -n "$ext_key" ] && [ "$(enabled "$ext_key")" = "0" ]; then
        echo "SKIP (extension disabled)"
        continue
    fi

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
