/**
 * @file count_checker.c
 * @brief Vérificateurs de comptage de mots — CORE / rules / checkers
 *
 * RESPONSABLE : DEV-D
 */

#include "../../../include/rules.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include <cjson/cJSON.h>   /* IMPORTANT : nécessaire pour parser rule->parameter */

/* ============================================================================
 * Compteur de mots simple
 * ============================================================================ */

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

/* ============================================================================
 * Extraction d'une section (utilise find_section() de section_checker.c)
 * ============================================================================ */

extern size_t find_section(const char *text, const char *section_name);

/* ============================================================================
 * CHECK_WORD_COUNT_MIN
 * ============================================================================ */

RuleResult check_word_count_min(const Rule *rule, const char *text, size_t len) {
    RuleResult result;
    memset(&result, 0, sizeof(result));
    strncpy(result.rule_id, rule->id, RULES_MAX_ID_LEN - 1);

    /* Valeurs par défaut */
    int min_words = 0;
    char section_name_buf[RULES_MAX_SECTION_LEN] = {0};

    /* Parser JSON */
    cJSON *param = cJSON_Parse(rule->parameter);
    if (param) {
        cJSON *mw = cJSON_GetObjectItem(param, "min_words");
        if (cJSON_IsNumber(mw))
            min_words = (int)mw->valuedouble;

        cJSON *sn = cJSON_GetObjectItem(param, "section");
        if (cJSON_IsString(sn))
            strncpy(section_name_buf, sn->valuestring, RULES_MAX_SECTION_LEN - 1);
    }

    /* Déterminer la zone de texte à analyser */
    const char *target = text;
    size_t target_len = len;

    if (strlen(section_name_buf) > 0) {
        size_t pos = find_section(text, section_name_buf);
        if (pos != SIZE_MAX) {
            target = text + pos;
            target_len = len - pos;
        }
    }

    /* Compter les mots */
    size_t words = count_words(target, target_len);

    if ((int)words >= min_words) {
        result.status = STATUS_PASS;
        snprintf(result.message, sizeof(result.message),
                 "%zu mots (minimum requis : %d)", words, min_words);
    } else {
        result.status = STATUS_FAIL;
        snprintf(result.message, sizeof(result.message),
                 "%zu mots — il en faut au moins %d", words, min_words);
    }

    if (param) cJSON_Delete(param);
    return result;
}

/* ============================================================================
 * CHECK_WORD_COUNT_MAX
 * ============================================================================ */

RuleResult check_word_count_max(const Rule *rule, const char *text, size_t len) {
    RuleResult result;
    memset(&result, 0, sizeof(result));
    strncpy(result.rule_id, rule->id, RULES_MAX_ID_LEN - 1);

    /* Valeurs par défaut */
    int max_words = 999999;
    char section_name_buf[RULES_MAX_SECTION_LEN] = {0};

    /* Parser JSON */
    cJSON *param = cJSON_Parse(rule->parameter);
    if (param) {
        cJSON *mw = cJSON_GetObjectItem(param, "max_words");
        if (cJSON_IsNumber(mw))
            max_words = (int)mw->valuedouble;

        cJSON *sn = cJSON_GetObjectItem(param, "section");
        if (cJSON_IsString(sn))
            strncpy(section_name_buf, sn->valuestring, RULES_MAX_SECTION_LEN - 1);
    }

    /* Déterminer la zone de texte à analyser */
    const char *target = text;
    size_t target_len = len;

    if (strlen(section_name_buf) > 0) {
        size_t pos = find_section(text, section_name_buf);
        if (pos != SIZE_MAX) {
            target = text + pos;
            target_len = len - pos;
        }
    }

    /* Compter les mots */
    size_t words = count_words(target, target_len);

    if ((int)words <= max_words) {
        result.status = STATUS_PASS;
        snprintf(result.message, sizeof(result.message),
                 "%zu mots (maximum autorisé : %d)", words, max_words);
    } else {
        result.status = STATUS_WARNING;
        snprintf(result.message, sizeof(result.message),
                 "%zu mots — ne doit pas dépasser %d", words, max_words);
    }

    if (param) cJSON_Delete(param);
    return result;
}
