/**
 * @file test_rule_parser.c
 * @brief Tests unitaires — rule_parser (JSON -> RuleSet)
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "../../include/rules.h"
#include <string.h>
#include <stdlib.h>

static void test_parse_valid_json(void **state) {
    (void)state;
    const char *json = "{"
        "\"document_type\": \"Test\","
        "\"rules\": [{"
            "\"id\": \"T001\","
            "\"description\": \"Test rule\","
            "\"check_type\": \"CHECK_WORD_FORBIDDEN\","
            "\"parameter\": \"interdit\""
        "}]"
    "}";
    RuleSet *set = ruleset_parse_json(json);
    assert_non_null(set);
    assert_int_equal(set->count, 1);
    assert_string_equal(set->rules[0].id, "T001");
    ruleset_destroy(set);
}

static void test_parse_null_json(void **state) {
    (void)state;
    RuleSet *set = ruleset_parse_json(NULL);
    assert_null(set);
}

static void test_parse_invalid_json(void **state) {
    (void)state;
    RuleSet *set = ruleset_parse_json("{ invalid json }");
    assert_null(set);
}

static void test_parse_empty_rules(void **state) {
    (void)state;
    const char *json = "{"
        "\"document_type\": \"Test\","
        "\"rules\": []"
    "}";
    RuleSet *set = ruleset_parse_json(json);
    assert_non_null(set);
    assert_int_equal(set->count, 0);
    ruleset_destroy(set);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_parse_valid_json),
        cmocka_unit_test(test_parse_null_json),
        cmocka_unit_test(test_parse_invalid_json),
        cmocka_unit_test(test_parse_empty_rules),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
