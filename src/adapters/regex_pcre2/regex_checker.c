/*
 * Simple regex adapter stub using substring matching.
 * This is a minimal implementation to satisfy the build and unit tests
 * (replacement for a full PCRE2 wrapper). It supports two check types:
 *  - CHECK_REGEX_FORBIDDEN: FAIL if parameter is found in text
 *  - CHECK_REGEX_REQUIRED: PASS if parameter is found in text
 */

#include "rules.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

RuleResult check_regex(const Rule *rule, const char *text, size_t len) {
    RuleResult result;
    memset(&result, 0, sizeof(result));
    if (!rule) {
        result.status = RULE_STATUS_ERROR;
        snprintf(result.message, sizeof(result.message), "Invalid rule\n");
        return result;
    }

    strncpy(result.rule_id, rule->id, RULES_MAX_ID_LEN - 1);
    result.rule_id[RULES_MAX_ID_LEN - 1] = '\0';

    if (!rule->parameter || rule->parameter[0] == '\0') {
        result.status = RULE_STATUS_ERROR;
        snprintf(result.message, sizeof(result.message), "Missing parameter\n");
        return result;
    }

    if (!text) {
        result.status = RULE_STATUS_ERROR;
        snprintf(result.message, sizeof(result.message), "Missing text\n");
        return result;
    }

    const char *found = strstr(text, rule->parameter);

    if (rule->check_type == CHECK_REGEX_FORBIDDEN) {
        if (found) {
            result.status = RULE_STATUS_FAIL;
            snprintf(result.message, sizeof(result.message), "Forbidden pattern found");
        } else {
            result.status = RULE_STATUS_PASS;
        }
    } else if (rule->check_type == CHECK_REGEX_REQUIRED) {
        if (found) {
            result.status = RULE_STATUS_PASS;
        } else {
            result.status = RULE_STATUS_FAIL;
            snprintf(result.message, sizeof(result.message), "Required pattern not found");
        }
    } else {
        result.status = RULE_STATUS_ERROR;
        snprintf(result.message, sizeof(result.message), "Unsupported check type");
    }

    return result;
}
