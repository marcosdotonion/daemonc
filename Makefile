CFLAGS = -Wall -pedantic -std=gnu99

all: daemonite

daemonite:
	@mkdir -p build
	gcc $(CFLAGS) `pkg-config --cflags --libs libnotify` daemonite.c -o build/daemonited

clean:
	rm -rf build/
