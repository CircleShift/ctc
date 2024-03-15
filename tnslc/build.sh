#!/bin/bash

mkdir -p out
../ctc main.tnsl out/tnslc.asm
nasm -f elf64 -o out/tnslc.o out/tnslc.asm
gcc -o out/tnslc out/tnslc.o
# rm tnslc.asm tnslc.o

