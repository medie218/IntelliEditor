/**
 * @file count_checker.c
 * @brief Vérificateur de comptage de mots — CORE / rules / checkers
 */

#include "../../../include/rules.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* ============================================================================
 * Helpers internes
 * ============================================================================ */

/**
 * @brief Compte les mots dans un bloc de texte.
 *
 * TODO futur : améliorer pour gérer apostrophes, UTF-8, ponctuation.
 */
static size_t count_words(const char *text, size_t len) {
    size_t count = 0;
    bool in_word = false;

    for (size_t i = 0; i < len; i++) {
        if (isspace((unsigned char)text[i])) {
            in_word = false;
        } else if (!in_word) {
            in_word = true;
            count++;
        }
    }

    return count;
}

/**
 * @brief Recherche une section dans le texte (H1 ou H2).
 *
 * Retourne l’offset en octets, ou SIZE_MAX si absent.
 */
static size_t find_section(const char *text, const char *section_name) {
    if (!text || !section_name) return SIZE_MAX;

    size_t pos = 0;
    const char *p = text;

    char buffer[512];

    while (*p) {
        size_t i = 0;
        while (p[i] && p[i] != '\n' && i < sizeof(buffer) - 1) {
            buffer[i] = p[i];
            i++;
        }
        buffer[i] = 0;

        /* Nettoyage */
        while (buffer[0] && isspace((unsigned char)buffer[0]))
            memmove(buffer, buffer + 1, strlen(buffer));

        /* H1 : tout en majuscules */
        bool is_h1 = true;
        bool has_alpha = false;
        for (size_t k = 0; buffer[k]; k++) {
            if (isalpha((unsigned char)buffer[k])) {
                has_alpha = true;
                if (!isupper((unsigned char)buffer[k])) {
                    is_h1 = false;
                    break;
                }
            }
        }
        if (!has_alpha) is_h1 = false;

        /* H2 : commence par "## " */
        bool is_h2 = (strncmp(buffer, "## ", 3) == 0);

        const char *title = buffer;
        if (is_h2) title = buffer + 3;

        if ((is_h1 || is_h2) && strcasecmp(title, section_name) == 0) {
            return pos;
        }

        /* Avancer */
        while (*p && *p != '\n') p++;
        if (*p == '\n') p++;
        pos = p - text;
    }

    return SIZE_MAX;
}

/**
 * @brief Extrait le texte d'une section jusqu'à la section suivante.
 */
static char *extract_section_text(const char *text, size_t start_pos) {
    if (start_pos == SIZE_MAX) return NULL;

    const char *start = text + start_pos;

    /* Avancer d'une ligne (le titre) */
    while (*start && *start != '\n') start++;
    if (*start == '\n') start++;

    const char *end = start;

    /* Chercher la prochaine section */
    while (*end) {
        if (*end == '\n') {
            const char *line = end + 1;

            /* Vérifier si c'est un titre */
            bool is_title = false;

            /* H1 */
            bool has_alpha = false;
            bool all_upper = true;
            size_t i = 0;
            while (line[i] && line[i] != '\n') {
                if (isalpha((unsigned char)line[i])) {
                    has_alpha = true;
                    if (!isupper((unsigned char)line[i])) all_upper = false;
                }
                i++;
            }
            if (has_alpha && all_upper) is_title = true;

            /* H2 */
            if (strncmp(line, "## ", 3) == 0) is_title = true;

            if (is_title) break;
        }
        end++;
    }

    size_t len = end - start;
    char *out = malloc(len + 1);
    if (!out) return NULL;

    memcpy(out, start, len);
    out[len] = 0;
    return out;
}

/**
 * @brief Parse manuellement un JSON simple : {"section":"X","min_words":300}
 */
static void parse_min_rule(const char *json, char *section_out, int *min_words) {
    *min_words = 0;
    section_out[0] = 0;

    const char *s = strstr(json, "\"section\"");
    if (s) {
        s = strchr(s, ':');
        if (s) {
            s++;
            while (*s && *s != '"') s++;
            if (*s == '"') {
                s++;
                const char *start = s;
                while (*s && *s != '"') s++;
                size_t len = s - start;
                strncpy(section_out, start, len);
                section_out[len] = 0;
            }
        }
    }

    const char *m = strstr(json, "\"min_words\"");
    if (m) {
        m = strchr(m, ':');
        if (m) *min_words = atoi(m + 1);
    }
}

static void parse_max_rule(const char *json, char *section_out, int *max_words) {
    *max_words = 0;
    section_out[0] = 0;

    const char *s = strstr(json, "\"section\"");
    if (s) {
        s = strchr(s, ':');
        if (s) {
            s++;
            while (*s && *s != '"') s++;
            if (*s == '"') {
                s++;
                const char *start = s;
                while (*s && *s != '"') s++;
                size_t len = s - start;
                strncpy(section_out, start, len);
                section_out[len] = 0;
            }
        }
    }

    const char *m = strstr(json, "\"max_words\"");
    if (m) {
        m = strchr(m, ':');
        if (m) *max_words = atoi(m + 1);
    }
}

/* ============================================================================
 * CHECK_WORD_COUNT_MIN
 * ============================================================================ */

RuleResult check_word_count_min(const Rule *rule, const char *text, size_t len) {
    RuleResult result = {0};
    strncpy(result.rule_id, rule->id, RULES_MAX_ID_LEN - 1);

    char section[128];
    int min_words = 0;

    parse_min_rule(rule->parameter, section, &min_words);

    if (min_words <= 0 || section[0] == 0) {
        result.status = STATUS_ERROR;
        snprintf(result.message, sizeof(result.message),
                 "Paramètre invalide pour word_count_min");
        return result;
    }

    size_t pos = find_section(text, section);
    if (pos == SIZE_MAX) {
        result.status = STATUS_FAIL;
        snprintf(result.message, sizeof(result.message),
                 "Section '%s' introuvable", section);
        return result;
    }

    char *section_text = extract_section_text(text, pos);
    if (!section_text) {
        result.status = STATUS_ERROR;
        snprintf(result.message, sizeof(result.message),
                 "Impossible d'extraire la section '%s'", section);
        return result;
    }

    size_t words = count_words(section_text, strlen(section_text));
    free(section_text);

    if ((int)words >= min_words) {
        result.status = STATUS_PASS;
        snprintf(result.message, sizeof(result.message),
                 "%zu mots (minimum requis : %d)", words, min_words);
    } else {
        result.status = STATUS_FAIL;
        snprintf(result.message, sizeof(result.message),
                 "%zu mots — il en faut au moins %d", words, min_words);
    }

    result.position = pos;
    return result;
}

/* ============================================================================
 * CHECK_WORD_COUNT_MAX
 * ============================================================================ */

RuleResult check_word_count_max(const Rule *rule, const char *text, size_t len) {
    RuleResult result = {0};
    strncpy(result.rule_id, rule->id, RULES_MAX_ID_LEN - 1);

    char section[128];
    int max_words = 0;

    parse_max_rule(rule->parameter, section, &max_words);

    if (max_words <= 0 || section[0] == 0) {
        result.status = STATUS_ERROR;
        snprintf(result.message, sizeof(result.message),
                 "Paramètre invalide pour word_count_max");
        return result;
    }

    return result;
}
