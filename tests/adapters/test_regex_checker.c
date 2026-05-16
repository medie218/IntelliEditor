/**
 * @file test_regex_checker.c
 * @brief Tests unitaires — regex_checker (PCRE2)
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "../../include/rules.h"
#include <string.h>

static void test_regex_forbidden_found(void **state) {
    (void)state;
    Rule rule;
    memset(&rule, 0, sizeof(rule));
    strncpy(rule.id, "R001", RULES_MAX_ID_LEN - 1);
    rule.id[RULES_MAX_ID_LEN - 1] = '\0';
    strncpy(rule.parameter, "interdit", RULES_MAX_PARAM_LEN - 1);
    rule.check_type = CHECK_REGEX_FORBIDDEN;

    const char *text = "Ce mot est interdit ici";
    RuleResult r = check_regex(&rule, text, strlen(text));
    assert_int_equal(r.status, RULE_STATUS_FAIL);
}

static void test_regex_forbidden_not_found(void **state) {
    (void)state;
    Rule rule;
    memset(&rule, 0, sizeof(rule));
    strncpy(rule.id, "R002", RULES_MAX_ID_LEN - 1);
    rule.id[RULES_MAX_ID_LEN - 1] = '\0';
    strncpy(rule.parameter, "interdit", RULES_MAX_PARAM_LEN - 1);
    rule.check_type = CHECK_REGEX_FORBIDDEN;

    const char *text = "Texte propre sans le mot";
    RuleResult r = check_regex(&rule, text, strlen(text));
    assert_int_equal(r.status, RULE_STATUS_PASS);
}

static void test_regex_required_found(void **state) {
    (void)state;
    Rule rule;
    memset(&rule, 0, sizeof(rule));
    strncpy(rule.id, "R003", RULES_MAX_ID_LEN - 1);
    rule.id[RULES_MAX_ID_LEN - 1] = '\0';
    strncpy(rule.parameter, "requis", RULES_MAX_PARAM_LEN - 1);
    rule.check_type = CHECK_REGEX_REQUIRED;

    const char *text = "Ce mot est requis ici";
    RuleResult r = check_regex(&rule, text, strlen(text));
    assert_int_equal(r.status, RULE_STATUS_PASS);
}

static void test_regex_null_params(void **state) {
    (void)state;
    Rule rule;
    memset(&rule, 0, sizeof(rule));
    strncpy(rule.id, "R004", RULES_MAX_ID_LEN - 1);
    rule.id[RULES_MAX_ID_LEN - 1] = '\0';
    rule.check_type = CHECK_REGEX_FORBIDDEN;

    RuleResult r = check_regex(&rule, NULL, 0);
    assert_int_equal(r.status, RULE_STATUS_ERROR);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_regex_forbidden_found),
        cmocka_unit_test(test_regex_forbidden_not_found),
        cmocka_unit_test(test_regex_required_found),
        cmocka_unit_test(test_regex_null_params),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
