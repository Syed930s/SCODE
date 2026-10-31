# SCODE

**Syed's code**: a small terminal tool that encodes text into numeric codes (and back) to transfer or hide messages. One file of C, no dependencies.

## Encoding

Letters alternate upper/lower case across the numbers:

| Code | Char | Code | Char | | Code | Char |
|------|------|------|------|-|------|------|
| 1 | A | 2 | a | | 51 | Z |
| 3 | B | 4 | b | | 52 | z |

- Uppercase letters are odd (`A`=1, `B`=3 ... `Z`=51), lowercase are even (`a`=2, `b`=4 ... `z`=52).
- Digits `0`-`9` are `91`-`100`.
- Codes are joined with `_`; spaces and newlines pass through.

`Abab ba` becomes `1_4_2_4 4_2`.

### Symbols

| Char | Code | Char | Code | Char | Code | Char | Code |
|------|------|------|------|------|------|------|------|
| `!` | 57 | `"` | 58 | `$` | 59 | `€` | 60 |
| `^` | 61 | `&` | 62 | `*` | 63 | `(` | 64 |
| `)` | 65 | `<` | 66 | `>` | 67 | `,` | 68 |
| `?` | 69 | `/` | 70 | `;` | 71 | `:` | 72 |
| `~` | 73 | `#` | 74 | `{` | 75 | `}` | 76 |
| `'` | 77 | `@` | 78 | `-` | 79 | `_` | 80 |
| `+` | 81 | `=` | 82 | `` ` `` | 83 | `£` | 84 |
| `%` | 85 | `.` | 86 | `[` | 87 | `]` | 88 |
| `\` | 89 | `|` | 90 | | | | |

## Usage

Run `SCODE`. It starts in insert mode, vim-style, and shows the input and the converted output live.

| Key | Action |
|-----|--------|
| `Esc` | Normal mode |
| `i` / `a` | Back to insert mode |
| `:s` | Encode mode: type plain text, get SCODE |
| `:d` | Decode mode: type SCODE, get plain text |
| `:w [file]` | Write the output to `file` (default `scode.txt`) and exit |
| `:q` | Quit |
| `Ctrl+C` | Quit |

Switching between `:s` and `:d` clears the buffer.

## Build

Source is `src/main.c`. Prebuilt binaries are in `Downloads/`:

```
Downloads/
  Windows/Windows 32/SCODE.EXE
  Windows/Windows 64/SCODE.EXE
  Linux/Linux 32/SCODE
  Linux/Linux 64/SCODE
  Apple/MacOS x86_64/SCODE
  Apple/MacOS Apple/SCODE
```

Build natively with GCC:

```
gcc -O2 -s src/main.c -o SCODE
```

## License

See [LICENSE](LICENSE).
