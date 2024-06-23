#!/bin/sh
#
set -e
tmpFile=$(mktemp)
gcc -lcurl -lz app/*.c -o $tmpFile -lm 
exec "$tmpFile" "$@"
