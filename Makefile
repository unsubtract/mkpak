CFLAGS = -Wall -Wextra -Wpedantic --std=c99 -g3
all: mkpak unpak 

clean:
	rm -f mkpak unpak

clean_win:
	del mkpak.exe unpak.exe
