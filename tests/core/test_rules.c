/**
 * @file test_rules.c
 * @brief Tests unitaires du moteur de règles — cmocka
 *
 * Ces tests vérifient :
 *   - La création et destruction des structures (RuleSet, RuleReport)
 *   - L'évaluation des règles sur des textes de démonstration
 *   - Les cas limites (texte vide, règle inconnue, etc.)
 *
 * RESPONSABLE : DEV-D
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "../include/rules.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


/* ============================================================================
 * HELPERS DE TEST
 * ============================================================================ */

/**
 * @brief Crée un RuleSet minimal avec une seule règle, pour les tests.
 */
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
    ruleset_destroy(NULL); /* Ne doit pas crasher */
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
 * TESTS — ÉVALUATION DE RÈGLES
 * ============================================================================ */

/**
 * @brief Test : rules_evaluate() sur texte NULL retourne NULL sans crash.
 */
static void test_rules_evaluate_null_text(void **state) {
    (void)state;
    RuleSet *set = ruleset_create();
    if (!set) skip();

    RuleReport *report = rules_evaluate(set, NULL, 0);
    assert_null(report); /* Doit retourner NULL sur entrée invalide */

    ruleset_destroy(set);
}

/**
 * @brief Test : rules_evaluate() sur RuleSet vide produit un rapport vide.
 */
static void test_rules_evaluate_empty_ruleset(void **state) {
    (void)state;
    RuleSet *set = ruleset_create();
    if (!set) skip();

    const char *text = "Texte de test";
    RuleReport *report = rules_evaluate(set, text, strlen(text));

    assert_non_null(report);
    assert_int_equal(report->result_count, 0);

    rulereport_destroy(report);
    ruleset_destroy(set);
}

/**
 * @brief Test : CHECK_SECTION_EXISTS → PASS quand la section est présente.
 *
 * TODO [DEV-D] : Doit passer après TODO-SECTION-003.
 */
static void test_check_section_exists_found(void **state) {
    (void)state;

    RuleSet *set = create_test_ruleset_one_rule(CHECK_SECTION_EXISTS, "Introduction");
    if (!set) skip();

    /* Texte qui CONTIENT la section "Introduction" */
    const char *text = "INTRODUCTION\n\nCe document présente notre recherche.\n";
    RuleReport *report = rules_evaluate(set, text, strlen(text));

    assert_non_null(report);
    assert_int_equal(report->result_count, 1);

    /*
     * TODO [DEV-D] : Ce test échoue jusqu'à implémentation du checker.
     * Il doit retourner STATUS_PASS quand "Introduction" est dans le texte.
     */
    /* assert_int_equal(report->results[0].status, STATUS_PASS); */

    printf("[TODO] test_check_section_exists_found: à activer après TODO-SECTION-003\n");

    rulereport_destroy(report);
    ruleset_destroy(set);
}

/**
 * @brief Test : CHECK_SECTION_EXISTS → FAIL quand la section est absente.
 *
 * TODO [DEV-D] : Doit passer après TODO-SECTION-003.
 */
static void test_check_section_exists_not_found(void **state) {
    (void)state;

    RuleSet *set = create_test_ruleset_one_rule(CHECK_SECTION_EXISTS, "Conclusion");
    if (!set) skip();

    /* Texte qui NE CONTIENT PAS "Conclusion" */
    const char *text = "INTRODUCTION\n\nVoici le début du document.";
    RuleReport *report = rules_evaluate(set, text, strlen(text));

    assert_non_null(report);
    /* assert_int_equal(report->results[0].status, STATUS_FAIL); */

    printf("[TODO] test_check_section_exists_not_found: à activer après TODO-SECTION-003\n");

    rulereport_destroy(report);
    ruleset_destroy(set);
}

/**
 * @brief Test : CHECK_LLM_SEMANTIC → STATUS_PENDING (toujours).
 *
 * Les règles LLM ne sont pas évaluées synchronement.
 * Elles doivent toujours retourner STATUS_PENDING.
 */
static void test_check_llm_semantic_always_pending(void **state) {
    (void)state;

    RuleSet *set = create_test_ruleset_one_rule(
        CHECK_LLM_SEMANTIC,
        "La problématique est-elle posée ?"
    );
    if (!set) skip();

    const char *text = "Une recherche sur l'impact de l'IA sur la société.";
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

/**
 * @brief Test : rules_update_llm_result() met à jour un STATUS_PENDING.
 */
static void test_rules_update_llm_result_valid(void **state) {
    (void)state;

    RuleSet *set = create_test_ruleset_one_rule(CHECK_LLM_SEMANTIC, "Question?");
    if (!set) skip();

    const char *text = "Texte de test.";
    RuleReport *report = rules_evaluate(set, text, strlen(text));
    if (!report) { ruleset_destroy(set); skip(); }

    /* Simuler la réponse du thread LLM */
    rules_update_llm_result(report, "T001", STATUS_PASS, "Problématique bien posée.");

    assert_int_equal(report->results[0].status, STATUS_PASS);
    assert_int_equal(report->pending_count, 0);
    assert_int_equal(report->pass_count, 1);

    rulereport_destroy(report);
    ruleset_destroy(set);
}

/**
 * @brief Test : rules_update_llm_result() avec ID inexistant ne crash pas.
 */
static void test_rules_update_llm_result_unknown_id(void **state) {
    (void)state;

    RuleReport *report = rulereport_create();
    if (!report) skip();

    /* Ne doit pas crasher */
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
    /* Vérifier que toutes les valeurs d'enum ont une chaîne associée */
    assert_non_null(rule_status_to_string(STATUS_PASS));
    assert_non_null(rule_status_to_string(STATUS_FAIL));
    assert_non_null(rule_status_to_string(STATUS_WARNING));
    assert_non_null(rule_status_to_string(STATUS_PENDING));
    assert_non_null(rule_status_to_string(STATUS_ERROR));
    assert_non_null(rule_status_to_string(STATUS_SKIPPED));
}


/* ============================================================================
 * CONFIGURATION DES TESTS
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
