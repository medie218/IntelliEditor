/**
 * @file main_window.c
 * @brief Fenêtre principale Win32 — ADAPTER / ui_win32
 *
 * =============================================================================
 * RÔLE
 * =============================================================================
 * Ce fichier contient :
 *   - WinMain() : point d'entrée de l'application Windows
 *   - WndProc() : procédure de traitement des messages Windows
 *   - Création de la fenêtre principale, des menus, de la barre de statut
 *   - Orchestration des autres composants UI (Scintilla, panneau règles)
 *
 * ARCHITECTURE IMPORTANT :
 *   WndProc reçoit les événements Windows et les traduit en appels Core.
 *   Elle ne contient PAS de logique métier.
 *
 *   Exemple de flux :
 *     [Ctrl+Z dans Scintilla]
 *          │
 *          ▼ WndProc reçoit WM_COMMAND (ID_EDIT_UNDO)
 *          │
 *          ▼ editor_undo(ctx->doc)          ← appel Core
 *          │
 *          ▼ ui_sync_text(ctx)              ← mise à jour UI
 *
 * RESPONSABLE : DEV-B
 * =============================================================================
 */

#include "../../../include/ui.h"
#include "../../../include/editor.h"
#include "../../../include/config.h"
#include <commctrl.h>
#include <stdbool.h>
#include <windows.h>
#include <shlobj.h> 
#include <process.h>   
#include <stdio.h>

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================ */
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
static bool create_menu(HWND hwnd);
static bool create_statusbar(AppContext *ctx);
static bool create_toolbar(AppContext *ctx);


/* ============================================================================
 * POINT D'ENTRÉE WINDOWS
 * ============================================================================ */

static void __cdecl llm_loader_thread(void *arg) {
    AppContext *ctx = (AppContext *)arg;
      AppConfig *config;
    // Ici tu peux lancer ton initialisation LLM
    // ex: llm_init(ctx);
    _endthread();
}

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR     lpCmdLine,
                   int       nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;

    /* Initialiser les contrôles communs Windows */
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icc.dwICC  = ICC_WIN95_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icc);

    /* Contexte application */
    AppContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.hinstance = hInstance;

       AppConfig cfg;
    char config_path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, config_path))) {
        strcat(config_path, "\\IntelliEditor\\config.ini");
        if (!config_load(&cfg, config_path)) {
            MessageBoxA(NULL,
                        "Impossible de charger la configuration.\nConfiguration par défaut appliquée.",
                        "IntelliEditor — Avertissement",
                        MB_ICONWARNING | MB_OK);
            config_init_defaults(&cfg); // fonction à prévoir pour valeurs par défaut
        }
    } else {
        MessageBoxA(NULL,
                    "Impossible de localiser le dossier APPDATA.\nConfiguration par défaut appliquée.",
                    "IntelliEditor — Avertissement",
                    MB_ICONWARNING | MB_OK);
        config_init_defaults(&cfg);
    }
   

    ctx.doc = editor_create();
    if (!ctx.doc) {
        MessageBoxA(NULL,
                    "Impossible de créer le document éditeur.\nMémoire insuffisante.",
                    "IntelliEditor — Erreur fatale",
                    MB_ICONERROR | MB_OK);
        return 1;
    }

    /* Initialiser l'interface graphique */
    if (!ui_init(&ctx, hInstance, nCmdShow)) {
        MessageBoxA(NULL,
                    "Impossible de créer la fenêtre principale.",
                    "IntelliEditor — Erreur fatale",
                    MB_ICONERROR | MB_OK);
        editor_destroy(ctx.doc);
        return 1;
    }

    /* Lancer la boucle de messages */
    int exit_code = ui_run(&ctx);

    /* Nettoyage */
    ui_cleanup(&ctx);
    editor_destroy(ctx.doc);

    return exit_code;
}


/* ============================================================================
 * INITIALISATION DE L'UI
 * ============================================================================ */

bool ui_init(AppContext *ctx, HINSTANCE hinstance, int ncmdshow) {
    /* Enregistrer la classe de fenêtre */
    WNDCLASSEXA wc = {0};
    wc.cbSize        = sizeof(WNDCLASSEXA);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hinstance;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = UI_WINDOW_CLASS;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassExA(&wc)) {
        fprintf(stderr, "[ERROR] RegisterClassEx échoué: %lu\n", GetLastError());
        return false;
    }

    /* Créer la fenêtre principale */
    ctx->hwnd_main = CreateWindowExA(
        0,
        UI_WINDOW_CLASS,
        UI_WINDOW_TITLE,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1200, 700,
        NULL, NULL, hinstance, ctx   /* lpParam = ctx pour WM_CREATE */
    );

    if (!ctx->hwnd_main) {
        fprintf(stderr, "[ERROR] CreateWindowEx échoué: %lu\n", GetLastError());
        return false;
    }

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
    /* TODO [DEV-B / TODO-UI-CLEANUP-001] : libérer ressources GDI, polices, etc. */
    (void)ctx;
}


/* ============================================================================
 * PROCÉDURE DE FENÊTRE (cœur de l'UI Win32)
 * ============================================================================ */

/**
 * @brief Procédure principale de traitement des messages Windows.
 *
 * C'est ici que tous les événements UI sont traités.
 *
 * TODO [DEV-B / TODO-WNDPROC-001] :
 *   Implémenter tous les cas WM_COMMAND manquants.
 *   Implémenter WM_SIZE pour le redimensionnement des contrôles.
 *   Implémenter WM_DROPFILES pour le glisser-déposer.
 */
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    /* Récupérer le contexte depuis GWLP_USERDATA */
    AppContext *ctx = (AppContext *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

    switch (msg) {

        case WM_CREATE: {
            /*
             * Stocker le contexte dans la fenêtre pour pouvoir le récupérer plus tard.
             * lpCreateParams = le paramètre passé à CreateWindowEx.
             */
            CREATESTRUCTA *cs = (CREATESTRUCTA *)lp;
            SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);

            AppContext *new_ctx = (AppContext *)cs->lpCreateParams;
            new_ctx->hwnd_main = hwnd;

            /* Créer les menus */
            create_menu(hwnd);

            /*
             * TODO [DEV-B / TODO-WNDPROC-002] :
             *   - create_statusbar(new_ctx)
             *   - create_toolbar(new_ctx)
             *   - Initialiser Scintilla (voir scintilla_wrapper.c)
             *   - Créer le panneau de règles (voir rules_panel.c)
             */

            SetWindowTextA(hwnd, UI_WINDOW_TITLE " — Nouveau document");
            return 0;
        }

        case WM_SIZE: {
            /*
             * TODO [DEV-B / TODO-WNDPROC-003] :
             *   Redimensionner Scintilla et le panneau de règles.
             *   Layout :
             *     - Scintilla : toute la zone client moins UI_RULES_PANEL_W à droite
             *     - Panneau règles : colonne droite, largeur UI_RULES_PANEL_W
             *     - Barre de statut : en bas
             *
             *   RECT rc;
             *   GetClientRect(hwnd, &rc);
             *   int panel_w = UI_RULES_PANEL_W;
             *   int toolbar_h = 30; // approximatif
             *   int status_h  = 20;
             *   MoveWindow(ctx->hwnd_scintilla, 0, toolbar_h,
             *              rc.right - panel_w, rc.bottom - toolbar_h - status_h, TRUE);
             *   MoveWindow(ctx->hwnd_rules_panel, rc.right - panel_w, toolbar_h,
             *              panel_w, rc.bottom - toolbar_h - status_h, TRUE);
             */
            return 0;
        }

        case WM_COMMAND: {
            int cmd_id = LOWORD(wp);
            switch (cmd_id) {

                case ID_FILE_NEW:
                    /*
                     * TODO [DEV-B / TODO-CMD-001] :
                     *   Si doc->dirty, demander confirmation (MessageBox)
                     *   Détruire l'ancien document, en créer un nouveau
                     *   Vider Scintilla
                     */
                    fprintf(stderr, "[STUB] ID_FILE_NEW: TODO-CMD-001\n");
                    break;

                case ID_FILE_OPEN:
                    /*
                     * TODO [DEV-B / TODO-CMD-002] :
                     *   Ouvrir une boîte de dialogue GetOpenFileName
                     *   Lire le fichier avec storage_read_file()
                     *   Charger dans le gap buffer avec editor_insert()
                     *   Synchroniser Scintilla avec ui_sync_text()
                     */
                    fprintf(stderr, "[STUB] ID_FILE_OPEN: TODO-CMD-002\n");
                    break;

                case ID_FILE_SAVE:
                    /*
                     * TODO [DEV-B / TODO-CMD-003] :
                     *   Si doc->filepath est NULL → ID_FILE_SAVE_AS
                     *   Sinon : récupérer le texte (editor_get_text)
                     *   Écrire avec storage_write_txt() ou selon l'extension
                     *   doc->dirty = false
                     *   Mettre à jour le titre de la fenêtre
                     */
                    fprintf(stderr, "[STUB] ID_FILE_SAVE: TODO-CMD-003\n");
                    break;

                case ID_EDIT_UNDO:
                    if (ctx && ctx->doc) {
                        editor_undo(ctx->doc);
                        ui_sync_text(ctx);
                    }
                    break;

                case ID_EDIT_REDO:
                    if (ctx && ctx->doc) {
                        editor_redo(ctx->doc);
                        ui_sync_text(ctx);
                    }
                    break;

                case ID_TOOLS_RULES_LOAD:
                    /*
                     * TODO [DEV-B / TODO-CMD-004] :
                     *   Ouvrir GetOpenFileName pour fichier .json
                     *   Appeler ruleset_load_from_file()
                     *   Appeler rules_evaluate() sur le texte actuel
                     *   Appeler ui_update_rules_panel()
                     */
                    fprintf(stderr, "[STUB] ID_TOOLS_RULES_LOAD: TODO-CMD-004\n");
                    break;

                case ID_FILE_EXIT:
                    DestroyWindow(hwnd);
                    break;
            }
            return 0;
        }

        case WM_LLM_RESPONSE: {
            /*
             * TODO [DEV-B / TODO-LLM-MSG-001] :
             *   Reçu quand le thread LLM a terminé une requête.
             *   lp = pointeur vers LlmResponse (alloué par le thread LLM)
             *   Traiter la réponse selon son type (grammaire, reformulation, règle)
             *   Libérer la LlmResponse
             */
            fprintf(stderr, "[STUB] WM_LLM_RESPONSE: TODO-LLM-MSG-001\n");
            return 0;
        }

        case WM_RULES_RESULT: {
            /*
             * TODO [DEV-B / TODO-RULES-MSG-001] :
             *   Reçu quand une évaluation de règles est terminée.
             *   Appeler ui_update_rules_panel()
             */
            return 0;
        }

        case WM_CLOSE:
            /*
             * TODO [DEV-B / TODO-CLOSE-001] :
             *   Vérifier doc->dirty, proposer de sauvegarder
             */
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcA(hwnd, msg, wp, lp);
}


/* ============================================================================
 * CRÉATION DES ÉLÉMENTS UI
 * ============================================================================ */

static bool create_menu(HWND hwnd) {
    HMENU menu_bar  = CreateMenu();
    HMENU menu_file = CreatePopupMenu();
    HMENU menu_edit = CreatePopupMenu();
    HMENU menu_tools= CreatePopupMenu();

    /* Menu Fichier */
    AppendMenuA(menu_file, MF_STRING, ID_FILE_NEW,    "Nouveau\tCtrl+N");
    AppendMenuA(menu_file, MF_STRING, ID_FILE_OPEN,   "Ouvrir...\tCtrl+O");
    AppendMenuA(menu_file, MF_STRING, ID_FILE_SAVE,   "Enregistrer\tCtrl+S");
    AppendMenuA(menu_file, MF_STRING, ID_FILE_SAVE_AS,"Enregistrer sous...");
    AppendMenuA(menu_file, MF_SEPARATOR, 0, NULL);
    AppendMenuA(menu_file, MF_STRING, ID_FILE_EXIT,   "Quitter\tAlt+F4");

    /* Menu Édition */
    AppendMenuA(menu_edit, MF_STRING, ID_EDIT_UNDO,  "Annuler\tCtrl+Z");
    AppendMenuA(menu_edit, MF_STRING, ID_EDIT_REDO,  "Rétablir\tCtrl+Y");
    AppendMenuA(menu_edit, MF_SEPARATOR, 0, NULL);
    AppendMenuA(menu_edit, MF_STRING, ID_EDIT_CUT,   "Couper\tCtrl+X");
    AppendMenuA(menu_edit, MF_STRING, ID_EDIT_COPY,  "Copier\tCtrl+C");
    AppendMenuA(menu_edit, MF_STRING, ID_EDIT_PASTE, "Coller\tCtrl+V");

    /* Menu Outils */
    AppendMenuA(menu_tools, MF_STRING, ID_TOOLS_RULES_LOAD, "Charger un fichier de règles...");
    AppendMenuA(menu_tools, MF_STRING, ID_TOOLS_GRAMMAR,    "Vérifier la grammaire (LLM)");

    AppendMenuA(menu_bar, MF_POPUP, (UINT_PTR)menu_file,  "Fichier");
    AppendMenuA(menu_bar, MF_POPUP, (UINT_PTR)menu_edit,  "Édition");
    AppendMenuA(menu_bar, MF_POPUP, (UINT_PTR)menu_tools, "Outils");

    SetMenu(hwnd, menu_bar);
    return true;
}

static bool create_statusbar(AppContext *ctx) {
    /*
     * TODO [DEV-B / TODO-STATUSBAR-001] :
     *   ctx->hwnd_statusbar = CreateWindowExA(
     *       0, STATUSCLASSNAMEA, NULL,
     *       WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
     *       0, 0, 0, 0,
     *       ctx->hwnd_main, NULL, ctx->hinstance, NULL);
     *
     *   // Définir les 4 parties de la barre de statut
     *   int parts[4] = {200, 400, 550, -1};
     *   SendMessage(ctx->hwnd_statusbar, SB_SETPARTS, 4, (LPARAM)parts);
     *   SendMessageA(ctx->hwnd_statusbar, SB_SETTEXTA, 0, (LPARAM)"Mots: 0");
     *   SendMessageA(ctx->hwnd_statusbar, SB_SETTEXTA, 2, (LPARAM)"UTF-8");
     *   SendMessageA(ctx->hwnd_statusbar, SB_SETTEXTA, 3, (LPARAM)"Prêt");
     */
    fprintf(stderr, "[STUB] create_statusbar: TODO-STATUSBAR-001\n");
    (void)ctx;
    return true;
}

static bool create_toolbar(AppContext *ctx) {
    /*
     * TODO [DEV-B / TODO-TOOLBAR-001] :
     *   Créer une toolbar Win32 avec les boutons :
     *   Nouveau, Ouvrir, Enregistrer | Gras, Italique, Souligné | Vérifier
     */
    fprintf(stderr, "[STUB] create_toolbar: TODO-TOOLBAR-001\n");
    (void)ctx;
    return true;
}


/* ============================================================================
 * MISE À JOUR DE L'AFFICHAGE
 * ============================================================================ */

void ui_sync_text(AppContext *ctx) {
    /*
     * TODO [DEV-B / TODO-SYNC-001] :
     *   1. char *text = editor_get_text(ctx->doc);
     *   2. Envoyer à Scintilla : SendMessage(ctx->hwnd_scintilla, SCI_SETTEXT, 0, (LPARAM)text);
     *   3. free(text);
     *   ATTENTION : ne PAS déclencher d'événement SCN_MODIFIED depuis ici
     *   (risque de boucle infinie modifications → sync → modifications)
     */
    fprintf(stderr, "[STUB] ui_sync_text: TODO-SYNC-001\n");
    (void)ctx;
}

void ui_apply_nlp_markers(AppContext *ctx, const NlpResult *result) {
    /*
     * TODO [DEV-B / TODO-NLP-MARKERS-001] :
     *   Pour chaque erreur dans result->errors[] :
     *   - SendMessage(SCI_SETINDICATORCURRENT, ...) pour choisir l'indicateur
     *   - SendMessage(SCI_INDICATORFILLRANGE, start, length) pour souligner
     *
     *   Indicateurs Scintilla suggérés :
     *   - INDIC_SQUIGGLE (rouge) pour orthographe
     *   - INDIC_DOTS (bleu) pour grammaire
     *   - INDIC_BOX (orange) pour style
     */
    fprintf(stderr, "[STUB] ui_apply_nlp_markers: TODO-NLP-MARKERS-001\n");
    (void)ctx;
    (void)result;
}

void ui_update_rules_panel(AppContext *ctx, const RuleReport *report) {
    /*
     * TODO [DEV-B / TODO-RULES-PANEL-001] :
     *   Effacer le contenu du panneau
     *   Pour chaque résultat dans report->results[] :
     *   - Afficher l'icône de statut (✅ ❌ ⚠️ 🔄)
     *   - Afficher l'ID et le message
     *   - Rendre cliquable (clic → positionner curseur dans Scintilla)
     *   Afficher le résumé : "Conformité: X/Y règles OK"
     */
    if (!ctx || !report) return;
    fprintf(stderr, "[STUB] ui_update_rules_panel: %zu résultats, %zu OK\n",
            report->result_count, report->pass_count);
}

void ui_update_statusbar(AppContext *ctx, size_t words, int line, int col) {
    /*
     * TODO [DEV-B / TODO-STATUSBAR-UPDATE-001] :
     *   char buf[64];
     *   snprintf(buf, sizeof(buf), "Mots: %zu", words);
     *   SendMessageA(ctx->hwnd_statusbar, SB_SETTEXTA, 0, (LPARAM)buf);
     *   snprintf(buf, sizeof(buf), "Ligne %d, Col %d", line, col);
     *   SendMessageA(ctx->hwnd_statusbar, SB_SETTEXTA, 1, (LPARAM)buf);
     */
    fprintf(stderr, "[STUB] ui_update_statusbar: mots=%zu, ligne=%d, col=%d\n",
            words, line, col);
    (void)ctx;
}
