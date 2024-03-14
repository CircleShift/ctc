#!/bin/bash

../ctc main.tnsl tnslc.asm
nasm -f elf64 -o tnslc.o tnslc.asm
gcc -o tnslc tnslc.o
# rm tnslc.asm tnslc.o

