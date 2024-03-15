#!/bin/bash

mkdir -p ./out/artifact
filename=$1
filename="${filename%.*}"
../ctc $1 out/artifact/$filename.asm
nasm -f elf64 -o ./out/artifact/$filename.o ./out/artifact/$filename.asm
gcc -o ./out/$filename ./out/artifact/$filename.o

