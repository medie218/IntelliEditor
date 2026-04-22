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
 *
 * Chaque action de l'utilisateur est encapsulée dans une Command.
 * "Annuler" = appeler undo_fn ; "Rétablir" = appeler redo_fn.
 */
typedef enum {
    CMD_INSERT,    /**< Insertion de texte           */
    CMD_DELETE,    /**< Suppression de texte         */
    CMD_REPLACE,   /**< Remplacement (insert+delete) */
    CMD_STYLE,     /**< Changement de style inline   */
} CommandType;

/**
 * @brief Représentation d'une commande réversible.
 *
 * Stocke toutes les informations nécessaires pour annuler ou rétablir
 * une action utilisateur.
 */
typedef struct {
    CommandType  type;      /**< Nature de la commande                        */
    size_t       position;  /**< Position dans le buffer au moment de l'acte  */
    char        *text;      /**< Texte inséré ou supprimé (copie, null-term.)  */
    size_t       length;    /**< Longueur du texte en octets                  */
    uint32_t     style;     /**< Style appliqué (pour CMD_STYLE)               */
} Command;

/**
 * @brief Pile undo/redo.
 *
 * Implémentée comme deux piles : une pour "annuler", une pour "rétablir".
 */
typedef struct {
    Command  stack[UNDO_STACK_MAX_DEPTH];  /**< Tableau statique des commandes */
    int      top;                          /**< Index du sommet de la pile      */
    int      redo_top;                     /**< Index du sommet redo            */
} UndoRedoStack;

/**
 * @brief Statistiques du document, mises à jour en temps réel.
 */
typedef struct {
    size_t word_count;       /**< Nombre de mots (séparés par espaces/ponct.)  */
    size_t char_count;       /**< Nombre de caractères Unicode (pas d'octets)  */
    size_t line_count;       /**< Nombre de lignes                             */
    size_t paragraph_count;  /**< Nombre de paragraphes (blocs séparés par \n\n) */
} DocStats;

/**
 * @brief Document d'édition — objet principal de ce module.
 *
 * Un EditorDocument regroupe :
 *   - le gap buffer (stockage du texte),
 *   - la pile undo/redo (historique),
 *   - les statistiques (compteurs).
 *
 * Il est créé par editor_create() et détruit par editor_destroy().
 * C'est le seul objet que les adapters reçoivent (via un pointeur opaque).
 */
typedef struct {
    GapBuffer     gap;    /**< Buffer de stockage du texte                    */
    UndoRedoStack history;/**< Historique des actions                         */
    DocStats      stats;  /**< Statistiques calculées                         */
    bool          dirty;  /**< Vrai si le document a des modifications non sauvées */
    char         *filepath; /**< Chemin du fichier (NULL si nouveau document) */
} EditorDocument;


/* ============================================================================
 * API PUBLIQUE — CYCLE DE VIE
 * ============================================================================ */

/**
 * @brief Crée un nouveau document vide.
 *
 * Alloue et initialise un EditorDocument avec un gap buffer vide.
 * L'appelant est responsable d'appeler editor_destroy() pour libérer la mémoire.
 *
 * @return Pointeur vers le nouveau document, ou NULL si l'allocation échoue.
 *
 * @example
 *   EditorDocument *doc = editor_create();
 *   if (!doc) { fprintf(stderr, "Mémoire insuffisante\n"); return 1; }
 */
EditorDocument *editor_create(void);

/**
 * @brief Détruit un document et libère toute la mémoire associée.
 *
 * Libère le gap buffer, les commandes dans la pile undo/redo,
 * et la structure elle-même.
 *
 * @param doc  Pointeur vers le document à détruire. Si NULL, ne fait rien.
 */
void editor_destroy(EditorDocument *doc);


/* ============================================================================
 * API PUBLIQUE — OPÉRATIONS SUR LE TEXTE
 * ============================================================================ */

/**
 * @brief Insère du texte UTF-8 à la position courante du curseur.
 *
 * L'insertion déplace le gap et met à jour les statistiques.
 * Une commande CMD_INSERT est poussée dans l'historique undo/redo.
 *
 * @param doc     Document cible (non NULL).
 * @param text    Texte UTF-8 à insérer (null-terminé).
 * @param length  Nombre d'octets à insérer (strlen si 0).
 * @return true   Si l'insertion a réussi.
 * @return false  Si la mémoire est insuffisante.
 */
bool editor_insert(EditorDocument *doc, const char *text, size_t length);

/**
 * @brief Supprime 'count' caractères à partir de 'position'.
 *
 * @param doc       Document cible (non NULL).
 * @param position  Position du premier caractère à supprimer.
 * @param count     Nombre de caractères à supprimer.
 * @return true     Suppression réussie.
 * @return false    Position ou count invalide.
 */
bool editor_delete(EditorDocument *doc, size_t position, size_t count);

/**
 * @brief Déplace le curseur (gap) à la position donnée.
 *
 * Cette opération est O(n) dans le pire cas (déplacement du gap).
 * Grouper les opérations proches du curseur pour minimiser les déplacements.
 *
 * @param doc       Document cible.
 * @param position  Nouvelle position du curseur (en octets depuis le début).
 */
void editor_move_cursor(EditorDocument *doc, size_t position);

/**
 * @brief Retourne le contenu textuel complet du document (sans le gap).
 *
 * Alloue une nouvelle chaîne null-terminée contenant tout le texte.
 * L'appelant doit libérer la mémoire avec free().
 *
 * @param doc  Document source (non NULL).
 * @return     Pointeur vers le texte (à libérer), ou NULL si erreur.
 */
char *editor_get_text(const EditorDocument *doc);

/**
 * @brief Retourne la longueur totale du texte (en octets, sans le gap).
 *
 * @param doc  Document source (non NULL).
 * @return     Longueur en octets.
 */
size_t editor_get_length(const EditorDocument *doc);


/* ============================================================================
 * API PUBLIQUE — UNDO / REDO
 * ============================================================================ */

/**
 * @brief Annule la dernière action (undo).
 *
 * Dépile la dernière Command de l'historique et l'inverse.
 * Si la pile est vide, ne fait rien.
 *
 * @param doc  Document cible.
 * @return true  Si une action a été annulée.
 * @return false Si la pile est vide.
 */
bool editor_undo(EditorDocument *doc);

/**
 * @brief Rétablit la dernière action annulée (redo).
 *
 * @param doc  Document cible.
 * @return true  Si une action a été rétablie.
 * @return false Si la pile redo est vide.
 */
bool editor_redo(EditorDocument *doc);

/**
 * @brief Vérifie si une action peut être annulée.
 * @param doc  Document cible.
 * @return true si undo est disponible.
 */
bool editor_can_undo(const EditorDocument *doc);

/**
 * @brief Vérifie si une action peut être rétablie.
 * @param doc  Document cible.
 * @return true si redo est disponible.
 */
bool editor_can_redo(const EditorDocument *doc);


/* ============================================================================
 * API PUBLIQUE — STATISTIQUES
 * ============================================================================ */

/**
 * @brief Recalcule et retourne les statistiques du document.
 *
 * Cette fonction parcourt le texte complet pour compter mots, lignes, etc.
 * Elle est coûteuse (O(n)) et ne doit pas être appelée à chaque frappe.
 * L'UI appelle cette fonction après 2 secondes d'inactivité.
 *
 * @param doc    Document source (non NULL).
 * @param stats  Pointeur de sortie pour les statistiques calculées.
 */
void editor_compute_stats(const EditorDocument *doc, DocStats *stats);


#endif /* INTELLIEDITOR_EDITOR_H */
