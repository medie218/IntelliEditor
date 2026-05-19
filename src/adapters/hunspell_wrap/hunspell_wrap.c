/**
 * @file hunspell_wrap.c
 * @brief Wrapper Hunspell — ADAPTER / hunspell_wrap
 */

#include "nlp.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#ifdef HAVE_HUNSPELL
#include <hunspell/hunspell.h>
#endif

/* ============================================================================
 * STRUCTURE INTERNE
 * ============================================================================ */

struct SpellChecker {
    void  *hunspell_handle;
    char   aff_path[512];
    char   dic_path[512];
    bool   loaded;
};

/* ============================================================================
 * API
 * ============================================================================ */

SpellChecker *spellchecker_create(const char *aff_path, const char *dic_path) {
    SpellChecker *sc = calloc(1, sizeof(SpellChecker));
    if (!sc) return NULL;

    strncpy(sc->aff_path, aff_path ? aff_path : "", 511);
    sc->aff_path[511] = '\0';
    strncpy(sc->dic_path, dic_path ? dic_path : "", 511);
    sc->dic_path[511] = '\0';
    sc->loaded = false;
    sc->hunspell_handle = NULL;

#ifdef HAVE_HUNSPELL
    sc->hunspell_handle = Hunspell_create(aff_path, dic_path);
    if (!sc->hunspell_handle) {
        fprintf(stderr, "[ERROR] Hunspell_create échoué pour: %s / %s\n", aff_path, dic_path);
        free(sc);
        return NULL;
    }
    sc->loaded = true;
    printf("[INFO] Hunspell chargé avec succès: %s\n", dic_path);
#else
    fprintf(stderr, "[STUB] Hunspell désactivé à la compilation (HAVE_HUNSPELL non défini)\n");
#endif

    return sc;
}

void spellchecker_destroy(SpellChecker *sc) {
    if (!sc) return;
#ifdef HAVE_HUNSPELL
    if (sc->hunspell_handle) Hunspell_destroy(sc->hunspell_handle);
#endif
    free(sc);
}

bool spellcheck_word(const SpellChecker *sc, const char *word) {
    if (!sc || !word) return true;
    if (!sc->loaded || !sc->hunspell_handle) return true;

#ifdef HAVE_HUNSPELL
    int result = Hunspell_spell(sc->hunspell_handle, word);
    return result != 0;
#else
    return true;
#endif
}

void spellcheck_suggest(const SpellChecker *sc,
                        const char         *word,
                        NlpSuggestion       suggestions[NLP_MAX_SUGGESTIONS],
                        size_t             *count) {
    if (!sc || !word || !suggestions || !count) return;
    *count = 0;

    if (!sc->loaded || !sc->hunspell_handle) return;

#ifdef HAVE_HUNSPELL
    char **hsuggestions = NULL;
    int n = Hunspell_suggest(sc->hunspell_handle, &hsuggestions, word);
    for (int i = 0; i < n && i < NLP_MAX_SUGGESTIONS; i++) {
        strncpy(suggestions[i].word, hsuggestions[i], NLP_MAX_WORD_LEN - 1);
        suggestions[i].word[NLP_MAX_WORD_LEN - 1] = '\0';
        suggestions[i].confidence = 1.0f - (float)i / (float)n;
    }
    *count = (size_t)(n < NLP_MAX_SUGGESTIONS ? n : NLP_MAX_SUGGESTIONS);
    Hunspell_free_list(sc->hunspell_handle, &hsuggestions, n);
#endif
}

void spellcheck_analyze(const SpellChecker *sc,
                        const char         *text,
                        size_t              len,
                        NlpResult          *out) {
    if (!sc || !text || !out) return;
    out->error_count = 0;
    out->is_complete = true;

    if (!sc->loaded) return;

    size_t i = 0;
    char word[NLP_MAX_WORD_LEN];
    size_t word_len = 0;
    size_t word_start = 0;

    while (i <= len) {
        unsigned char c = (i < len) ? (unsigned char)text[i] : ' ';
        bool is_sep = (c == ' ' || c == '\n' || c == '\r' ||
                       c == '\t' || c == '.' || c == ',' ||
                       c == '!' || c == '?' || c == ';' ||
                       c == ':' || c == '"' || c == '(' ||
                       c == ')');

        if (!is_sep && word_len < NLP_MAX_WORD_LEN - 1) {
            word[word_len++] = (char)c;
        } else if (word_len > 0) {
            word[word_len] = '\0';

            if (!spellcheck_word(sc, word) &&
                out->error_count < NLP_MAX_ERRORS) {

                NlpError *err = &out->errors[out->error_count++];
                err->type  = NLP_ERROR_SPELLING;
                err->start = word_start;
                err->length = word_len;
                strncpy(err->original, word, NLP_MAX_WORD_LEN - 1);
                err->original[NLP_MAX_WORD_LEN - 1] = '\0';
                snprintf(err->message, sizeof(err->message),
                         "Mot inconnu : '%s'", word);

                spellcheck_suggest(sc, word,
                                   err->suggestions,
                                   &err->suggestion_count);
            }
            word_len  = 0;
        }
        
        if (is_sep) {
            word_start = i + 1;
        }
        i++;
    }
}
