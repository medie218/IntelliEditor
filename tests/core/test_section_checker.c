/**
 * @file test_section_checker.c
 * @brief Tests unitaires — section_checker
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "../../include/rules.h"
#include <string.h>

static void test_find_section_found(void **state) {
    (void)state;
    const char *text = "Introduction\nDeveloppement\nConclusion\n";
    size_t pos = find_section(text, "Introduction");
    assert_int_not_equal(pos, SIZE_MAX);
}

static void test_find_section_not_found(void **state) {
    (void)state;
    const char *text = "Introduction\nDeveloppement\nConclusion\n";
    size_t pos = find_section(text, "Bibliographie");
    assert_int_equal(pos, SIZE_MAX);
}

static void test_find_section_case_insensitive(void **state) {
    (void)state;
    const char *text = "INTRODUCTION\nDeveloppement\n";
    size_t pos = find_section(text, "introduction");
    assert_int_not_equal(pos, SIZE_MAX);
}

static void test_find_section_null(void **state) {
    (void)state;
    size_t pos = find_section(NULL, "Introduction");
    assert_int_equal(pos, SIZE_MAX);
}

static void test_find_section_long_line(void **state) {
    (void)state;
    /* Ligne de plus de 255 caractères */
    char long_text[512];
    memset(long_text, 'x', 300);
    long_text[300] = '\0';
    strcat(long_text, "\nIntroduction\n");
    size_t pos = find_section(long_text, "Introduction");
    assert_int_not_equal(pos, SIZE_MAX);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_find_section_found),
        cmocka_unit_test(test_find_section_not_found),
        cmocka_unit_test(test_find_section_case_insensitive),
        cmocka_unit_test(test_find_section_null),
        cmocka_unit_test(test_find_section_long_line),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
