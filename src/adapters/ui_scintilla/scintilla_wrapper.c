/**
 * @file scintilla_wrapper.c
 * @brief Wrapper Scintilla — ADAPTER / ui_scintilla
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui.h"
#include "Scintilla.h"
#include "SciLexer.h"

/* Couleurs Thème Sombre (One Dark Inspired) */
#define THEME_BG          RGB(40, 44, 52)
#define THEME_FG          RGB(171, 178, 191)
#define THEME_LINENUM_FG  RGB(76, 82, 99)
#define THEME_LINENUM_BG  RGB(40, 44, 52)
#define THEME_SEL_BG      RGB(62, 68, 81)
#define THEME_CURSOR      RGB(82, 139, 255)
#define THEME_COMMENT     RGB(92, 99, 112)
#define THEME_STRING      RGB(152, 195, 121)
#define THEME_KEYWORD     RGB(198, 120, 221)
#define THEME_NUMBER      RGB(209, 154, 102)

/* Handle vers la DLL Scintilla */
static HMODULE g_scintilla_dll = NULL;

/**
 * @brief Charge la DLL Scintilla et enregistre la classe de contrôle.
 */
bool scintilla_load(void) {
    if (g_scintilla_dll) return true;

    g_scintilla_dll = LoadLibraryA("SciLexer.dll");
    if (!g_scintilla_dll) {
        g_scintilla_dll = LoadLibraryA("Scintilla.dll");
    }

    if (!g_scintilla_dll) {
        fprintf(stderr, "[ERROR] Impossible de charger SciLexer.dll ou Scintilla.dll\n");
        return false;
    }

    return true;
}

/**
 * @brief Configure les paramètres par défaut de Scintilla avec un thème sombre.
 */
void scintilla_configure_defaults(HWND sci) {
    /* Mode UTF-8 pour Scintilla */
    SendMessageA(sci, SCI_SETCODEPAGE, SC_CP_UTF8, 0);

    /* Retour à la ligne automatique */
    SendMessageA(sci, SCI_SETWRAPMODE, SC_WRAP_WORD, 0);

    /* --- THÈME ET STYLES --- */

    /* Style par défaut (Hérité par tous les autres) */
    SendMessageA(sci, SCI_STYLESETFONT, STYLE_DEFAULT, (LPARAM)"Consolas");
    SendMessageA(sci, SCI_STYLESETSIZE, STYLE_DEFAULT, 11);
    SendMessageA(sci, SCI_STYLESETBACK, STYLE_DEFAULT, THEME_BG);
    SendMessageA(sci, SCI_STYLESETFORE, STYLE_DEFAULT, THEME_FG);
    SendMessageA(sci, SCI_STYLECLEARALL, 0, 0);

    /* Marge des numéros de ligne (marge 0) */
    SendMessageA(sci, SCI_SETMARGINWIDTHN, 0, 45);
    SendMessageA(sci, SCI_STYLESETBACK, STYLE_LINENUMBER, THEME_LINENUM_BG);
    SendMessageA(sci, SCI_STYLESETFORE, STYLE_LINENUMBER, THEME_LINENUM_FG);

    /* Curseur et Sélection */
    SendMessageA(sci, SCI_SETCARETFORE, THEME_CURSOR, 0);
    SendMessageA(sci, SCI_SETSELBACK, 1, THEME_SEL_BG);

    /* Marge de séparation */
    SendMessageA(sci, SCI_SETMARGINWIDTHN, 1, 1);
    SendMessageA(sci, SCI_SETMARGINSENSITIVEN, 1, 0);

    /* --- INDICATEURS (NLP/Règles) --- */

    // 0: Erreur Critique (Rouge - Orthographe/Règle cassée)
    SendMessageA(sci, SCI_INDICSETSTYLE, 0, INDIC_SQUIGGLE);
    SendMessageA(sci, SCI_INDICSETFORE,  0, RGB(224, 108, 117)); // Soft Red

    // 1: Avertissement (Jaune - Grammaire)
    SendMessageA(sci, SCI_INDICSETSTYLE, 1, INDIC_SQUIGGLE);
    SendMessageA(sci, SCI_INDICSETFORE,  1, RGB(229, 192, 123)); // Soft Yellow

    // 2: Info/Style (Bleu - NLP)
    SendMessageA(sci, SCI_INDICSETSTYLE, 2, INDIC_SQUIGGLE);
    SendMessageA(sci, SCI_INDICSETFORE,  2, RGB(97, 175, 239)); // Soft Blue
}

/**
 * @brief Crée le contrôle Scintilla dans une fenêtre parent.
 */
HWND scintilla_create(HWND parent, HINSTANCE hi, int x, int y, int w, int h) {
    if (!scintilla_load()) return NULL;

    HWND hwnd = CreateWindowExA(
        0, "Scintilla", "",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPCHILDREN,
        x, y, w, h,
        parent, NULL, hi, NULL
    );

    if (hwnd) {
        scintilla_configure_defaults(hwnd);
    }
    return hwnd;
}

/* ============================================================================
 * IMPLEMENTATION DES FONCTIONS DE ui.h
 * ============================================================================ */

void ui_sync_text(AppContext *ctx) {
    if (!ctx || !ctx->hwnd_scintilla || !ctx->doc) return;

    /* On récupère le texte du Core */
    char *text = editor_get_text(ctx->doc);
    if (!text) return;

    /* On l'envoie à Scintilla */
    SendMessageA(ctx->hwnd_scintilla, SCI_SETTEXT, 0, (LPARAM)text);
    free(text);

    /* Mise à jour des stats dans la barre de statut */
    DocStats stats;
    editor_compute_stats(ctx->doc, &stats);
    ui_update_statusbar(ctx, stats.word_count, 0, 0);
}

void ui_apply_nlp_markers(AppContext *ctx, const NlpResult *result) {
    if (!ctx || !ctx->hwnd_scintilla || !result) return;
    HWND sci = ctx->hwnd_scintilla;

    /* 1. Effacer tous les indicateurs existants */
    LRESULT total_len = SendMessageA(sci, SCI_GETLENGTH, 0, 0);
    for (int ind = 0; ind <= 2; ind++) {
        SendMessageA(sci, SCI_SETINDICATORCURRENT, ind, 0);
        SendMessageA(sci, SCI_INDICATORCLEARRANGE, 0, total_len);
    }

    /* 2. Appliquer les nouveaux marqueurs */
    for (size_t i = 0; i < result->error_count; i++) {
        const NlpError *err = &result->errors[i];

        int indicator = 0; // Défaut : orthographe
        if (err->type == NLP_ERROR_GRAMMAR)     indicator = 1;
        else if (err->type == NLP_ERROR_STYLE) indicator = 2;

        SendMessageA(sci, SCI_SETINDICATORCURRENT, indicator, 0);
        SendMessageA(sci, SCI_INDICATORFILLRANGE, (WPARAM)err->start, (LPARAM)err->length);
    }
}
