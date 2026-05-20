/**
 * @file main_window.c
 * @brief Point d'entrée et gestionnaire de fenêtre Win32 — ADAPTER / ui_win32
 */

#include "ui.h"
#include "editor.h"
#include "config.h"
#include "storage.h"
#include "llm.h"

#include <commctrl.h>
#include <commdlg.h>
#include <stdbool.h>
#include <windows.h>
#include <stdio.h>
#include "Scintilla.h"
#include "SciLexer.h"

/* ============================================================================
 * PROTOTYPES ET GLOBALES
 * ============================================================================ */

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
static bool create_menu(HWND hwnd);
static bool create_statusbar(AppContext *ctx);
static bool create_toolbar(AppContext *ctx);
static void ui_llm_callback(const LlmResponse *response, void *userdata);

extern bool scintilla_load(void);
extern HWND scintilla_create(HWND parent, HINSTANCE hinstance, int x, int y, int w, int h);

static bool g_syncing = false;

/* ============================================================================
 * CALLBACKS
 * ============================================================================ */

static void ui_llm_callback(const LlmResponse *response, void *userdata) {
    AppContext *ctx = (AppContext *)userdata;
    if (!ctx || !ctx->hwnd_main) return;

    LlmResponse *copy = malloc(sizeof(LlmResponse));
    if (copy) {
        memcpy(copy, response, sizeof(LlmResponse));
        PostMessage(ctx->hwnd_main, WM_LLM_RESPONSE, 0, (LPARAM)copy);
    }
}

/* ============================================================================
 * WINMAIN
 * ============================================================================ */

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    (void)hPrev; (void)lpCmd;

    if (!scintilla_load()) {
        MessageBoxA(NULL, "Impossible de charger SciLexer.dll", "Erreur", MB_ICONERROR);
        return 1;
    }

    INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_WIN95_CLASSES | ICC_BAR_CLASSES};
    InitCommonControlsEx(&icc);

    AppContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.hinstance = hInst;

    ctx.doc = editor_create();
    if (!ctx.doc) return 1;

    /* Initialisation IA */
    char model_path[MAX_PATH];
    GetPrivateProfileStringA("AI", "model_path", "data/models/tinyllama.gguf", model_path, MAX_PATH, "config.ini");
    ctx.llm_engine = llm_create(model_path, 4, 2048);
    if (ctx.llm_engine) {
        if (llm_start_worker(ctx.llm_engine)) {
            ctx.llm_ready = true;
        }
    }

    if (!ui_init(&ctx, hInst, nShow)) {
        if (ctx.llm_engine) llm_destroy(ctx.llm_engine);
        editor_destroy(ctx.doc);
        return 1;
    }

    int code = ui_run(&ctx);

    ui_cleanup(&ctx);
    if (ctx.active_rules) ruleset_destroy(ctx.active_rules);
    if (ctx.report) rulereport_destroy(ctx.report);
    if (ctx.llm_engine) llm_destroy(ctx.llm_engine);
    editor_destroy(ctx.doc);

    return code;
}

/* ============================================================================
 * CYCLE DE VIE UI
 * ============================================================================ */

bool ui_init(AppContext *ctx, HINSTANCE hinstance, int ncmdshow) {
    WNDCLASSEXA wc = {0};
    wc.cbSize        = sizeof(WNDCLASSEXA);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hinstance;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = UI_WINDOW_CLASS;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassExA(&wc)) return false;

    ctx->hwnd_main = CreateWindowExA(
        0, UI_WINDOW_CLASS, UI_WINDOW_TITLE,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1200, 700,
        NULL, NULL, hinstance, ctx
    );

    if (!ctx->hwnd_main) return false;

    ShowWindow(ctx->hwnd_main, ncmdshow);
    UpdateWindow(ctx->hwnd_main);
    return true;
}

int ui_run(AppContext *ctx) {
    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return (int)msg.wParam;
}

void ui_cleanup(AppContext *ctx) {
    // Les fenêtres enfants sont détruites par Windows
}

/* ============================================================================
 * WNDPROC
 * ============================================================================ */

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    AppContext *ctx = (AppContext *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

    switch (msg) {
    case WM_CREATE: {
        CREATESTRUCTA *cs = (CREATESTRUCTA *)lp;
        ctx = (AppContext *)cs->lpCreateParams;
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)ctx);
        ctx->hwnd_main = hwnd;

        create_menu(hwnd);
        create_toolbar(ctx);
        create_statusbar(ctx);

        ctx->hwnd_scintilla = scintilla_create(hwnd, ctx->hinstance, 0, 0, 0, 0);
        ctx->hwnd_rules_panel = CreateWindowExA(
            WS_EX_CLIENTEDGE, "LISTBOX", "",
            WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL | WS_HSCROLL,
            0, 0, 0, 0,
            hwnd, (HMENU)ID_RULES_PANEL, ctx->hinstance, NULL
        );

        /* Chargement automatique des règles par défaut pour le MVP */
        const char *default_rules = "data/rule_templates/regles_universelles_fr.json";
        ctx->active_rules = ruleset_load_from_file(default_rules);
        if (ctx->active_rules) {
            printf("[UI] R├¿gles par d├®faut charg├®es : %s\n", default_rules);
            /* Premier check à vide ou avec le texte actuel */
            char *text = editor_get_text(ctx->doc);
            if (text) {
                ctx->report = rules_evaluate(ctx->active_rules, text, strlen(text));
                ui_update_rules_panel(ctx, ctx->report);
                free(text);
            }
        } else {
            /* Essayer dans le dossier local si data/ n'est pas trouvé */
            ctx->active_rules = ruleset_load_from_file("regles_universelles_fr.json");
            if (ctx->active_rules) {
                ui_update_rules_panel(ctx, ctx->report);
            }
        }

        return 0;
    }

    case WM_SIZE: {
        if (!ctx) break;
        RECT rc; GetClientRect(hwnd, &rc);
        int toolbar_h = 30;
        if (ctx->hwnd_toolbar) {
            SendMessage(ctx->hwnd_toolbar, TB_AUTOSIZE, 0, 0);
            RECT rb; SendMessage(ctx->hwnd_toolbar, TB_GETITEMRECT, 0, (LPARAM)&rb);
            toolbar_h = rb.bottom;
        }
        int status_h = 20;
        if (ctx->hwnd_statusbar) {
            SendMessage(ctx->hwnd_statusbar, WM_SIZE, 0, 0);
            RECT rs; GetWindowRect(ctx->hwnd_statusbar, &rs);
            status_h = rs.bottom - rs.top;
        }
        int panel_w = UI_RULES_PANEL_W;
        int edit_w = rc.right - panel_w;
        int edit_h = rc.bottom - toolbar_h - status_h;
        MoveWindow(ctx->hwnd_scintilla, 0, toolbar_h, edit_w, edit_h, TRUE);
        MoveWindow(ctx->hwnd_rules_panel, edit_w, toolbar_h, panel_w, edit_h, TRUE);
        return 0;
    }

    case WM_COMMAND: {
        int id = LOWORD(wp);
        switch (id) {
        case ID_FILE_NEW:
            editor_destroy(ctx->doc);
            ctx->doc = editor_create();
            ui_sync_text(ctx);
            break;
        case ID_FILE_OPEN: {
            char path[MAX_PATH] = {0};
            OPENFILENAMEA ofn = {0}; ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = hwnd;
            ofn.lpstrFile = path; ofn.nMaxFile = MAX_PATH;
            ofn.lpstrFilter = "Texte (*.txt)\0*.txt\0Tous (*.*)\0*.*\0";
            if (GetOpenFileNameA(&ofn)) {
                size_t len = 0; char *content = storage_read_file(path, &len);
                if (content) {
                    editor_destroy(ctx->doc); ctx->doc = editor_create();
                    editor_insert(ctx->doc, content, len);
                    ctx->doc->filepath = _strdup(path); ctx->doc->dirty = false;
                    free(content); ui_sync_text(ctx);
                }
            }
            break;
        }
        case ID_FILE_SAVE:
            if (ctx->doc->filepath) {
                char *text = editor_get_text(ctx->doc);
                if (text) {
                    storage_write_txt(ctx->doc->filepath, text, strlen(text));
                    ctx->doc->dirty = false; free(text);
                }
            } else SendMessage(hwnd, WM_COMMAND, ID_FILE_SAVE_AS, 0);
            break;
        case ID_FILE_SAVE_AS: {
            char path[MAX_PATH] = {0};
            OPENFILENAMEA ofn = {0}; ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = hwnd;
            ofn.lpstrFile = path; ofn.nMaxFile = MAX_PATH;
            ofn.lpstrFilter = "Texte (*.txt)\0*.txt\0";
            if (GetSaveFileNameA(&ofn)) {
                char *text = editor_get_text(ctx->doc);
                if (text) {
                    storage_write_txt(path, text, strlen(text));
                    if (ctx->doc->filepath) free(ctx->doc->filepath);
                    ctx->doc->filepath = _strdup(path);
                    ctx->doc->dirty = false; free(text);
                }
            }
            break;
        }
        case ID_TOOLS_RULES_LOAD: {
            char path[MAX_PATH] = {0};
            OPENFILENAMEA ofn = {0}; ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = hwnd;
            ofn.lpstrFile = path; ofn.nMaxFile = MAX_PATH;
            ofn.lpstrFilter = "Règles JSON (*.json)\0*.json\0";
            if (GetOpenFileNameA(&ofn)) {
                if (ctx->active_rules) ruleset_destroy(ctx->active_rules);
                ctx->active_rules = ruleset_load_from_file(path);
                if (ctx->active_rules) {
                    char *text = editor_get_text(ctx->doc);
                    if (text) {
                        if (ctx->report) rulereport_destroy(ctx->report);
                        ctx->report = rules_evaluate(ctx->active_rules, text, strlen(text));
                        ui_update_rules_panel(ctx, ctx->report);
                        free(text);
                    }
                }
            }
            break;
        }
        case ID_TOOLS_GRAMMAR: {
            if (!ctx->llm_ready) { MessageBoxA(hwnd, "IA non disponible", "IA", MB_ICONWARNING); break; }
            char *text = editor_get_text(ctx->doc);
            if (text) {
                llm_submit_request(ctx->llm_engine, LLM_TASK_GRAMMAR_CHECK, text, ui_llm_callback, ctx);
                SendMessageA(ctx->hwnd_statusbar, SB_SETTEXTA, 3, (LPARAM)"IA en cours...");
                free(text);
            }
            break;
        }
        case ID_FILE_EXIT: DestroyWindow(hwnd); break;
        case ID_EDIT_UNDO: editor_undo(ctx->doc); ui_sync_text(ctx); break;
        case ID_EDIT_REDO: editor_redo(ctx->doc); ui_sync_text(ctx); break;
        }
        return 0;
    }

    case WM_NOTIFY: {
        NMHDR *nm = (NMHDR *)lp;
        if (nm->hwndFrom == ctx->hwnd_scintilla && nm->code == SCN_MODIFIED) {
            SCNotification *scn = (SCNotification *)lp;
            if (!g_syncing) {
                if (scn->modificationType & SC_MOD_INSERTTEXT) editor_insert(ctx->doc, scn->text, scn->length);
                else if (scn->modificationType & SC_MOD_DELETETEXT) editor_delete(ctx->doc, scn->position, scn->length);
                
                if (ctx->active_rules) {
                    char *text = editor_get_text(ctx->doc);
                    if (text) {
                        if (ctx->report) rulereport_destroy(ctx->report);
                        ctx->report = rules_evaluate(ctx->active_rules, text, strlen(text));
                        ui_update_rules_panel(ctx, ctx->report);
                        free(text);
                    }
                }
            }
        }
        return 0;
    }

    case WM_LLM_RESPONSE: {
        LlmResponse *resp = (LlmResponse *)lp;
        if (resp) {
            MessageBoxA(hwnd, resp->text, "Correction IA", MB_OK | MB_ICONINFORMATION);
            SendMessageA(ctx->hwnd_statusbar, SB_SETTEXTA, 3, (LPARAM)"Prêt");
            free(resp);
        }
        return 0;
    }

    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

/* ============================================================================
 * HELPERS UI
 * ============================================================================ */

static bool create_menu(HWND hwnd) {
    HMENU bar = CreateMenu();
    HMENU mFile = CreatePopupMenu();
    HMENU mEdit = CreatePopupMenu();
    HMENU mTools = CreatePopupMenu();
    AppendMenuA(mFile, MF_STRING, ID_FILE_NEW, "Nouveau");
    AppendMenuA(mFile, MF_STRING, ID_FILE_OPEN, "Ouvrir...");
    AppendMenuA(mFile, MF_STRING, ID_FILE_SAVE, "Enregistrer");
    AppendMenuA(mFile, MF_STRING, ID_FILE_SAVE_AS, "Enregistrer sous...");
    AppendMenuA(mFile, MF_SEPARATOR, 0, NULL);
    AppendMenuA(mFile, MF_STRING, ID_FILE_EXIT, "Quitter");
    AppendMenuA(mEdit, MF_STRING, ID_EDIT_UNDO, "Annuler");
    AppendMenuA(mEdit, MF_STRING, ID_EDIT_REDO, "Rétablir");
    AppendMenuA(mTools, MF_STRING, ID_TOOLS_RULES_LOAD, "Charger règles...");
    AppendMenuA(mTools, MF_STRING, ID_TOOLS_GRAMMAR, "IA : Grammaire");
    AppendMenuA(bar, MF_POPUP, (UINT_PTR)mFile, "Fichier");
    AppendMenuA(bar, MF_POPUP, (UINT_PTR)mEdit, "Edition");
    AppendMenuA(bar, MF_POPUP, (UINT_PTR)mTools, "Outils");
    SetMenu(hwnd, bar); return true;
}

static bool create_toolbar(AppContext *ctx) {
    ctx->hwnd_toolbar = CreateWindowExA(0, TOOLBARCLASSNAMEA, NULL, WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT, 0, 0, 0, 0, ctx->hwnd_main, NULL, ctx->hinstance, NULL);
    SendMessage(ctx->hwnd_toolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    TBBUTTON tbb[] = {
        {STD_FILENEW, ID_FILE_NEW, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
        {STD_FILEOPEN, ID_FILE_OPEN, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
        {STD_FILESAVE, ID_FILE_SAVE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
        {0, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},
        {STD_UNDO, ID_EDIT_UNDO, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
        {STD_REDOW, ID_EDIT_REDO, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
    };
    SendMessage(ctx->hwnd_toolbar, TB_ADDBUTTONS, 6, (LPARAM)tbb);
    SendMessage(ctx->hwnd_toolbar, TB_AUTOSIZE, 0, 0); return true;
}

static bool create_statusbar(AppContext *ctx) {
    ctx->hwnd_statusbar = CreateWindowExA(0, STATUSCLASSNAMEA, NULL, WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 0, 0, ctx->hwnd_main, NULL, ctx->hinstance, NULL);
    int parts[4] = {200, 450, 600, -1};
    SendMessageA(ctx->hwnd_statusbar, SB_SETPARTS, 4, (LPARAM)parts);
    return true;
}

void ui_update_statusbar(AppContext *ctx, size_t words, int line, int col) {
    if (!ctx->hwnd_statusbar) return;
    char buf[64];
    snprintf(buf, sizeof(buf), "Mots: %zu", words); SendMessageA(ctx->hwnd_statusbar, SB_SETTEXTA, 0, (LPARAM)buf);
    snprintf(buf, sizeof(buf), "Lig %d, Col %d", line+1, col+1); SendMessageA(ctx->hwnd_statusbar, SB_SETTEXTA, 1, (LPARAM)buf);
}

void ui_update_rules_panel(AppContext *ctx, const RuleReport *report) {
    if (!ctx->hwnd_rules_panel || !report) return;
    SendMessageA(ctx->hwnd_rules_panel, LB_RESETCONTENT, 0, 0);
    for (size_t i = 0; i < report->result_count; i++) {
        char line[512]; const char *prefix = "";
        switch(report->results[i].status) {
            case RULE_STATUS_PASS: prefix = "✅ "; break;
            case RULE_STATUS_FAIL: prefix = "❌ "; break;
            case RULE_STATUS_WARNING: prefix = "⚠️ "; break;
            case RULE_STATUS_PENDING: prefix = "🔄 "; break;
            default: prefix = "❓ "; break;
        }
        snprintf(line, sizeof(line), "%s%s: %s", prefix, report->results[i].rule_id, report->results[i].message);
        SendMessageA(ctx->hwnd_rules_panel, LB_ADDSTRING, 0, (LPARAM)line);
    }
}
