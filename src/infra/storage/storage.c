#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "encoding.h"
#include "storage.h"

char *storage_read_file(const char *filepath, size_t *out_len) {
    FILE *f = fopen(filepath, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buf = malloc(size + 1);
    if (!buf) { fclose(f); return NULL; }

    size_t r = fread(buf, 1, size, f);
    fclose(f);
    buf[r] = 0;

    if (out_len) *out_len = r;

    /* Détecter BOM UTF-16 LE (FF FE) */
    if (r >= 2 && (unsigned char)buf[0] == 0xFF && (unsigned char)buf[1] == 0xFE) {
        /* Convertir UTF-16 vers UTF-8 via encoding.h */
        wchar_t *w = (wchar_t *)(buf + 2);
        char *utf8 = encoding_utf16_to_utf8(w, out_len);
        free(buf);
        return utf8;
    }

    return buf;
}

bool storage_write_file(const char *filepath, const char *text) {
    if (!filepath || !text) return false;

    FILE *f = fopen(filepath, "wb");
    if (!f) return false;

    size_t len = strlen(text);
    size_t written = fwrite(text, 1, len, f);
    fclose(f);

    return written == len;
}
