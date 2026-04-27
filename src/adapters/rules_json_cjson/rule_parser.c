/**
 * @file rule_parser.c
 * @brief Parser JSON des fichiers de règles — ADAPTER / rules_json_cjson
 *
 * =============================================================================
 * RÔLE
 * =============================================================================
 * Transforme un fichier JSON de règles en une structure RuleSet (définie
 * dans rules.h). C'est le seul endroit où cJSON est utilisé pour les règles.
 *
 * Ce fichier appartient à la couche ADAPTERS :
 *   - Il dépend de cJSON (lib externe)
 *   - Il dépend de storage.h (lecture de fichiers)
 *   - Il NE contient AUCUNE logique métier (les checkers sont dans core/rules/)
 *
 * =============================================================================
 * NOTES ARCHITECTURE (DEV-D — Ehud)
 * =============================================================================
 * - Le Core ne doit jamais voir cJSON → ce fichier fait la conversion.
 * - Le champ "parameter" peut être :
 *        * une chaîne simple
 *        * un objet JSON
 *        * un tableau JSON
 *   → Dans les deux derniers cas, on doit sérialiser avec
 *     cJSON_PrintUnformatted() avant de stocker dans Rule.parameter.
 *
 * - Ce fichier est volontairement un STUB en Phase 1.
 *   L’implémentation complète viendra en Phase 2.
 *
 * =============================================================================
 */

#include "../../../include/rules.h"
#include "../../../include/storage.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*
 * TODO [DEV-D / TODO-PARSER-001] :
 *   Décommenter l'include cJSON quand la bibliothèque est disponible.
 *   Pour l'instant, le parser est un stub qui charge zéro règles.
 */
/* #include <cjson/cJSON.h> */

/* ============================================================================
 *  FONCTIONS INTERNES — SQUELETTE (DEV-D)
 * ============================================================================
 * Ces fonctions internes ne sont pas encore implémentées.
 * Elles servent à structurer le travail pour Phase 2.
 */

/**
 * @brief Parse un objet JSON représentant une règle et remplit une structure Rule.
 *
 * TODO-PARSER-003-A : extraire id, description, category
 * TODO-PARSER-003-B : extraire check_type + severity
 * TODO-PARSER-003-C : extraire parameter (string / objet / tableau)
 * TODO-PARSER-003-D : extraire flags (case_insensitive)
 * TODO-PARSER-003-E : extraire target_section
 */
static void parse_rule_object(/* cJSON *rule_obj, */ Rule *out_rule) {
    // Stub Phase 1 : ne rien faire
    (void)out_rule;
}

/* ============================================================================
 *  PARSEURS DE CHAMPS SIMPLES
 * ============================================================================
 */

/**
 * @brief Convertit la chaîne "check_type" du JSON en enum CheckType.
 *
 * TODO [DEV-D / TODO-PARSER-002] :
 *   Ajouter tous les types manquants si nécessaire.
 */
static CheckType parse_check_type(const char *str) {
    if (!str) return CHECK_UNKNOWN;
    if (strcmp(str, "section_exists")  == 0) return CHECK_SECTION_EXISTS;
    if (strcmp(str, "section_order")   == 0) return CHECK_SECTION_ORDER;
    if (strcmp(str, "word_count_min")  == 0) return CHECK_WORD_COUNT_MIN;
    if (strcmp(str, "word_count_max")  == 0) return CHECK_WORD_COUNT_MAX;
    if (strcmp(str, "regex_forbidden") == 0) return CHECK_REGEX_FORBIDDEN;
    if (strcmp(str, "regex_required")  == 0) return CHECK_REGEX_REQUIRED;
    if (strcmp(str, "heading_format")  == 0) return CHECK_HEADING_FORMAT;
    if (strcmp(str, "citation_present")== 0) return CHECK_CITATION_PRESENT;
    if (strcmp(str, "llm_semantic")    == 0) return CHECK_LLM_SEMANTIC;

    fprintf(stderr, "[WARN] Type de check inconnu: '%s'\n", str);
    return CHECK_UNKNOWN;
}

/**
 * @brief Convertit la chaîne "severity" du JSON en enum Severity.
 */
static Severity parse_severity(const char *str) {
    if (!str) return SEVERITY_WARNING;
    if (strcmp(str, "error")   == 0) return SEVERITY_ERROR;
    if (strcmp(str, "warning") == 0) return SEVERITY_WARNING;
    if (strcmp(str, "info")    == 0) return SEVERITY_INFO;
    return SEVERITY_WARNING;
}

/* ============================================================================
 *  FONCTION PRINCIPALE — ruleset_load_from_file()
 * ============================================================================
 */

RuleSet *ruleset_load_from_file(const char *filepath) {

    /*
     * =============================================================================
     * TODO [DEV-D / TODO-PARSER-003] : IMPLÉMENTATION PRINCIPALE
     * =============================================================================
     *
     * Étapes prévues pour Phase 2 :
     *
     *   1. Lire le fichier avec storage_read_file()
     *   2. Parser le JSON avec cJSON_Parse()
     *   3. Extraire les métadonnées depuis l'objet "meta"
     *   4. Parcourir le tableau "rules" et remplir set->rules[]
     *   5. Pour chaque règle :
     *        a. Lire id, description, category, check_type, severity
     *        b. Lire "parameter" (string / objet / tableau)
     *           → sérialiser en JSON string si objet/tableau
     *        c. Lire flags (case_insensitive)
     *        d. Lire target_section
     *   6. Libérer le JSON avec cJSON_Delete()
     *   7. Retourner le RuleSet
     *
     * Pour l’instant (Phase 1) :
     *   → retourner un RuleSet vide pour permettre à l’UI et aux tests d’avancer.
     * =============================================================================
     */

    RuleSet *set = calloc(1, sizeof(RuleSet));
    if (!set) {
        fprintf(stderr, "[ERROR] Allocation RuleSet échouée\n");
        return NULL;
    }

    // Stub : aucun parsing pour l’instant
    set->rule_count = 0;

    return set;
}
