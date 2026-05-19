/**
 * @file test_integration.c
 * @brief Test d'intégration DEV-C — NLP + Règles
 * @author DEV-C
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nlp.h"
#include "rules.h"

SpellChecker *spellchecker_create(const char *aff, const char *dic);
void spellchecker_destroy(SpellChecker *sc);
RuleSet *ruleset_load_from_file(const char *filepath);

int main(void) {
    printf("=== Test Integration DEV-C ===\n\n");

    /* Test 1 : NLP */
    printf("[1] Test Hunspell...\n");
    SpellChecker *sc = spellchecker_create(
        "data/dicts/fr_FR.aff",
        "data/dicts/fr_FR.dic"
    );
    if (!sc) {
        fprintf(stderr, "ERREUR: Hunspell non charge\n");
        return 1;
    }

    NlpResult result;
    result.error_count = 0;
    result.is_complete = true;
    const char *text = "Ce texte contient une fote et un langague incorrect.";
    spellcheck_analyze(sc, text, strlen(text), &result);
    printf("    Erreurs NLP: %zu\n", result.error_count);
    for (size_t i = 0; i < result.error_count; i++)
        printf("    - '%s' a position %zu\n",
               result.errors[i].original,
               result.errors[i].start);
    spellchecker_destroy(sc);

    /* Test 2 : Règles */
    printf("\n[2] Test moteur de regles...\n");
    RuleSet *set = ruleset_load_from_file(
        "data/rule_templates/memoire_licence.json");
    if (!set) {
        fprintf(stderr, "ERREUR: fichier JSON non charge\n");
        return 1;
    }
    printf("    Regles chargees: %zu\n", set->rule_count);

    const char *doc_text =
        "INTRODUCTION\n\nCeci est une introduction.\n\n"
        "CONCLUSION\n\nVoici la conclusion.\n\n"
        "BIBLIOGRAPHIE\n\nReferences ici.";

    RuleReport *report = rules_evaluate(set, doc_text, strlen(doc_text));
    if (report) {
        printf("    Resultats: %zu regles\n", report->result_count);
        printf("    Pass: %zu | Fail: %zu | Pending: %zu\n",
               report->pass_count,
               report->fail_count,
               report->pending_count);
        for (size_t i = 0; i < report->result_count; i++) {
            printf("    [%s] %s - %s\n",
                   rule_status_to_string(report->results[i].status),
                   report->results[i].rule_id,
                   report->results[i].message);
        }
        rulereport_destroy(report);
    }
    ruleset_destroy(set);

    printf("\n=== Integration OK ! ===\n");
    return 0;
}
