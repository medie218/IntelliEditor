/**
 * @file storage.c
 * @brief Lecture/écriture de fichiers — INFRA
 * RESPONSABLE : DEV-A
 */

#include "storage.h"
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
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size < 0) { fclose(f); return NULL; }

    char *buf = malloc((size_t)size + 1);
    if (!buf) { fclose(f); return NULL; }

    size_t read = fread(buf, 1, (size_t)size, f);
    fclose(f);

    buf[read] = '\0';
    if (out_len) *out_len = read;

    /*
     * TODO [DEV-A / TODO-STORAGE-001] :
     *   Détecter l'encodage (BOM UTF-16, UTF-8, Latin-1)
     *   et convertir en UTF-8 si nécessaire.
     *   Voir encoding.h pour les fonctions de conversion.
     */

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
