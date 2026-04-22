/**
 * @file rules.h
 * @brief Contrat public du moteur de règles métier — CORE
 *
 * =============================================================================
 * RESPONSABILITÉ DE CE MODULE
 * =============================================================================
 * Ce module définit les structures et fonctions pour :
 *   1. Représenter un ensemble de règles chargées depuis un fichier JSON.
 *   2. Évaluer ces règles sur un texte donné.
 *   3. Produire un rapport de conformité structuré.
 *
 * RÈGLES ARCHITECTURALES :
 *   - Ce module est PURE CORE : aucune dépendance vers Windows, UI ou fichiers.
 *   - Il reçoit du texte en entrée, il produit un rapport en sortie.
 *   - Le parsing JSON est délégué à l'adapter rules_json_cjson.
 *   - La vérification regex est déléguée à l'adapter regex_pcre2.
 *   - La vérification LLM est déléguée à l'adapter llm_llama_cpp.
 *
 * FLUX DE DONNÉES :
 *
 *   fichier.json
 *        │
 *        ▼ (adapter: rules_json_cjson)
 *      RuleSet
 *        │
 *        ▼ (rules_evaluate)
 *     texte du document
 *        │
 *        ▼
 *      RuleReport  ──► UI (panneau de conformité)
 *
 * APPARTIENT À LA COUCHE : CORE
 * AUTEUR(S) RESPONSABLE(S) : DEV-D
 *
 * =============================================================================
 */

#ifndef INTELLIEDITOR_RULES_H
#define INTELLIEDITOR_RULES_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/* ============================================================================
 * CONSTANTES
 * ============================================================================ */

#define RULES_MAX_RULES          64    /**< Nombre max de règles par fichier   */
#define RULES_MAX_ID_LEN         16    /**< Longueur max d'un ID de règle      */
#define RULES_MAX_DESC_LEN      256    /**< Longueur max d'une description      */
#define RULES_MAX_PARAM_LEN     512    /**< Longueur max d'un paramètre         */
#define RULES_MAX_SECTIONS       32    /**< Nombre max de sections dans section_order */


/* ============================================================================
 * TYPES ÉNUMÉRÉS
 * ============================================================================ */

/**
 * @brief Type de vérification à effectuer pour une règle.
 *
 * Chaque type correspond à un vérificateur (checker) dans rules/checkers/.
 */
typedef enum {
    CHECK_SECTION_EXISTS,    /**< Vérifie qu'un titre de section existe          */
    CHECK_SECTION_ORDER,     /**< Vérifie l'ordre des sections                   */
    CHECK_WORD_COUNT_MIN,    /**< Vérifie un nombre minimum de mots              */
    CHECK_WORD_COUNT_MAX,    /**< Vérifie un nombre maximum de mots              */
    CHECK_REGEX_FORBIDDEN,   /**< Vérifie qu'une regex n'est PAS trouvée         */
    CHECK_REGEX_REQUIRED,    /**< Vérifie qu'une regex EST trouvée               */
    CHECK_HEADING_FORMAT,    /**< Vérifie le format des titres                   */
    CHECK_CITATION_PRESENT,  /**< Vérifie la présence d'une bibliographie        */
    CHECK_LLM_SEMANTIC,      /**< Vérification sémantique via LLM (asynchrone)  */
    CHECK_UNKNOWN,           /**< Type inconnu / non supporté                    */
} CheckType;

/**
 * @brief Niveau de sévérité d'une règle.
 *
 * Détermine comment le résultat est affiché dans le panneau UI.
 */
typedef enum {
    SEVERITY_ERROR,    /**< ❌ Erreur bloquante — règle clairement violée       */
    SEVERITY_WARNING,  /**< ⚠️  Avertissement — partiellement respectée         */
    SEVERITY_INFO,     /**< ℹ️  Information — règle facultative                 */
} Severity;

/**
 * @brief Statut d'évaluation d'une règle.
 *
 * Résultat produit par l'évaluation de chaque règle sur le document.
 */
typedef enum {
    STATUS_PASS,       /**< ✅ Conforme — la règle est respectée                */
    STATUS_FAIL,       /**< ❌ Non conforme                                      */
    STATUS_WARNING,    /**< ⚠️  Partiellement conforme                           */
    STATUS_PENDING,    /**< 🔄 En cours — vérification LLM asynchrone           */
    STATUS_ERROR,      /**< Erreur interne lors de l'évaluation                 */
    STATUS_SKIPPED,    /**< Règle ignorée (ex : type non supporté)              */
} RuleStatus;


/* ============================================================================
 * STRUCTURES
 * ============================================================================ */

/**
 * @brief Définition d'une règle métier.
 *
 * Correspond à un objet dans le tableau "rules" du fichier JSON.
 */
typedef struct {
    char       id[RULES_MAX_ID_LEN];          /**< Ex : "R001"                 */
    char       description[RULES_MAX_DESC_LEN]; /**< Texte humain              */
    char       category[32];                  /**< Ex : "structure", "style"   */
    CheckType  check_type;                    /**< Vérificateur à utiliser     */
    Severity   severity;                      /**< Niveau de sévérité          */
    char       parameter[RULES_MAX_PARAM_LEN]; /**< Paramètre du check (JSON) */
    bool       case_insensitive;              /**< Pour regex : ignorer la casse */
    char       target_section[64];            /**< Section cible (si applicable) */
} Rule;

/**
 * @brief Métadonnées d'un fichier de règles.
 */
typedef struct {
    char document_type[64];  /**< Ex : "Mémoire de Licence"                   */
    char version[16];        /**< Ex : "1.0"                                  */
    char author[128];        /**< Auteur du fichier de règles                 */
    char description[256];   /**< Description courte du fichier               */
} RuleMeta;

/**
 * @brief Ensemble de règles chargées depuis un fichier JSON.
 *
 * Créé par l'adapter rules_json_cjson, consommé par le core rules.
 */
typedef struct {
    RuleMeta  meta;                    /**< Métadonnées du fichier             */
    Rule      rules[RULES_MAX_RULES];  /**< Tableau des règles                 */
    size_t    rule_count;              /**< Nombre de règles chargées          */
} RuleSet;

/**
 * @brief Résultat de l'évaluation d'une règle sur un document.
 */
typedef struct {
    char       rule_id[RULES_MAX_ID_LEN]; /**< ID de la règle évaluée          */
    RuleStatus status;                    /**< Résultat de l'évaluation         */
    char       message[256];              /**< Message humain (ex : "Section manquante") */
    size_t     position;                  /**< Position dans le texte (0 si N/A) */
    size_t     length;                    /**< Longueur de la zone concernée    */
} RuleResult;

/**
 * @brief Rapport de conformité complet pour un document.
 *
 * Produit par rules_evaluate() et envoyé à l'UI.
 */
typedef struct {
    RuleResult results[RULES_MAX_RULES]; /**< Un résultat par règle            */
    size_t     result_count;             /**< Nombre de résultats              */
    size_t     pass_count;               /**< Nombre de règles conformes       */
    size_t     fail_count;               /**< Nombre de règles non conformes   */
    size_t     warning_count;            /**< Nombre d'avertissements          */
    size_t     pending_count;            /**< Nombre de vérifications en attente */
} RuleReport;


/* ============================================================================
 * API PUBLIQUE — CYCLE DE VIE
 * ============================================================================ */

/**
 * @brief Crée un RuleSet vide.
 *
 * @return Pointeur alloué, ou NULL si erreur mémoire.
 */
RuleSet *ruleset_create(void);

/**
 * @brief Libère la mémoire d'un RuleSet.
 *
 * @param set  Pointeur vers le RuleSet à détruire (NULL = no-op).
 */
void ruleset_destroy(RuleSet *set);

/**
 * @brief Crée un RuleReport vide prêt à être rempli.
 *
 * @return Pointeur alloué, ou NULL si erreur mémoire.
 */
RuleReport *rulereport_create(void);

/**
 * @brief Libère la mémoire d'un RuleReport.
 *
 * @param report  Pointeur vers le rapport (NULL = no-op).
 */
void rulereport_destroy(RuleReport *report);


/* ============================================================================
 * API PUBLIQUE — ÉVALUATION
 * ============================================================================ */

/**
 * @brief Évalue toutes les règles d'un RuleSet sur un texte donné.
 *
 * C'est la fonction principale de ce module.
 * Elle appelle les vérificateurs appropriés (checkers) pour chaque règle.
 *
 * Les règles de type CHECK_LLM_SEMANTIC ne sont pas évaluées ici :
 * elles sont marquées STATUS_PENDING et déléguées au thread LLM.
 *
 * @param set   Ensemble de règles à évaluer (non NULL).
 * @param text  Texte UTF-8 du document (non NULL).
 * @param len   Longueur du texte en octets.
 * @return      Rapport de conformité alloué (à libérer avec rulereport_destroy).
 */
RuleReport *rules_evaluate(const RuleSet *set, const char *text, size_t len);

/**
 * @brief Met à jour le résultat d'une règle LLM dans un rapport existant.
 *
 * Appelée depuis le thread LLM quand une vérification sémantique
 * est terminée.
 *
 * @param report   Rapport à mettre à jour.
 * @param rule_id  ID de la règle LLM terminée.
 * @param status   Nouveau statut (PASS, FAIL, WARNING).
 * @param message  Message explicatif (peut être NULL).
 */
void rules_update_llm_result(RuleReport *report,
                              const char *rule_id,
                              RuleStatus  status,
                              const char *message);


/* ============================================================================
 * UTILITAIRES
 * ============================================================================ */

/**
 * @brief Convertit un CheckType en chaîne lisible (pour debug/logs).
 * @param type  Le type de check.
 * @return      Chaîne statique (ne pas libérer).
 */
const char *check_type_to_string(CheckType type);

/**
 * @brief Convertit un RuleStatus en chaîne lisible (pour debug/logs).
 * @param status  Le statut.
 * @return        Chaîne statique.
 */
const char *rule_status_to_string(RuleStatus status);


#endif /* INTELLIEDITOR_RULES_H */
