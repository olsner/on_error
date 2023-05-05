DISASM_DIR = bddisasm
DISASM_LIBDIR = $(DISASM_DIR)/bin/x64/Release
DISASM_LIB = $(DISASM_LIBDIR)/libbddisasm.a

CFLAGS = -g -Og -Wall -Werror
CFLAGS += -I$(DISASM_DIR)/inc
CFLAGS += -MD -MP
SO_CFLAGS = -fPIC -fvisibility=hidden -fdata-sections -ffunction-sections
SO_CFLAGS += -DON_ERROR_API='__attribute__((visibility("default")))'
SO_LDFLAGS = -shared

# Just to make ./segvign_dyn work without LD_LIBRARY_PATH settings...
LDFLAGS += -Wl,--rpath='$$ORIGIN:'
LDFLAGS += -Wl,--gc-sections -s
LDFLAGS += -L$(DISASM_LIBDIR)
LIBS += -lbddisasm
LIB = lib_on_error.so
LIBNOCRASH = libnocrash.so
BINS = segvign segvign_dyn crash

default: $(BINS) $(LIB) $(LIBNOCRASH)

$(DISASM_LIB): export CC := $(CC) $(SO_CFLAGS)
$(DISASM_LIB): $(DISASM_DIR)
	make -C $(DISASM_DIR) RELEASE=y VERBOSE=y

$(DISASM_DIR):
	@echo "Check out and build libbddisasm first."
	@echo "$$ git clone https://github.com/bitdefender/bddisasm"
	exit 1

LIB_SRCS = on_error.c nextinst.c
BIN_SRCS = segvign.c
SRCS = $(BIN_SRCS) $(LIB_SRCS)
OBJS = $(SRCS:.c=.o)
BIN_OBJS = $(BIN_SRCS:.c=.o)
LIB_OBJS = $(LIB_SRCS:.c=.l.o)
DEPS = $(OBJS:.o=.d) $(LIB_OBJS:.o=.d)

NOCRASH_OBJS = libnocrash.l.o

%.l.o: %.c
	$(CC) $(CFLAGS) $(SO_CFLAGS) -c -o $@ $<

$(LIB): $(LIB_OBJS) $(DISASM_LIB)
	$(CC) $(LDFLAGS) $(SO_LDFLAGS) -o $@ $(LIB_OBJS) $(LIBS)

$(LIBNOCRASH): $(LIB_OBJS) $(NOCRASH_OBJS) $(DISASM_LIB)
	$(CC) $(LDFLAGS) $(SO_LDFLAGS) -o $@ $(LIB_OBJS) $(NOCRASH_OBJS) $(LIBS)

segvign: $(OBJS) $(DISASM_LIB)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

segvign_dyn: $(BIN_OBJS) $(LIB)
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	rm -f $(LIB_OBJS) $(OBJS) $(DEPS) $(BINS) $(LIB)
	rm -f $(LIBNOCRASH) $(NOCRASH_OBJS)

-include $(DEPS)
