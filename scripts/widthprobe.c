/* Measure the real column width of every candidate character in xterm.
 *
 * Build:   gcc -O2 -o /tmp/widthprobe scripts/widthprobe.c
 * Run:     xterm -e /tmp/widthprobe /tmp/cps.txt > /tmp/widths.txt
 *
 * For each cp: position cursor to 1,1, print the char, query the cursor
 * column via CPR (\033[6n). width = col - 1.  The result is the
 * authoritative layout width as seen by a standard xterm.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>

static int read_cpr_col(int fd)
{
    char buf[64];
    int n = 0;
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    fd_set rfds;
    while (n < 63) {
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        if (select(fd + 1, &rfds, NULL, NULL, &tv) <= 0)
            return -1;
        if (read(fd, buf + n, 1) != 1)
            return -1;
        n++;
        if (buf[n - 1] == 'R')
            break;
    }
    buf[n] = 0;
    int row = 0, col = 0;
    if (sscanf(buf, "\033[%d;%dR", &row, &col) == 2)
        return col;
    if (sscanf(buf, "\033[%dR", &col) == 1)
        return col;
    return -1;
}

static int utf8_encode(uint32_t cp, char *out)
{
    if (cp < 0x80) {
        out[0] = (char)cp;
        return 1;
    }
    if (cp < 0x800) {
        out[0] = (char)(0xC0 | (cp >> 6));
        out[1] = (char)(0x80 | (cp & 63));
        return 2;
    }
    if (cp < 0x10000) {
        out[0] = (char)(0xE0 | (cp >> 12));
        out[1] = (char)(0x80 | ((cp >> 6) & 63));
        out[2] = (char)(0x80 | (cp & 63));
        return 3;
    }
    out[0] = (char)(0xF0 | (cp >> 18));
    out[1] = (char)(0x80 | ((cp >> 12) & 63));
    out[2] = (char)(0x80 | ((cp >> 6) & 63));
    out[3] = (char)(0x80 | (cp & 63));
    return 4;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s <cp-list-file>\n", argv[0]);
        return 1;
    }
    int fd = open("/dev/tty", O_RDWR | O_NOCTTY);
    if (fd < 0) {
        perror("/dev/tty");
        return 1;
    }
    /* give xterm time to map the window and start its event loop */
    usleep(800000);
    /* flush any startup cruft */
    struct timeval tv0; tv0.tv_sec = 0; tv0.tv_usec = 100000;
    fd_set rfds0; FD_ZERO(&rfds0); FD_SET(fd, &rfds0);
    char drain[256];
    while (select(fd + 1, &rfds0, NULL, NULL, &tv0) > 0) {
        if (read(fd, drain, sizeof(drain)) <= 0) break;
    }
    write(fd, "\033[?25l", 6); /* hide cursor */
    write(fd, "\033[2J\033[1;1H", 11);

    FILE *list = fopen(argv[1], "r");
    if (!list) {
        perror(argv[1]);
        return 1;
    }
    char line[64];
    char utf8[8];
    while (fgets(line, sizeof(line), list)) {
        uint32_t cp = (uint32_t)strtoul(line, NULL, 16);
        if (cp < 0x20 || (cp >= 0x7F && cp < 0xA0))
            continue; /* controls */
        if (cp == 0x200B || cp == 0x200C || cp == 0x200D || cp == 0xFEFF)
            continue; /* zero-width */
        int n = utf8_encode(cp, utf8);
        write(fd, "\033[1;1H", 6);
        write(fd, utf8, (size_t)n);
        usleep(1200);
        int col = read_cpr_col(fd);
        if (col < 0) {
            fprintf(stderr, "no CPR for U+%04X\n", cp);
            continue;
        }
        printf("%04X %d\n", cp, col - 1);
        fflush(stdout);
    }
    fclose(list);
    write(fd, "\033[2J\033[1;1H\033[?25h", 14);
    close(fd);
    return 0;
}
