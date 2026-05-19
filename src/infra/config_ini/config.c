/**
 * @file config.c
 * @brief Parser de configuration INI — INFRA
 *
 * Parse manuellement le format INI :
 *   [Section]
 *   cle=valeur
 *   ; commentaire
 *
 * RESPONSABLE : DEV-A
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/** Supprime les espaces en début et fin de chaîne (in-place). */
static void trim(char *s) {
    /* Espaces en fin */
    int len = (int)strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) s[--len] = '\0';
    /* Espaces en début */
    char *start = s;
    while (*start && isspace((unsigned char)*start)) start++;
    if (start != s) memmove(s, start, strlen(start) + 1);
}

bool config_load(AppConfig *cfg, const char *filepath) {
    if (!cfg || !filepath) return false;
    memset(cfg, 0, sizeof(AppConfig));
    strncpy(cfg->filepath, filepath, 511); /* patched */
    cfg->filepath[511] = '\0';

    FILE *f = fopen(filepath, "r");
    if (!f) {
        /* Le fichier n'existe pas encore — créer avec les valeurs par défaut */
        config_set(cfg, "General", "language", "fr_FR");
        config_set(cfg, "General", "theme", "light");
        config_set(cfg, "General", "autosave_interval", "300");
        config_set(cfg, "LLM", "model_path", "");
        config_set(cfg, "LLM", "n_threads", "4");
        config_set(cfg, "LLM", "n_ctx", "4096");
        config_set(cfg, "Editor", "font_name", "Consolas");
        config_set(cfg, "Editor", "font_size", "12");
        config_set(cfg, "Editor", "line_numbers", "1");
        config_set(cfg, "Editor", "word_wrap", "1");
        config_save(cfg);
        return true;
    }

    char line[512];
    char current_section[CONFIG_MAX_SECTION_LEN] = "";

    while (fgets(line, sizeof(line), f)) {
        trim(line);

        /* Ignorer lignes vides et commentaires */
        if (line[0] == '\0' || line[0] == ';' || line[0] == '#') continue;

        /* Section [xxx] */
        if (line[0] == '[') {
            char *end = strchr(line, ']');
            if (end) {
                *end = '\0';
                strncpy(current_section, line + 1, CONFIG_MAX_SECTION_LEN - 1); /* patched */
                current_section[CONFIG_MAX_SECTION_LEN - 1] = '\0';
                trim(current_section);
            }
            continue;
        }

        /* Clé=valeur */
        char *eq = strchr(line, '=');
        if (eq && current_section[0]) {
            *eq = '\0';
            char key[CONFIG_MAX_KEY_LEN]     = "";
            char value[CONFIG_MAX_VALUE_LEN] = "";
            strncpy(key,   line, CONFIG_MAX_KEY_LEN - 1); /* patched */
            key[CONFIG_MAX_KEY_LEN - 1] = '\0';
            strncpy(value, eq + 1, CONFIG_MAX_VALUE_LEN - 1); /* patched */
            value[CONFIG_MAX_VALUE_LEN - 1] = '\0';
            trim(key);
            trim(value);
            config_set(cfg, current_section, key, value);
        }
    }

    fclose(f);
    return true;
}

bool config_save(const AppConfig *cfg) {
    if (!cfg || !cfg->filepath[0]) return false;

    FILE *f = fopen(cfg->filepath, "w");
    if (!f) return false;

    fprintf(f, "; Configuration IntelliEditor\n");
    fprintf(f, "; Généré automatiquement — modifiable manuellement\n\n");

    const char *last_section = NULL;
    for (size_t i = 0; i < cfg->count; i++) {
        const ConfigEntry *e = &cfg->entries[i];
        if (!last_section || strcmp(last_section, e->section) != 0) {
            if (last_section) fprintf(f, "\n");
            fprintf(f, "[%s]\n", e->section);
            last_section = e->section;
        }
        fprintf(f, "%s=%s\n", e->key, e->value);
    }

    fclose(f);
    return true;
}

const char *config_get(const AppConfig *cfg,
                       const char *section,
                       const char *key) {
    if (!cfg || !section || !key) return NULL;
    for (size_t i = 0; i < cfg->count; i++) {
        if (strcmp(cfg->entries[i].section, section) == 0 &&
            strcmp(cfg->entries[i].key,     key)     == 0) {
            return cfg->entries[i].value;
        }
    }
    return NULL;
}

void config_set(AppConfig *cfg,
                const char *section,
                const char *key,
                const char *value) {
    if (!cfg || !section || !key || !value) return;

    /* Mettre à jour si déjà présent */
    for (size_t i = 0; i < cfg->count; i++) {
        if (strcmp(cfg->entries[i].section, section) == 0 &&
            strcmp(cfg->entries[i].key,     key)     == 0) {
            strncpy(cfg->entries[i].value, value, CONFIG_MAX_VALUE_LEN - 1); /* patched */
            cfg->entries[i].value[CONFIG_MAX_VALUE_LEN - 1] = '\0';
            return;
        }
    }

    /* Ajouter une nouvelle entrée */
    if (cfg->count >= CONFIG_MAX_ENTRIES) {
        fprintf(stderr, "[WARN] config_set: limite CONFIG_MAX_ENTRIES atteinte\n");
        return;
    }

    ConfigEntry *e = &cfg->entries[cfg->count++];
    strncpy(e->section, section, CONFIG_MAX_SECTION_LEN - 1); /* patched */
    e->section[CONFIG_MAX_SECTION_LEN - 1] = '\0';
    strncpy(e->key,     key,     CONFIG_MAX_KEY_LEN     - 1); /* patched */
    e->key[CONFIG_MAX_KEY_LEN     - 1] = '\0';
    strncpy(e->value,   value,   CONFIG_MAX_VALUE_LEN   - 1); /* patched */
    e->value[CONFIG_MAX_VALUE_LEN   - 1] = '\0';
}

int config_get_int(const AppConfig *cfg,
                   const char *section,
                   const char *key,
                   int default_val) {
    const char *val = config_get(cfg, section, key);
    if (!val) return default_val;
    return atoi(val);
}

