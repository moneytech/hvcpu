CFLAGS = -Wall -g -Wno-format 
LDFLAGS = 
.PHONY: all clean
all: vcpu casm

vcpu: cpu.o log.o vcpu.o decode.o run.o
	cc $(CFLAGS) -o $@ $^ $(LDFLAGS) 
casm: assembler.o assembler.h
	cc $(CFLAGS) -o $@ $^ $(LDFLAGS) 

clean:
	rm -f *.o vcpu casm
