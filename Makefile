CFLAGS = -Wall -Wextra -Wpedantic --std=c99
all: pak

clean:
	rm -f pak

clean_win:
	del pak.exe
