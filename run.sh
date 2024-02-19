#!/bin/bash

mkdir -p ./build/artifact
filename=$1
filename="${filename%.*}"
./ctc $1 build/artifact/$filename.asm
nasm -f elf64 -o ./build/artifact/$filename.o ./build/artifact/$filename.asm
gcc -o ./build/$filename ./build/artifact/$filename.o

