/**
 * @file test_nlp.c
 * @brief Tests unitaires NLP — cmocka
 * @author DEV-C
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "nlp.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

SpellChecker *spellchecker_create(const char *aff, const char *dic);
void spellchecker_destroy(SpellChecker *sc);

static void test_spellchecker_create_not_null(void **state) {
    (void)state;
    SpellChecker *sc = spellchecker_create(
        "data/dicts/fr_FR.aff", "data/dicts/fr_FR.dic");
    assert_non_null(sc);
    spellchecker_destroy(sc);
}

static void test_spellcheck_word_correct(void **state) {
    (void)state;
    SpellChecker *sc = spellchecker_create(
        "data/dicts/fr_FR.aff", "data/dicts/fr_FR.dic");
    if (!sc) skip();
    assert_true(spellcheck_word(sc, "bonjour"));
    assert_true(spellcheck_word(sc, "introduction"));
    spellchecker_destroy(sc);
}

static void test_spellcheck_word_incorrect(void **state) {
    (void)state;
    SpellChecker *sc = spellchecker_create(
        "data/dicts/fr_FR.aff", "data/dicts/fr_FR.dic");
    if (!sc) skip();
    assert_false(spellcheck_word(sc, "langague"));
    assert_false(spellcheck_word(sc, "introductoin"));
    spellchecker_destroy(sc);
}

static void test_spellcheck_suggest_not_empty(void **state) {
    (void)state;
    SpellChecker *sc = spellchecker_create(
        "data/dicts/fr_FR.aff", "data/dicts/fr_FR.dic");
    if (!sc) skip();
    NlpSuggestion suggestions[NLP_MAX_SUGGESTIONS];
    size_t count = 0;
    spellcheck_suggest(sc, "langague", suggestions, &count);
    assert_true(count > 0);
    printf("[TEST] %zu suggestions pour 'langague'\n", count);
    spellchecker_destroy(sc);
}

static void test_spellcheck_null_safe(void **state) {
    (void)state;
    bool result = spellcheck_word(NULL, "bonjour");
    assert_true(result);
}

static void test_spellcheck_analyze_finds_error(void **state) {
    (void)state;
    SpellChecker *sc = spellchecker_create(
        "data/dicts/fr_FR.aff", "data/dicts/fr_FR.dic");
    if (!sc) skip();
    NlpResult result;
    result.error_count = 0;
    result.is_complete = true;
    const char *text = "Ce texte contient une fote.";
    spellcheck_analyze(sc, text, strlen(text), &result);
    assert_true(result.error_count > 0);
    printf("[TEST] %zu erreurs trouvees\n", result.error_count);
    spellchecker_destroy(sc);
}

int main(void) {
    printf("=== Tests NLP — DEV-C ===\n\n");
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_spellchecker_create_not_null),
        cmocka_unit_test(test_spellcheck_word_correct),
        cmocka_unit_test(test_spellcheck_word_incorrect),
        cmocka_unit_test(test_spellcheck_suggest_not_empty),
        cmocka_unit_test(test_spellcheck_null_safe),
        cmocka_unit_test(test_spellcheck_analyze_finds_error),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
