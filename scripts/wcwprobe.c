#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <wchar.h>
#include <locale.h>

int main(int argc, char **argv) {
    setlocale(LC_ALL, "C.UTF-8");
    FILE *list = fopen(argv[1], "r");
    if (!list) { perror(argv[1]); return 1; }
    char line[64];
    while (fgets(line, sizeof(line), list)) {
        uint32_t cp = (uint32_t)strtoul(line, NULL, 16);
        if (cp < 0x20 || (cp >= 0x7F && cp < 0xA0)) continue;
        if (cp == 0x200B || cp == 0x200C || cp == 0x200D || cp == 0xFEFF) continue;
        printf("%04X %d\n", cp, wcwidth((wchar_t)cp));
    }
    fclose(list);
    return 0;
}
