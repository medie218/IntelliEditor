#include "../../../include/ui.h"
#include "../../../include/editor.h"
#include "../../../include/config.h"

#include <commctrl.h>
#include <stdbool.h>
#include <windows.h>
#include <shlobj.h>
#include <process.h>
#include <stdio.h>

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
static bool create_menu(HWND hwnd);
static bool create_statusbar(AppContext *ctx);
static bool create_toolbar(AppContext *ctx);

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;

    INITCOMMONCONTROLSEX icc = {sizeof(INITCOMMONCONTROLSEX),
                                ICC_WIN95_CLASSES | ICC_BAR_CLASSES};
    InitCommonControlsEx(&icc);

    AppContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.hinstance = hInstance;

    ctx.doc = editor_create();
    if (!ctx.doc) {
        MessageBoxA(NULL, "Impossible de créer le document éditeur.",
                    "Erreur fatale", MB_ICONERROR | MB_OK);
        return 1;
    }

    if (!ui_init(&ctx, hInstance, nCmdShow)) {
        MessageBoxA(NULL, "Impossible de créer la fenêtre principale.",
                    "Erreur fatale", MB_ICONERROR | MB_OK);
        editor_destroy(ctx.doc);
        return 1;
    }

    int exit_code = ui_run(&ctx);
    ui_cleanup(&ctx);
    editor_destroy(ctx.doc);
    return exit_code;
}

bool ui_init(AppContext *ctx, HINSTANCE hinstance, int ncmdshow) {
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

        ctx->hwnd_scintilla = CreateWindowExA(
            0, "Scintilla", "",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPCHILDREN,
            0, 0, 800, 600,
            hwnd, (HMENU)ID_SCINTILLA, ctx->hinstance, NULL);

        ctx->hwnd_rules_panel = CreateWindowExA(
            WS_EX_CLIENTEDGE, "STATIC", "Panneau règles",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
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
    return true;
}
   