/**
 * @file regex_checker.c
 * @brief Vérificateur regex via PCRE2 — ADAPTER / regex_pcre2
 *
 * Utilisé pour :
 *   - CHECK_REGEX_FORBIDDEN
 *   - CHECK_REGEX_REQUIRED
 *
 * Dépendance : PCRE2 (8-bit)
 */

#include "../../../include/rules.h"

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ============================================================================
 * check_regex() — checker générique
 * ============================================================================ */

RuleResult check_regex(const Rule *rule, const char *text, size_t len) {
    RuleResult result;
    memset(&result, 0, sizeof(result));
    strncpy(result.rule_id, rule->id, RULES_MAX_ID_LEN - 1);
    result.rule_id[RULES_MAX_ID_LEN - 1] = '\0';

    if (!rule || !text) {
        result.status = RULE_STATUS_ERROR;
        snprintf(result.message, sizeof(result.message),
                 "Paramètres invalides (NULL)");
        return result;
    }

    /* Compiler la regex */
    int errcode = 0;
    PCRE2_SIZE erroffset = 0;

    uint32_t flags = rule->flags.case_insensitive ? PCRE2_CASELESS : 0;

    pcre2_code *re = pcre2_compile(
        (PCRE2_SPTR)rule->parameter,
        PCRE2_ZERO_TERMINATED,
        flags,
        &errcode,
        &erroffset,
        NULL
    );

    if (!re) {
        result.status = RULE_STATUS_ERROR;
        snprintf(result.message, sizeof(result.message),
                 "Regex invalide à la position %zu", (size_t)erroffset);
        return result;
    }

    /* Préparer la structure de match */
    pcre2_match_data *md = pcre2_match_data_create_from_pattern(re, NULL);
    if (!md) { result.status = RULE_STATUS_ERROR; snprintf(result.message, sizeof(result.message), "Allocation match_data échouée"); pcre2_code_free(re); return result; }

    int rc = pcre2_match(
        re,
        (PCRE2_SPTR)text,
        len,
        0,
        0,
        md,
        NULL
    );

    bool found = (rc >= 0);

    if (found) {
        PCRE2_SIZE *ov = pcre2_get_ovector_pointer(md);
        result.position = ov[0];
        result.length   = ov[1] - ov[0];
    }

    pcre2_match_data_free(md);
    pcre2_code_free(re);

    /* Interprétation selon le type */
    if (rule->check_type == CHECK_REGEX_FORBIDDEN) {
        if (found) {
            result.status = RULE_STATUS_FAIL;
            snprintf(result.message, sizeof(result.message),
                     "Expression interdite trouvée en position %zu",
                     result.position);
        } else {
            result.status = RULE_STATUS_PASS;
            snprintf(result.message, sizeof(result.message),
                     "Aucune expression interdite trouvée");
        }
    }
    else if (rule->check_type == CHECK_REGEX_REQUIRED) {
        if (found) {
            result.status = RULE_STATUS_PASS;
            snprintf(result.message, sizeof(result.message),
                     "Expression requise trouvée");
        } else {
            result.status = RULE_STATUS_FAIL;
            snprintf(result.message, sizeof(result.message),
                     "Expression requise absente");
        }
    }
    else {
        result.status = RULE_STATUS_ERROR;
        snprintf(result.message, sizeof(result.message),
                 "Type de règle non supporté par regex_checker");
    }

    return result;
}
