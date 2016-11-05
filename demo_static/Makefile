.PHONY: all clean

CFLAGS = -Wall -Wextra -Werror -pedantic -std=c11 -Og -ggdb -I .. -iquote .
OBJECTS = main.o ../esh.o ../esh_hist.o ../esh_argparser.o
OUTPUT = demo

all: ${OUTPUT}

${OUTPUT}: ${OBJECTS}
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $^

clean:
	rm -f ${OBJECTS} ${OUTPUT}
