SRC = $(wildcard src/*.c)
MYSRC = $(wildcard mysrc/*.c)
# INPUT ?= $(wildcard inputs/*/*.x)
INPUT = ./inputs/inst/add.x

.PHONY: all verify clean

all: sim

sim: $(MYSRC)
	gcc -std=c99 -g -O2 $^ -o $@

basesim: $(SRC)
	gcc -g -O2 $^ -o $@

run: sim
	python3 run.py $(INPUT)
	# cat $(INPUT).cmd | ./sim $(INPUT).x > sim_main.out 

clean:
	rm -rf *.o *~ sim

