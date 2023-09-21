CFLAGS = -pipe -O2 -s -Wall -Wextra -Wpedantic -std=c99
all: mkpak unpak

clean:
	rm -f mkpak unpak *.exe
