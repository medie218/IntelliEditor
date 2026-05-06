#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "encoding.h"

char *encoding_utf16_to_utf8(const wchar_t *w, size_t *out_len) {
    if (!w) return NULL;
    size_t len = wcslen(w);
    size_t buf_size = len * 4 + 1;
    char *utf8 = malloc(buf_size);
    if (!utf8) return NULL;

    int r = wcstombs(utf8, w, buf_size);
    if (r < 0) { free(utf8); return NULL; }
    if (out_len) *out_len = r;
    utf8[r] = 0;
    return utf8;
}
