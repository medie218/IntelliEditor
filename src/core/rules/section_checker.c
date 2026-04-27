/**
 * @file section_checker.c
 * @brief Vérificateur de sections — CORE / rules / checkers
 *
 * =============================================================================
 * RÔLE
 * =============================================================================
 * Implémente les checks :
 *   - CHECK_SECTION_EXISTS   → "le document contient-il une section X ?"
 *   - CHECK_SECTION_ORDER    → "les sections sont-elles dans le bon ordre ?"
 *   - CHECK_HEADING_FORMAT   → "les titres sont-ils correctement formatés ?"
 *
 * Ce fichier appartient au CORE :
 *   - Aucune dépendance Windows
 *   - Aucune dépendance cJSON (le parsing JSON est dans l’adapter)
 *   - Logique métier pure, testable, déterministe
 *
 * =============================================================================
 * DEV-D (Ehud) — Notes d’architecture
 * =============================================================================
 * Ce fichier contient la logique métier des règles structurelles.
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
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

/* =============================================================================
 * 1. Détection simple des titres (Phase 1 — STUB)
 * =============================================================================
 */

/**
 * @brief Vérifie si une ligne est entièrement en majuscules (titre H1).
 *
 * TODO [DEV-D / TODO-SECTION-001] :
 *   Améliorer cette détection :
 *     - ignorer ponctuation
 *     - gérer UTF-8
 *     - ignorer espaces
 */
static bool is_uppercase_line(const char *line, size_t len) {
    /* STUB simple — ne gère pas l'UTF-8 correctement pour l'instant */
    for (size_t i = 0; i < len; i++) {
        if (isalpha((unsigned char)line[i]) && islower((unsigned char)line[i])) {
            return false;
        }
    }
    return len > 0;
}

/* =============================================================================
 * 2. Recherche d'une section dans le texte (Phase 1 — STUB)
 * =============================================================================
 */

/**
 * @brief Recherche un titre de section dans le texte.
 *
 * TODO [DEV-D / TODO-SECTION-002] :
 *   Implémenter une recherche robuste :
 *     - Parcourir le texte ligne par ligne
 *     - Détecter les titres (H1 = majuscules, H2 = "## Titre", etc.)
 *     - Comparer avec rule->parameter
 *     - Retourner la position du titre trouvé, ou SIZE_MAX si absent
 */
static size_t find_section(const char *text, const char *section_name) {
    /* STUB — recherche naïve, à améliorer */
    const char *pos = strstr(text, section_name);
    if (!pos) return SIZE_MAX;
    return (size_t)(pos - text);
}

/* =============================================================================
 * 3. CHECK_SECTION_EXISTS
 * =============================================================================
 */

RuleResult check_section_exists(const Rule *rule, const char *text, size_t len) {
    RuleResult result;
    memset(&result, 0, sizeof(result));
    strncpy(result.rule_id, rule->id, RULES_MAX_ID_LEN - 1);

    (void)len; /* TODO : utiliser len pour les bornes */

    /*
     * TODO [DEV-D / TODO-SECTION-003] :
     *   - Lire rule->parameter (nom de la section)
     *   - Appeler find_section()
     *   - Si trouvé : STATUS_PASS
     *   - Si absent : STATUS_FAIL
     */

    size_t pos = find_section(text, rule->parameter);

    if (pos != SIZE_MAX) {
        result.status = STATUS_PASS;
        snprintf(result.message, sizeof(result.message),
                 "Section '%s' trouvée", rule->parameter);
        result.position = pos;
    } else {
        result.status = STATUS_FAIL;
        snprintf(result.message, sizeof(result.message),
                 "Section '%s' manquante dans le document", rule->parameter);
        result.position = 0;
    }

    return result;
}

/* =============================================================================
 * 4. CHECK_SECTION_ORDER
 * =============================================================================
 */

RuleResult check_section_order(const Rule *rule, const char *text, size_t len) {
    RuleResult result;
    memset(&result, 0, sizeof(result));
    strncpy(result.rule_id, rule->id, RULES_MAX_ID_LEN - 1);

    (void)text;
    (void)len;

    /*
     * TODO [DEV-D / TODO-SECTION-004] :
     *   - Parser rule->parameter comme un tableau JSON (via cJSON)
     *   - Trouver la position de chaque section dans le texte
     *   - Vérifier que les positions sont croissantes
     *   - Si une section est absente, l'ignorer ou l'indiquer dans le message
     */

    result.status = STATUS_SKIPPED;
    snprintf(result.message, sizeof(result.message),
             "TODO: check_section_order non implémenté (nécessite cJSON)");

    return result;
}

/* =============================================================================
 * 5. CHECK_HEADING_FORMAT
 * =============================================================================
 */

RuleResult check_heading_format(const Rule *rule, const char *text, size_t len) {
    RuleResult result;
    memset(&result, 0, sizeof(result));
    strncpy(result.rule_id, rule->id, RULES_MAX_ID_LEN - 1);

    (void)text;
    (void)len;

    /*
     * TODO [DEV-D / TODO-SECTION-005] :
     *   - Parser rule->parameter : { "level": 1, "case": "uppercase" }
     *   - Trouver tous les titres du niveau demandé
     *   - Vérifier leur format (majuscules, title case, etc.)
     */

    result.status = STATUS_SKIPPED;
    snprintf(result.message, sizeof(result.message),
             "TODO: check_heading_format non implémenté");

    return result;
}
