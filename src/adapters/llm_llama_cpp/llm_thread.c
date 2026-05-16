/**
 * @file llm_thread.c
 * @brief Thread worker LLM asynchrone — ADAPTER / llm_llama_cpp
 *
 * =============================================================================
 * RÔLE
 * =============================================================================
 * Ce fichier gère :
 *   1. La file de requêtes LLM (thread-safe avec mutex + condition variable)
 *   2. Le thread worker qui traite les requêtes séquentiellement
 *   3. L'appel au moteur llama.cpp pour l'inférence
 *   4. Le dispatch des réponses via callback
 *
 * PRINCIPE ASYNCHRONE :
 *
 *   Thread UI (principal)         Thread LLM Worker
 *   ──────────────────            ─────────────────
 *   llm_submit_request()    ───►  [attend dans condvar_wait()]
 *          │                              │
 *          │  (push dans la file)         │ (réveillé par condvar_signal)
 *          │                              │
 *   (retourne immédiatement)      llama_eval(...)  ← peut prendre 5-30s
 *          │                              │
 *   (UI continue de tourner)      callback(response, userdata)
 *                                         │
 *                                 (callback = PostMessage vers UI)
 *
 * RESPONSABLE : DEV-C
 * =============================================================================
 */

#include "llm.h"
#include "threads.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*
 * TODO [DEV-C / TODO-LLM-001] :
 *   Décommenter quand llama.cpp est disponible.
 *   Voir : https://github.com/ggerganov/llama.cpp
 */
 #ifdef HAVE_LLAMA
#include <llama.h>
#endif

 

/* ============================================================================
 * STRUCTURES INTERNES (privées)
 * ============================================================================ */

/**
 * @brief Une entrée dans la file de requêtes LLM.
 */
typedef struct {
    LlmRequestId  id;                         /**< ID unique de la requête    */
    LlmTaskType   task;                        /**< Type de tâche              */
    char          prompt[LLM_MAX_PROMPT_LEN];  /**< Le prompt à envoyer        */
    LlmCallback   callback;                    /**< Callback de réponse        */
    void         *userdata;                    /**< Données utilisateur        */
    bool          cancelled;                   /**< Vrai si annulée            */
} LlmRequest;

/**
 * @brief Implémentation concrète du moteur LLM.
 *
 * Masquée derrière le type opaque LlmEngine dans llm.h
 */
struct LlmEngine {
    /* Contexte llama.cpp */
    /* TODO [DEV-C / TODO-LLM-002] : ajouter les champs llama_context, llama_model */
    void         *llama_ctx;    /**< Opaque pour l'instant (llama_context*)    */
    void         *llama_model;  /**< Opaque pour l'instant (llama_model*)      */

    /* File de requêtes */
    LlmRequest    queue[LLM_REQUEST_QUEUE_SIZE];
    size_t        queue_head;   /**< Index de lecture (consommateur)           */
    size_t        queue_tail;   /**< Index d'écriture (producteur)             */
    size_t        queue_count;  /**< Nombre d'éléments dans la file            */

    /* Synchronisation */
    IeMutex      *mutex;        /**< Protège l'accès à la file                 */
    IeCondVar    *cond_work;    /**< Signal "nouvelle requête disponible"      */
    IeThread     *worker;       /**< Thread worker                             */

    /* État */
    bool          running;      /**< Vrai si le worker est actif               */
    bool          model_loaded; /**< Vrai si le modèle est chargé              */
    LlmRequestId  next_id;      /**< Compteur pour les IDs                     */
    char          model_path[512]; /**< Chemin du modèle                       */
};


/* ============================================================================
 * THREAD WORKER — FONCTION PRINCIPALE
 * ============================================================================ */

/**
 * @brief Fonction exécutée par le thread worker LLM.
 *
 * Boucle infinie qui :
 *   1. Attend qu'une requête arrive (condvar_wait)
 *   2. La dépile
 *   3. Appelle llama.cpp pour l'inférence
 *   4. Appelle le callback avec la réponse
 *
 * TODO [DEV-C / TODO-LLM-003] : Implémenter la boucle complète.
 */
static void llm_worker_func(void *arg) {
    LlmEngine *engine = (LlmEngine *)arg;
    printf("[LLM Worker] Thread démarré\n");

    while (engine->running) {
        /* Attendre une requête */
        mutex_lock(engine->mutex);
        while (engine->queue_count == 0 && engine->running) {
            condvar_wait(engine->cond_work, engine->mutex);
        }

        if (!engine->running) {
            mutex_unlock(engine->mutex);
            break;
        }

        /* Dépiler une requête */
        LlmRequest req = engine->queue[engine->queue_head];
        engine->queue_head = (engine->queue_head + 1) % LLM_REQUEST_QUEUE_SIZE;
        engine->queue_count--;
        mutex_unlock(engine->mutex);

        if (req.cancelled) {
            printf("[LLM Worker] Requête %u annulée\n", req.id);
            continue;
        }

        printf("[LLM Worker] Traitement requête %u (task=%d)\n", req.id, req.task);

        /* Préparer la réponse */
        LlmResponse response;
        memset(&response, 0, sizeof(response));
        response.id = req.id;

        char result_buf[LLM_MAX_RESPONSE_LEN] = {0};
        /*
        /* Tokeniser le prompt */
        const int max_tokens = 512;
#ifdef HAVE_LLAMA
        llama_token *tokens = malloc(max_tokens * sizeof(llama_token));
        if (!tokens) {
            response.status = LLM_RULE_STATUS_ERROR;
            goto send_response;
        }
        const struct llama_vocab *vocab = llama_model_get_vocab(engine->llama_model);
        int n_tokens = llama_tokenize(vocab, req.prompt, (int)strlen(req.prompt), tokens, max_tokens, true, true);
        if (n_tokens <= 0) {
            free(tokens);
            response.status = LLM_RULE_STATUS_ERROR;
            goto send_response;
        }
        printf("[LLM] %d tokens\n", n_tokens);
        struct llama_batch batch = llama_batch_get_one(tokens, n_tokens);
        llama_decode(engine->llama_ctx, batch);
        char result_buf[LLM_MAX_RESPONSE_LEN] = {0};
        int pos = 0;
        struct llama_sampler *sampler = llama_sampler_chain_init(llama_sampler_chain_default_params());
        llama_sampler_chain_add(sampler, llama_sampler_init_greedy());
        for (int t = 0; t < 200 && pos < LLM_MAX_RESPONSE_LEN - 64; t++) {
            llama_token tok = llama_sampler_sample(sampler, engine->llama_ctx, -1);
            if (llama_vocab_is_eog(vocab, tok)) break;
            char piece[64] = {0};
            int plen = llama_token_to_piece(vocab, tok, piece, 63, 0, true);
            if (plen > 0) { memcpy(result_buf + pos, piece, plen); pos += plen; printf("%s", piece); fflush(stdout); }
            struct llama_batch nb = llama_batch_get_one(&tok, 1);
            llama_decode(engine->llama_ctx, nb);
        }
        llama_sampler_free(sampler);
        free(tokens);
#endif /* HAVE_LLAMA */
        strncpy(response.text, result_buf, LLM_MAX_RESPONSE_LEN - 1);
        response.text[LLM_MAX_RESPONSE_LEN - 1] = '\0';
        response.status = LLM_STATUS_DONE;
        response.confidence = 1.0f;
        send_response:

        /* Appeler le callback si défini */
        if (req.callback) {
            req.callback(&response, req.userdata);
        }
    }

    printf("[LLM Worker] Thread arrêté\n");
}


/* ============================================================================
 * API PUBLIQUE
 * ============================================================================ */

LlmEngine *llm_create(const char *model_path, int n_threads, int n_ctx) {
    LlmEngine *engine = calloc(1, sizeof(LlmEngine));
    if (!engine) return NULL;

    strncpy(engine->model_path, model_path ? model_path : "", 511);
    engine->model_path[511] = '\0';
    engine->next_id = 1;
    engine->running = false;
    engine->model_loaded = false;

    engine->mutex = mutex_create();
    engine->cond_work = condvar_create();

    if (!engine->mutex || !engine->cond_work) {
        fprintf(stderr, "[ERROR] llm_create: impossible de créer mutex/condvar\n");
        llm_destroy(engine);
        return NULL;
    }

    /*
     * TODO [DEV-C / TODO-LLM-005] : Charger le modèle llama.cpp
     *
     *   llama_backend_init();
     *
     *   struct llama_model_params mparams = llama_model_default_params();
     *   engine->llama_model = llama_load_model_from_file(model_path, mparams);
     *   if (!engine->llama_model) {
     *       fprintf(stderr, "Impossible de charger le modèle: %s\n", model_path);
     *       llm_destroy(engine);
     *       return NULL;
     *   }
     *
     *   struct llama_context_params cparams = llama_context_default_params();
     *   cparams.n_ctx = n_ctx;
     *   cparams.n_threads = n_threads;
     *   engine->llama_ctx = llama_new_context_with_model(engine->llama_model, cparams);
     */

#ifdef HAVE_LLAMA
    llama_backend_init();

    struct llama_model_params mparams = llama_model_default_params();
    engine->llama_model = llama_model_load_from_file(model_path, mparams);
    if (!engine->llama_model) {
        fprintf(stderr, "[ERROR] Impossible de charger le modèle: %s\n", model_path);
        llm_destroy(engine);
        return NULL;
    }

    struct llama_context_params cparams = llama_context_default_params();
    cparams.n_ctx     = (uint32_t)n_ctx;
    cparams.n_threads = (uint32_t)n_threads;
    engine->llama_ctx = llama_init_from_model(engine->llama_model, cparams);
    if (!engine->llama_ctx) {
        fprintf(stderr, "[ERROR] Impossible de créer le contexte LLM\n");
        llm_destroy(engine);
        return NULL;
    }

    engine->model_loaded = true;
    printf("[INFO] Modèle LLM chargé: %s\n", model_path);

    return engine;
}

bool llm_start_worker(LlmEngine *engine) {
    if (!engine) return false;
    engine->running = true;
    engine->worker = thread_create(llm_worker_func, engine);
    if (!engine->worker) {
        engine->running = false;
        return false;
    }
    printf("[INFO] Thread LLM worker démarré\n");
    return true;
}

void llm_destroy(LlmEngine *engine) {
    if (!engine) return;

    /* Arrêter le worker */
    if (engine->running) {
        mutex_lock(engine->mutex);
        engine->running = false;
        condvar_broadcast(engine->cond_work);
        mutex_unlock(engine->mutex);

        if (engine->worker) {
            thread_join(engine->worker);
        }
    }

 if (engine->llama_ctx)   llama_free(engine->llama_ctx);
    if (engine->llama_model) llama_model_free(engine->llama_model);
    llama_backend_free();
#endif /* HAVE_LLAMA */

    mutex_destroy(engine->mutex);
    condvar_destroy(engine->cond_work);
    free(engine);
}

bool llm_is_ready(const LlmEngine *engine) {
   return engine && engine->running && engine->model_loaded;
}

LlmRequestId llm_submit_request(LlmEngine   *engine,
                                 LlmTaskType  task,
                                 const char  *prompt,
                                 LlmCallback  callback,
                                 void        *userdata) {
    if (!engine || !prompt) return 0;

    mutex_lock(engine->mutex);

    if (engine->queue_count >= LLM_REQUEST_QUEUE_SIZE) {
        mutex_unlock(engine->mutex);
        fprintf(stderr, "[WARN] File LLM pleine (%d slots)\n", LLM_REQUEST_QUEUE_SIZE);
        return 0;
    }

    LlmRequestId id = engine->next_id++;
    LlmRequest *req = &engine->queue[engine->queue_tail];
    req->id = id;
    req->task = task;
    req->callback = callback;
    req->userdata = userdata;
    req->cancelled = false;
    strncpy(req->prompt, prompt, LLM_MAX_PROMPT_LEN - 1);
    req->prompt[LLM_MAX_PROMPT_LEN - 1] = '\0';

    engine->queue_tail = (engine->queue_tail + 1) % LLM_REQUEST_QUEUE_SIZE;
    engine->queue_count++;

    condvar_signal(engine->cond_work);
    mutex_unlock(engine->mutex);

    printf("[INFO] Requête LLM #%u soumise (task=%d)\n", id, task);
    return id;
}

bool llm_cancel_request(LlmEngine *engine, LlmRequestId id) {
    if (!engine || id == 0) return false;

    mutex_lock(engine->mutex);
    bool found = false;
    for (size_t i = 0; i < engine->queue_count; i++) {
        size_t idx = (engine->queue_head + i) % LLM_REQUEST_QUEUE_SIZE;
        if (engine->queue[idx].id == id) {
            engine->queue[idx].cancelled = true;
            found = true;
            break;
        }
    }
    mutex_unlock(engine->mutex);

    return found;
}

size_t llm_queue_size(const LlmEngine *engine) {
    if (!engine) return 0;
    return engine->queue_count;
}

