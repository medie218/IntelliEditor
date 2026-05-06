/**
 * @file nlp_engine.c
 * @brief Pipeline NLP complet — CORE / nlp
 * @author DEV-C
 */

#include "../../../include/nlp.h"
#include "../../../include/llm.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>


static bool is_separator(char c) {
    return c == ' ' || c == '\n' || c == '\r' ||
           c == '\t' || c == '.' || c == ',' ||
           c == '!' || c == '?' || c == ';' ||
           c == ':' || c == '"' || c == '(' ||
           c == ')' || c == '\'' || c == '-';
}

NlpResult *nlp_analyze(NlpEngine *engine, const char *text, size_t len) {
    if (!text || len == 0) return NULL;

    NlpResult *result = calloc(1, sizeof(NlpResult));
    if (!result) return NULL;

    result->error_count = 0;
    result->is_complete = true;

    printf("[NLP] Analyse de %zu octets...\n", len);

    if (engine && engine->spell_checker) {
        spellcheck_analyze(engine->spell_checker, text, len, result);
        printf("[NLP] Orthographe : %zu erreurs\n", result->error_count);
    } else {
        fprintf(stderr, "[NLP] WARN: Hunspell non disponible\n");
    }

    return result;
}

void nlp_result_destroy(NlpResult *result) {
    free(result);
}
