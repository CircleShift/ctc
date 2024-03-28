empty :=

out_dir := out
obj_dir := $(out_dir)/artifacts

src_files := $(wildcard *.tnsl)
obj_files := $(src_files:.tnsl=.o)
out_files := $(src_files:.tnsl=.out)

all: outdir $(out_files)
.PHONY: all test

%.asm: %.tnsl
	../ctc $< $(obj_dir)/$@

%.o: %.asm
	nasm -f elf64 -o $(obj_dir)/$@ $(obj_dir)/$<

%.out: %.o
	gcc -o $(out_dir)/$@ $(obj_dir)/$<

outdir:
	mkdir -p $(out_dir)
	mkdir -p $(obj_dir)

clean:
	rm -rf out/*

