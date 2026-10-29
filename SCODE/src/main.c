#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#endif

#define MAXN 65536
#define K_NONE (-2)
#define K_QUIT (-1)
#define K_ESC 27
#define K_ENTER 10
#define K_BS 8

typedef struct {
    int cp;
    int code;
} Map;

static const Map pmap[] = {
    {'!', 57}, {'"', 58}, {'$', 59}, {0x20AC, 60}, {'^', 61}, {'&', 62},
    {'*', 63}, {'(', 64}, {')', 65}, {'<', 66}, {'>', 67}, {',', 68},
    {'?', 69}, {'/', 70}, {';', 71}, {':', 72}, {'~', 73}, {'#', 74},
    {'{', 75}, {'}', 76}, {'\'', 77}, {'@', 78}, {'-', 79}, {'_', 80},
    {'+', 81}, {'=', 82}, {'`', 83}, {0xA3, 84}, {'%', 85}, {'.', 86},
    {'[', 87}, {']', 88}, {'\\', 89}, {'|', 90}
};

#define NP (sizeof(pmap) / sizeof(pmap[0]))

static int buf[MAXN];
static size_t len;
static char out[MAXN * 4 + 16];

static int enc(int c)
{
    size_t i;
    if (c >= 'A' && c <= 'Z') return 2 * (c - 'A') + 1;
    if (c >= 'a' && c <= 'z') return 2 * (c - 'a') + 2;
    if (c >= '0' && c <= '9') return 2 * (c - '0') + 1;
    for (i = 0; i < NP; i++)
        if (pmap[i].cp == c) return pmap[i].code;
    return -1;
}

static int dec(int n)
{
    size_t i;
    if (n >= 1 && n <= 52)
        return (n & 1) ? 'A' + (n - 1) / 2 : 'a' + (n - 2) / 2;
    for (i = 0; i < NP; i++)
        if (pmap[i].code == n) return pmap[i].cp;
    return -1;
}

static size_t put8(char *o, size_t p, int c)
{
    if (c < 0x80) {
        o[p++] = (char)c;
    } else if (c < 0x800) {
        o[p++] = (char)(0xC0 | (c >> 6));
        o[p++] = (char)(0x80 | (c & 63));
    } else if (c < 0x10000) {
        o[p++] = (char)(0xE0 | (c >> 12));
        o[p++] = (char)(0x80 | ((c >> 6) & 63));
        o[p++] = (char)(0x80 | (c & 63));
    } else {
        o[p++] = (char)(0xF0 | (c >> 18));
        o[p++] = (char)(0x80 | ((c >> 12) & 63));
        o[p++] = (char)(0x80 | ((c >> 6) & 63));
        o[p++] = (char)(0x80 | (c & 63));
    }
    return p;
}

static size_t convert(const int *in, size_t n, int decode, char *o)
{
    size_t i, p = 0;
    int prev = 0, num = -1, c, v;

    if (!decode) {
        for (i = 0; i < n; i++) {
            c = in[i];
            if (c == ' ' || c == '\t' || c == '\n') {
                o[p++] = (char)(c == '\t' ? ' ' : c);
                prev = 0;
                continue;
            }
            v = enc(c);
            if (v < 0) continue;
            if (prev) o[p++] = '_';
            p += (size_t)sprintf(o + p, "%d", v);
            prev = 1;
        }
    } else {
        for (i = 0; i <= n; i++) {
            c = i < n ? in[i] : 0;
            if (c >= '0' && c <= '9') {
                num = (num < 0 ? 0 : num) * 10 + (c - '0');
                if (num > 100000) num = 100000;
                continue;
            }
            if (num >= 0) {
                v = dec(num);
                if (v >= 0) p = put8(o, p, v);
                num = -1;
            }
            if (c == ' ' || c == '\n' || c == '\t')
                o[p++] = (char)(c == '\t' ? ' ' : c);
        }
    }
    o[p] = 0;
    return p;
}

#ifdef _WIN32

static void term_on(void)
{
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD m = 0;
    SetConsoleOutputCP(65001);
    if (GetConsoleMode(h, &m)) SetConsoleMode(h, m | 0x0004);
}

static void term_off(void)
{
}

static int key(void)
{
    int c = (int)_getwch();
    if (c == 0 || c == 0xE0) {
        _getwch();
        return K_NONE;
    }
    if (c == 3) return K_QUIT;
    if (c == 27) return K_ESC;
    if (c == '\r') return K_ENTER;
    if (c == 8 || c == 127) return K_BS;
    if (c < 32) return K_NONE;
    if (c >= 0xD800 && c < 0xE000) return K_NONE;
    return c;
}

#else

static struct termios old;

static void term_off(void)
{
    tcsetattr(0, TCSAFLUSH, &old);
}

static void term_on(void)
{
    struct termios t;
    tcgetattr(0, &old);
    t = old;
    t.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
    t.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    t.c_cc[VMIN] = 1;
    t.c_cc[VTIME] = 0;
    tcsetattr(0, TCSAFLUSH, &t);
}

static int wait_in(int ms)
{
    fd_set f;
    struct timeval tv;
    FD_ZERO(&f);
    FD_SET(0, &f);
    tv.tv_sec = 0;
    tv.tv_usec = ms * 1000;
    return select(1, &f, NULL, NULL, &tv) > 0;
}

static int key(void)
{
    unsigned char c, d;
    int n, cp, i;

    if (read(0, &c, 1) != 1) return K_QUIT;
    if (c == 3) return K_QUIT;
    if (c == 27) {
        if (!wait_in(50)) return K_ESC;
        if (read(0, &d, 1) != 1) return K_ESC;
        if (d == '[' || d == 'O') {
            while (wait_in(20) && read(0, &d, 1) == 1)
                if (d >= 0x40 && d <= 0x7E) break;
        }
        return K_NONE;
    }
    if (c == '\r' || c == '\n') return K_ENTER;
    if (c == 127 || c == 8) return K_BS;
    if (c < 32) return K_NONE;
    if (c < 0x80) return c;
    if (c >= 0xF0) {
        n = 3;
        cp = c & 7;
    } else if (c >= 0xE0) {
        n = 2;
        cp = c & 15;
    } else if (c >= 0xC0) {
        n = 1;
        cp = c & 31;
    } else {
        return K_NONE;
    }
    for (i = 0; i < n; i++) {
        if (read(0, &d, 1) != 1) return K_NONE;
        cp = (cp << 6) | (d & 63);
    }
    return cp;
}

#endif

static void draw(int insert, int decode, const char *cmd)
{
    size_t i, p;
    char tmp[8];

    convert(buf, len, decode, out);
    fputs("\x1b[2J\x1b[H", stdout);
    printf("SCODE  %s  [%s]\n", insert ? "-- INSERT --" : "-- NORMAL --",
           decode ? "DECODE" : "ENCODE");
    printf("---- input (%s) ----\n", decode ? "SCODE" : "plain");
    for (i = 0; i < len; i++) {
        p = put8(tmp, 0, buf[i]);
        fwrite(tmp, 1, p, stdout);
    }
    printf("\n---- output (%s) ----\n", decode ? "plain" : "SCODE");
    fputs(out, stdout);
    fputs("\n----\n", stdout);
    if (cmd)
        printf(":%s", cmd);
    else if (insert)
        fputs("esc: normal mode", stdout);
    else
        fputs("i: insert | :s encode | :d decode | :w [file] write+exit | :q quit", stdout);
    fflush(stdout);
}

int main(void)
{
    int insert = 1, decode = 0, k;
    char cmd[256];
    char msg[320];
    size_t cl;
    char *p;
    const char *fn;
    FILE *f;

    msg[0] = 0;
    term_on();
    atexit(term_off);

    for (;;) {
        draw(insert, decode, NULL);
        k = key();
        if (k == K_QUIT) break;
        if (k == K_NONE) continue;

        if (insert) {
            if (k == K_ESC) {
                insert = 0;
            } else if (k == K_BS) {
                if (len) len--;
            } else if (len < MAXN - 1) {
                buf[len++] = (k == K_ENTER) ? '\n' : k;
            }
            continue;
        }

        if (k == 'i' || k == 'a') {
            insert = 1;
            continue;
        }
        if (k != ':') continue;

        cl = 0;
        cmd[0] = 0;
        for (;;) {
            draw(0, decode, cmd);
            k = key();
            if (k == K_QUIT) goto done;
            if (k == K_NONE) continue;
            if (k == K_ESC) {
                cmd[0] = 0;
                break;
            }
            if (k == K_BS) {
                if (!cl) break;
                cmd[--cl] = 0;
                continue;
            }
            if (k == K_ENTER) {
                p = cmd;
                while (*p == ' ') p++;
                if (*p == 'q') goto done;
                if (*p == 's') {
                    if (decode) {
                        decode = 0;
                        len = 0;
                    }
                    insert = 1;
                } else if (*p == 'd') {
                    if (!decode) {
                        decode = 1;
                        len = 0;
                    }
                    insert = 1;
                } else if (*p == 'w') {
                    p++;
                    if (*p == 'q') p++;
                    while (*p == ' ') p++;
                    fn = *p ? p : "scode.txt";
                    f = fopen(fn, "w");
                    if (f) {
                        fputs(out, f);
                        fputc('\n', f);
                        fclose(f);
                        snprintf(msg, sizeof(msg), "written: %s", fn);
                    } else {
                        snprintf(msg, sizeof(msg), "cannot write: %s", fn);
                    }
                    goto done;
                }
                break;
            }
            if (k >= 32 && k < 127 && cl < sizeof(cmd) - 1) {
                cmd[cl++] = (char)k;
                cmd[cl] = 0;
            }
        }
    }

done:
    term_off();
    fputs("\n", stdout);
    if (msg[0]) puts(msg);
    return 0;
}
