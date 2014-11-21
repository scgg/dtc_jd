#!/bin/sh


"$1" "$2" -o conftest conftest.c >/dev/null 2>&1 && \
		(echo ./conftest | sh 2>/dev/null) && echo "$2"
rm -f conftest.o conftest core
