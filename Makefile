GCC=gcc
TARGETS=gl
CFLAGS=-g -fPIC --std=c11 -Wfatal-errors
INCLUDES=.
#LFLAGS=-framework OpenGL -lpthread -lglfw
LFLAGS=-lGL -lglfw -lpthread
OBJECTS:=$(patsubst %.c,%.o,$(wildcard *.c))

.SUFFIXES:

all: $(TARGETS)

%.o: %.c
	$(GCC) -I$(INCLUDES) $(CFLAGS) -c $< -o $@

gl: gl.o hilbert.o
	$(GCC) $^ -o $@ $(LFLAGS)

%: %.o
	$(GCC) $^ -o $@ $(LFLAGS)

clean:
	rm -rf $(TARGETS) $(OBJECTS)
