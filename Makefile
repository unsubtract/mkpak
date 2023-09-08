CFLAGS = -pipe -Og -Wall -Wextra -Wpedantic --std=c99 -g3 -fsanitize=address -fsanitize=undefined
# CFLAGS = -pipe -flto -O2 -s -Wall -Wextra -Wpedantic --std=c99
all: mkpak unpak

clean:
	rm -f mkpak unpak *.exe
