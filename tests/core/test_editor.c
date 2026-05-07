/**
 * @file test_editor.c
 * @brief Tests unitaires du module éditeur (gap buffer, undo/redo) — cmocka
 *
 * =============================================================================
 * PHILOSOPHIE DES TESTS
 * =============================================================================
 * Ces tests servent de GUIDE autant que de vérification.
 * Ils documentent le COMPORTEMENT ATTENDU de chaque fonction.
 *
 * Comment lire ce fichier :
 *   - Chaque fonction test_xxx() décrit un comportement précis.
 *   - Les commentaires TODO indiquent ce qui échoue encore.
 *   - Quand une fonction est implémentée, son test doit passer.
 *
 * Lancer les tests :
 *   make test
 *
 * Framework : cmocka (https://cmocka.org/)
 * Installation MSYS2 : pacman -S mingw-w64-x86_64-cmocka
 *
 * =============================================================================
 * RESPONSABLE : DEV-D (tous les tests), avec chaque dev pour son module
 * =============================================================================
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "../../include/editor.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


/* ============================================================================
 * TESTS — CYCLE DE VIE
 * ============================================================================ */

/**
 * @brief Test : editor_create() retourne un pointeur non-NULL.
 *
 * TODO [DEV-A] : Doit passer dès que editor_create() alloue le gap buffer.
 */
static void test_editor_create_not_null(void **state) {
    (void)state;

    EditorDocument *doc = editor_create();

    /* Si ce test échoue : editor_create() n'alloue pas correctement la mémoire */
    assert_non_null(doc);

    editor_destroy(doc);
}

/**
 * @brief Test : un document nouvellement créé est vide.
 */
static void test_editor_create_empty(void **state) {
    (void)state;
    EditorDocument *doc = editor_create();
    if (!doc) skip(); /* Skip si editor_create() n'est pas encore implémenté */

    assert_int_equal(editor_get_length(doc), 0);
    assert_false(doc->dirty);
    assert_null(doc->filepath);

    editor_destroy(doc);
}

/**
 * @brief Test : editor_destroy(NULL) ne crash pas.
 */
static void test_editor_destroy_null_safe(void **state) {
    (void)state;
    /* Cette ligne ne doit pas provoquer de segfault */
    editor_destroy(NULL);
    /* Si on arrive ici, le test passe */
}


/* ============================================================================
 * TESTS — INSERTION DE TEXTE
 * ============================================================================ */

/**
 * @brief Test : insérer du texte augmente la longueur du document.
 *
 * TODO [DEV-A] : Doit passer après TODO-EDITOR-003.
 */
static void test_editor_insert_increases_length(void **state) {
    (void)state;
    EditorDocument *doc = editor_create();
    if (!doc) skip();

    bool ok = editor_insert(doc, "Bonjour", 7);

    assert_true(ok);
    assert_int_equal(editor_get_length(doc), 7);

    editor_destroy(doc);
}

/**
 * @brief Test : le texte inséré est bien récupérable.
 *
 * TODO [DEV-A] : Doit passer après TODO-EDITOR-003 et TODO-EDITOR-006.
 */
static void test_editor_insert_text_retrievable(void **state) {
    (void)state;
    EditorDocument *doc = editor_create();
    if (!doc) skip();

    editor_insert(doc, "Hello", 5);

    char *text = editor_get_text(doc);
    if (!text) {
        editor_destroy(doc);
        skip(); /* editor_get_text pas encore implémenté */
    }

    assert_string_equal(text, "Hello");
    free(text);
    editor_destroy(doc);
}

/**
 * @brief Test : deux insertions successives se concatènent.
 */
static void test_editor_insert_multiple(void **state) {
    (void)state;
    EditorDocument *doc = editor_create();
    if (!doc) skip();

    editor_insert(doc, "Bonjour ", 8);
    editor_insert(doc, "monde",    5);

    char *text = editor_get_text(doc);
    if (!text) { editor_destroy(doc); skip(); }

    assert_string_equal(text, "Bonjour monde");
    free(text);
    editor_destroy(doc);
}

/**
 * @brief Test : l'insertion marque le document comme modifié.
 */
static void test_editor_insert_sets_dirty(void **state) {
    (void)state;
    EditorDocument *doc = editor_create();
    if (!doc) skip();

    assert_false(doc->dirty);
    editor_insert(doc, "x", 1);
    assert_true(doc->dirty);

    editor_destroy(doc);
}


/* ============================================================================
 * TESTS — SUPPRESSION DE TEXTE
 * ============================================================================ */

/**
 * @brief Test : supprimer un caractère réduit la longueur.
 *
 * TODO [DEV-A] : Doit passer après TODO-EDITOR-004.
 */
static void test_editor_delete_reduces_length(void **state) {
    (void)state;
    EditorDocument *doc = editor_create();
    if (!doc) skip();

    editor_insert(doc, "Bonjour", 7);
    bool ok = editor_delete(doc, 0, 3); /* Supprimer "Bon" */

    assert_true(ok);
    assert_int_equal(editor_get_length(doc), 4);

    char *text = editor_get_text(doc);
    if (text) {
        assert_string_equal(text, "jour");
        free(text);
    }

    editor_destroy(doc);
}

/**
 * @brief Test : supprimer au-delà de la fin du document retourne false.
 */
static void test_editor_delete_out_of_bounds(void **state) {
    (void)state;
    EditorDocument *doc = editor_create();
    if (!doc) skip();

    editor_insert(doc, "Hi", 2);
    bool ok = editor_delete(doc, 0, 100); /* Trop grand */

    assert_false(ok);

    editor_destroy(doc);
}


/* ============================================================================
 * TESTS — UNDO / REDO
 * ============================================================================ */

/**
 * @brief Test : undo après insert restaure le texte vide.
 *
 * TODO [DEV-A] : Doit passer après TODO-UNDO-001.
 */
static void test_editor_undo_after_insert(void **state) {
    (void)state;
    EditorDocument *doc = editor_create();
    if (!doc) skip();

    editor_insert(doc, "Test", 4);
    assert_int_equal(editor_get_length(doc), 4);

    bool ok = editor_undo(doc);

    assert_true(ok);
    assert_int_equal(editor_get_length(doc), 0);

    editor_destroy(doc);
}

/**
 * @brief Test : redo après undo restaure le texte.
 */
static void test_editor_redo_after_undo(void **state) {
    (void)state;
    EditorDocument *doc = editor_create();
    if (!doc) skip();

    editor_insert(doc, "Test", 4);
    editor_undo(doc);

    bool ok = editor_redo(doc);

    assert_true(ok);
    assert_int_equal(editor_get_length(doc), 4);

    editor_destroy(doc);
}

/**
 * @brief Test : undo sur document vide retourne false.
 */
static void test_editor_undo_empty_stack(void **state) {
    (void)state;
    EditorDocument *doc = editor_create();
    if (!doc) skip();

    assert_false(editor_can_undo(doc));
    bool ok = editor_undo(doc);
    assert_false(ok);

    editor_destroy(doc);
}

/**
 * @brief Test : redo vide pile après nouvelle insertion.
 *
 * Comportement attendu : après un undo puis une nouvelle insertion,
 * la pile redo doit être vidée.
 */
static void test_editor_redo_cleared_after_new_insert(void **state) {
    (void)state;
    EditorDocument *doc = editor_create();
    if (!doc) skip();

    editor_insert(doc, "A", 1);
    editor_undo(doc);
    assert_true(editor_can_redo(doc));

    editor_insert(doc, "B", 1); /* Nouvelle action → vide redo */
    assert_false(editor_can_redo(doc));

    editor_destroy(doc);
}


/* ============================================================================
 * TESTS — STATISTIQUES
 * ============================================================================ */

/**
 * @brief Test : statistiques d'un texte simple.
 *
 * TODO [DEV-A] : Doit passer après TODO-STATS-001.
 */
static void test_editor_stats_basic(void **state) {
    (void)state;
    EditorDocument *doc = editor_create();
    if (!doc) skip();

    editor_insert(doc, "Bonjour le monde", 16);

    DocStats stats;
    editor_compute_stats(doc, &stats);

    assert_int_equal(stats.word_count, 3);
    assert_int_equal(stats.line_count, 1);

    editor_destroy(doc);
}


/* ============================================================================
 * CONFIGURATION ET LANCEMENT DES TESTS
 * ============================================================================ */

int main(void) {
    printf("=== Tests unitaires : module editor ===\n\n");

    const struct CMUnitTest tests[] = {
        /* Cycle de vie */
        cmocka_unit_test(test_editor_create_not_null),
        cmocka_unit_test(test_editor_create_empty),
        cmocka_unit_test(test_editor_destroy_null_safe),

        /* Insertion */
        cmocka_unit_test(test_editor_insert_increases_length),
        cmocka_unit_test(test_editor_insert_text_retrievable),
        cmocka_unit_test(test_editor_insert_multiple),
        cmocka_unit_test(test_editor_insert_sets_dirty),

        /* Suppression */
        cmocka_unit_test(test_editor_delete_reduces_length),
        cmocka_unit_test(test_editor_delete_out_of_bounds),

        /* Undo/Redo */
        cmocka_unit_test(test_editor_undo_after_insert),
        cmocka_unit_test(test_editor_redo_after_undo),
        cmocka_unit_test(test_editor_undo_empty_stack),
        cmocka_unit_test(test_editor_redo_cleared_after_new_insert),

        /* Statistiques */
        cmocka_unit_test(test_editor_stats_basic),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
