/**
 * @file storage.c
 * @brief Lecture/écriture de fichiers — INFRA
 * RESPONSABLE : DEV-A
 */

#include "storage.h"
#include "encoding.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *storage_read_file(const char *filepath, size_t *out_len) {
    if (!filepath) return NULL;

    FILE *f = fopen(filepath, "rb");
    if (!f) {
        fprintf(stderr, "[ERROR] storage_read_file: impossible d'ouvrir '%s'\n", filepath);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long long size = (long long)ftell(f);
    if (size < 0 || size > 100LL * 1024 * 1024) {
        fclose(f);
        fprintf(stderr, "[ERROR] Fichier trop grand ou erreur ftell\n");
        return NULL;
    }
    fseek(f, 0, SEEK_SET);

    if (size < 0) { fclose(f); return NULL; }

    char *buf = malloc((size_t)size + 1);
    if (!buf) { fclose(f); return NULL; }

    size_t read = fread(buf, 1, (size_t)size, f);
    fclose(f);

    buf[read] = '\0';
    if (out_len) *out_len = read;

    /* Détection simple du BOM pour UTF-8 et UTF-16. */
    if (read >= 3 && (unsigned char)buf[0] == 0xEF && (unsigned char)buf[1] == 0xBB && (unsigned char)buf[2] == 0xBF) {
        /* UTF-8 BOM : ignorer les 3 octets. */
        size_t utf8_len = read - 3;
        memmove(buf, buf + 3, utf8_len + 1);
        if (out_len) *out_len = utf8_len;
        return buf;
    }

    if (read >= 2) {
        /* UTF-16 LE BOM */
        if ((unsigned char)buf[0] == 0xFF && (unsigned char)buf[1] == 0xFE) {
            size_t utf16_len = (read - 2) / sizeof(wchar_t);
            wchar_t *utf16 = (wchar_t *)(buf + 2);
            char *utf8 = encoding_utf16_to_utf8(utf16, out_len);
            free(buf);
            return utf8;
        }
        /* UTF-16 BE BOM */
        if ((unsigned char)buf[0] == 0xFE && (unsigned char)buf[1] == 0xFF) {
            /* Conversion brute-force UTF-16 BE -> UTF-8 non implémentée. */
            fprintf(stderr, "[ERROR] storage_read_file: UTF-16 BE non supporté\n");
            free(buf);
            return NULL;
        }
    }

    return buf;
}

bool storage_write_txt(const char *filepath, const char *text, size_t len) {
    if (!filepath || !text) return false;

    FILE *f = fopen(filepath, "wb");
    if (!f) return false;

    size_t written = fwrite(text, 1, len, f);
    fclose(f);
    return written == len;
}

bool storage_write_rtf(const char *filepath, const char *text, size_t len) {
    /*
     * TODO [DEV-A / TODO-STORAGE-002] :
     *   Générer les balises RTF manuellement :
     *   {\rtf1\ansi\deff0
     *   {\fonttbl{\f0 Consolas;}}
     *   {\f0\fs24 texte ici...}
     *   }
     *
     *   ATTENTION : encoder les caractères UTF-8 en \uN? pour RTF
     */
    (void)text;
    (void)len;
    fprintf(stderr, "[STUB] storage_write_rtf: TODO-STORAGE-002\n");

    /* STUB : écrire un RTF minimal */
    FILE *f = fopen(filepath, "w");
    if (!f) return false;
    fprintf(f, "{\\rtf1\\ansi TODO: export RTF non implémenté}\n");
    fclose(f);
    return true;
}

FileFormat storage_detect_format(const char *filepath) {
    if (!filepath) return FILE_FORMAT_UNKNOWN;
    const char *ext = strrchr(filepath, '.');
    if (!ext) return FILE_FORMAT_UNKNOWN;
    if (_stricmp(ext, ".txt") == 0) return FILE_FORMAT_TXT;
    if (_stricmp(ext, ".rtf") == 0) return FILE_FORMAT_RTF;
    if (_stricmp(ext, ".ie")  == 0) return FILE_FORMAT_IE;
    return FILE_FORMAT_UNKNOWN;
}

