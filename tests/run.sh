#!/bin/bash

make

for i in out/*.out; do
	./$i
	if [[ $? -ne 69 ]]; then
		echo "[FAILED] $i"
	else
		echo "[  OK  ] $i"
	fi
done

