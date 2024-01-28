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

## mkpak:
Create a PAK archive from a directory
```
usage: mkpak [input directory] [output archive]
[input directory] will become the root of [output archive]
```

## unpak:
Extract files from a PAK archive into a directory
```
usage: unpak [input archive] [output directory]
the root of [input archive] will become [output directory]
```
