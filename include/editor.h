/**
 * @file editor.h
 * @brief Contrat public du module Éditeur — Cœur métier pur (CORE)
 *
 * =============================================================================
 * RESPONSABILITÉ DE CE MODULE
 * =============================================================================
 * Ce header définit l'API publique du cœur d'édition d'IntelliEditor.
 * Il expose les opérations sur le texte (gap buffer), l'historique undo/redo,
 * et les statistiques du document.
 *
 * RÈGLES ARCHITECTURALES STRICTES :
 *   - Ce module ne connaît PAS Windows, Win32, Scintilla, ni aucune UI.
 *   - Ce module ne fait PAS de NLP ni de vérification de règles.
 *   - Toute la logique de stockage du texte passe par ce module.
 *   - Scintilla est un afficheur : le texte "vrai" vit ici, dans le gap buffer.
 *
 * APPARTIENT À LA COUCHE : CORE
 * DÉPENDANCES AUTORISÉES   : aucune dépendance externe
 * AUTEUR(S) RESPONSABLE(S) : DEV-A
 *
 * =============================================================================
 * ARCHITECTURE INTERNE
 * =============================================================================
 *
 *   ┌─────────────────────────────────────────────┐
 *   │              EditorDocument                 │
 *   │  ┌──────────────┐   ┌──────────────────┐   │
 *   │  │  GapBuffer   │   │  UndoRedoStack   │   │
 *   │  │  (texte)     │   │  (historique)    │   │
 *   │  └──────────────┘   └──────────────────┘   │
 *   │  ┌──────────────┐                           │
 *   │  │  DocStats    │                           │
 *   │  │  (stats)     │                           │
 *   │  └──────────────┘                           │
 *   └─────────────────────────────────────────────┘
 *
 * =============================================================================
 */

#ifndef INTELLIEDITOR_EDITOR_H
#define INTELLIEDITOR_EDITOR_H

#include <stddef.h>   /* size_t       */
#include <stdbool.h>  /* bool         */
#include <stdint.h>   /* uint32_t     */

/* ============================================================================
 * CONSTANTES ET LIMITES
 * ============================================================================ */

/** Taille initiale du gap buffer (en octets). Peut grandir dynamiquement. */
#define GAP_BUFFER_INITIAL_SIZE  4096

/** Taille minimale du gap. En dessous, le buffer est re-agrandi. */
#define GAP_BUFFER_MIN_GAP_SIZE   512

/** Nombre maximum d'entrées dans la pile undo/redo. */
#define UNDO_STACK_MAX_DEPTH      512


/* ============================================================================
 * TYPES ET STRUCTURES
 * ============================================================================ */

/**
 * @brief Gap Buffer — structure de données centrale de l'éditeur.
 *
 * Un gap buffer divise le texte en deux parties séparées par un "gap" (trou).
 * Les insertions/suppressions à la position du curseur sont en O(1).
 *
 * Représentation mémoire :
 *
 *   [ texte_avant_curseur | --- GAP --- | texte_après_curseur ]
 *   ^                     ^             ^                     ^
 *   buf                   gap_start     gap_end               buf + capacity
 *
 * Déplacer le curseur = déplacer le gap.
 * Insérer un caractère = écrire dans le gap, réduire gap_start.
 * Supprimer = agrandir le gap.
 */
typedef struct {
    char    *buf;        /**< Bloc mémoire contenant le texte + le gap         */
    size_t   capacity;   /**< Taille totale du bloc alloué (en octets)         */
    size_t   gap_start;  /**< Index du début du gap (= position curseur)       */
    size_t   gap_end;    /**< Index de la fin du gap (exclus)                  */
} GapBuffer;

/**
 * @brief Type d'une commande undo/redo (pattern Command).
 */
typedef enum {
    CMD_INSERT,
    CMD_DELETE,
    CMD_REPLACE,
    CMD_STYLE,
} CommandType;

/**
 * @brief Représentation d'une commande réversible.
 */
typedef struct {
    CommandType  type;
    size_t       position;
    char        *text;
    size_t       length;
    uint32_t     style;
} Command;

/**
 * @brief Pile undo/redo.
 */
typedef struct {
    Command  stack[UNDO_STACK_MAX_DEPTH];
    int      top;
    int      redo_top;
} UndoRedoStack;

/**
 * @brief Statistiques du document, mises à jour en temps réel.
 */
typedef struct {
    size_t word_count;
    size_t char_count;
    size_t line_count;
    size_t paragraph_count;
} DocStats;

/**
 * @brief Document d'édition — objet principal de ce module.
 */
typedef struct {
    GapBuffer     gap;
    UndoRedoStack history;
    DocStats      stats;
    bool          dirty;
    char         *filepath;
} EditorDocument;


/* ============================================================================
 * API PUBLIQUE — CYCLE DE VIE
 * ============================================================================ */

EditorDocument *editor_create(void);
void editor_destroy(EditorDocument *doc);


/* ============================================================================
 * API PUBLIQUE — OPÉRATIONS SUR LE TEXTE
 * ============================================================================ */

bool editor_insert(EditorDocument *doc, const char *text, size_t length);
bool editor_delete(EditorDocument *doc, size_t position, size_t count);

void editor_move_cursor(EditorDocument *doc, size_t position);

char *editor_get_text(const EditorDocument *doc);

size_t editor_get_length(const EditorDocument *doc);


/* ============================================================================
 * API PUBLIQUE — RECHERCHE ET REMPLACEMENT
 * ============================================================================ */

/**
 * @brief Recherche une chaîne de caractères dans le document.
 * 
 * @param doc        Le document.
 * @param query      La chaîne à rechercher (UTF-8).
 * @param start_pos  Position de départ de la recherche.
 * @return           Index de la première occurrence, ou SIZE_MAX si non trouvé.
 */
size_t editor_search(const EditorDocument *doc, const char *query, size_t start_pos);

/**
 * @brief Remplace une plage de texte par une nouvelle chaîne.
 * 
 * @param doc       Le document.
 * @param pos       Position du texte à remplacer.
 * @param len       Longueur du texte à supprimer.
 * @param new_text  Texte de remplacement.
 * @return true     Si le remplacement a réussi.
 */
bool editor_replace(EditorDocument *doc, size_t pos, size_t len, const char *new_text);


/* ============================================================================
 * API PUBLIQUE — UNDO / REDO
 * ============================================================================ */

bool editor_undo(EditorDocument *doc);
bool editor_redo(EditorDocument *doc);

bool editor_can_undo(const EditorDocument *doc);
bool editor_can_redo(const EditorDocument *doc);


/* ============================================================================
 * API PUBLIQUE — STATISTIQUES
 * ============================================================================ */

void editor_compute_stats(const EditorDocument *doc, DocStats *stats);

#endif /* INTELLIEDITOR_EDITOR_H */