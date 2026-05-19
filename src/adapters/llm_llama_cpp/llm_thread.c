/**
 * @file llm_thread.c
 * @brief Thread worker LLM asynchrone — ADAPTER / llm_llama_cpp
 */

#include "llm.h"
#include "threads.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef HAVE_LLAMA
#include <llama.h>
#endif

/* ============================================================================
 * STRUCTURES INTERNES (privées)
 * ============================================================================ */

typedef struct {
    LlmRequestId  id;
    LlmTaskType   task;
    char          prompt[LLM_MAX_PROMPT_LEN];
    LlmCallback   callback;
    void         *userdata;
    bool          cancelled;
} LlmRequest;

struct LlmEngine {
#ifdef HAVE_LLAMA
    struct llama_model   *llama_model;
    struct llama_context *llama_ctx;
#else
    void *llama_model;
    void *llama_ctx;
#endif

    /* File de requêtes */
    LlmRequest    queue[LLM_REQUEST_QUEUE_SIZE];
    size_t        queue_head;
    size_t        queue_tail;
    size_t        queue_count;

    /* Synchronisation */
    IeMutex      *mutex;
    IeCondVar    *cond_work;
    IeThread     *worker;

    /* État */
    bool          running;
    bool          model_loaded;
    LlmRequestId  next_id;
    char          model_path[512];
};

/* ============================================================================
 * THREAD WORKER — FONCTION PRINCIPALE
 * ============================================================================ */

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
        response.status = LLM_STATUS_DONE;
        response.confidence = 1.0f;

#ifdef HAVE_LLAMA
        if (engine->model_loaded && engine->llama_ctx) {
            const int max_tokens = LLM_MAX_TOKENS;
            const struct llama_vocab *vocab = llama_model_get_vocab(engine->llama_model);
            
            llama_token *tokens = malloc(max_tokens * sizeof(llama_token));
            if (!tokens) {
                response.status = LLM_RULE_STATUS_ERROR;
                goto send_response;
            }

            int n_tokens = llama_tokenize(vocab, req.prompt, (int)strlen(req.prompt), tokens, max_tokens, true, true);
            if (n_tokens <= 0) {
                free(tokens);
                response.status = LLM_RULE_STATUS_ERROR;
                goto send_response;
            }

            struct llama_batch batch = llama_batch_get_one(tokens, n_tokens);
            if (llama_decode(engine->llama_ctx, batch) != 0) {
                free(tokens);
                response.status = LLM_RULE_STATUS_ERROR;
                goto send_response;
            }

            int pos = 0;
            struct llama_sampler *sampler = llama_sampler_chain_init(llama_sampler_chain_default_params());
            llama_sampler_chain_add(sampler, llama_sampler_init_greedy());

            for (int t = 0; t < max_tokens && pos < LLM_MAX_RESPONSE_LEN - 64; t++) {
                llama_token tok = llama_sampler_sample(sampler, engine->llama_ctx, -1);
                if (llama_vocab_is_eog(vocab, tok)) break;

                char piece[64] = {0};
                int plen = llama_token_to_piece(vocab, tok, piece, 63, 0, true);
                if (plen > 0) {
                    if (pos + plen < LLM_MAX_RESPONSE_LEN) {
                        memcpy(response.text + pos, piece, plen);
                        pos += plen;
                    }
                }

                struct llama_batch nb = llama_batch_get_one(&tok, 1);
                if (llama_decode(engine->llama_ctx, nb) != 0) break;
            }

            response.text[pos] = '\0';
            llama_sampler_free(sampler);
            free(tokens);
        } else {
            snprintf(response.text, LLM_MAX_RESPONSE_LEN, "LLM : Modèle non chargé (Mode STUB)");
        }
#else
        snprintf(response.text, LLM_MAX_RESPONSE_LEN, "LLM : Support désactivé à la compilation (Mode STUB)");
#endif

#ifdef HAVE_LLAMA
send_response:
#endif

#ifdef HAVE_CJSON
        /* Post-traitement si la réponse est un JSON (format attendu par nos prompts) */
        cJSON *root = cJSON_Parse(response.text);
        if (root) {
            char formatted[LLM_MAX_RESPONSE_LEN] = {0};
            int pos = 0;

            cJSON *errors = cJSON_GetObjectItem(root, "errors");
            if (cJSON_IsArray(errors)) {
                pos += snprintf(formatted + pos, LLM_MAX_RESPONSE_LEN - pos, "Suggestions de correction :\n");
                int n = cJSON_GetArraySize(errors);
                for (int i = 0; i < n && pos < LLM_MAX_RESPONSE_LEN - 50; i++) {
                    cJSON *item = cJSON_GetArrayItem(errors, i);
                    cJSON *orig = cJSON_GetObjectItem(item, "original");
                    cJSON *corr = cJSON_GetObjectItem(item, "corrected");
                    if (cJSON_IsString(orig) && cJSON_IsString(corr)) {
                        pos += snprintf(formatted + pos, LLM_MAX_RESPONSE_LEN - pos, 
                                        "- '%s' -> '%s'\n", orig->valuestring, corr->valuestring);
                    }
                }
            } else {
                cJSON *ref = cJSON_GetObjectItem(root, "reformulation");
                if (cJSON_IsString(ref)) {
                    pos += snprintf(formatted + pos, LLM_MAX_RESPONSE_LEN - pos, 
                                    "Reformulation proposée :\n%s", ref->valuestring);
                }
            }

            if (pos > 0) {
                strncpy(response.text, formatted, LLM_MAX_RESPONSE_LEN - 1);
                response.text[LLM_MAX_RESPONSE_LEN - 1] = '\0';
            }
            cJSON_Delete(root);
        }
#endif

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
        llm_destroy(engine);
        return NULL;
    }

#ifdef HAVE_LLAMA
    llama_backend_init();

    struct llama_model_params mparams = llama_model_default_params();
    engine->llama_model = llama_model_load_from_file(model_path, mparams);
    if (!engine->llama_model) {
        fprintf(stderr, "[LLM] Erreur chargement modèle : %s\n", model_path);
        // On ne détruit pas tout ici pour permettre le mode stub si souhaité, 
        // ou on détruit si c'est fatal. Le guide dit que c'est fatal.
        llm_destroy(engine);
        return NULL;
    }

    struct llama_context_params cparams = llama_context_default_params();
    cparams.n_ctx     = (uint32_t)n_ctx;
    cparams.n_threads = (uint32_t)n_threads;
    engine->llama_ctx = llama_init_from_model(engine->llama_model, cparams);
    if (!engine->llama_ctx) {
        llm_destroy(engine);
        return NULL;
    }

    engine->model_loaded = true;
#endif

    return engine;
}

bool llm_start_worker(LlmEngine *engine) {
    if (!engine || engine->running) return false;
    engine->running = true;
    engine->worker = thread_create(llm_worker_func, engine);
    if (!engine->worker) {
        engine->running = false;
        return false;
    }
    return true;
}

void llm_destroy(LlmEngine *engine) {
    if (!engine) return;

    if (engine->running) {
        mutex_lock(engine->mutex);
        engine->running = false;
        condvar_broadcast(engine->cond_work);
        mutex_unlock(engine->mutex);

        if (engine->worker) {
            thread_join(engine->worker);
        }
    }

#ifdef HAVE_LLAMA
    if (engine->llama_ctx)   llama_free(engine->llama_ctx);
    if (engine->llama_model) llama_model_free(engine->llama_model);
    llama_backend_free();
#endif

    if (engine->mutex) mutex_destroy(engine->mutex);
    if (engine->cond_work) condvar_destroy(engine->cond_work);
    free(engine);
}

bool llm_is_ready(const LlmEngine *engine) {
    return engine && engine->running && engine->model_loaded;
}

LlmRequestId llm_submit_request(LlmEngine *engine, LlmTaskType task, const char *prompt, LlmCallback callback, void *userdata) {
    if (!engine || !prompt) return 0;

    mutex_lock(engine->mutex);
    if (engine->queue_count >= LLM_REQUEST_QUEUE_SIZE) {
        mutex_unlock(engine->mutex);
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
    return id;
}

bool llm_cancel_request(LlmEngine *engine, LlmRequestId id) {
    if (!engine || id == 0) return false;
    mutex_lock(engine->mutex);
    for (size_t i = 0; i < engine->queue_count; i++) {
        size_t idx = (engine->queue_head + i) % LLM_REQUEST_QUEUE_SIZE;
        if (engine->queue[idx].id == id) {
            engine->queue[idx].cancelled = true;
            mutex_unlock(engine->mutex);
            return true;
        }
    }
    mutex_unlock(engine->mutex);
    return false;
}

size_t llm_queue_size(const LlmEngine *engine) {
    if (!engine) return 0;
    return engine->queue_count;
}
