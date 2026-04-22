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

/* ============================================================================
 * CONSTANTES SCINTILLA
 * Ces constantes viennent normalement de Scintilla.h (dans le SDK Scintilla)
 * Elles sont définies ici pour que le projet compile sans le SDK.
 * TODO [DEV-B / TODO-SCI-001] : Inclure le vrai Scintilla.h
 * ============================================================================ */

#define SCI_SETTEXT            2181
#define SCI_GETTEXT            2182
#define SCI_GETTEXTLENGTH      2183
#define SCI_GETLENGTH          2006
#define SCI_SETSEL             2160
#define SCI_GOTOPOS            2025
#define SCI_GOTOLINE           2024
#define SCI_GETLINECOUNT       2154
#define SCI_SETMARGINWIDTHN    2242
#define SCI_STYLESETFORE       2051
#define SCI_STYLESETBACK       2052
#define SCI_STYLESETSIZE       2056
#define SCI_STYLESETFONT       2056
#define SCI_SETINDENTATIONGUIDES 2132
#define SCI_SETWRAPMODE        2268
#define SCI_SETINDICATORCURRENT 2500
#define SCI_INDICATORFILLRANGE 2504
#define SCI_INDICSETSTYLE      2080
#define SCI_INDICSETFORE       2082
#define INDIC_SQUIGGLE         1
#define INDIC_DOTS             2
#define INDIC_BOX              6
#define SC_WRAP_WORD           1
#define SC_WRAP_NONE           0
#define SCN_MODIFIED           2008
#define SC_MOD_INSERTTEXT      0x1
#define SC_MOD_DELETETEXT      0x2

/* Handle vers la DLL Scintilla */
static HMODULE g_scintilla_dll = NULL;

/**
 * @brief Charge la DLL Scintilla et enregistre la classe de contrôle.
 *
 * TODO [DEV-B / TODO-SCI-002] :
 *   Tenter dans cet ordre :
 *   1. "SciLexer.dll" (dans le même dossier)
 *   2. "Scintilla.dll"
 *   3. Chemin de config (config.ini → [Editor] scintilla_path)
 *
 * @return true Si Scintilla est chargé.
 */
bool scintilla_load(void) {
    if (g_scintilla_dll) return true; /* Déjà chargé */

    g_scintilla_dll = LoadLibraryA("SciLexer.dll");
    if (!g_scintilla_dll) {
        g_scintilla_dll = LoadLibraryA("Scintilla.dll");
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
 *
 * TODO [DEV-B / TODO-SCI-003] :
 *   Après création, configurer :
 *   - Police (Consolas 12pt)
 *   - Numéros de ligne (marge 0)
 *   - Retour à la ligne automatique
 *   - Indicateurs NLP (soulignements rouges/bleus)
 *   - Couleurs de fond et de texte
 *
 * @param parent  Fenêtre parent.
 * @param hinstance Instance Win32.
 * @param x, y, w, h  Position et taille initiales.
 * @return  Handle du contrôle Scintilla, ou NULL si erreur.
 */
HWND scintilla_create(HWND parent, HINSTANCE hinstance, int x, int y, int w, int h) {
    if (!scintilla_load()) return NULL;

    HWND hwnd = CreateWindowExA(
        0,
        "Scintilla",
        "",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPCHILDREN,
        x, y, w, h,
        parent, NULL, hinstance, NULL
    );

    if (!hwnd) {
        fprintf(stderr, "[ERROR] Impossible de créer le contrôle Scintilla\n");
        return NULL;
    }

    /* Configuration initiale */
    scintilla_configure_defaults(hwnd);

    printf("[INFO] Contrôle Scintilla créé\n");
    return hwnd;
}

/**
 * @brief Configure les paramètres par défaut de Scintilla.
 *
 * TODO [DEV-B / TODO-SCI-004] :
 *   Compléter la configuration :
 *   - Thème clair / sombre selon la config
 *   - Taille de police configurable
 *   - Marges (numéros de ligne, symboles de debug)
 *   - Indicateurs NLP (3 types : rouge squiggle, bleu dots, orange box)
 */
void scintilla_configure_defaults(HWND hwnd_sci) {
    if (!hwnd_sci) return;

    /* Retour à la ligne sur les mots */
    SendMessageA(hwnd_sci, SCI_SETWRAPMODE, SC_WRAP_WORD, 0);

    /* Marge pour les numéros de ligne (marge 0, largeur 40px) */
    SendMessageA(hwnd_sci, SCI_SETMARGINWIDTHN, 0, 40);

    /*
     * TODO [DEV-B / TODO-SCI-004] :
     *   // Police Consolas 12pt
     *   SendMessageA(hwnd_sci, SCI_STYLESETFONT, STYLE_DEFAULT, (LPARAM)"Consolas");
     *   SendMessageA(hwnd_sci, SCI_STYLESETSIZE, STYLE_DEFAULT, 12);
     *
     *   // Indicateur 0 : fautes orthographiques (rouge squiggle)
     *   SendMessageA(hwnd_sci, SCI_INDICSETSTYLE, 0, INDIC_SQUIGGLE);
     *   SendMessageA(hwnd_sci, SCI_INDICSETFORE,  0, RGB(220, 50, 47));
     *
     *   // Indicateur 1 : fautes grammaticales (bleu)
     *   SendMessageA(hwnd_sci, SCI_INDICSETSTYLE, 1, INDIC_DOTS);
     *   SendMessageA(hwnd_sci, SCI_INDICSETFORE,  1, RGB(38, 139, 210));
     *
     *   // Indicateur 2 : style (orange)
     *   SendMessageA(hwnd_sci, SCI_INDICSETSTYLE, 2, INDIC_BOX);
     *   SendMessageA(hwnd_sci, SCI_INDICSETFORE,  2, RGB(203, 75, 22));
     */

    fprintf(stderr, "[STUB] scintilla_configure_defaults: configuration partielle (TODO-SCI-004)\n");
}

/**
 * @brief Envoie un texte UTF-8 vers Scintilla.
 *
 * @param hwnd_sci  Handle Scintilla.
 * @param text      Texte UTF-8 null-terminé.
 */
void scintilla_set_text(HWND hwnd_sci, const char *text) {
    if (!hwnd_sci || !text) return;
    SendMessageA(hwnd_sci, SCI_SETTEXT, 0, (LPARAM)text);
}

/**
 * @brief Lit le texte actuel de Scintilla.
 *
 * Alloue un buffer. L'appelant doit libérer avec free().
 *
 * NOTE ARCHITECTURALE : Cette fonction ne devrait servir QU'À la
 * synchronisation initiale. Le texte "vrai" est dans le gap buffer.
 *
 * @param hwnd_sci  Handle Scintilla.
 * @return          Texte UTF-8 alloué, ou NULL si erreur.
 */
char *scintilla_get_text(HWND hwnd_sci) {
    if (!hwnd_sci) return NULL;

    LRESULT len = SendMessageA(hwnd_sci, SCI_GETLENGTH, 0, 0);
    char *buf = malloc((size_t)(len + 1));
    if (!buf) return NULL;

    SendMessageA(hwnd_sci, SCI_GETTEXT, (WPARAM)(len + 1), (LPARAM)buf);
    return buf;
}

/**
 * @brief Positionne le curseur Scintilla à une position donnée.
 *
 * Utilisé quand l'utilisateur clique sur une règle dans le panneau
 * pour naviguer vers l'endroit concerné.
 *
 * @param hwnd_sci  Handle Scintilla.
 * @param pos       Position en octets (depuis le début du document).
 */
void scintilla_goto_pos(HWND hwnd_sci, size_t pos) {
    if (!hwnd_sci) return;
    SendMessageA(hwnd_sci, SCI_GOTOPOS, (WPARAM)pos, 0);
}

/**
 * @brief Décharge la DLL Scintilla.
 * À appeler à la fermeture de l'application.
 */
void scintilla_unload(void) {
    if (g_scintilla_dll) {
        FreeLibrary(g_scintilla_dll);
        g_scintilla_dll = NULL;
    }
}
