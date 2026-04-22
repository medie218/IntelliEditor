/**
 * @file gap_buffer.c
 * @brief Implémentation du Gap Buffer — CORE / editor
 *
 * =============================================================================
 * QU'EST-CE QU'UN GAP BUFFER ?
 * =============================================================================
 * C'est la structure de données qui stocke le texte de l'éditeur.
 * Elle est utilisée par Emacs, et de nombreux éditeurs professionnels.
 *
 * Principe :
 *   Le texte est stocké dans un seul bloc mémoire, divisé en deux parties
 *   par un "gap" (un trou vide) positionné là où se trouve le curseur.
 *
 *   AVANT insertion :
 *   [ "Hello, " | _ _ _ _ _ _ | "World!" ]
 *                 ^           ^
 *               gap_start   gap_end
 *
 *   APRÈS insertion de "dear " :
 *   [ "Hello, dear " | _ _ | "World!" ]
 *
 *   L'insertion est O(1) si le curseur ne bouge pas.
 *   Déplacer le curseur = copier du texte pour déplacer le gap.
 *
 * =============================================================================
 * RESPONSABLE : DEV-A
 * =============================================================================
 */

#include "../../include/editor.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * FONCTIONS STATIQUES (privées à ce fichier)
 * ============================================================================ */

/**
 * @brief Agrandit le gap si nécessaire pour insérer 'needed' octets.
 *
 * TODO [DEV-A / TODO-GAPBUF-001] :
 *   Implémenter le redimensionnement du buffer.
 *   - Si (gap_end - gap_start) < needed, appeler realloc()
 *   - Calculer la nouvelle taille (doubler la capacité est une bonne heuristique)
 *   - Décaler la partie "après le gap" vers la fin du nouveau buffer
 *   - Mettre à jour gap_end
 *   - Retourner false si realloc() échoue
 *
 * @param gb      Gap buffer à agrandir.
 * @param needed  Nombre d'octets à insérer.
 * @return true   Si le gap est suffisant (ou a été agrandi avec succès).
 */
static bool gap_buffer_ensure_gap(GapBuffer *gb, size_t needed) {
    /* STUB — TODO [DEV-A / TODO-GAPBUF-001] */
    (void)gb;
    (void)needed;
    fprintf(stderr, "[STUB] gap_buffer_ensure_gap: pas encore implémenté\n");
    return false;
}

/**
 * @brief Déplace le gap vers la position 'pos'.
 *
 * TODO [DEV-A / TODO-GAPBUF-002] :
 *   - Si pos < gap_start : copier les octets de [pos, gap_start[ vers la fin
 *   - Si pos > gap_start : copier les octets de [gap_end, pos+gap_size[ vers gap_start
 *   - Mettre à jour gap_start et gap_end
 *
 * @param gb   Gap buffer.
 * @param pos  Nouvelle position du curseur (en octets).
 */
static void gap_buffer_move_gap(GapBuffer *gb, size_t pos) {
    /* STUB — TODO [DEV-A / TODO-GAPBUF-002] */
    (void)gb;
    (void)pos;
    fprintf(stderr, "[STUB] gap_buffer_move_gap: pas encore implémenté\n");
}


/* ============================================================================
 * API PUBLIQUE — CYCLE DE VIE DU DOCUMENT
 * ============================================================================ */

EditorDocument *editor_create(void) {
    /*
     * TODO [DEV-A / TODO-EDITOR-001] :
     *   1. Allouer un EditorDocument avec calloc()
     *   2. Allouer le buffer initial (GAP_BUFFER_INITIAL_SIZE octets)
     *   3. Initialiser gap_start = 0, gap_end = GAP_BUFFER_INITIAL_SIZE
     *   4. Initialiser la pile undo/redo (top = -1)
     *   5. Initialiser les statistiques à zéro
     *   6. dirty = false, filepath = NULL
     */

    EditorDocument *doc = calloc(1, sizeof(EditorDocument));
    if (!doc) return NULL;

    /* TODO [DEV-A / TODO-EDITOR-001] : initialiser le gap buffer ici */
    doc->gap.buf = NULL;
    doc->gap.capacity = 0;
    doc->gap.gap_start = 0;
    doc->gap.gap_end = 0;

    doc->history.top = -1;
    doc->history.redo_top = -1;
    doc->dirty = false;
    doc->filepath = NULL;

    fprintf(stderr, "[STUB] editor_create: gap buffer non alloué (TODO)\n");
    return doc;
}

void editor_destroy(EditorDocument *doc) {
    if (!doc) return;

    /*
     * TODO [DEV-A / TODO-EDITOR-002] :
     *   1. Libérer doc->gap.buf
     *   2. Parcourir la pile undo et libérer chaque Command.text
     *   3. Libérer doc->filepath si non NULL
     *   4. Libérer doc lui-même
     */

    free(doc->gap.buf);
    free(doc->filepath);

    /* TODO : libérer les textes de chaque commande dans la pile undo */
    for (int i = 0; i <= doc->history.top; i++) {
        free(doc->history.stack[i].text);
    }

    free(doc);
}


/* ============================================================================
 * API PUBLIQUE — OPÉRATIONS SUR LE TEXTE
 * ============================================================================ */

bool editor_insert(EditorDocument *doc, const char *text, size_t length) {
    /*
     * TODO [DEV-A / TODO-EDITOR-003] :
     *   1. Vérifier que doc et text ne sont pas NULL
     *   2. Si length == 0, calculer strlen(text)
     *   3. Appeler gap_buffer_ensure_gap(gb, length)
     *   4. Copier 'text' dans le gap (à partir de gap_start)
     *   5. Avancer gap_start de 'length'
     *   6. Créer une Command CMD_INSERT et la pousser dans la pile undo
     *   7. Vider la pile redo
     *   8. doc->dirty = true
     *   9. Recalculer les stats (ou les marquer comme "dirty")
     */

    (void)doc;
    (void)text;
    (void)length;
    fprintf(stderr, "[STUB] editor_insert: TODO-EDITOR-003\n");
    return false;
}

bool editor_delete(EditorDocument *doc, size_t position, size_t count) {
    /*
     * TODO [DEV-A / TODO-EDITOR-004] :
     *   1. Vérifier les bornes (position + count <= longueur du texte)
     *   2. Déplacer le gap vers 'position'
     *   3. Sauvegarder le texte supprimé (pour undo)
     *   4. Agrandir le gap de 'count' (gap_end += count pour supprimer après)
     *      OU réduire gap_start de 'count' (pour supprimer avant)
     *   5. Créer une Command CMD_DELETE et la pousher
     *   6. doc->dirty = true
     */

    (void)doc;
    (void)position;
    (void)count;
    fprintf(stderr, "[STUB] editor_delete: TODO-EDITOR-004\n");
    return false;
}

void editor_move_cursor(EditorDocument *doc, size_t position) {
    if (!doc) return;
    /* TODO [DEV-A / TODO-EDITOR-005] : appeler gap_buffer_move_gap */
    gap_buffer_move_gap(&doc->gap, position);
}

char *editor_get_text(const EditorDocument *doc) {
    /*
     * TODO [DEV-A / TODO-EDITOR-006] :
     *   1. Calculer la longueur : (gap_start) + (capacity - gap_end)
     *   2. Allouer un buffer de (longueur + 1) octets
     *   3. Copier la partie avant le gap : buf[0..gap_start]
     *   4. Copier la partie après le gap : buf[gap_end..capacity]
     *   5. Ajouter le null-terminator
     *   6. Retourner le buffer (l'appelant le libère avec free())
     */

    (void)doc;
    fprintf(stderr, "[STUB] editor_get_text: TODO-EDITOR-006\n");
    return NULL;
}

size_t editor_get_length(const EditorDocument *doc) {
    if (!doc) return 0;
    /* Longueur = taille totale moins la taille du gap */
    return doc->gap.capacity - (doc->gap.gap_end - doc->gap.gap_start);
}


/* ============================================================================
 * UNDO / REDO
 * ============================================================================ */

bool editor_undo(EditorDocument *doc) {
    /*
     * TODO [DEV-A / TODO-UNDO-001] :
     *   1. Vérifier que history.top >= 0
     *   2. Récupérer la Command au sommet de la pile
     *   3. Inverser l'opération :
     *      - CMD_INSERT → editor_delete (sans ajouter à l'historique)
     *      - CMD_DELETE → editor_insert (sans ajouter à l'historique)
     *   4. Déplacer la commande dans la pile redo
     *   5. history.top--
     */

    if (!doc || doc->history.top < 0) return false;
    fprintf(stderr, "[STUB] editor_undo: TODO-UNDO-001\n");
    return false;
}

bool editor_redo(EditorDocument *doc) {
    /* TODO [DEV-A / TODO-UNDO-002] : symétrique de editor_undo */
    if (!doc || doc->history.redo_top < 0) return false;
    fprintf(stderr, "[STUB] editor_redo: TODO-UNDO-002\n");
    return false;
}

bool editor_can_undo(const EditorDocument *doc) {
    return doc && doc->history.top >= 0;
}

bool editor_can_redo(const EditorDocument *doc) {
    return doc && doc->history.redo_top >= 0;
}


/* ============================================================================
 * STATISTIQUES
 * ============================================================================ */

void editor_compute_stats(const EditorDocument *doc, DocStats *stats) {
    /*
     * TODO [DEV-A / TODO-STATS-001] :
     *   1. Récupérer le texte avec editor_get_text()
     *   2. Compter les mots (séparés par espaces, tabulations, ponctuation)
     *   3. Compter les lignes (occurrences de '\n' + 1)
     *   4. Compter les paragraphes (séquences de '\n\n')
     *   5. Compter les caractères Unicode (pas les octets — voir encoding.h)
     *   6. Libérer le texte obtenu
     *   7. Remplir le pointeur 'stats'
     *
     * ASTUCE : une approche simple est de parcourir le texte une seule fois
     * et incrémenter les compteurs selon les transitions (espace/non-espace).
     */

    if (!stats) return;
    stats->word_count = 0;
    stats->char_count = 0;
    stats->line_count = 0;
    stats->paragraph_count = 0;
    fprintf(stderr, "[STUB] editor_compute_stats: TODO-STATS-001\n");
}
