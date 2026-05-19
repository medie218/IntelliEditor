#include <windows.h>
#include <commdlg.h>
#include "ui.h"
#include "editor.h"
#include "config.h"
#include "storage.h"

#include <commctrl.h>
#include <stdbool.h>
#include <shlobj.h>
#include <process.h>
#include <stdio.h>
#include "Scintilla.h"
#include "SciLexer.h"

extern bool scintilla_load(void);
extern HWND scintilla_create(HWND parent, HINSTANCE hinstance, int x, int y, int w, int h);

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
static bool create_menu(HWND hwnd);
static bool create_statusbar(AppContext *ctx);
static bool create_toolbar(AppContext *ctx);

static bool g_syncing = false;

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    (void)hPrev; (void)lpCmd;
    INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_WIN95_CLASSES|ICC_BAR_CLASSES};
    InitCommonControlsEx(&icc);
    AppContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.hinstance = hInst;
    ctx.doc = editor_create();
    if (!ctx.doc) {
        MessageBoxA(NULL, "Memoire insuffisante", "Erreur", MB_ICONERROR);
        return 1;
    }
    if (!ui_init(&ctx, hInst, nShow)) return 1;
    int code = ui_run(&ctx);
    ui_cleanup(&ctx);
    editor_destroy(ctx.doc);
    return code;
}

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
        NULL, NULL, hinstance, ctx);
    if (!ctx->hwnd_main) return false;
    ShowWindow(ctx->hwnd_main, ncmdshow);
    UpdateWindow(ctx->hwnd_main);
    return true;
}

int ui_run(AppContext *ctx) {
    (void)ctx;
    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return (int)msg.wParam;
}

void ui_cleanup(AppContext *ctx) {
    if (ctx->hwnd_statusbar)   DestroyWindow(ctx->hwnd_statusbar);
    if (ctx->hwnd_toolbar)     DestroyWindow(ctx->hwnd_toolbar);
    if (ctx->hwnd_rules_panel) DestroyWindow(ctx->hwnd_rules_panel);
    if (ctx->hwnd_scintilla)   DestroyWindow(ctx->hwnd_scintilla);
}

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
        if (!ctx) break;
        RECT rc;
        GetClientRect(hwnd, &rc);
        int panel_w   = UI_RULES_PANEL_W;
        int toolbar_h = 30;
        int status_h  = 20;
        int edit_w    = rc.right - panel_w;
        int edit_h    = rc.bottom - toolbar_h - status_h;
        if (ctx->hwnd_scintilla)
            MoveWindow(ctx->hwnd_scintilla, 0, toolbar_h, edit_w, edit_h, TRUE);
        if (ctx->hwnd_rules_panel)
            MoveWindow(ctx->hwnd_rules_panel, edit_w, toolbar_h, panel_w, edit_h, TRUE);
        if (ctx->hwnd_statusbar)
            SendMessageA(ctx->hwnd_statusbar, WM_SIZE, 0, 0);
        return 0;
    }

    case WM_COMMAND: {
        if (!ctx) break;
        int id = LOWORD(wp);
        switch (id) {
        case ID_FILE_NEW:
            editor_destroy(ctx->doc);
            ctx->doc = editor_create();
            ui_sync_text(ctx);
            SetWindowTextA(hwnd, UI_WINDOW_TITLE " - Nouveau document");
            break;
        case ID_FILE_OPEN: {
            char path[MAX_PATH] = {0};
            OPENFILENAMEA ofn = {0};
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner   = hwnd;
            ofn.lpstrFile   = path;
            ofn.nMaxFile    = MAX_PATH;
            ofn.lpstrFilter = "Texte (*.txt)\0*.txt\0Tous (*.*)\0*.*\0";
            ofn.Flags       = OFN_FILEMUSTEXIST;
            if (GetOpenFileNameA(&ofn)) {
                size_t len = 0;
                char *content = storage_read_file(path, &len);
                if (content) {
                    editor_destroy(ctx->doc);
                    ctx->doc = editor_create();
                    editor_insert(ctx->doc, content, len);
                    ctx->doc->dirty = false;
                    free(content);
                    ui_sync_text(ctx);
                }
            }
            break;
        }
        case ID_FILE_EXIT:
            DestroyWindow(hwnd);
            break;
        case ID_EDIT_UNDO:
            editor_undo(ctx->doc);
            ui_sync_text(ctx);
            break;
        case ID_EDIT_REDO:
            editor_redo(ctx->doc);
            ui_sync_text(ctx);
            break;
        }
        return 0;
    }

    case WM_NOTIFY: {
        if (!ctx) break;
        NMHDR *nm = (NMHDR *)lp;
        if (nm->hwndFrom == ctx->hwnd_scintilla && nm->code == SCN_MODIFIED) {
            SCNotification *scn = (SCNotification *)lp;
            if (!g_syncing) {
                if (scn->modificationType & SC_MOD_INSERTTEXT)
                    editor_insert(ctx->doc, scn->text, scn->length);
                else if (scn->modificationType & SC_MOD_DELETETEXT)
                    editor_delete(ctx->doc, scn->position, scn->length);
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

static bool create_menu(HWND hwnd) {
    HMENU bar    = CreateMenu();
    HMENU mFile  = CreatePopupMenu();
    HMENU mEdit  = CreatePopupMenu();
    HMENU mTools = CreatePopupMenu();
    AppendMenuA(mFile,  MF_STRING,    ID_FILE_NEW,         "Nouveau\tCtrl+N");
    AppendMenuA(mFile,  MF_STRING,    ID_FILE_OPEN,        "Ouvrir...\tCtrl+O");
    AppendMenuA(mFile,  MF_STRING,    ID_FILE_SAVE,        "Enregistrer\tCtrl+S");
    AppendMenuA(mFile,  MF_SEPARATOR, 0,                   NULL);
    AppendMenuA(mFile,  MF_STRING,    ID_FILE_EXIT,        "Quitter\tAlt+F4");
    AppendMenuA(mEdit,  MF_STRING,    ID_EDIT_UNDO,        "Annuler\tCtrl+Z");
    AppendMenuA(mEdit,  MF_STRING,    ID_EDIT_REDO,        "Retablir\tCtrl+Y");
    AppendMenuA(mTools, MF_STRING,    ID_TOOLS_RULES_LOAD, "Charger regles...");
    AppendMenuA(bar,    MF_POPUP,     (UINT_PTR)mFile,     "Fichier");
    AppendMenuA(bar,    MF_POPUP,     (UINT_PTR)mEdit,     "Edition");
    AppendMenuA(bar,    MF_POPUP,     (UINT_PTR)mTools,    "Outils");
    SetMenu(hwnd, bar);
    return true;
}

static bool create_statusbar(AppContext *ctx) {
    ctx->hwnd_statusbar = CreateWindowExA(
        0, STATUSCLASSNAMEA, NULL,
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        0, 0, 0, 0,
        ctx->hwnd_main, NULL, ctx->hinstance, NULL);
    if (!ctx->hwnd_statusbar) return false;
    int parts[4] = {200, 400, 550, -1};
    SendMessageA(ctx->hwnd_statusbar, SB_SETPARTS, 4, (LPARAM)parts);
    SendMessageA(ctx->hwnd_statusbar, SB_SETTEXTA, 0, (LPARAM)"Mots: 0");
    SendMessageA(ctx->hwnd_statusbar, SB_SETTEXTA, 2, (LPARAM)"UTF-8");
    SendMessageA(ctx->hwnd_statusbar, SB_SETTEXTA, 3, (LPARAM)"Pret");
    return true;
}

static bool create_toolbar(AppContext *ctx) {
    ctx->hwnd_toolbar = CreateWindowExA(
        0, TOOLBARCLASSNAMEA, NULL,
        WS_CHILD | WS_VISIBLE | TBSTYLE_WRAPABLE,
        0, 0, 0, 0,
        ctx->hwnd_main, NULL, ctx->hinstance, NULL);
    if (!ctx->hwnd_toolbar) return false;
    SendMessage(ctx->hwnd_toolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    TBBUTTON tbb[] = {
        {STD_FILENEW,  ID_FILE_NEW,  TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)"Nouveau"},
        {STD_FILEOPEN, ID_FILE_OPEN, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)"Ouvrir"},
        {STD_FILESAVE, ID_FILE_SAVE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)"Sauvegarder"},
        {0,            0,            TBSTATE_ENABLED, BTNS_SEP,    {0}, 0, 0},
        {STD_UNDO,     ID_EDIT_UNDO, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)"Annuler"},
        {STD_REDOW,    ID_EDIT_REDO, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)"Retablir"},
    };
    SendMessage(ctx->hwnd_toolbar, TB_ADDBUTTONS, sizeof(tbb)/sizeof(TBBUTTON), (LPARAM)tbb);
    SendMessage(ctx->hwnd_toolbar, TB_AUTOSIZE, 0, 0);
    return true;
}

void ui_update_statusbar(AppContext *ctx, size_t words, int line, int col) {
    if (!ctx || !ctx->hwnd_statusbar) return;
    char buf[64];
    snprintf(buf, sizeof(buf), "Mots: %zu", words);
    SendMessageA(ctx->hwnd_statusbar, SB_SETTEXTA, 0, (LPARAM)buf);
    snprintf(buf, sizeof(buf), "Ligne %d, Col %d", line+1, col+1);
    SendMessageA(ctx->hwnd_statusbar, SB_SETTEXTA, 1, (LPARAM)buf);
}

void ui_update_rules_panel(AppContext *ctx, const RuleReport *report) {
    if (!ctx || !ctx->hwnd_rules_panel || !report) return;
    SendMessageA(ctx->hwnd_rules_panel, LB_RESETCONTENT, 0, 0);
    for (size_t i = 0; i < report->result_count; i++) {
        char line[300];
        const char *icon =
            report->results[i].status == RULE_STATUS_PASS    ? "[OK] " :
            report->results[i].status == RULE_STATUS_FAIL    ? "[KO] " :
            report->results[i].status == RULE_STATUS_PENDING ? "[..] " : "[!!] ";
        snprintf(line, sizeof(line), "%s%s - %s",
                 icon, report->results[i].rule_id, report->results[i].message);
        SendMessageA(ctx->hwnd_rules_panel, LB_ADDSTRING, 0, (LPARAM)line);
    }
    char summary[128];
    snprintf(summary, sizeof(summary), "Conformite: %zu/%zu OK",
             report->pass_count, report->result_count);
    SendMessageA(ctx->hwnd_rules_panel, LB_ADDSTRING, 0, (LPARAM)summary);
}
