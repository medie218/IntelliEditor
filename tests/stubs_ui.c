#include "ui.h"
#include <stdio.h>

void ui_update_statusbar(AppContext *ctx, size_t words, int line, int col) {
    (void)ctx; (void)words; (void)line; (void)col;
}

void ui_update_rules_panel(AppContext *ctx, const RuleReport *report) {
    (void)ctx; (void)report;
}
