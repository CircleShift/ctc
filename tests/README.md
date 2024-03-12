This is a folder of all current tests for tnsl.

Each compiled program should return a status of 69.

To compile:

First run ./build.sh in the parent directory to generate the compiler
Second make sure you have nasm and gcc installed

    ../ctc <file to compile>
    nasm -f elf64 -o out.o out.asm
    gcc out.o

The final executable should be in this directory called "a.out"
