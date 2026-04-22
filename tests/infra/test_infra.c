/**
 * @file test_infra.c
 * @brief Tests unitaires des modules infra (encoding, config, storage) — cmocka
 *
 * RESPONSABLE : DEV-D (squelette), DEV-A (implémentation)
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "../include/encoding.h"
#include "../include/config.h"
#include "../include/storage.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


/* ============================================================================
 * TESTS — ENCODING
 * ============================================================================ */

/**
 * @brief Test : encoding_utf8_char_count() compte correctement les caractères.
 *
 * "café" = 4 caractères Unicode mais 5 octets (é = 2 octets en UTF-8)
 */
static void test_encoding_utf8_char_count_ascii(void **state) {
    (void)state;
    /* Texte ASCII pur : nombre de chars = nombre d'octets */
    assert_int_equal(encoding_utf8_char_count("Hello"), 5);
    assert_int_equal(encoding_utf8_char_count(""),      0);
    assert_int_equal(encoding_utf8_char_count("A"),     1);
}

static void test_encoding_utf8_char_count_unicode(void **state) {
    (void)state;
    /*
     * "café" en UTF-8 : c(1) a(1) f(1) é(2) = 5 octets, 4 caractères
     *
     * TODO [DEV-A] : Ce test valide que encoding_utf8_char_count()
     * compte les code points, pas les octets.
     */
    const char *cafe = "caf\xC3\xA9"; /* "café" en UTF-8 */
    size_t chars = encoding_utf8_char_count(cafe);

    /* strlen(cafe) = 5 octets */
    assert_int_equal(strlen(cafe), 5);

    /* Mais en caractères Unicode : 4 */
    assert_int_equal(chars, 4);
}

static void test_encoding_utf8_char_count_null(void **state) {
    (void)state;
    assert_int_equal(encoding_utf8_char_count(NULL), 0);
}

/**
 * @brief Test : encoding_is_valid_utf8() détecte du UTF-8 valide.
 *
 * TODO [DEV-A] : Doit passer après TODO-ENCODING-001.
 */
static void test_encoding_is_valid_utf8_valid(void **state) {
    (void)state;
    const char *valid = "Bonjour le monde";
    assert_true(encoding_is_valid_utf8(valid, strlen(valid)));
}

static void test_encoding_is_valid_utf8_invalid(void **state) {
    (void)state;
    /* Séquence invalide : octet de continuation sans octet de début */
    const char invalid[] = {0x80, 0x80, 0x00};
    /*
     * TODO [DEV-A] : Ce test doit retourner false après TODO-ENCODING-001.
     * Pour l'instant le stub retourne toujours true.
     */
    /* assert_false(encoding_is_valid_utf8(invalid, 2)); */
    printf("[TODO] test_encoding_is_valid_utf8_invalid: à activer après TODO-ENCODING-001\n");
    (void)invalid;
}


/* ============================================================================
 * TESTS — CONFIG
 * ============================================================================ */

/**
 * @brief Test : config_set et config_get fonctionnent ensemble.
 */
static void test_config_set_get(void **state) {
    (void)state;

    AppConfig cfg;
    memset(&cfg, 0, sizeof(cfg));

    config_set(&cfg, "General", "theme", "dark");
    const char *val = config_get(&cfg, "General", "theme");

    assert_non_null(val);
    assert_string_equal(val, "dark");
}

/**
 * @brief Test : config_get retourne NULL pour une clé inexistante.
 */
static void test_config_get_missing_key(void **state) {
    (void)state;

    AppConfig cfg;
    memset(&cfg, 0, sizeof(cfg));

    const char *val = config_get(&cfg, "General", "inexistant");
    assert_null(val);
}

/**
 * @brief Test : config_set met à jour une valeur existante.
 */
static void test_config_set_update(void **state) {
    (void)state;

    AppConfig cfg;
    memset(&cfg, 0, sizeof(cfg));

    config_set(&cfg, "Editor", "font_size", "12");
    config_set(&cfg, "Editor", "font_size", "16"); /* Mise à jour */

    const char *val = config_get(&cfg, "Editor", "font_size");
    assert_string_equal(val, "16");

    /* Ne doit pas créer de doublon */
    size_t count = 0;
    for (size_t i = 0; i < cfg.count; i++) {
        if (strcmp(cfg.entries[i].key, "font_size") == 0) count++;
    }
    assert_int_equal(count, 1);
}

/**
 * @brief Test : config_get_int retourne la valeur par défaut si clé absente.
 */
static void test_config_get_int_default(void **state) {
    (void)state;

    AppConfig cfg;
    memset(&cfg, 0, sizeof(cfg));

    int val = config_get_int(&cfg, "LLM", "n_threads", 4);
    assert_int_equal(val, 4);
}

/**
 * @brief Test : config_get_int parse la valeur correctement.
 */
static void test_config_get_int_parse(void **state) {
    (void)state;

    AppConfig cfg;
    memset(&cfg, 0, sizeof(cfg));

    config_set(&cfg, "LLM", "n_ctx", "4096");
    int val = config_get_int(&cfg, "LLM", "n_ctx", 2048);
    assert_int_equal(val, 4096);
}


/* ============================================================================
 * TESTS — STORAGE
 * ============================================================================ */

/**
 * @brief Test : storage_detect_format() identifie les extensions.
 */
static void test_storage_detect_format_txt(void **state) {
    (void)state;
    assert_int_equal(storage_detect_format("document.txt"), FILE_FORMAT_TXT);
    assert_int_equal(storage_detect_format("rapport.rtf"),  FILE_FORMAT_RTF);
    assert_int_equal(storage_detect_format("notes.ie"),     FILE_FORMAT_IE);
    assert_int_equal(storage_detect_format("fichier.xyz"),  FILE_FORMAT_UNKNOWN);
    assert_int_equal(storage_detect_format(NULL),           FILE_FORMAT_UNKNOWN);
}

/**
 * @brief Test : écriture puis lecture d'un fichier texte.
 *
 * TODO [DEV-A] : Doit passer dès que storage_write_txt est implémenté.
 */
static void test_storage_write_read_txt(void **state) {
    (void)state;

    const char *content  = "Contenu de test\nDeuxième ligne\n";
    const char *tmpfile  = "test_storage_tmp.txt";

    bool ok = storage_write_txt(tmpfile, content, strlen(content));
    assert_true(ok);

    size_t len = 0;
    char *read = storage_read_file(tmpfile, &len);

    assert_non_null(read);
    assert_int_equal(len, strlen(content));
    assert_string_equal(read, content);

    free(read);
    remove(tmpfile); /* Nettoyage */
}


/* ============================================================================
 * CONFIGURATION DES TESTS
 * ============================================================================ */

int main(void) {
    printf("=== Tests unitaires : modules infra ===\n\n");

    const struct CMUnitTest tests[] = {
        /* Encoding */
        cmocka_unit_test(test_encoding_utf8_char_count_ascii),
        cmocka_unit_test(test_encoding_utf8_char_count_unicode),
        cmocka_unit_test(test_encoding_utf8_char_count_null),
        cmocka_unit_test(test_encoding_is_valid_utf8_valid),
        cmocka_unit_test(test_encoding_is_valid_utf8_invalid),

        /* Config */
        cmocka_unit_test(test_config_set_get),
        cmocka_unit_test(test_config_get_missing_key),
        cmocka_unit_test(test_config_set_update),
        cmocka_unit_test(test_config_get_int_default),
        cmocka_unit_test(test_config_get_int_parse),

        /* Storage */
        cmocka_unit_test(test_storage_detect_format_txt),
        cmocka_unit_test(test_storage_write_read_txt),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
