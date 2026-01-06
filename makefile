TARGET=main
SOURCE=main.c

LIBS=/opt/X11/lib
INCS=/opt/X11/include
CFLAGS=-lx11 -O2 -Wall -lXext -lXrender

make:
	${CC} ${SOURCE} -o ${TARGET} -L${LIBS} -I${INCS} ${CFLAGS}

clean:
	rm ${TARGET}
