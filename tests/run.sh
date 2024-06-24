#!/bin/bash

FAILURE=0

for i in out/*.out; do
	./$i
	EXC=$?
	if [[ $EXC -ne 69 ]]; then
		echo "[FAILED] (Code $EXC) $i"
		FAILURE=1
	else
		echo "[  OK  ] $i"
	fi
done

echo     "=============="
if [[ $FAILURE -ne 0 ]]; then
	echo " TESTS FAILED "
else
	echo " TESTS PASSED "
fi
echo     "=============="
