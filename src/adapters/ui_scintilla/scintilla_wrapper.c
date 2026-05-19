/**
 * @file scintilla_wrapper.c
 * @brief Wrapper Scintilla — ADAPTER / ui_scintilla
 *
 * Encapsule toute la communication avec le contrôle Scintilla.
 * Scintilla est le composant d'édition avancé utilisé par Notepad++ et CLion.
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../../include/ui.h"
#include "Scintilla.h"
#include "SciLexer.h"

// Structure Context
typedef struct Context Context;

struct Context {
    HWND hwnd_scintilla;
    HINSTANCE hInstance;
};

// Prototypes des fonctions
void scintilla_configure_defaults(HWND sci);
HWND scintilla_create(HWND parent, HINSTANCE hi, int x, int y, int w, int h);
void create_scintilla_context(HWND parent, Context **out_ctx);
void scintilla_set_text(HWND hwnd_sci, const char *text);
char *scintilla_get_text(HWND hwnd_sci);
void ui_sync_text(AppContext *ctx);
void ui_apply_nlp_markers(AppContext *ctx, const NlpResult *result);
void scintilla_goto_pos(HWND hwnd_sci, size_t pos);

/* Handle vers la DLL Scintilla */
static HMODULE g_scintilla_dll = NULL;

/**
 * @brief Charge la DLL Scintilla et enregistre la classe de contrôle.
 * @return true si la DLL est chargée avec succès, false sinon.
 */
bool scintilla_load(void) {
    if (g_scintilla_dll) return true; /* Déjà chargé */

    g_scintilla_dll = LoadLibraryA("SciLexer.dll");
    if (!g_scintilla_dll) {
        g_scintilla_dll = LoadLibraryA("Scintilla.dll");
    }

    if (!g_scintilla_dll) {
        char path[MAX_PATH];
        if (GetPrivateProfileStringA("Editor", "scintilla_path", "", path, MAX_PATH, "config.ini") > 0) {
            g_scintilla_dll = LoadLibraryA(path);
        }
    }

    if (!g_scintilla_dll) {
        fprintf(stderr, "[ERROR] Impossible de charger SciLexer.dll ou Scintilla.dll\n");
        fprintf(stderr, "[INFO] Placez la DLL dans le même dossier que IntelliEditor.exe\n");
        fprintf(stderr, "[INFO] Téléchargement : https://www.scintilla.org/\n");
        return false;
    }

    printf("[INFO] Scintilla chargé avec succès\n");
    return true;
}

/**
 * @brief Crée le contrôle Scintilla dans une fenêtre parent.
 * @param parent Fenêtre parente.
 * @param hi Instance de l'application.
 * @param x Position X.
 * @param y Position Y.
 * @param w Largeur.
 * @param h Hauteur.
 * @return HWND du contrôle Scintilla créé, ou NULL en cas d'erreur.
 */
HWND scintilla_create(HWND parent, HINSTANCE hi, int x, int y, int w, int h) {
    if (!scintilla_load()) {
        fprintf(stderr, "[ERROR] Échec du chargement de Scintilla\n");
        return NULL;
    }

    HWND hwnd = CreateWindowExA(
        0, "Scintilla", NULL,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPCHILDREN,
        x, y, w, h,
        parent, NULL, hi, NULL
    );

    if (!hwnd) {
        fprintf(stderr, "[ERROR] Échec de la création de la fenêtre Scintilla\n");
        return NULL;
    }

    scintilla_configure_defaults(hwnd);
    return hwnd;
}

/**
 * @brief Crée le contrôle Scintilla avec des paramètres par défaut.
 * @param parent Fenêtre parente.
 * @return HWND du contrôle Scintilla créé, ou NULL en cas d'erreur.
 */
HWND scintilla_create_default(HWND parent) {
    return scintilla_create(parent, GetModuleHandle(NULL), 0, 0, 800, 600);
}

/**
 * @brief Configure les paramètres par défaut de Scintilla.
 * @param sci Handle vers le contrôle Scintilla.
 */
void scintilla_configure_defaults(HWND sci) {
    if (!sci) {
        fprintf(stderr, "[ERROR] Handle Scintilla invalide\n");
        return;
    }

    /* Utiliser le mode UTF-8 dans Scintilla. */
    SendMessageA(sci, SCI_SETCODEPAGE, SC_CP_UTF8, 0);
    SendMessageA(sci, SCI_TARGETASUTF8, 1, 0);
    /* Retour à la ligne automatique */
    SendMessageA(sci, SCI_SETWRAPMODE, SC_WRAP_WORD, 0);
    /* Numéros de ligne (marge 0, largeur 40px) */
    SendMessageA(sci, SCI_SETMARGINWIDTHN, 0, 40);
    /* Police Consolas 12pt */
    SendMessageA(sci, SCI_STYLESETFONT, 32, (LPARAM)"Consolas");
    SendMessageA(sci, SCI_STYLESETSIZE, 32, 12);
    /* Indicateur 0 : fautes ortho (rouge squiggle) */
    SendMessageA(sci, SCI_INDICSETSTYLE, 0, INDIC_SQUIGGLE);
    SendMessageA(sci, SCI_INDICSETFORE, 0, RGB(220, 50, 47));
}

/**
 * @brief Crée un contexte Scintilla avec une taille calculée.
 * @param parent Fenêtre parente.
 * @param out_ctx Pointeur vers un pointeur de Context pour stocker le résultat.
 */
void create_scintilla_context(HWND parent, Context **out_ctx) {
    if (!parent || !out_ctx) {
        fprintf(stderr, "[ERROR] Paramètres invalides pour create_scintilla_context\n");
        return;
    }

    // Allouer le contexte
    Context *new_ctx = (Context *)malloc(sizeof(Context));
    if (!new_ctx) {
        fprintf(stderr, "[ERROR] Échec de l'allocation mémoire pour Context\n");
        return;
    }

    new_ctx->hInstance = GetModuleHandle(NULL);
    if (!new_ctx->hInstance) {
        fprintf(stderr, "[ERROR] Échec de la récupération de l'instance\n");
        free(new_ctx);
        return;
    }

    // Calculer la taille disponible
    RECT parent_rc;
    if (!GetClientRect(parent, &parent_rc)) {
        fprintf(stderr, "[ERROR] Échec de la récupération de la taille de la fenêtre parente\n");
        free(new_ctx);
        return;
    }

    int panel_w = UI_RULES_PANEL_W;
    int width = parent_rc.right - panel_w;
    int height = parent_rc.bottom - 30 - 20;  // toolbar + statusbar

    // Créer le contrôle Scintilla
    new_ctx->hwnd_scintilla = scintilla_create(
        parent,
        new_ctx->hInstance,
        0, 30,  // x=0, y=30 (sous la toolbar)
        width,
        height
    );

    if (!new_ctx->hwnd_scintilla) {
        fprintf(stderr, "[ERROR] Échec de la création du contrôle Scintilla\n");
        free(new_ctx);
        return;
    }

    *out_ctx = new_ctx;
}

/**
 * @brief Envoie un texte UTF-8 vers Scintilla.
 * @param hwnd_sci Handle vers le contrôle Scintilla.
 * @param text Texte à envoyer.
 */
void scintilla_set_text(HWND hwnd_sci, const char *text) {
    if (!hwnd_sci) {
        fprintf(stderr, "[ERROR] Handle Scintilla invalide dans scintilla_set_text\n");
        return;
    }
    if (!text) {
        fprintf(stderr, "[WARNING] Texte NULL dans scintilla_set_text\n");
        return;
    }
    SendMessageA(hwnd_sci, SCI_SETTEXT, 0, (LPARAM)text);
}

/**
 * @brief Lit le texte actuel de Scintilla.
 * @param hwnd_sci Handle vers le contrôle Scintilla.
 * @return Pointeur vers le texte (alloué dynamiquement), ou NULL en cas d'erreur.
 */
char *scintilla_get_text(HWND hwnd_sci) {
    if (!hwnd_sci) {
        fprintf(stderr, "[ERROR] Handle Scintilla invalide dans scintilla_get_text\n");
        return NULL;
    }

    LRESULT len = SendMessageA(hwnd_sci, SCI_GETLENGTH, 0, 0);
    if (len <= 0) {
        return NULL;
    }

    char *buf = (char *)malloc((size_t)(len + 1));
    if (!buf) {
        fprintf(stderr, "[ERROR] Échec de l'allocation mémoire pour le texte\n");
        return NULL;
    }

    SendMessageA(hwnd_sci, SCI_GETTEXT, (WPARAM)(len + 1), (LPARAM)buf);
    return buf;
}

/**
 * @brief Synchronise le texte entre le gap buffer et Scintilla.
 * @param ctx Contexte de l'application.
 */
void ui_sync_text(AppContext *ctx) {
    if (!ctx) {
        fprintf(stderr, "[ERROR] Contexte NULL dans ui_sync_text\n");
        return;
    }
    if (!ctx->doc) {
        fprintf(stderr, "[ERROR] Document NULL dans ui_sync_text\n");
        return;
    }
    if (!ctx->hwnd_scintilla) {
        fprintf(stderr, "[ERROR] Handle Scintilla NULL dans ui_sync_text\n");
        return;
    }

    char *text = editor_get_text(ctx->doc);
    if (!text) {
        fprintf(stderr, "[WARNING] Texte NULL retourné par editor_get_text\n");
        return;
    }

    /* Envoyer le texte à Scintilla */
    SendMessageA(ctx->hwnd_scintilla, SCI_SETTEXT, 0, (LPARAM)text);
    free(text);

    /* Mettre à jour la barre de statut */
    DocStats stats;
    editor_compute_stats(ctx->doc, &stats);
    ui_update_statusbar(ctx, stats.word_count, 0, 0);
}

/**
 * @brief Applique les marqueurs NLP (fautes d'orthographe, etc.).
 * @param ctx Contexte de l'application.
 * @param result Résultats NLP à appliquer.
 */
void ui_apply_nlp_markers(AppContext *ctx, const NlpResult *result) {
    if (!ctx) {
        fprintf(stderr, "[ERROR] Contexte NULL dans ui_apply_nlp_markers\n");
        return;
    }
    if (!ctx->hwnd_scintilla) {
        fprintf(stderr, "[ERROR] Handle Scintilla NULL dans ui_apply_nlp_markers\n");
        return;
    }
    if (!result) {
        fprintf(stderr, "[ERROR] Résultat NLP NULL dans ui_apply_nlp_markers\n");
        return;
    }

    HWND sci = ctx->hwnd_scintilla;

    /* Effacer les anciens marqueurs */
    SendMessageA(sci, SCI_SETINDICATORCURRENT, 0, 0);
    SendMessageA(sci, SCI_INDICATORCLEARRANGE, 0, SendMessageA(sci, SCI_GETLENGTH, 0, 0));

    /* Appliquer les nouveaux marqueurs */
    for (size_t i = 0; i < result->error_count; i++) {
        const NlpError *err = &result->errors[i];
        if (!err) {
            continue;
        }

        /* Choisir l'indicateur selon le type d'erreur */
        int indicator = (err->type == NLP_ERROR_SPELLING) ? 0 : 1;
        SendMessageA(sci, SCI_SETINDICATORCURRENT, indicator, 0);
        SendMessageA(sci, SCI_INDICATORFILLRANGE, (WPARAM)err->start, (LPARAM)err->length);
    }
}

/**
 * @brief Positionne le curseur Scintilla à une position donnée.
 * @param hwnd_sci Handle vers le contrôle Scintilla.
 * @param pos Position du curseur.
 */
void scintilla_goto_pos(HWND hwnd_sci, size_t pos) {
    if (!hwnd_sci) {
        fprintf(stderr, "[ERROR] Handle Scintilla invalide dans scintilla_goto_pos\n");
        return;
    }
    SendMessageA(hwnd_sci, SCI_GOTOPOS, (WPARAM)pos, 0);
}