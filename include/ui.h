/**
 * @file ui.h
 * @brief Contrat public du module UI Win32 — ADAPTER
 */

#ifndef INTELLIEDITOR_UI_H
#define INTELLIEDITOR_UI_H

#include <windows.h>
#include <stdbool.h>
#include <stddef.h>

#include "editor.h"
#include "rules.h"
#include "nlp.h"
#include "llm.h"

/* ============================================================================
 * CONSTANTES
 * ============================================================================ */

#define UI_WINDOW_TITLE    "IntelliEditor"
#define UI_WINDOW_CLASS    "IntelliEditorWnd"
#define UI_MIN_WIDTH        800
#define UI_MIN_HEIGHT       600
#define UI_RULES_PANEL_W    280

/* Identifiants de menus et boutons */
#define ID_FILE_NEW         1001
#define ID_FILE_OPEN        1002
#define ID_FILE_SAVE        1003
#define ID_FILE_SAVE_AS     1004
#define ID_FILE_EXIT        1099
#define ID_EDIT_UNDO        1101
#define ID_EDIT_REDO        1102
#define ID_EDIT_CUT         1103
#define ID_EDIT_COPY        1104
#define ID_EDIT_PASTE       1105
#define ID_TOOLS_RULES_LOAD 1201
#define ID_TOOLS_GRAMMAR    1202
#define ID_RULES_PANEL      1301

/* Messages personnalisés */
#define WM_LLM_RESPONSE     (WM_USER + 100)
#define WM_NLP_RESULT       (WM_USER + 101)
#define WM_RULES_RESULT     (WM_USER + 102)

/* ============================================================================
 * STRUCTURES
 * ============================================================================ */

typedef struct {
    HWND           hwnd_main;
    HWND           hwnd_scintilla;
    HWND           hwnd_rules_panel;
    HWND           hwnd_statusbar;
    HWND           hwnd_toolbar;
    HINSTANCE      hinstance;
    EditorDocument *doc;
    RuleReport     *report;
    RuleSet        *active_rules;
    LlmEngine      *llm_engine;
    bool            llm_ready;
} AppContext;

/* ============================================================================
 * API PUBLIQUE
 * ============================================================================ */

bool ui_init(AppContext *ctx, HINSTANCE hinstance, int ncmdshow);
int ui_run(AppContext *ctx);
void ui_cleanup(AppContext *ctx);

void ui_sync_text(AppContext *ctx);
void ui_apply_nlp_markers(AppContext *ctx, const NlpResult *result);
void ui_update_rules_panel(AppContext *ctx, const RuleReport *report);
void ui_update_statusbar(AppContext *ctx, size_t words, int line, int col);

#endif /* INTELLIEDITOR_UI_H */
