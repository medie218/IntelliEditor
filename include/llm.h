/**
 * @file llm.h
 * @brief Contrat public du module LLM — interface asynchrone — ADAPTER
 *
 * =============================================================================
 * RESPONSABILITÉ DE CE MODULE
 * =============================================================================
 * Ce header expose l'interface asynchrone vers le LLM local (llama.cpp).
 *
 * PRINCIPE CLÉ : L'UI ne doit JAMAIS être bloquée par le LLM.
 * Toutes les requêtes LLM sont :
 *   1. Soumises dans une file (queue thread-safe).
 *   2. Traitées par un thread worker dédié.
 *   3. Retournées via un callback ou un polling.
 *
 * FLUX ASYNCHRONE :
 *
 *   UI/Core                Thread LLM Worker
 *      │                         │
 *      │── llm_submit_request() ──► [File de requêtes]
 *      │                         │
 *      │   (continue sans bloquer)│
 *      │                         │── llama_eval() (CPU, lent)
 *      │                         │
 *      │◄── callback(response) ──│
 *      │
 *      └── Mise à jour UI
 *
 * APPARTIENT À LA COUCHE : ADAPTER (wrapping de llama.cpp)
 * AUTEUR(S) RESPONSABLE(S) : DEV-C
 *
 * =============================================================================
 */

#ifndef INTELLIEDITOR_LLM_H
#define INTELLIEDITOR_LLM_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/* ============================================================================
 * CONSTANTES
 * ============================================================================ */

#define LLM_MAX_PROMPT_LEN    4096  /**< Longueur max d'un prompt (octets)    */
#define LLM_MAX_RESPONSE_LEN  2048  /**< Longueur max d'une réponse           */
#define LLM_REQUEST_QUEUE_SIZE  32  /**< Taille de la file de requêtes        */
#define LLM_TIMEOUT_MS        30000 /**< Timeout par requête (30 secondes)    */
#define LLM_MAX_TOKENS         1024 /**< Nombre max de tokens générés          */


/* ============================================================================
 * TYPES
 * ============================================================================ */

/**
 * @brief Identifiant unique d'une requête LLM.
 *
 * Retourné par llm_submit_request(), utilisé pour suivre la réponse.
 */
typedef uint32_t LlmRequestId;

/**
 * @brief Type de tâche demandée au LLM.
 */
typedef enum {
    LLM_TASK_GRAMMAR_CHECK,    /**< Vérification grammaticale d'un texte      */
    LLM_TASK_REFORMULATE,      /**< Reformulation stylistique d'une phrase     */
    LLM_TASK_SEMANTIC_CHECK,   /**< Vérification sémantique (pour les règles)  */
    LLM_TASK_SUMMARIZE,        /**< Résumé d'une section (P3)                 */
    LLM_TASK_CUSTOM,           /**< Prompt libre                              */
} LlmTaskType;

/**
 * @brief Statut d'une requête LLM.
 */
typedef enum {
    LLM_STATUS_QUEUED,      /**< En attente dans la file                      */
    LLM_STATUS_PROCESSING,  /**< En cours de traitement                       */
    LLM_STATUS_DONE,        /**< Terminée — réponse disponible                */
    LLM_STATUS_TIMEOUT,     /**< Timeout dépassé                              */
    LLM_RULE_STATUS_ERROR,       /**< Erreur interne                               */
} LlmRequestStatus;

/**
 * @brief Réponse retournée par le LLM.
 */
typedef struct {
    LlmRequestId    id;                              /**< ID de la requête    */
    LlmRequestStatus status;                         /**< Statut              */
    char            text[LLM_MAX_RESPONSE_LEN];      /**< Texte de la réponse */
    float           confidence;                      /**< Score (si dispo)    */
    uint32_t        tokens_used;                     /**< Tokens consommés    */
} LlmResponse;

/**
 * @brief Signature du callback appelé quand une requête LLM est terminée.
 *
 * Ce callback est appelé depuis le thread LLM worker.
 * L'implémentation doit être thread-safe (ex : PostMessage vers l'UI Win32).
 *
 * @param response  La réponse produite.
 * @param userdata  Données utilisateur passées lors de llm_submit_request().
 */
typedef void (*LlmCallback)(const LlmResponse *response, void *userdata);

/**
 * @brief Handle opaque vers le moteur LLM.
 *
 * Créé par llm_create(), détruit par llm_destroy().
 * Contient le contexte llama.cpp et la file de requêtes.
 */
typedef struct LlmEngine LlmEngine;


/* ============================================================================
 * API PUBLIQUE — CYCLE DE VIE
 * ============================================================================ */

/**
 * @brief Initialise le moteur LLM et charge le modèle GGUF.
 *
 * Cette opération peut prendre 5 à 15 secondes selon le modèle.
 * Elle doit être appelée dans un thread séparé, avec un indicateur de
 * chargement dans l'UI.
 *
 * @param model_path  Chemin vers le fichier .gguf (UTF-8).
 * @param n_threads   Nombre de threads CPU à utiliser.
 * @param n_ctx       Taille du contexte KV (ex : 4096).
 * @return            Handle vers le moteur, ou NULL si erreur.
 */
LlmEngine *llm_create(const char *model_path, int n_threads, int n_ctx);

/**
 * @brief Démarre le thread worker LLM.
 *
 * Doit être appelé après llm_create() et avant llm_submit_request().
 *
 * @param engine  Handle LLM (non NULL).
 * @return true   Si le thread a démarré.
 */
bool llm_start_worker(LlmEngine *engine);

/**
 * @brief Arrête le thread worker et libère toutes les ressources.
 *
 * Les requêtes en attente sont annulées.
 *
 * @param engine  Handle LLM (NULL = no-op).
 */
void llm_destroy(LlmEngine *engine);

/**
 * @brief Vérifie si le moteur LLM est prêt à recevoir des requêtes.
 *
 * @param engine  Handle LLM.
 * @return true   Si le modèle est chargé et le worker actif.
 */
bool llm_is_ready(const LlmEngine *engine);


/* ============================================================================
 * API PUBLIQUE — SOUMISSION DE REQUÊTES
 * ============================================================================ */

/**
 * @brief Soumet une requête LLM de façon asynchrone.
 *
 * La requête est ajoutée à la file et sera traitée par le thread worker.
 * Cette fonction retourne IMMÉDIATEMENT sans bloquer l'UI.
 *
 * @param engine    Handle LLM (non NULL).
 * @param task      Type de tâche.
 * @param prompt    Texte du prompt (UTF-8, null-terminé).
 * @param callback  Fonction appelée quand la réponse est prête (peut être NULL).
 * @param userdata  Données utilisateur passées au callback.
 * @return          ID de la requête (> 0), ou 0 si la file est pleine.
 */
LlmRequestId llm_submit_request(LlmEngine   *engine,
                                 LlmTaskType  task,
                                 const char  *prompt,
                                 LlmCallback  callback,
                                 void        *userdata);

/**
 * @brief Annule une requête en attente dans la file.
 *
 * Si la requête est déjà en cours de traitement, elle ne peut pas être annulée.
 *
 * @param engine  Handle LLM.
 * @param id      ID de la requête à annuler.
 * @return true   Si la requête a été annulée avec succès.
 */
bool llm_cancel_request(LlmEngine *engine, LlmRequestId id);

/**
 * @brief Retourne le nombre de requêtes en attente dans la file.
 *
 * @param engine  Handle LLM.
 * @return        Nombre de requêtes en file.
 */
size_t llm_queue_size(const LlmEngine *engine);


/* ============================================================================
 * TEMPLATES DE PROMPTS (définis dans adapters/llm_llama_cpp/prompts.c)
 * ============================================================================ */

/**
 * @brief Construit un prompt pour la vérification grammaticale.
 *
 * @param text      Texte à vérifier.
 * @param out_buf   Buffer de sortie.
 * @param buf_size  Taille du buffer.
 * @return          Longueur du prompt généré.
 */
size_t llm_prompt_grammar_check(const char *text, char *out_buf, size_t buf_size);

/**
 * @brief Construit un prompt pour la reformulation stylistique.
 *
 * @param text      Phrase à reformuler.
 * @param out_buf   Buffer de sortie.
 * @param buf_size  Taille du buffer.
 * @return          Longueur du prompt généré.
 */
size_t llm_prompt_reformulate(const char *text, char *out_buf, size_t buf_size);

/**
 * @brief Construit un prompt pour une vérification sémantique (règle LLM).
 *
 * @param question  La question sémantique (ex : "La problématique est-elle posée ?").
 * @param section   Le texte de la section cible.
 * @param out_buf   Buffer de sortie.
 * @param buf_size  Taille du buffer.
 * @return          Longueur du prompt généré.
 */
size_t llm_prompt_semantic_check(const char *question,
                                 const char *section,
                                 char       *out_buf,
                                 size_t      buf_size);


#endif /* INTELLIEDITOR_LLM_H */
