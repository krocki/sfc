GCC=gcc
TARGETS=gl
CFLAGS=-g -fPIC --std=c11 -Wfatal-errors
INCLUDES=.
LFLAGS=-framework OpenGL -lpthread -lglfw
OBJECTS:=$(patsubst %.c,%.o,$(wildcard *.c))

.SUFFIXES:

all: $(TARGETS)

%.o: %.c
	$(GCC) -I$(INCLUDES) $(CFLAGS) -c $< -o $@

gl: gl.o hilbert.o
	$(GCC) $(LFLAGS) $^ -o $@

%: %.o
	$(GCC) -o $@ $(LFLAGS) $^

clean:
	rm -rf $(TARGETS) $(OBJECTS)
