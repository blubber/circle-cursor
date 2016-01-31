
all:
	cc circle-cursor.c -o circle-cursor -std=c99 -lX11 -Wall -D_BSD_SOURCE

clean:
	rm -rf *.o

dist-clean: clean
	rm -rf circle-cursor

install:
	install --strip -o root -g root circle-cursor /bin/
