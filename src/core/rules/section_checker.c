#ifdef HAVE_CJSON
#include <cjson/cJSON.h>
#endif
/**
 * @file section_checker.c
 * @brief Vérificateur de sections — CORE / rules / checkers
 */

#include "rules.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

/* ============================================================================
 * Helpers internes
 * ============================================================================ */

/**
 * @brief Trim des espaces en début/fin.
 */
static void trim(char *s) {
    if (!s) return;
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) s[--len] = '\0';
    while (*s && isspace((unsigned char)*s)) memmove(s, s + 1, strlen(s));
}

/**
 * @brief Vérifie si une ligne est un titre H1 (tout en majuscules).
 */
static bool is_h1(const char *line) {
    bool has_alpha = false;
    for (size_t i = 0; line[i]; i++) {
        if (isalpha((unsigned char)line[i])) {
            has_alpha = true;
            if (islower((unsigned char)line[i])) return false;
        }
    }
    return has_alpha;
}

/**
 * @brief Vérifie si une ligne est un titre H2 (commence par "## ").
 */
static bool is_h2(const char *line) {
    return strncmp(line, "## ", 3) == 0;
}

/**
 * @brief Extrait une ligne du texte (jusqu'au '\n').
 */
static size_t extract_line(const char *text, size_t start, char *out, size_t max) {
    size_t i = 0;
    while (text[start + i] && text[start + i] != '\n' && i < max - 1) {
        out[i] = text[start + i];
        i++;
    }
    out[i] = '\0';
    return i;
}

/**
 * @brief Recherche une section par son nom (H1 ou H2).
 */
static size_t find_section(const char *text, const char *section_name) {
    size_t len = strlen(text);
    size_t pos = 0;

    char line[512];

    while (pos < len) {
        size_t consumed = extract_line(text, pos, line, sizeof(line));
        trim(line);

        if (is_h1(line) || is_h2(line)) {
            if (strcasecmp(line, section_name) == 0) {
                return pos;
            }
        }

        pos += consumed + 1;
    }

    return SIZE_MAX;
}

/* ============================================================================
 * CHECK_SECTION_EXISTS
 * ============================================================================ */

RuleResult check_section_exists(const Rule *rule, const char *text, size_t len) {
    RuleResult result = {0};
    strncpy(result.rule_id, rule->id, RULES_MAX_ID_LEN - 1);
    result.rule_id[RULES_MAX_ID_LEN - 1] = '\0';

    (void)len;

    size_t pos = find_section(text, rule->parameter);

    if (pos != SIZE_MAX) {
        result.status = STATUS_PASS;
        snprintf(result.message, sizeof(result.message),
                 "Section '%s' trouvée", rule->parameter);
        result.position = pos;
    } else {
        result.status = STATUS_FAIL;
        snprintf(result.message, sizeof(result.message),
                 "Section '%s' manquante", rule->parameter);
        result.position = 0;
    }

    return result;
}

/* ============================================================================
 * CHECK_SECTION_ORDER
 * ============================================================================ */

RuleResult check_section_order(const Rule *rule, const char *text, size_t len) {
    RuleResult result = {0};
    strncpy(result.rule_id, rule->id, RULES_MAX_ID_LEN - 1);
    result.rule_id[RULES_MAX_ID_LEN - 1] = '\0';

    (void)len;

    /**
     * rule->parameter contient un tableau JSON sérialisé :
     *   ["Introduction", "Méthodologie", "Résultats", "Conclusion"]
     *
     * On doit :
     *   1. parser ce JSON
     *   2. trouver la position de chaque section
     *   3. vérifier que pos[i] < pos[i+1]
     */

    cJSON *arr = cJSON_Parse(rule->parameter);
    if (!arr || !cJSON_IsArray(arr)) {
        result.status = STATUS_ERROR;
        snprintf(result.message, sizeof(result.message),
                 "Paramètre invalide pour section_order");
        cJSON_Delete(arr);
        return result;
    }

    size_t last_pos = 0;
    bool first = true;

    for (int i = 0; i < cJSON_GetArraySize(arr); i++) {
        cJSON *item = cJSON_GetArrayItem(arr, i);
        if (!cJSON_IsString(item)) continue;

        size_t pos = find_section(text, item->valuestring);

        if (pos == SIZE_MAX) {
            // section absente → on ignore mais on note
            continue;
        }

        if (!first && pos < last_pos) {
            result.status = STATUS_FAIL;
            snprintf(result.message, sizeof(result.message),
                     "Ordre incorrect : '%s' apparaît avant '%s'",
                     item->valuestring,
                     cJSON_GetArrayItem(arr, i - 1)->valuestring);
            cJSON_Delete(arr);
            return result;
        }

        last_pos = pos;
        first = false;
    }

    cJSON_Delete(arr);

    result.status = STATUS_PASS;
    snprintf(result.message, sizeof(result.message),
             "Ordre des sections correct");
    return result;
}

/* ============================================================================
 * CHECK_HEADING_FORMAT
 * ============================================================================ */

RuleResult check_heading_format(const Rule *rule, const char *text, size_t len) {
    RuleResult result = {0};
    strncpy(result.rule_id, rule->id, RULES_MAX_ID_LEN - 1);
    result.rule_id[RULES_MAX_ID_LEN - 1] = '\0';

    (void)len;

    /**
     * rule->parameter = {"level": 1, "case": "uppercase"}
     */

    cJSON *obj = cJSON_Parse(rule->parameter);
    if (!obj) {
        result.status = STATUS_ERROR;
        snprintf(result.message, sizeof(result.message),
                 "Paramètre JSON invalide");
        return result;
    }

    int level = cJSON_GetObjectItem(obj, "level")->valueint;
    const char *case_fmt = cJSON_GetObjectItem(obj, "case")->valuestring;

    bool want_upper = (strcmp(case_fmt, "uppercase") == 0);

    size_t pos = 0;
    char line[512];

    while (pos < strlen(text)) {
        size_t consumed = extract_line(text, pos, line, sizeof(line));
        trim(line);

        bool is_title =
            (level == 1 && is_h1(line)) ||
            (level == 2 && is_h2(line));

        if (is_title) {
            if (want_upper) {
                for (size_t i = 0; line[i]; i++) {
                    if (isalpha((unsigned char)line[i]) &&
                        islower((unsigned char)line[i])) {
                        result.status = STATUS_FAIL;
                        snprintf(result.message, sizeof(result.message),
                                 "Titre '%s' n'est pas en majuscules", line);
                        cJSON_Delete(obj);
                        return result;
                    }
                }
            }
        }

        pos += consumed + 1;
    }

    cJSON_Delete(obj);

    result.status = STATUS_PASS;
    snprintf(result.message, sizeof(result.message),
             "Format des titres correct");
    return result;
}

