/**
 * @file rules.h
 * @brief Structures et API du moteur de règles — CORE
 */

#ifndef RULES_H
#define RULES_H

#include <stddef.h>
#include <stdbool.h>

/* ============================================================================
 * CONSTANTES
 * ============================================================================ */

#define RULES_MAX_RULES        128
#define RULES_MAX_ID_LEN       32
#define RULES_MAX_DESC_LEN     256
#define RULES_MAX_CATEGORY_LEN 32
#define RULES_MAX_PARAM_LEN    256
#define RULES_MAX_SECTION_LEN  64
#define RULES_MAX_MESSAGE_LEN  256

/* ============================================================================
 * ENUMS
 * ============================================================================ */

typedef enum {
    CHECK_UNKNOWN = 0,
    CHECK_SECTION_EXISTS,
    CHECK_SECTION_ORDER,
    CHECK_WORD_COUNT_MIN,
    CHECK_WORD_COUNT_MAX,
    CHECK_REGEX_FORBIDDEN,
    CHECK_REGEX_REQUIRED,
    CHECK_HEADING_FORMAT,
    CHECK_CITATION_PRESENT,
    CHECK_LLM_SEMANTIC
} CheckType;

typedef enum {
    RULE_SEVERITY_ERROR = 0,
    RULE_SEVERITY_WARNING,
    RULE_SEVERITY_INFO
} Severity;

typedef enum {
    RULE_STATUS_PASS = 0,
    RULE_STATUS_FAIL,
    RULE_STATUS_WARNING,
    RULE_STATUS_PENDING,
    RULE_STATUS_ERROR,
    RULE_STATUS_SKIPPED
} RuleStatus;

/* ============================================================================
 * STRUCTURES
 * ============================================================================ */

typedef struct {
    char document_type[64];
    char version[32];
    char author[64];
} RuleMeta;

typedef struct {
    char id[RULES_MAX_ID_LEN];
    char description[RULES_MAX_DESC_LEN];
    char category[RULES_MAX_CATEGORY_LEN];

    CheckType check_type;
    Severity severity;

    char parameter[RULES_MAX_PARAM_LEN];

    struct {
        bool case_insensitive;
    } flags;

    char target_section[RULES_MAX_SECTION_LEN];

} Rule;

typedef struct {
    RuleMeta meta;
    Rule rules[RULES_MAX_RULES];
    size_t rule_count;
} RuleSet;

typedef struct {
    char rule_id[RULES_MAX_ID_LEN];
    RuleStatus status;
    char message[RULES_MAX_MESSAGE_LEN];

    size_t position;
    size_t length;

} RuleResult;

typedef struct {
    RuleResult results[RULES_MAX_RULES];
    size_t result_count;

    size_t pass_count;
    size_t fail_count;
    size_t warning_count;
    size_t pending_count;

} RuleReport;

/* ============================================================================
 * API
 * ============================================================================ */

/* Cycle de vie */
RuleSet    *ruleset_create(void);
void        ruleset_destroy(RuleSet *set);

RuleReport *rulereport_create(void);
void        rulereport_destroy(RuleReport *report);

/* Parser JSON */
RuleSet *ruleset_load_from_file(const char *filepath);

/* Évaluation */
RuleReport *rules_evaluate(const RuleSet *set, const char *text, size_t len);

/* Mise à jour LLM */
void rules_update_llm_result(RuleReport *report,
                             const char *rule_id,
                             RuleStatus  status,
                             const char *message);

/* Utilitaires */
const char *check_type_to_string(CheckType type);
const char *rule_status_to_string(RuleStatus status);

#endif /* RULES_H */
