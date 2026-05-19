/**
 * @file nlp.h
 * @brief Contrat public du module NLP — interfaces abstraites — CORE
 *
 * =============================================================================
 * RESPONSABILITÉ DE CE MODULE
 * =============================================================================
 * Ce header définit les INTERFACES ABSTRAITES du traitement linguistique.
 * Il représente le PORT du côté Core (architecture hexagonale).
 *
 * Le Core ne sait pas QUI fait le traitement (Hunspell ? LLM ? un stub ?).
 * Il sait seulement QUOI demander : "corrige ce texte", "suggère des mots".
 *
 * Les implémentations concrètes (Hunspell, LLM) sont dans :
 *   - adapters/hunspell_wrap/   → pour la correction orthographique
 *   - adapters/llm_llama_cpp/   → pour la grammaire et reformulation
 *
 * APPARTIENT À LA COUCHE : CORE (interfaces seulement)
 * AUTEUR(S) RESPONSABLE(S) : DEV-C
 *
 * =============================================================================
 * DEUX NIVEAUX DE CORRECTION
 * =============================================================================
 *
 *  Niveau 1 — Orthographe (Hunspell) :
 *    Synchrone, rapide, mot par mot.
 *    Appelé à chaque modification de texte.
 *
 *  Niveau 2 — Grammaire / Reformulation (LLM) :
 *    Asynchrone, lent, paragraphe par paragraphe.
 *    Appelé après 2 secondes d'inactivité ou à la demande.
 *
 * =============================================================================
 */

#ifndef INTELLIEDITOR_NLP_H
#define INTELLIEDITOR_NLP_H

#include <stddef.h>
#include <stdbool.h>

/* ============================================================================
 * CONSTANTES
 * ============================================================================ */

#define NLP_MAX_SUGGESTIONS    8    /**< Max suggestions par mot               */
#define NLP_MAX_WORD_LEN     128    /**< Longueur max d'un mot en octets       */
#define NLP_MAX_ERRORS       256    /**< Max erreurs par analyse               */


/* ============================================================================
 * TYPES
 * ============================================================================ */

/**
 * @brief Catégorie d'une erreur linguistique.
 */
typedef enum {
    NLP_ERROR_SPELLING,     /**< Faute d'orthographe (Hunspell)               */
    NLP_ERROR_GRAMMAR,      /**< Faute grammaticale (LLM)                     */
    NLP_ERROR_STYLE,        /**< Problème de style (LLM)                      */
    NLP_ERROR_PUNCTUATION,  /**< Ponctuation incorrecte (règles codées)        */
    NLP_ERROR_REPETITION,   /**< Mot répété trop proche                       */
    NLP_ERROR_ANGLICISM,    /**< Anglicisme détecté                           */
} NlpErrorType;

/**
 * @brief Suggestion de correction pour un mot ou une phrase.
 */
typedef struct {
    char   word[NLP_MAX_WORD_LEN];   /**< Suggestion de remplacement           */
    float  confidence;               /**< Score de confiance (0.0 à 1.0)       */
} NlpSuggestion;

/**
 * @brief Représentation d'une erreur détectée dans le texte.
 *
 * Contient l'erreur, sa position, et les suggestions de correction.
 * Ces informations sont utilisées par l'UI pour souligner le texte.
 */
typedef struct {
    NlpErrorType    type;                              /**< Catégorie d'erreur  */
    size_t          start;                             /**< Début (octet)       */
    size_t          length;                            /**< Longueur (octets)   */
    char            original[NLP_MAX_WORD_LEN];        /**< Texte fautif        */
    char            message[256];                      /**< Explication humaine */
    NlpSuggestion   suggestions[NLP_MAX_SUGGESTIONS];  /**< Corrections suggérées */
    size_t          suggestion_count;                  /**< Nombre de suggestions */
} NlpError;

/**
 * @brief Résultat complet d'une analyse NLP sur un texte.
 */
typedef struct {
    NlpError  errors[NLP_MAX_ERRORS]; /**< Tableau des erreurs trouvées        */
    size_t    error_count;            /**< Nombre d'erreurs                    */
    bool      is_complete;            /**< false si analyse LLM encore en cours */
} NlpResult;


/* ============================================================================
 * INTERFACE PORT — SPELL CHECKER (à implémenter par hunspell_wrap)
 * ============================================================================ */

/**
 * @brief Opaque handle vers un correcteur orthographique.
 *
 * Le Core ne connaît que ce type. L'implémentation concrète (Hunspell)
 * est cachée derrière ce pointeur.
 */
typedef struct SpellChecker SpellChecker;

/**
 * @brief Vérifie si un mot est correct orthographiquement.
 *
 * @param sc    Handle vers le correcteur (non NULL).
 * @param word  Mot UTF-8 à vérifier (null-terminé).
 * @return true  Si le mot est dans le dictionnaire.
 * @return false Si le mot est inconnu (potentielle faute).
 */
bool spellcheck_word(const SpellChecker *sc, const char *word);

/**
 * @brief Retourne les suggestions pour un mot incorrect.
 *
 * @param sc          Handle vers le correcteur.
 * @param word        Mot incorrect.
 * @param suggestions Tableau de sortie (NLP_MAX_SUGGESTIONS max).
 * @param count       Pointeur vers le nombre de suggestions retournées.
 */
void spellcheck_suggest(const SpellChecker *sc,
                        const char         *word,
                        NlpSuggestion       suggestions[NLP_MAX_SUGGESTIONS],
                        size_t             *count);

/**
 * @brief Analyse un texte complet et retourne toutes les fautes orthographiques.
 *
 * @param sc    Handle vers le correcteur.
 * @param text  Texte UTF-8 complet.
 * @param len   Longueur en octets.
 * @param out   Résultat à remplir (error_count, errors[]).
 */
void spellcheck_analyze(const SpellChecker *sc,
                        const char         *text,
                        size_t              len,
                        NlpResult          *out);


/* ============================================================================
 * INTERFACE PORT — NLP ENGINE (pipeline complet)
 * ============================================================================ */

/**
 * @brief Opaque handle vers le moteur NLP complet.
 *
 * Regroupe Hunspell + pipeline de règles de ponctuation/répétitions.
 * Les analyses LLM sont séparées (asynchrones, voir llm.h).
 */
typedef struct NlpEngine NlpEngine;
/**
 * @brief Implémentation concrète du moteur NLP.
 * Regroupe Hunspell + pipeline d'analyse.
 */
struct NlpEngine {
    SpellChecker *spell_checker; /**< Handle Hunspell (peut être NULL) */
};

/**
 * @brief Lance une analyse NLP complète sur un texte.
 *
 * Cette fonction est SYNCHRONE. Elle effectue :
 *  - la détection des fautes orthographiques (Hunspell),
 *  - la vérification de la ponctuation française,
 *  - la détection des répétitions,
 *  - la détection des anglicismes.
 *
 * Elle ne fait PAS d'analyse LLM (celle-ci est asynchrone).
 *
 * @param engine  Handle vers le moteur NLP.
 * @param text    Texte UTF-8 à analyser.
 * @param len     Longueur en octets.
 * @return        Résultat alloué (à libérer avec nlp_result_destroy).
 */
NlpResult *nlp_analyze(NlpEngine *engine, const char *text, size_t len);

/**
 * @brief Libère un résultat NLP.
 *
 * @param result  Pointeur vers le résultat (NULL = no-op).
 */
void nlp_result_destroy(NlpResult *result);


#endif /* INTELLIEDITOR_NLP_H */
