/**
 * @file count_checker.c
 * @brief Vérificateur de comptage de mots — CORE / rules / checkers
 *
 * =============================================================================
 * RÔLE
 * =============================================================================
 * Implémente les checks :
 *   - CHECK_WORD_COUNT_MIN  → "la section X contient-elle au moins N mots ?"
 *   - CHECK_WORD_COUNT_MAX  → "la section X contient-elle au plus N mots ?"
 *
 * Ce fichier appartient au CORE :
 *   - Aucune dépendance Windows
 *   - Aucune dépendance cJSON (le parsing JSON est dans l’adapter)
 *   - Logique métier pure, testable, déterministe
 *
 * =============================================================================
 * DEV-D (Ehud) — Notes d’architecture
 * =============================================================================
 * Ce fichier contient la logique métier des règles de comptage.
 * Il doit rester simple, lisible, et découplé de l’UI et des adapters.
 *
 * Les checkers reçoivent :
 *   - Rule* (déjà parsée depuis JSON)
 *   - text (texte complet du document)
 *   - len  (longueur du texte)
 *
 * Ils retournent un RuleResult :
 *   - status (PASS / FAIL / WARNING / SKIPPED)
 *   - message explicatif
 *   - position (offset dans le texte)
 *
 * =============================================================================
 */

#include "../../../include/rules.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* =============================================================================
 * 1. Fonction interne : count_words()
 * =============================================================================
 */

/**
 * @brief Compte les mots dans un bloc de texte (séparés par espaces/newlines).
 *
 * TODO [DEV-D / TODO-COUNT-001] :
 *   Améliorer pour gérer :
 *     - apostrophes françaises ("l'homme", "aujourd'hui")
 *     - ponctuation
 *     - UTF-8
 */
static size_t count_words(const char *text, size_t len) {
    size_t count = 0;
    bool in_word = false;

    for (size_t i = 0; i < len; i++) {
        if (isspace((unsigned char)text[i])) {
            in_word = false;
        } else if (!in_word) {
            in_word = true;
            count++;
        }
    }

    return count;
}

/* =============================================================================
 * 2. CHECK_WORD_COUNT_MIN
 * =============================================================================
 */

/**
 * @brief Checker : CHECK_WORD_COUNT_MIN
 *
 * TODO [DEV-D / TODO-COUNT-002] :
 *   - Extraire min_words et section depuis rule->parameter (JSON)
 *   - Trouver le texte de la section cible dans le document
 *   - Appeler count_words() sur ce texte
 *   - Comparer avec min_words
 *
 * EXEMPLE de parameter : {"section": "Introduction", "min_words": 300}
 */
RuleResult check_word_count_min(const Rule *rule, const char *text, size_t len) {
    RuleResult result;
    memset(&result, 0, sizeof(result));
    strncpy(result.rule_id, rule->id, RULES_MAX_ID_LEN - 1);

    /*
     * Phase 1 : STUB
     *   - On compte les mots dans tout le document
     *   - On utilise une valeur fixe (300) en attendant le parsing JSON
     */

    size_t words = count_words(text, len);
    (void)rule; /* TODO : extraire min_words et section depuis rule->parameter */

    int min_words = 300; /* TODO : lire depuis rule->parameter via cJSON */

    if ((int)words >= min_words) {
        result.status = STATUS_PASS;
        snprintf(result.message, sizeof(result.message),
                 "%zu mots (minimum requis : %d)", words, min_words);
    } else {
        result.status = STATUS_FAIL;
        snprintf(result.message, sizeof(result.message),
                 "%zu mots — il en faut au moins %d", words, min_words);
    }

    result.position = 0; /* TODO : position de la section */
    return result;
}

/* =============================================================================
 * 3. CHECK_WORD_COUNT_MAX
 * =============================================================================
 */

/**
 * @brief Checker : CHECK_WORD_COUNT_MAX
 *
 * TODO [DEV-D / TODO-COUNT-003] :
 *   - Extraire max_words et section depuis rule->parameter
 *   - Trouver la section cible
 *   - Compter les mots
 *   - Comparer avec max_words
 */
RuleResult check_word_count_max(const Rule *rule, const char *text, size_t len) {
    RuleResult result;
    memset(&result, 0, sizeof(result));
    strncpy(result.rule_id, rule->id, RULES_MAX_ID_LEN - 1);

    /*
     * Phase 1 : STUB
     *   - On compte les mots dans tout le document
     *   - On utilise une valeur fixe (250)
     */

    size_t words = count_words(text, len);
    (void)rule;

    int max_words = 250; /* TODO : lire depuis rule->parameter */

    if ((int)words <= max_words) {
        result.status = STATUS_PASS;
        snprintf(result.message, sizeof(result.message),
                 "%zu mots (maximum autorisé : %d)", words, max_words);
    } else {
        result.status = STATUS_WARNING;
        snprintf(result.message, sizeof(result.message),
                 "%zu mots — ne doit pas dépasser %d", words, max_words);
    }

    result.position = 0; /* TODO : position de la section */
    return result;
}
