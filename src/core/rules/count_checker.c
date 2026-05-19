/**
 * @file count_checker.c
 * @brief Vérificateurs de comptage de mots — CORE / rules / checkers
 */

#include "rules.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#ifdef HAVE_CJSON
#include <cjson/cJSON.h>
#endif

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
    result.rule_id[RULES_MAX_ID_LEN - 1] = '\0';

    /* Valeurs par défaut */
    int min_words = 0;
    char section_name_buf[RULES_MAX_SECTION_LEN] = {0};

#ifndef HAVE_CJSON
    /* Si pas de cJSON, on essaie de parser manuellement si c'est juste un nombre */
    if (isdigit((unsigned char)rule->parameter[0])) {
        min_words = atoi(rule->parameter);
    }
#else
    /* Parser JSON */
    cJSON *param = cJSON_Parse(rule->parameter);
    if (param) {
        cJSON *mw = cJSON_GetObjectItem(param, "min_words");
        if (mw && (mw->type == 3)) // cJSON_Number is 3 in some versions, but better use cJSON_IsNumber
            min_words = mw->valueint; // Simplified for stub safety

        cJSON *sn = cJSON_GetObjectItem(param, "section");
        if (sn && (sn->type == 4)) // cJSON_String
            strncpy(section_name_buf, sn->valuestring, RULES_MAX_SECTION_LEN - 1);
        
        cJSON_Delete(param);
    }
#endif

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
        result.status = RULE_STATUS_PASS;
        snprintf(result.message, sizeof(result.message),
                 "%zu mots (minimum requis : %d)", words, min_words);
    } else {
        result.status = RULE_STATUS_FAIL;
        snprintf(result.message, sizeof(result.message),
                 "%zu mots — il en faut au moins %d", words, min_words);
    }

    return result;
}

/* ============================================================================
 * CHECK_WORD_COUNT_MAX
 * ============================================================================ */

RuleResult check_word_count_max(const Rule *rule, const char *text, size_t len) {
    RuleResult result;
    memset(&result, 0, sizeof(result));
    strncpy(result.rule_id, rule->id, RULES_MAX_ID_LEN - 1);
    result.rule_id[RULES_MAX_ID_LEN - 1] = '\0';

    /* Valeurs par défaut */
    int max_words = 999999;
    char section_name_buf[RULES_MAX_SECTION_LEN] = {0};

#ifndef HAVE_CJSON
    if (isdigit((unsigned char)rule->parameter[0])) {
        max_words = atoi(rule->parameter);
    }
#else
    /* Parser JSON */
    cJSON *param = cJSON_Parse(rule->parameter);
    if (param) {
        cJSON *mw = cJSON_GetObjectItem(param, "max_words");
        if (mw && (mw->type == 3))
            max_words = mw->valueint;

        cJSON *sn = cJSON_GetObjectItem(param, "section");
        if (sn && (sn->type == 4))
            strncpy(section_name_buf, sn->valuestring, RULES_MAX_SECTION_LEN - 1);
        
        cJSON_Delete(param);
    }
#endif

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
        result.status = RULE_STATUS_PASS;
        snprintf(result.message, sizeof(result.message),
                 "%zu mots (maximum autorisé : %d)", words, max_words);
    } else {
        result.status = RULE_STATUS_WARNING;
        snprintf(result.message, sizeof(result.message),
                 "%zu mots — ne doit pas dépasser %d", words, max_words);
    }

    return result;
}
