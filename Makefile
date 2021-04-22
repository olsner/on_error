DISASM_DIR = bddisasm
DISASM_LIBDIR = $(DISASM_DIR)/bin/x64/Release
DISASM_LIB = $(DISASM_LIBDIR)/libbddisasm.a

CFLAGS = -g -Og -Wall -Werror=return-type
CFLAGS += -I$(DISASM_DIR)/inc
CFLAGS += -MD -MP

LDFLAGS += -L$(DISASM_LIBDIR)
LIBS += -lbddisasm

default: segvign

$(DISASM_LIB):
	@echo "Check out and build libbddisasm first."
	@echo "\\$$ git clone https://github.com/bitdefender/bddisasm"
	@echo "\\$$ make -C bddisasm"
	exit 1


SRCS = segvign.c nextinst.c on_error.c
OBJS = $(SRCS:.c=.o)
DEPS = $(OBJS:.o=.d)

segvign: $(OBJS) $(DISASM_LIB)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

clean:
	rm -f $(OBJS) $(DEPS) segvign

-include $(DEPS)
