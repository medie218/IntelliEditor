/**
 * @file encoding.c
 * @brief Conversion UTF-8 ↔ UTF-16 — INFRA
 *
 * Utilise les API Win32 MultiByteToWideChar / WideCharToMultiByte.
 * RESPONSABLE : DEV-A
 */

#include "../../include/encoding.h"
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

wchar_t *encoding_utf8_to_utf16(const char *utf8, size_t *out_len) {
    if (!utf8) return NULL;

    /*
     * Étape 1 : calculer la taille nécessaire
     * Étape 2 : allouer le buffer
     * Étape 3 : effectuer la conversion
     */
    int needed = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    if (needed <= 0) {
        fprintf(stderr, "[ERROR] encoding_utf8_to_utf16: calcul taille échoué\n");
        return NULL;
    }

    wchar_t *buf = malloc((size_t)needed * sizeof(wchar_t));
    if (!buf) return NULL;

    int written = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, buf, needed);
    if (written <= 0) {
        free(buf);
        return NULL;
    }

    if (out_len) *out_len = (size_t)(written - 1); /* -1 pour le null-terminator */
    return buf;
}

char *encoding_utf16_to_utf8(const wchar_t *utf16, size_t *out_len) {
    if (!utf16) return NULL;

    int needed = WideCharToMultiByte(CP_UTF8, 0, utf16, -1, NULL, 0, NULL, NULL);
    if (needed <= 0) return NULL;

    char *buf = malloc((size_t)needed);
    if (!buf) return NULL;

    int written = WideCharToMultiByte(CP_UTF8, 0, utf16, -1, buf, needed, NULL, NULL);
    if (written <= 0) {
        free(buf);
        return NULL;
    }

    if (out_len) *out_len = (size_t)(written - 1);
    return buf;
}

bool encoding_is_valid_utf8(const char *data, size_t len) {
    /*
     * TODO [DEV-A / TODO-ENCODING-001] :
     *   Implémenter la validation UTF-8 manuellement.
     *   Algorithme :
     *   - Octet < 0x80 : ASCII valide
     *   - Octet 0xC2-0xDF : séquence 2 octets (suivie d'un 0x80-0xBF)
     *   - Octet 0xE0-0xEF : séquence 3 octets
     *   - Octet 0xF0-0xF4 : séquence 4 octets
     *   - Tout autre : invalide
     */
    (void)data;
    (void)len;
    fprintf(stderr, "[STUB] encoding_is_valid_utf8: TODO-ENCODING-001\n");
    return true; /* STUB : supposer valide */
}

size_t encoding_utf8_char_count(const char *utf8) {
    /*
     * TODO [DEV-A / TODO-ENCODING-002] :
     *   Compter les code points Unicode (pas les octets).
     *   Un caractère UTF-8 commence par : 0xxxxxxx, 110xxxxx, 1110xxxx, 11110xxx
     *   Les octets de continuation commencent par 10xxxxxx (ne pas compter).
     */
    if (!utf8) return 0;
    size_t count = 0;
    while (*utf8) {
        /* Compter les octets de début de séquence UTF-8 (pas les continuations) */
        if ((*utf8 & 0xC0) != 0x80) count++;
        utf8++;
    }
    return count;
}

const char *encoding_utf8_char_at(const char *utf8, size_t index) {
    if (!utf8) return NULL;
    size_t count = 0;
    while (*utf8) {
        if ((*utf8 & 0xC0) != 0x80) {
            if (count == index) return utf8;
            count++;
        }
        utf8++;
    }
    return NULL;
}
