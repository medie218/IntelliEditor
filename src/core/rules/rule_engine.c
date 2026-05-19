/**
 * @file rule_engine.c
 * @brief Moteur d'évaluation des règles métier — CORE / rules
 *
 * RÔLE :
 *   - Orchestrer l'évaluation des règles sur un texte brut.
 *   - Déléguer aux checkers spécialisés selon CheckType.
 *   - Produire un RuleReport agrégé.
 *
 * CHECKERS :
 *   - section_checker.c → CHECK_SECTION_EXISTS, CHECK_SECTION_ORDER, CHECK_HEADING_FORMAT
 *   - count_checker.c   → CHECK_WORD_COUNT_MIN, CHECK_WORD_COUNT_MAX
 *   - regex_checker.c   → CHECK_REGEX_FORBIDDEN, CHECK_REGEX_REQUIRED
 *   - LLM (asynchrone)  → CHECK_LLM_SEMANTIC (RULE_STATUS_PENDING)
 */

#include "rules.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * FORWARD DECLARATIONS DES CHECKERS
 * ============================================================================ */

extern RuleResult check_section_exists(const Rule*, const char*, size_t);
extern RuleResult check_section_order(const Rule*, const char*, size_t);
extern RuleResult check_word_count_min(const Rule*, const char*, size_t);
extern RuleResult check_word_count_max(const Rule*, const char*, size_t);
extern RuleResult check_heading_format(const Rule*, const char*, size_t);
extern RuleResult check_regex(const Rule*, const char*, size_t);

/* ============================================================================
 * CYCLE DE VIE
 * ============================================================================ */

RuleSet *ruleset_create(void) {
    RuleSet *set = calloc(1, sizeof(RuleSet));
    if (!set) return NULL;
    set->rule_count = 0;
    return set;
}

void ruleset_destroy(RuleSet *set) {
    free(set);
}

RuleReport *rulereport_create(void) {
    RuleReport *report = calloc(1, sizeof(RuleReport));
    if (!report) return NULL;
    report->result_count = 0;
    return report;
}

void rulereport_destroy(RuleReport *report) {
    free(report);
}

/* ============================================================================
 * ÉVALUATION D'UNE SEULE RÈGLE
 * ============================================================================ */

static RuleResult evaluate_single_rule(const Rule *rule,
                                       const char *text,
                                       size_t      len) {
    RuleResult result;
    memset(&result, 0, sizeof(result));
    strncpy(result.rule_id, rule->id, RULES_MAX_ID_LEN - 1);
    result.rule_id[RULES_MAX_ID_LEN - 1] = '\0';

    printf("[DEBUG] Évaluation règle %s (type=%s)\n",
           rule->id,
           check_type_to_string(rule->check_type));

    switch (rule->check_type) {
        case CHECK_SECTION_EXISTS:
            return check_section_exists(rule, text, len);

        case CHECK_SECTION_ORDER:
            return check_section_order(rule, text, len);

        case CHECK_WORD_COUNT_MIN:
            return check_word_count_min(rule, text, len);

        case CHECK_WORD_COUNT_MAX:
            return check_word_count_max(rule, text, len);

        case CHECK_REGEX_FORBIDDEN:
        case CHECK_REGEX_REQUIRED:
            return check_regex(rule, text, len);

        case CHECK_HEADING_FORMAT:
            return check_heading_format(rule, text, len);

        case CHECK_LLM_SEMANTIC:
            /*
             * Les règles LLM sont ASYNCHRONES.
             * On les marque PENDING ici, et le thread LLM
             * mettra à jour le résultat via rules_update_llm_result().
             */
            result.status = RULE_STATUS_PENDING;
            snprintf(result.message, sizeof(result.message),
                     "Vérification sémantique LLM en attente...");
            return result;

        default:
            result.status = RULE_STATUS_SKIPPED;
            snprintf(result.message, sizeof(result.message),
                     "Type de règle inconnu: %d", rule->check_type);
            return result;
    }
}

/* ============================================================================
 * ÉVALUATION D'UN ENSEMBLE DE RÈGLES
 * ============================================================================ */

RuleReport *rules_evaluate(const RuleSet *set, const char *text, size_t len) {
    if (!set || !text) return NULL;

    RuleReport *report = rulereport_create();
    if (!report) return NULL;

    report->result_count = set->rule_count;

    for (size_t i = 0; i < set->rule_count; i++) {
        report->results[i] = evaluate_single_rule(&set->rules[i], text, len);

        switch (report->results[i].status) {
            case RULE_STATUS_PASS:    report->pass_count++;    break;
            case RULE_STATUS_FAIL:    report->fail_count++;    break;
            case RULE_STATUS_WARNING: report->warning_count++; break;
            case RULE_STATUS_PENDING: report->pending_count++; break;
            default: break;
        }
    }

    printf("[INFO] Évaluation terminée: %zu règles, %zu OK, %zu KO, %zu en attente\n",
           report->result_count,
           report->pass_count,
           report->fail_count,
           report->pending_count);

    return report;
}

/* ============================================================================
 * MISE À JOUR DES RÉSULTATS LLM (ASYNCHRONE)
 * ============================================================================ */

void rules_update_llm_result(RuleReport *report,
                             const char *rule_id,
                             RuleStatus  status,
                             const char *message) {
    if (!report || !rule_id) return;

    for (size_t i = 0; i < report->result_count; i++) {
        if (strcmp(report->results[i].rule_id, rule_id) == 0) {
            if (report->results[i].status == RULE_STATUS_PENDING)
                report->pending_count--;

            report->results[i].status = status;

            if (message) {
                strncpy(report->results[i].message, message, sizeof(report->results[i].message) - 1);
                report->results[i].message[sizeof(report->results[i].message) - 1] = '\0';
            }

            if (status == RULE_STATUS_PASS)    report->pass_count++;
            if (status == RULE_STATUS_FAIL)    report->fail_count++;
            if (status == RULE_STATUS_WARNING) report->warning_count++;
            return;
        }
    }

    fprintf(stderr, "[WARN] rules_update_llm_result: règle '%s' non trouvée\n", rule_id);
}

/* ============================================================================
 * UTILITAIRES
 * ============================================================================ */

const char *check_type_to_string(CheckType type) {
    switch (type) {
        case CHECK_SECTION_EXISTS:   return "section_exists";
        case CHECK_SECTION_ORDER:    return "section_order";
        case CHECK_WORD_COUNT_MIN:   return "word_count_min";
        case CHECK_WORD_COUNT_MAX:   return "word_count_max";
        case CHECK_REGEX_FORBIDDEN:  return "regex_forbidden";
        case CHECK_REGEX_REQUIRED:   return "regex_required";
        case CHECK_HEADING_FORMAT:   return "heading_format";
        case CHECK_CITATION_PRESENT: return "citation_present";
        case CHECK_LLM_SEMANTIC:     return "llm_semantic";
        default:                     return "unknown";
    }
}

const char *rule_status_to_string(RuleStatus status) {
    switch (status) {
        case RULE_STATUS_PASS:    return "✅ PASS";
        case RULE_STATUS_FAIL:    return "❌ FAIL";
        case RULE_STATUS_WARNING: return "⚠️  WARNING";
        case RULE_STATUS_PENDING: return "🔄 PENDING";
        case RULE_STATUS_ERROR:   return "💥 ERROR";
        case RULE_STATUS_SKIPPED: return "⏭️  SKIPPED";
        default:             return "?";
    }
}
