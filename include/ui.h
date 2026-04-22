/**
 * @file ui.h
 * @brief Contrat public du module UI Win32 — ADAPTER
 *
 * =============================================================================
 * RESPONSABILITÉ DE CE MODULE
 * =============================================================================
 * Définit les types et fonctions pour créer et gérer l'interface graphique
 * Win32 + Scintilla d'IntelliEditor.
 *
 * RÈGLE IMPORTANTE :
 *   - L'UI ne stocke PAS le texte : elle lit depuis EditorDocument (Core).
 *   - L'UI ne valide PAS les règles : elle affiche RuleReport (Core).
 *   - L'UI ne corrige PAS l'orthographe : elle affiche NlpResult (Core).
 *
 * FLUX ENTRANT (UI reçoit) :
 *   EditorDocument → texte à afficher dans Scintilla
 *   NlpResult      → soulignements rouges dans Scintilla
 *   RuleReport     → statuts dans le panneau de règles
 *
 * FLUX SORTANT (UI émet) :
 *   Frappes clavier → editor_insert() / editor_delete()
 *   Commandes menu  → editor_undo() / editor_redo() / etc.
 *
 * APPARTIENT À LA COUCHE : ADAPTER
 * AUTEUR(S) RESPONSABLE(S) : DEV-B
 *
 * =============================================================================
 */

#ifndef INTELLIEDITOR_UI_H
#define INTELLIEDITOR_UI_H

/* NOTE : Ce header inclut windows.h uniquement pour les types Win32.
 * Le Core ne l'inclut JAMAIS. */
#include <windows.h>
#include <stdbool.h>
#include <stddef.h>

#include "editor.h"
#include "rules.h"
#include "nlp.h"

/* ============================================================================
 * CONSTANTES
 * ============================================================================ */

#define UI_WINDOW_TITLE    "IntelliEditor"
#define UI_WINDOW_CLASS    "IntelliEditorWnd"
#define UI_MIN_WIDTH        800
#define UI_MIN_HEIGHT       600
#define UI_RULES_PANEL_W    280   /**< Largeur du panneau de règles (pixels)  */

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

/* Messages personnalisés (WM_USER + offset) */
#define WM_LLM_RESPONSE     (WM_USER + 100)  /**< LLM a fini une requête     */
#define WM_NLP_RESULT       (WM_USER + 101)  /**< Analyse NLP terminée       */
#define WM_RULES_RESULT     (WM_USER + 102)  /**< Évaluation règles terminée */


/* ============================================================================
 * STRUCTURES
 * ============================================================================ */

/**
 * @brief Contexte global de l'application UI.
 *
 * Rassemble toutes les handles et pointeurs nécessaires à l'UI.
 * Un seul AppContext existe par processus.
 */
typedef struct {
    HWND           hwnd_main;       /**< Fenêtre principale                   */
    HWND           hwnd_scintilla;  /**< Contrôle Scintilla                   */
    HWND           hwnd_rules_panel;/**< Panneau des règles                   */
    HWND           hwnd_statusbar;  /**< Barre de statut                      */
    HWND           hwnd_toolbar;    /**< Barre d'outils                       */
    HINSTANCE      hinstance;       /**< Handle de l'instance Win32           */
    EditorDocument *doc;            /**< Document actif (pointer, non owner)  */
    RuleReport     *report;         /**< Dernier rapport de règles            */
    bool            llm_loading;    /**< true si le LLM est en cours de chargement */
} AppContext;


/* ============================================================================
 * API PUBLIQUE — INITIALISATION
 * ============================================================================ */

/**
 * @brief Crée et affiche la fenêtre principale de l'application.
 *
 * Enregistre la classe de fenêtre Win32, crée la fenêtre, intègre Scintilla,
 * crée le panneau de règles et la barre de statut.
 *
 * @param ctx        Contexte applicatif à initialiser.
 * @param hinstance  Handle d'instance Win32 (depuis WinMain).
 * @param ncmdshow   Paramètre d'affichage (depuis WinMain).
 * @return true      Si l'initialisation a réussi.
 */
bool ui_init(AppContext *ctx, HINSTANCE hinstance, int ncmdshow);

/**
 * @brief Lance la boucle de messages Win32 (bloquant jusqu'à la fermeture).
 *
 * @param ctx  Contexte applicatif.
 * @return     Code de sortie (wParam du WM_QUIT).
 */
int ui_run(AppContext *ctx);

/**
 * @brief Libère toutes les ressources UI.
 *
 * @param ctx  Contexte applicatif.
 */
void ui_cleanup(AppContext *ctx);


/* ============================================================================
 * API PUBLIQUE — MISE À JOUR DE L'AFFICHAGE
 * ============================================================================ */

/**
 * @brief Synchronise Scintilla avec le texte du document Core.
 *
 * Envoie le texte du gap buffer vers Scintilla.
 * À appeler après toute modification du document.
 *
 * @param ctx  Contexte applicatif.
 */
void ui_sync_text(AppContext *ctx);

/**
 * @brief Applique les soulignements d'erreur NLP dans Scintilla.
 *
 * Traduit les NlpError (positions + types) en indicateurs Scintilla.
 *
 * @param ctx     Contexte applicatif.
 * @param result  Résultat NLP à afficher.
 */
void ui_apply_nlp_markers(AppContext *ctx, const NlpResult *result);

/**
 * @brief Met à jour le panneau de règles avec le rapport de conformité.
 *
 * @param ctx     Contexte applicatif.
 * @param report  Rapport de conformité à afficher.
 */
void ui_update_rules_panel(AppContext *ctx, const RuleReport *report);

/**
 * @brief Met à jour la barre de statut (mots, ligne, colonne, encodage).
 *
 * @param ctx     Contexte applicatif.
 * @param words   Nombre de mots.
 * @param line    Numéro de ligne courante.
 * @param col     Numéro de colonne courante.
 */
void ui_update_statusbar(AppContext *ctx, size_t words, int line, int col);


#endif /* INTELLIEDITOR_UI_H */
