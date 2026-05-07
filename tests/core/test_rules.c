/**
 * @file test_rules.c
 * @brief Tests unitaires du moteur de règles — cmocka
 *
 * =============================================================================
 * RÔLE
 * =============================================================================
 * Ce fichier teste :
 *   - La création/destruction des structures (RuleSet, RuleReport)
 *   - L’évaluation des règles structurelles (sections, comptage)
 *   - Le comportement des règles LLM (toujours STATUS_PENDING)
 *   - La mise à jour des résultats LLM
 *
 * =============================================================================
 * DEV-D (Ehud) — Notes d’architecture
 * =============================================================================
 * - Ce fichier sert de documentation vivante pour le moteur de règles.
 * - Les tests sont organisés par catégories :
 *      1. Cycle de vie
 *      2. Évaluation
 *      3. LLM
 *      4. Utilitaires
 *
 * - Certains tests restent TODO tant que les checkers ne sont pas complets.
 *
 * =============================================================================
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "../../include/rules.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ============================================================================
 * HELPERS
 * ============================================================================ */

static RuleSet *create_test_ruleset_one_rule(CheckType type,
                                             const char *param) {
    RuleSet *set = ruleset_create();
    if (!set) return NULL;

    strncpy(set->meta.document_type, "Test", 63);
    strncpy(set->rules[0].id,          "T001",  RULES_MAX_ID_LEN - 1);
    strncpy(set->rules[0].description, "Règle de test", RULES_MAX_DESC_LEN - 1);
    strncpy(set->rules[0].parameter,   param,  RULES_MAX_PARAM_LEN - 1);
    set->rules[0].check_type = type;
    set->rules[0].severity   = SEVERITY_ERROR;
    set->rule_count = 1;

    return set;
}

/* ============================================================================
 * TESTS — CYCLE DE VIE
 * ============================================================================ */

static void test_ruleset_create_not_null(void **state) {
    (void)state;
    RuleSet *set = ruleset_create();
    assert_non_null(set);
    assert_int_equal(set->rule_count, 0);
    ruleset_destroy(set);
}

static void test_ruleset_destroy_null_safe(void **state) {
    (void)state;
    ruleset_destroy(NULL);
}

static void test_rulereport_create_not_null(void **state) {
    (void)state;
    RuleReport *r = rulereport_create();
    assert_non_null(r);
    assert_int_equal(r->result_count, 0);
    assert_int_equal(r->pass_count,   0);
    assert_int_equal(r->fail_count,   0);
    rulereport_destroy(r);
}

/* ============================================================================
 * TESTS — ÉVALUATION
 * ============================================================================ */

static void test_rules_evaluate_null_text(void **state) {
    (void)state;
    RuleSet *set = ruleset_create();
    RuleReport *report = rules_evaluate(set, NULL, 0);
    assert_null(report);
    ruleset_destroy(set);
}

static void test_rules_evaluate_empty_ruleset(void **state) {
    (void)state;
    RuleSet *set = ruleset_create();
    const char *text = "Texte de test";
    RuleReport *report = rules_evaluate(set, text, strlen(text));
    assert_non_null(report);
    assert_int_equal(report->result_count, 0);
    rulereport_destroy(report);
    ruleset_destroy(set);
}

/**
 * @brief Test : CHECK_SECTION_EXISTS → PASS quand la section est présente.
 */
static void test_check_section_exists_found(void **state) {
    (void)state;

    RuleSet *set = create_test_ruleset_one_rule(CHECK_SECTION_EXISTS, "INTRODUCTION");
    const char *text = "INTRODUCTION\n\nContenu...\n";

    RuleReport *report = rules_evaluate(set, text, strlen(text));

    assert_non_null(report);
    assert_int_equal(report->result_count, 1);
    assert_int_equal(report->results[0].status, STATUS_PASS);

    rulereport_destroy(report);
    ruleset_destroy(set);
}

/**
 * @brief Test : CHECK_SECTION_EXISTS → FAIL quand la section est absente.
 */
static void test_check_section_exists_not_found(void **state) {
    (void)state;

    RuleSet *set = create_test_ruleset_one_rule(CHECK_SECTION_EXISTS, "Conclusion");
    const char *text = "INTRODUCTION\n\nContenu...\n";

    RuleReport *report = rules_evaluate(set, text, strlen(text));

    assert_non_null(report);
    assert_int_equal(report->results[0].status, STATUS_FAIL);

    rulereport_destroy(report);
    ruleset_destroy(set);
}

/* ============================================================================
 * TESTS — RÈGLES LLM
 * ============================================================================ */

static void test_check_llm_semantic_always_pending(void **state) {
    (void)state;

    RuleSet *set = create_test_ruleset_one_rule(
        CHECK_LLM_SEMANTIC,
        "La problématique est-elle posée ?"
    );

    const char *text = "Une recherche sur l'impact de l'IA.";
    RuleReport *report = rules_evaluate(set, text, strlen(text));

    assert_non_null(report);
    assert_int_equal(report->result_count, 1);
    assert_int_equal(report->results[0].status, STATUS_PENDING);
    assert_int_equal(report->pending_count, 1);

    rulereport_destroy(report);
    ruleset_destroy(set);
}

/* ============================================================================
 * TESTS — MISE À JOUR DES RÉSULTATS LLM
 * ============================================================================ */

static void test_rules_update_llm_result_valid(void **state) {
    (void)state;

    RuleSet *set = create_test_ruleset_one_rule(CHECK_LLM_SEMANTIC, "Question?");
    const char *text = "Texte de test.";

    RuleReport *report = rules_evaluate(set, text, strlen(text));

    rules_update_llm_result(report, "T001", STATUS_PASS, "OK");

    assert_int_equal(report->results[0].status, STATUS_PASS);
    assert_int_equal(report->pending_count, 0);
    assert_int_equal(report->pass_count, 1);

    rulereport_destroy(report);
    ruleset_destroy(set);
}

static void test_rules_update_llm_result_unknown_id(void **state) {
    (void)state;

    RuleReport *report = rulereport_create();
    rules_update_llm_result(report, "INEXISTANT", STATUS_PASS, NULL);
    rulereport_destroy(report);
}

/* ============================================================================
 * TESTS — UTILITAIRES
 * ============================================================================ */

static void test_check_type_to_string_known(void **state) {
    (void)state;
    assert_string_equal(check_type_to_string(CHECK_SECTION_EXISTS), "section_exists");
    assert_string_equal(check_type_to_string(CHECK_REGEX_FORBIDDEN), "regex_forbidden");
    assert_string_equal(check_type_to_string(CHECK_LLM_SEMANTIC),    "llm_semantic");
}

static void test_rule_status_to_string_all(void **state) {
    (void)state;
    assert_non_null(rule_status_to_string(STATUS_PASS));
    assert_non_null(rule_status_to_string(STATUS_FAIL));
    assert_non_null(rule_status_to_string(STATUS_WARNING));
    assert_non_null(rule_status_to_string(STATUS_PENDING));
    assert_non_null(rule_status_to_string(STATUS_ERROR));
    assert_non_null(rule_status_to_string(STATUS_SKIPPED));
}

/* ============================================================================
 * MAIN
 * ============================================================================ */

int main(void) {
    printf("=== Tests unitaires : module rules ===\n\n");

    const struct CMUnitTest tests[] = {
        /* Cycle de vie */
        cmocka_unit_test(test_ruleset_create_not_null),
        cmocka_unit_test(test_ruleset_destroy_null_safe),
        cmocka_unit_test(test_rulereport_create_not_null),

        /* Évaluation */
        cmocka_unit_test(test_rules_evaluate_null_text),
        cmocka_unit_test(test_rules_evaluate_empty_ruleset),
        cmocka_unit_test(test_check_section_exists_found),
        cmocka_unit_test(test_check_section_exists_not_found),
        cmocka_unit_test(test_check_llm_semantic_always_pending),

        /* Mise à jour LLM */
        cmocka_unit_test(test_rules_update_llm_result_valid),
        cmocka_unit_test(test_rules_update_llm_result_unknown_id),

        /* Utilitaires */
        cmocka_unit_test(test_check_type_to_string_known),
        cmocka_unit_test(test_rule_status_to_string_all),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
