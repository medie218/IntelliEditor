#ifndef INTELLIEDITOR_SECTION_CHECKER_H
#define INTELLIEDITOR_SECTION_CHECKER_H

#include "rules.h"

RuleResult check_section_exists(const Rule *rule,
                                const char *text,
                                size_t len);

RuleResult check_section_order(const Rule *rule,
                               const char *text,
                               size_t len);

RuleResult check_heading_format(const Rule *rule,
                                const char *text,
                                size_t len);

#endif /* INTELLIEDITOR_SECTION_CHECKER_H */
