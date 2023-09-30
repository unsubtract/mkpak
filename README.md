<h2 align="center">Implementation of the <a href="https://quakewiki.org/wiki/.pak">Quake PAK archive format</a></h2>

---

### Features:
- Native support for both POSIX and Win32
- Quite fast and extremely small
- Support for big endian systems (PowerPC, m68k, SPARC, etc.)
- Uses MIT License

### Building:
```sh
make
```
Or manually manually compile `mkpak.c` for mkpak and `unpak.c` for unpak.

## mkpak:
Create a PAK archive from a directory
```
usage: mkpak INPUT_DIRECTORY ARCHIVE
INPUT_DIRECTORY will become the root directory of ARCHIVE
```

## unpak:
Extract files from a PAK archive into a directory
```
usage: unpak ARCHIVE OUTPUT_DIRECTORY
the root directory of ARCHIVE will become OUTPUT_DIRECTORY
```
