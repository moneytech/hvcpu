CFLAGS = -Wall -g -Wno-format 
LDFLAGS = 
.PHONY: all clean
all: vcpu

vcpu: cpu.o log.o vcpu.o decode.o run.o
	cc $(CFLAGS) -o $@ $^ $(LDFLAGS) 

clean:
	rm -f *.o vcpu
