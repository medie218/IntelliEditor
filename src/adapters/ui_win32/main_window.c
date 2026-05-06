#include "../../../include/ui.h"
#include "../../../include/editor.h"
#include "../../../include/config.h"

#include <commctrl.h>
#include <stdbool.h>
#include <windows.h>
#include <shlobj.h>
#include <process.h>
#include <stdio.h>
#include "../../../include/Scintilla.h"
#include "../../../include/SciLexer.h"

// Déclaration externe pour le wrapper Scintilla
extern bool scintilla_load(void);
extern HWND scintilla_create(HWND parent, HINSTANCE hinstance, int x, int y, int w, int h);

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
static bool create_menu(HWND hwnd);
static bool create_statusbar(AppContext *ctx);
static bool create_toolbar(AppContext *ctx);

static bool g_syncing = false;  // Flag pour éviter les boucles infinies

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev,
                   LPSTR lpCmd, int nShow) {
    (void)hPrev; (void)lpCmd;
 
    /* Initialiser les contrôles communs Windows */
    INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_WIN95_CLASSES|ICC_BAR_CLASSES};
    InitCommonControlsEx(&icc);
 
    /* Créer le contexte application */
    AppContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.hinstance = hInst;
 
    /* Créer le document Core */
    ctx.doc = editor_create();
    if (!ctx.doc) {
        MessageBoxA(NULL, 'Mémoire insuffisante', 'Erreur', MB_ICONERROR);
        return 1;
    }
 
    /* Initialiser et lancer l'UI */
    if (!ui_init(&ctx, hInst, nShow)) return 1;
    int code = ui_run(&ctx);
    ui_cleanup(&ctx);
    editor_destroy(ctx.doc);
    return code;
}


  

bool ui_init(AppContext *ctx, HINSTANCE hinstance, int ncmdshow) {
    if (!scintilla_load()) {
        MessageBoxA(NULL, "Impossible de charger Scintilla.",
                    "Erreur fatale", MB_ICONERROR | MB_OK);
        return false;
    }

    WNDCLASSEXA wc = {0};
    wc.cbSize        = sizeof(WNDCLASSEXA);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
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
        NULL, NULL, hinstance, ctx);

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
    if (ctx->hwnd_statusbar) DestroyWindow(ctx->hwnd_statusbar);
    if (ctx->hwnd_toolbar) DestroyWindow(ctx->hwnd_toolbar);
    if (ctx->hwnd_rules_panel) DestroyWindow(ctx->hwnd_rules_panel);
    if (ctx->hwnd_scintilla) DestroyWindow(ctx->hwnd_scintilla);
}

/* ------------------------------------------------------------------------- */

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    AppContext *ctx = (AppContext *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

    switch (msg) {
    case WM_CREATE: {
        CREATESTRUCTA *cs = (CREATESTRUCTA *)lp;
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
        ctx = (AppContext *)cs->lpCreateParams;
        ctx->hwnd_main = hwnd;

        create_menu(hwnd);
        create_statusbar(ctx);
        create_toolbar(ctx);

        ctx->hwnd_scintilla = scintilla_create(hwnd, ctx->hinstance, 0, 0, 800, 600);

        ctx->hwnd_rules_panel = CreateWindowExA(
            WS_EX_CLIENTEDGE, "LISTBOX", "",
            WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL,
            0, 0, 200, 600,
            hwnd, (HMENU)ID_RULES_PANEL, ctx->hinstance, NULL);

        return 0;
    }

    case WM_SIZE: {
        RECT rc;
        GetClientRect(hwnd, &rc);
        int panel_w   = UI_RULES_PANEL_W;
        int toolbar_h = 30;
        int status_h  = 20;

        MoveWindow(ctx->hwnd_scintilla, 0, toolbar_h,
                   rc.right - panel_w,
                   rc.bottom - toolbar_h - status_h, TRUE);

        MoveWindow(ctx->hwnd_rules_panel, rc.right - panel_w, toolbar_h,
                   panel_w,
                   rc.bottom - toolbar_h - status_h, TRUE);

        MoveWindow(ctx->hwnd_statusbar, 0, rc.bottom - status_h,
                   rc.right, status_h, TRUE);
        return 0;
    }

    case WM_COMMAND: {
        switch (LOWORD(wp)) {
        case ID_FILE_EXIT:
            DestroyWindow(hwnd);
            break;
        }
        return 0;
    }

    case WM_NOTIFY: {
        NMHDR *nm = (NMHDR *)lp;
        if (nm->hwndFrom == ctx->hwnd_scintilla && nm->code == SCN_MODIFIED) {
            SCNotification *scn = (SCNotification *)lp;
            if (!g_syncing) {
                if (scn->modificationType & SC_MOD_INSERTTEXT) {
                    editor_insert(ctx->doc, scn->text, scn->length);
                } else if (scn->modificationType & SC_MOD_DELETETEXT) {
                    editor_delete(ctx->doc, scn->position, scn->length);
                }
            }
        }
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

/* ------------------------------------------------------------------------- */

static bool create_menu(HWND hwnd) {
    HMENU menu_bar  = CreateMenu();
    HMENU menu_file = CreatePopupMenu();
    AppendMenuA(menu_file, MF_STRING, ID_FILE_EXIT, "Quitter");
    AppendMenuA(menu_bar, MF_POPUP, (UINT_PTR)menu_file, "Fichier");
    SetMenu(hwnd, menu_bar);
    return true;
}

static bool create_statusbar(AppContext *ctx) {
    ctx->hwnd_statusbar = CreateWindowExA(
        0, STATUSCLASSNAMEA, NULL,
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        0, 0, 0, 0,
        ctx->hwnd_main, NULL, ctx->hinstance, NULL);

    int parts[3] = {200, 400, -1};
    SendMessage(ctx->hwnd_statusbar, SB_SETPARTS, 3, (LPARAM)parts);
    SendMessageA(ctx->hwnd_statusbar, SB_SETTEXTA, 0, (LPARAM)"Mots: 0");
    SendMessageA(ctx->hwnd_statusbar, SB_SETTEXTA, 1, (LPARAM)"UTF-8");
    SendMessageA(ctx->hwnd_statusbar, SB_SETTEXTA, 2, (LPARAM)"Prêt");
    return true;
}

static bool create_toolbar(AppContext *ctx) {
    ctx->hwnd_toolbar = CreateWindowExA(
        0, TOOLBARCLASSNAMEA, NULL,
        WS_CHILD | WS_VISIBLE | TBSTYLE_WRAPABLE,
        0, 0, 0, 0,
        ctx->hwnd_main, NULL, ctx->hinstance, NULL);

    if (!ctx->hwnd_toolbar) return false;

    // Envoyer TB_BUTTONSTRUCTSIZE
    SendMessage(ctx->hwnd_toolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

    // Définir les boutons
    TBBUTTON tbb[] = {
        {STD_FILENEW, ID_FILE_NEW, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)"Nouveau"},
        {STD_FILEOPEN, ID_FILE_OPEN, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)"Ouvrir"},
        {STD_FILESAVE, ID_FILE_SAVE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)"Sauvegarder"},
        {0, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},  // Séparateur
        {STD_UNDO, ID_EDIT_UNDO, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)"Annuler"},
        {STD_REDO, ID_EDIT_REDO, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)"Rétablir"},
    };

    // Ajouter les boutons
    SendMessage(ctx->hwnd_toolbar, TB_ADDBUTTONS, sizeof(tbb)/sizeof(TBBUTTON), (LPARAM)tbb);

    // Redimensionner la barre d'outils
    SendMessage(ctx->hwnd_toolbar, TB_AUTOSIZE, 0, 0);

    return true;
}

/* ------------------------------------------------------------------------- */

void ui_update_statusbar(AppContext *ctx, size_t words, int line, int col) {
    if (!ctx->hwnd_statusbar) return;

    char buf[256];
    sprintf(buf, "Mots: %zu", words);
    SendMessageA(ctx->hwnd_statusbar, SB_SETTEXTA, 0, (LPARAM)buf);

    sprintf(buf, "Ligne %d, Col %d", line + 1, col + 1);  // 1-based
    SendMessageA(ctx->hwnd_statusbar, SB_SETTEXTA, 1, (LPARAM)buf);

    SendMessageA(ctx->hwnd_statusbar, SB_SETTEXTA, 2, (LPARAM)"Prêt");
}

/* ------------------------------------------------------------------------- */

void ui_sync_text(AppContext *ctx) {
    if (!ctx->hwnd_scintilla || !ctx->doc) return;

    g_syncing = true;
    char *text = editor_get_text(ctx->doc);
    if (!text) {
        g_syncing = false;
        return;
    }

    SendMessageA(ctx->hwnd_scintilla, SCI_SETTEXT, 0, (LPARAM)text);
    free(text);
    g_syncing = false;
}

/* ------------------------------------------------------------------------- */

void ui_apply_nlp_markers(AppContext *ctx, const NlpResult *result) {
    if (!ctx->hwnd_scintilla || !result) return;

    // Effacer tous les indicateurs
    SendMessageA(ctx->hwnd_scintilla, SCI_SETINDICATORCURRENT, 0, 0);
    SendMessageA(ctx->hwnd_scintilla, SCI_INDICATORCLEARRANGE, 0, (LPARAM)SendMessageA(ctx->hwnd_scintilla, SCI_GETLENGTH, 0, 0));

    SendMessageA(ctx->hwnd_scintilla, SCI_SETINDICATORCURRENT, 1, 0);
    SendMessageA(ctx->hwnd_scintilla, SCI_INDICATORCLEARRANGE, 0, (LPARAM)SendMessageA(ctx->hwnd_scintilla, SCI_GETLENGTH, 0, 0));

    SendMessageA(ctx->hwnd_scintilla, SCI_SETINDICATORCURRENT, 2, 0);
    SendMessageA(ctx->hwnd_scintilla, SCI_INDICATORCLEARRANGE, 0, (LPARAM)SendMessageA(ctx->hwnd_scintilla, SCI_GETLENGTH, 0, 0));

    // Appliquer les nouveaux
    for (size_t i = 0; i < result->error_count; i++) {
        const NlpError *err = &result->errors[i];
        int indicator = 0;  // défaut spelling
        if (err->type == NLP_ERROR_GRAMMAR) indicator = 1;
        else if (err->type == NLP_ERROR_STYLE) indicator = 2;

        SendMessageA(ctx->hwnd_scintilla, SCI_SETINDICATORCURRENT, indicator, 0);
        SendMessageA(ctx->hwnd_scintilla, SCI_INDICATORFILLRANGE, (WPARAM)err->position, (LPARAM)err->length);
    }
}

/* ------------------------------------------------------------------------- */

void ui_update_rules_panel(AppContext *ctx, const RuleReport *report) {
    if (!ctx->hwnd_rules_panel || !report) return;

    // Vider la liste
    SendMessageA(ctx->hwnd_rules_panel, LB_RESETCONTENT, 0, 0);

    // Ajouter chaque résultat
    for (size_t i = 0; i < report->result_count; i++) {
        const RuleResult *res = &report->results[i];
        char buf[512];
        sprintf(buf, "%s : %s — %s",
                res->rule_id,
                rule_status_to_string(res->status),
                res->message ? res->message : "");
        SendMessageA(ctx->hwnd_rules_panel, LB_ADDSTRING, 0, (LPARAM)buf);
    }
}
   