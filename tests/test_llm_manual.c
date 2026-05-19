/**
 * test_llm_manual.c
 * Test rapide du moteur LLM — DEV-C
 */

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "llama.h"
#include "llm.h"
#include "threads.h"

void ma_callback(const LlmResponse *resp, void *userdata) {
    (void)userdata;
    printf("\n=== RÉPONSE LLM ===\n");
    printf("Status : %d\n", resp->status);
    printf("Texte  : %s\n", resp->text);
    printf("==================\n");
}

int main(void) {
    printf("=== Test LLM — IntelliEditor DEV-C ===\n\n");
    fflush(stdout);

    printf("[1] Creation du moteur LLM...\n");
    fflush(stdout);
    LlmEngine *engine = llm_create(
        "models/qwen2.5-3b-instruct-q4.gguf",
        4, 2048
    );
    if (!engine) {
        fprintf(stderr, "Erreur : llm_create echoue\n");
        return 1;
    }

    printf("[2] Demarrage worker...\n");
    fflush(stdout);
    if (!llm_start_worker(engine)) {
        fprintf(stderr, "Erreur : worker non demarre\n");
        llm_destroy(engine);
        return 1;
    }

    printf("[3] Attente que le modele soit pret (10s)...\n");
    fflush(stdout);
    Sleep(10000);

    printf("[4] Envoi requete...\n");
    fflush(stdout);
    LlmRequestId id = llm_submit_request(
        engine,
        LLM_TASK_GRAMMAR_CHECK,
        "[INST] Reponds en une phrase : Quelle est la capitale de la France ? [/INST]",
        ma_callback,
        NULL
    );
    printf("Requete #%u soumise, attente reponse (120s)...\n", id);
    fflush(stdout);

    Sleep(120000);

    llm_destroy(engine);
    printf("\nTest termine !\n");
    return 0;
}