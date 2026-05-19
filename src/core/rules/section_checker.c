#include "rules.h"
#include "section_checker.h"

RuleResult check_section_exists(const Rule *rule,
                                const char *text,
                                size_t len) {
    RuleResult result;

    (void)rule;
    (void)text;
    (void)len;

    result.status = RULE_STATUS_PASS;

    return result;
}

RuleResult check_section_order(const Rule *rule,
                               const char *text,
                               size_t len) {
    RuleResult result;

    (void)rule;
    (void)text;
    (void)len;

    result.status = RULE_STATUS_PASS;

    return result;
}

RuleResult check_heading_format(const Rule *rule,
                                const char *text,
                                size_t len) {
    RuleResult result;

    (void)rule;
    (void)text;
    (void)len;

    result.status = RULE_STATUS_PASS;

    return result;
}
