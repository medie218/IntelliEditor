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
 * DÉPENDANCE : cJSON (https://github.com/DaveGamble/cJSON)
 * Compilation : linker avec -lcjson
 *
 * RESPONSABLE : DEV-D
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

/**
 * @brief Convertit la chaîne "check_type" du JSON en enum CheckType.
 *
 * TODO [DEV-D / TODO-PARSER-002] :
 *   Ajouter tous les types manquants.
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

/**
 * @brief Charge un fichier JSON de règles et retourne un RuleSet.
 *
 * TODO [DEV-D / TODO-PARSER-003] : IMPLÉMENTATION PRINCIPALE
 *
 * Étapes à suivre :
 *   1. Lire le fichier avec storage_read_file()
 *   2. Parser le JSON avec cJSON_Parse()
 *   3. Extraire les métadonnées depuis l'objet "meta"
 *   4. Parcourir le tableau "rules" et remplir set->rules[]
 *   5. Pour chaque règle :
 *      a. Lire "id", "description", "category", "check_type", "severity"
 *      b. Lire "parameter" (peut être string, objet ou tableau)
 *         → sérialiser en JSON string si c'est un objet/tableau
 *      c. Lire "flags" si présent (case_insensitive)
 *      d. Lire "target_section" si présent
 *   6. Libérer le JSON avec cJSON_Delete()
 *   7. Retourner le RuleSet
 *
 * @param filepath  Chemin vers le fichier .json (UTF-8).
 * @return          RuleSet alloué, ou NULL si erreur.
 */
RuleSet *ruleset_load_from_file(const char *filepath) {
    if (!filepath) return NULL;

    printf("[INFO] Chargement des règles depuis: %s\n", filepath);

    /* Étape 1 : lire le fichier */
    size_t file_len = 0;
    char *json_text = storage_read_file(filepath, &file_len);
    if (!json_text) {
        fprintf(stderr, "[ERROR] Impossible de lire le fichier: %s\n", filepath);
        return NULL;
    }

    RuleSet *set = ruleset_create();
    if (!set) {
        free(json_text);
        return NULL;
    }

    /*
     * TODO [DEV-D / TODO-PARSER-003] : Parser le JSON ici.
     *
     * Code à écrire avec cJSON :
     *
     *   cJSON *root = cJSON_Parse(json_text);
     *   if (!root) {
     *       fprintf(stderr, "Erreur JSON: %s\n", cJSON_GetErrorPtr());
     *       goto cleanup;
     *   }
     *
     *   // Métadonnées
     *   cJSON *meta = cJSON_GetObjectItem(root, "meta");
     *   if (meta) {
     *       const char *doctype = cJSON_GetStringValue(
     *           cJSON_GetObjectItem(meta, "document_type"));
     *       if (doctype)
     *           strncpy(set->meta.document_type, doctype, 63);
     *       // ... idem pour version, author, description
     *   }
     *
     *   // Règles
     *   cJSON *rules_arr = cJSON_GetObjectItem(root, "rules");
     *   cJSON *rule_json = NULL;
     *   cJSON_ArrayForEach(rule_json, rules_arr) {
     *       if (set->rule_count >= RULES_MAX_RULES) break;
     *       Rule *r = &set->rules[set->rule_count];
     *
     *       const char *id = cJSON_GetStringValue(
     *           cJSON_GetObjectItem(rule_json, "id"));
     *       if (id) strncpy(r->id, id, RULES_MAX_ID_LEN - 1);
     *
     *       // ... autres champs ...
     *
     *       const char *check = cJSON_GetStringValue(
     *           cJSON_GetObjectItem(rule_json, "check_type"));
     *       r->check_type = parse_check_type(check);
     *
     *       set->rule_count++;
     *   }
     *
     *   cJSON_Delete(root);
     */

    /* STUB : pour l'instant, on simule 1 règle de test */
    fprintf(stderr, "[STUB] ruleset_load_from_file: parser JSON non implémenté\n");
    fprintf(stderr, "[STUB] Fichier lu (%zu octets), mais non parsé.\n", file_len);

    /* Règle de démo pour que le projet compile et tourne */
    strncpy(set->meta.document_type, "Test", 63);
    strncpy(set->meta.version, "0.0", 15);
    strncpy(set->rules[0].id, "STUB-01", RULES_MAX_ID_LEN - 1);
    strncpy(set->rules[0].description, "Règle de démonstration (stub)", RULES_MAX_DESC_LEN - 1);
    set->rules[0].check_type = CHECK_SECTION_EXISTS;
    set->rules[0].severity   = SEVERITY_WARNING;
    strncpy(set->rules[0].parameter, "Introduction", RULES_MAX_PARAM_LEN - 1);
    set->rule_count = 1;

    free(json_text);
    return set;
}
