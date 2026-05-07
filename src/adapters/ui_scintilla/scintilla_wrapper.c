/**
 * @file scintilla_wrapper.c
 * @brief Wrapper Scintilla — ADAPTER / ui_scintilla
 *
 * =============================================================================
 * RÔLE
 * =============================================================================
 * Encapsule toute la communication avec le contrôle Scintilla.
 * Scintilla est le composant d'édition avancé utilisé par Notepad++ et CLion.
 *
 * PRINCIPE IMPORTANT :
 *   Scintilla est UN AFFICHEUR, pas la source de vérité du texte.
 *   Le texte "réel" est dans le gap buffer (EditorDocument).
 *   Scintilla affiche ce texte et reçoit les frappes clavier.
 *   Les modifications clavier sont interceptées via SCN_MODIFIED et
 *   répercutées dans le gap buffer.
 *
 * COMMUNICATION AVEC SCINTILLA :
 *   Scintilla utilise des messages Windows (SendMessage) pour tout.
 *   Les constantes commencent par SCI_ (ex: SCI_SETTEXT, SCI_GETTEXT).
 *
 *   Deux façons de communiquer :
 *   1. SendMessage(hwnd, SCI_XXX, wParam, lParam) — plus portable
 *   2. Appel direct via SciFnDirect (plus rapide, évite le dispatch Win32)
 *
 * CHARGEMENT :
 *   Scintilla est une DLL (SciLexer.dll ou Scintilla.dll).
 *   Elle s'enregistre comme classe de contrôle Win32 appelée "Scintilla".
 *
 *   Étapes :
 *   1. LoadLibrary("SciLexer.dll")
 *   2. CreateWindow("Scintilla", ...)
 *   3. Configurer les styles, les couleurs, les indicateurs
 *
 * RESPONSABLE : DEV-B
 * =============================================================================
 */

#include "../../../include/ui.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../../../include/
#include "Scintilla.h"
#include "SciLexer.h"   // TODO-SCI-001 : inclure les vrais headers

/* Handle vers la DLL Scintilla */
static HMODULE g_scintilla_dll = NULL;

/**
 * @brief Charge la DLL Scintilla et enregistre la classe de contrôle.
 */
bool scintilla_load(void) {
    if (g_scintilla_dll) return true; /* Déjà chargé */

    g_scintilla_dll = LoadLibraryA("SciLexer.dll");
    if (!g_scintilla_dll) {
        g_scintilla_dll = LoadLibraryA("Scintilla.dll");
    }

    if (!g_scintilla_dll) {
        /* TODO-SCI-002 : chercher dans config.ini */
        char path[MAX_PATH];
        GetPrivateProfileStringA("Editor", "scintilla_path", "", path, MAX_PATH, "config.ini");
        if (strlen(path) > 0) {
            g_scintilla_dll = LoadLibraryA(path);
        }
    }

    if (!g_scintilla_dll) {
        fprintf(stderr, "[ERROR] Impossible de charger SciLexer.dll ou Scintilla.dll\n");
        fprintf(stderr, "[INFO]  Placez la DLL dans le même dossier que IntelliEditor.exe\n");
        fprintf(stderr, "[INFO]  Téléchargement : https://www.scintilla.org/\n");
        return false;
    }

    printf("[INFO] Scintilla chargé avec succès\n");
    return true;
}

/**
 * @brief Crée le contrôle Scintilla dans une fenêtre parent.
 */
HWND scintilla_create(HWND parent, HINSTANCE hi, int x, int y, int w, int h) {
    if (!scintilla_load()) return NULL;
 
    HWND hwnd = CreateWindowExA(
        0, 'Scintilla', '',
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPCHILDREN,
        x, y, w, h,
        parent, NULL, hi, NULL
    );
    if (!hwnd) return NULL;
    scintilla_configure_defaults(hwnd);
    return hwnd;
}
 
void scintilla_configure_defaults(HWND sci) {
    /* Retour à la ligne automatique */
    SendMessageA(sci, SCI_SETWRAPMODE, SC_WRAP_WORD, 0);
    /* Numéros de ligne (marge 0, largeur 40px) */
    SendMessageA(sci, SCI_SETMARGINWIDTHN, 0, 40);
    /* Police Consolas 12pt */
    SendMessageA(sci, SCI_STYLESETFONT, 32, (LPARAM)'Consolas');
    SendMessageA(sci, SCI_STYLESETSIZE, 32, 12);
    /* Indicateur 0 : fautes ortho (rouge squiggle) */
    SendMessageA(sci, SCI_INDICSETSTYLE, 0, INDIC_SQUIGGLE);
    SendMessageA(sci, SCI_INDICSETFORE,  0, RGB(220,50,47));
}
RECT rc; GetClientRect(hwnd, &rc);
int panel_w = UI_RULES_PANEL_W;
new_ctx->hwnd_scintilla = scintilla_create(
    hwnd, new_ctx->hinstance,
    0, 30,                          /* x=0, y=30 (sous la toolbar) */
    rc.right - panel_w,             /* largeur */
    rc.bottom - 30 - 20             /* hauteur - toolbar - statusbar */
);
void ui_sync_text(AppContext *ctx) {
    if (!ctx || !ctx->doc || !ctx->hwnd_scintilla) return;
 
    char *text = editor_get_text(ctx->doc);
    if (!text) return;
 
    /* Envoyer le texte à Scintilla */
    SendMessageA(ctx->hwnd_scintilla, SCI_SETTEXT, 0, (LPARAM)text);
    free(text);
 
    /* Mettre à jour la barre de statut */
    DocStats stats;
    editor_compute_stats(ctx->doc, &stats);
    ui_update_statusbar(ctx, stats.word_count, 0, 0);
}



/**
 * @brief Envoie un texte UTF-8 vers Scintilla.
 */
void scintilla_set_text(HWND hwnd_sci, const char *text) {
    if (!hwnd_sci || !text) return;
    SendMessageA(hwnd_sci, SCI_SETTEXT, 0, (LPARAM)text);
}

/**
 * @brief Lit le texte actuel de Scintilla.
 */
char *scintilla_get_text(HWND hwnd_sci) {
    if (!hwnd_sci) return NULL;

    LRESULT len = SendMessageA(hwnd_sci, SCI_GETLENGTH, 0, 0);
    char *buf = malloc((size_t)(len + 1));
    if (!buf) return NULL;

    SendMessageA(hwnd_sci, SCI_GETTEXT, (WPARAM)(len + 1), (LPARAM)buf);
    return buf;
}
void ui_apply_nlp_markers(AppContext *ctx, const NlpResult *result) {
    if (!ctx->hwnd_scintilla || !result) return;
    HWND sci = ctx->hwnd_scintilla;
 
    /* Effacer les anciens marqueurs */
    SendMessageA(sci, SCI_SETINDICATORCURRENT, 0, 0);
    SendMessageA(sci, SCI_INDICATORCLEARRANGE, 0,
        SendMessageA(sci, SCI_GETLENGTH, 0, 0));
 
    /* Appliquer les nouveaux marqueurs */
    for (size_t i = 0; i < result->error_count; i++) {
        const NlpError *err = &result->errors[i];
        /* Choisir l'indicateur selon le type d'erreur */
        int indicator = (err->type == NLP_ERROR_SPELLING) ? 0 : 1;
        SendMessageA(sci, SCI_SETINDICATORCURRENT, indicator, 0);
        SendMessageA(sci, SCI_INDICATORFILLRANGE,
            (WPARAM)err->start, (LPARAM)err->length);
    }
}

/**
 * @brief Positionne le curseur Scintilla à une position donnée.
 */
void scintilla_goto_pos(HWND hwnd_sci, size_t pos) {
    if (!hwnd_sci) return;
    SendMessageA(hwnd_sci, SCI_GOTOPOS, (WPARAM)pos, 0);
}
