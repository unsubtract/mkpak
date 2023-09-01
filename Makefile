CFLAGS = -Wall -Wextra -Wpedantic --std=c99 -g3 -fsanitize=address
all: mkpak unpak 

clean:
	rm -f mkpak unpak

clean_win:
	del mkpak.exe unpak.exe
