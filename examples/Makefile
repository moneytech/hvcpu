SRC := $(shell find . -name '*.v')
PROG_LIST := $(basename $(SRC))
all: $(PROG_LIST)
% : %.v
	./../casm -i $^ -o $@
clean:
	rm -f $(PROG_LIST)
