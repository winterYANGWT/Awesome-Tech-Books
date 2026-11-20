#!/bin/sh
printf '%s\n' "$*" >> compile.log
exec "$@"
