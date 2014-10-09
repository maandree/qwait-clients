# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.


# The package path prefix, if you want to install to another root, set DESTDIR to that root.
PREFIX = /usr
# The binary path excluding prefix.
BIN = /bin
# The library binary path excluding prefix.
LIBEXEC = /libexec
# The resource path excluding prefix.
DATA = /share
# The binary path including prefix.
BINDIR = $(PREFIX)$(BIN)
# The library binary path including prefix.
LIBEXECDIR = $(PREFIX)$(LIBEXEC)
# The resource path including prefix.
DATADIR = $(PREFIX)$(DATA)
# The license base path including prefix.
LICENSEDIR = $(DATADIR)/licenses


# The name of the package as it should be installed.
PKGNAME = qwait-clients


SHARED = -shared
LDSO = -Wl,-soname,libqwaitclient.so
PIC = -fPIC


# Flags to compile with.
OPTIMISE = -O3 -g
STD = c99
#  These work on clang and gcc.
WARN = -Wall -Wextra -pedantic -Wformat=2 -Winit-self -Wmissing-include-dirs   \
       -Wfloat-equal -Wshadow -Wmissing-prototypes -Wmissing-declarations      \
       -Wredundant-decls -Wnested-externs -Winline -Wno-variadic-macros        \
       -Wswitch-default -Wconversion -Wcast-align -Wstrict-overflow            \
       -Wdeclaration-after-statement -Wundef -Wcast-qual -Wbad-function-cast   \
       -Wwrite-strings -Waggregate-return -Wpacked -Wstrict-prototypes         \
       -Wold-style-definition -fstrict-aliasing -fstrict-overflow -fno-builtin
#  These work gcc but not on clang.
WARN += -Wdouble-promotion -Wtrampolines -Wsign-conversion -Wsync-nand  \
        -Wlogical-op -Wvector-operation-performance                     \
        -Wunsuffixed-float-constants -Wsuggest-attribute=const          \
        -Wsuggest-attribute=noreturn -Wsuggest-attribute=pure           \
        -Wsuggest-attribute=format -Wnormalized=nfkc                    \
        -Wunsafe-loop-optimizations -fstack-usage -ftree-vrp            \
	-fipa-pure-const -funsafe-loop-optimizations

C_FLAGS = $(WARN) $(OPTIMISE) -std=$(STD) $(CFLAGS) $(CPPFLAGS) -D'LIBEXECDIR="$(LIBEXECDIR)"'
LD_FLAGS = $(WARN) $(OPTIMISE) -std=$(STD) $(LDFLAGS)

LIBQWAITCLIENT_LIBFLAGS = -lrt
LIBQWAITCLIENT_CFLAGS =
LIBQWAITCLIENT_OBJ = http-message http-socket json qwait-position qwait-protocol qwait-queue authentication  \
                     qwait-user computers login-information

QWAIT_CMD_LIBFLAGS = -lqwaitclient -Lbin
QWAIT_CMD_CFLAGS = -Isrc
QWAIT_CMD_OBJ = qwait-cmd globals queues queue authentication user users miscellaneous

QWAIT_CURSES_LIBFLAGS = -lqwaitclient -Lbin
QWAIT_CURSES_CFLAGS = -Isrc
QWAIT_CURSES_OBJ = qwait-curses terminal globals

ifeq ($(USE_LIBPASSPHRASE),y)
QWAIT_CMD_LIBFLAGS += -lpassphrase
C_FLAGS += -DUSE_LIBPASSPHRASE
endif


# Build rules.

.PHONY: all
all: libqwaitclient qwait-cmd qwait-curses


.PHONY: libqwaitclient
libqwaitclient: bin/libqwaitclient.so bin/libqwaitclient-test

obj/libqwaitclient/%.o: src/libqwaitclient/%.c src/libqwaitclient/*.h
	@mkdir -p obj/libqwaitclient
	$(CC) $(C_FLAGS) $(LIBQWAITCLIENT_CFLAGS) $(PIC) -c $< -o $@

bin/libqwaitclient-test: $(foreach O,$(LIBQWAITCLIENT_OBJ) test,obj/libqwaitclient/$(O).o)
	@mkdir -p bin
	$(CC) $(LD_FLAGS) $(LIBQWAITCLIENT_LIBFLAGS) $^ -o $@

bin/libqwaitclient.so: $(foreach O,$(LIBQWAITCLIENT_OBJ),obj/libqwaitclient/$(O).o)
	@mkdir -p bin
	$(CC) $(LD_FLAGS) $(LIBQWAITCLIENT_LIBFLAGS) $(SHARED) $(LDSO) $^ -o $@


.PHONY: qwait-cmd
qwait-cmd: bin/qwait-cmd

obj/qwait-cmd/%.o: src/qwait-cmd/%.c src/qwait-cmd/*.h
	@mkdir -p obj/qwait-cmd
	$(CC) $(C_FLAGS) $(QWAIT_CMD_CFLAGS) -c $< -o $@

bin/qwait-cmd: $(foreach O,$(QWAIT_CMD_OBJ),obj/qwait-cmd/$(O).o) bin/libqwaitclient.so
	@mkdir -p bin
	$(CC) $(LD_FLAGS) $(QWAIT_CMD_LIBFLAGS) $^ -o $@


.PHONY: qwait-curses
qwait-curses: bin/qwait-curses

obj/qwait-curses/%.o: src/qwait-curses/%.c src/qwait-curses/*.h
	@mkdir -p obj/qwait-curses
	$(CC) $(C_FLAGS) $(QWAIT_CURSES_CFLAGS) -c $< -o $@

bin/qwait-curses: $(foreach O,$(QWAIT_CURSES_OBJ),obj/qwait-curses/$(O).o) bin/libqwaitclient.so
	@mkdir -p bin
	$(CC) $(LD_FLAGS) $(QWAIT_CURSES_LIBFLAGS) $^ -o $@


# Clean rules.

.PHONY: clean
clean:
	-rm -rf obj bin *.su

