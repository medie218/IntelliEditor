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
 * COMMENT DÉTECTER UNE SECTION ?
 *   Dans le texte brut, une section H1 est une ligne en MAJUSCULES.
 *   Une section H2 est une ligne commençant par "## " ou en Title Case.
 *   (Convention à adapter selon les décisions de l'équipe)
 *
 * RESPONSABLE : DEV-D
 * =============================================================================
 */

#include "../../../include/rules.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

/**
 * @brief Vérifie si une ligne est entièrement en majuscules (titre H1).
 *
 * TODO [DEV-D / TODO-SECTION-001] :
 *   Affiner cette détection : ignorer ponctuation, gérer UTF-8.
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

/**
 * @brief Recherche un titre de section dans le texte.
 *
 * TODO [DEV-D / TODO-SECTION-002] :
 *   Implémenter une recherche robuste :
 *   - Parcourir le texte ligne par ligne
 *   - Détecter les titres (H1 = majuscules, H2 = "## Titre", etc.)
 *   - Retourner la position du titre trouvé, ou SIZE_MAX si absent
 *
 * @param text         Texte complet du document.
 * @param section_name Nom de la section cherchée (ex : "Introduction").
 * @return             Position en octets, ou SIZE_MAX si non trouvé.
 */
static size_t find_section(const char *text, const char *section_name) {
    /* STUB — recherche naïve, à améliorer */
    const char *pos = strstr(text, section_name);
    if (!pos) return SIZE_MAX;
    return (size_t)(pos - text);
}

/**
 * @brief Checker : CHECK_SECTION_EXISTS
 *
 * TODO [DEV-D / TODO-SECTION-003] :
 *   - Lire rule->parameter (nom de la section)
 *   - Appeler find_section()
 *   - Si trouvé : STATUS_PASS
 *   - Si absent : STATUS_FAIL avec message explicatif
 */
RuleResult check_section_exists(const Rule *rule, const char *text, size_t len) {
    RuleResult result;
    memset(&result, 0, sizeof(result));
    strncpy(result.rule_id, rule->id, RULES_MAX_ID_LEN - 1);

    (void)len; /* TODO : utiliser len pour les bornes */

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

/**
 * @brief Checker : CHECK_SECTION_ORDER
 *
 * TODO [DEV-D / TODO-SECTION-004] :
 *   - Parser rule->parameter comme un tableau JSON (via cJSON)
 *   - Trouver la position de chaque section dans le texte
 *   - Vérifier que les positions sont croissantes
 *   - Si une section est absente, l'ignorer ou l'indiquer dans le message
 */
RuleResult check_section_order(const Rule *rule, const char *text, size_t len) {
    RuleResult result;
    memset(&result, 0, sizeof(result));
    strncpy(result.rule_id, rule->id, RULES_MAX_ID_LEN - 1);

    (void)text;
    (void)len;

    /* STUB — TODO [DEV-D / TODO-SECTION-004] */
    result.status = STATUS_SKIPPED;
    snprintf(result.message, sizeof(result.message),
             "TODO: check_section_order non implémenté (nécessite cJSON)");

    return result;
}

/**
 * @brief Checker : CHECK_HEADING_FORMAT
 *
 * TODO [DEV-D / TODO-SECTION-005] :
 *   - Parser rule->parameter : { "level": 1, "case": "uppercase" }
 *   - Trouver tous les titres du niveau demandé
 *   - Vérifier leur format (majuscules, title case, etc.)
 */
RuleResult check_heading_format(const Rule *rule, const char *text, size_t len) {
    RuleResult result;
    memset(&result, 0, sizeof(result));
    strncpy(result.rule_id, rule->id, RULES_MAX_ID_LEN - 1);

    (void)text;
    (void)len;

    /* STUB — TODO [DEV-D / TODO-SECTION-005] */
    result.status = STATUS_SKIPPED;
    snprintf(result.message, sizeof(result.message),
             "TODO: check_heading_format non implémenté");

    return result;
}
