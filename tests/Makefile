empty :=

out_dir := out
obj_dir := $(out_dir)/artifacts

src_files := $(wildcard *.tnsl)
obj_files := $(src_files:.tnsl=.o)
out_files := $(src_files:.tnsl=.out)

.PHONY: all run outdir clean
all: outdir $(out_files)

run: all
	./run.sh

%.asm: %.tnsl
	@../ctc $< $(obj_dir)/$@

%.o: %.asm
	@nasm -f elf64 -o $(obj_dir)/$@ $(obj_dir)/$<

%.out: %.o
	@gcc -o $(out_dir)/$@ $(obj_dir)/$<
	@echo "Built $(out_dir)/$@"

outdir:
	@mkdir -p $(out_dir)
	@mkdir -p $(obj_dir)
	@echo "Created output directories"

clean:
	rm -rf out/*

