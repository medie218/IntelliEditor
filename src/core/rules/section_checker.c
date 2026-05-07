#include <cjson/cJSON.h>
/**
 * @file section_checker.c
 * @brief Vérificateurs de sections — CORE / rules / checkers
 *
 * RESPONSABLE : DEV-D
 */

<<<<<<< HEAD
#include "rules.h"
#include <string.h>
=======
#include "../../../include/rules.h"
>>>>>>> aa759bb (feat(dev-D):Finalisation des vérificateurs du moteur de règles, corrections de sécurité mémoire, nettoyage JSON, prêt pour l'intégration)
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include <cjson/cJSON.h>   /* IMPORTANT : nécessaire pour parser rule->parameter */

/* ============================================================================
 * find_section() — recherche insensible à la casse
 * ============================================================================ */

size_t find_section(const char *text, const char *section_name) {
    if (!text || !section_name) return SIZE_MAX;

    char name_lower[128] = {0};
    strncpy(name_lower, section_name, 127);

    for (int i = 0; name_lower[i]; i++)
        name_lower[i] = tolower((unsigned char)name_lower[i]);

    const char *line = text;

    while (*line) {
        const char *end = strchr(line, '\n');
        if (!end) end = line + strlen(line);

        size_t line_len = end - line;
        char line_copy[256] = {0};
        size_t copy_len = (line_len < 255) ? line_len : 255;
        strncpy(line_copy, line, copy_len);

        char line_lower[256] = {0};
        strncpy(line_lower, line_copy, 255);

        for (int i = 0; line_lower[i]; i++)
            line_lower[i] = tolower((unsigned char)line_lower[i]);

        if (strstr(line_lower, name_lower))
            return (size_t)(line - text);

        line = (*end) ? end + 1 : end;
    }

    return SIZE_MAX;
}

/* ============================================================================
 * CHECK_SECTION_EXISTS
 * ============================================================================ */

RuleResult check_section_exists(const Rule *rule, const char *text, size_t len) {
    RuleResult result;
    memset(&result, 0, sizeof(result));
    strncpy(result.rule_id, rule->id, RULES_MAX_ID_LEN - 1);

    const char *section = rule->parameter;

    if (!section || strlen(section) == 0) {
        result.status = STATUS_ERROR;
        snprintf(result.message, sizeof(result.message),
                 "Paramètre section manquant");
        return result;
    }

    size_t pos = find_section(text, section);

    if (pos == SIZE_MAX) {
        result.status = STATUS_FAIL;
        snprintf(result.message, sizeof(result.message),
                 "Section '%s' introuvable", section);
    } else {
        result.status = STATUS_PASS;
        snprintf(result.message, sizeof(result.message),
                 "Section '%s' trouvée", section);
    }

    return result;
}

/* ============================================================================
 * CHECK_SECTION_ORDER
 * ============================================================================ */

RuleResult check_section_order(const Rule *rule, const char *text, size_t len) {
    RuleResult result;
    memset(&result, 0, sizeof(result));
    strncpy(result.rule_id, rule->id, RULES_MAX_ID_LEN - 1);

    cJSON *arr = cJSON_Parse(rule->parameter);
    if (!arr || !cJSON_IsArray(arr)) {
        result.status = STATUS_ERROR;
        snprintf(result.message, sizeof(result.message),
                 "Paramètre invalide pour section_order");
        if (arr) cJSON_Delete(arr);
        return result;
    }

    size_t last_pos = 0;
    bool order_ok = true;

    for (int i = 0; i < cJSON_GetArraySize(arr); i++) {
        cJSON *item = cJSON_GetArrayItem(arr, i);
        if (!cJSON_IsString(item)) continue;

        size_t pos = find_section(text, item->valuestring);

        if (pos == SIZE_MAX) continue;

        if (pos < last_pos) {
            order_ok = false;
            snprintf(result.message, sizeof(result.message),
                     "Section '%s' avant '%s'",
                     item->valuestring,
                     cJSON_GetArrayItem(arr, i - 1)->valuestring);
            break;
        }

        last_pos = pos;
    }

    cJSON_Delete(arr);

    if (order_ok) {
        result.status = STATUS_PASS;
        snprintf(result.message, sizeof(result.message),
                 "Ordre des sections correct");
    } else {
        result.status = STATUS_WARNING;
    }

    return result;
}

/* ============================================================================
 * CHECK_HEADING_FORMAT
 * ============================================================================ */

RuleResult check_heading_format(const Rule *rule, const char *text, size_t len) {
    RuleResult result;
    memset(&result, 0, sizeof(result));
    strncpy(result.rule_id, rule->id, RULES_MAX_ID_LEN - 1);

    cJSON *obj = cJSON_Parse(rule->parameter);
    if (!obj || !cJSON_IsObject(obj)) {
        result.status = STATUS_ERROR;
        snprintf(result.message, sizeof(result.message),
                 "Paramètre JSON invalide pour heading_format");
        if (obj) cJSON_Delete(obj);
        return result;
    }

    cJSON *level_item = cJSON_GetObjectItem(obj, "level");
    cJSON *case_item = cJSON_GetObjectItem(obj, "case");

    if (!cJSON_IsNumber(level_item) || !cJSON_IsString(case_item)) {
        result.status = STATUS_ERROR;
        snprintf(result.message, sizeof(result.message),
                 "Paramètres 'level' et 'case' requis pour heading_format");
        cJSON_Delete(obj);
        return result;
    }

    int level = level_item->valueint;
    const char *case_fmt = case_item->valuestring;

    cJSON_Delete(obj);

    char expected_prefix[8] = {0};
    for (int i = 0; i < level && i < 7; i++)
        expected_prefix[i] = '#';

    const char *line = text;

    while (*line) {
        const char *end = strchr(line, '\n');
        if (!end) end = line + strlen(line);

        size_t len_line = end - line;

        if (len_line >= (size_t)level && strncmp(line, expected_prefix, level) == 0) {
            if (strcmp(case_fmt, "uppercase") == 0) {
                for (size_t i = level; i < len_line; i++) {
                    if (isalpha((unsigned char)line[i]) &&
                        !isupper((unsigned char)line[i])) {
                        result.status = STATUS_FAIL;
                        snprintf(result.message, sizeof(result.message),
                                 "Titre non en majuscules : %.20s", line);
                        return result;
                    }
                }
            }
        }

        line = (*end) ? end + 1 : end;
    }

    result.status = STATUS_PASS;
    snprintf(result.message, sizeof(result.message),
             "Format des titres correct");

    return result;
}
