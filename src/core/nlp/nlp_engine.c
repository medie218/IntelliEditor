/**
 * @file nlp_engine.c
 * @brief Pipeline NLP complet — CORE / nlp
 * @author DEV-C
 *
 * ========================================================= * RÔLE DE CE FICHIER
 * ========================================================= * Ce fichier orchestre le pipeline NLP complet :
 *   1. Correction orthographique (Hunspell — synchrone)
 *   2. Vérification ponctuation française (règles codées)
 *   3. Détection répétitions de mots
 *   4. Détection anglicismes
 *
 * Les résultats (NlpResult) sont envoyés vers :
 *   - DEV-B (ui_apply_nlp_markers) → soulignements dans Scintilla
 *   - DEV-D (rules_evaluate)       → vérification sémantique LLM
 *
 * INTÉGRATION :
 *   Quand DEV-B livre son code → décommenter ui_apply_nlp_markers()
 *   Quand DEV-D livre son code → décommenter rules_update_llm_result()
 * ========================================================= */

#include "nlp.h"
#include "llm.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>

/* ========================================================= * LISTE DES ANGLICISMES COURANTS À DÉTECTER
 * ================================================================ */
static const char *ANGLICISMES[] = {
    "mail", "email", "meeting", "deadline", "feedback",
    "update", "check", "chat", "post", "like",
    "follow", "share", "browser", "online", "offline",
    NULL
};

/* ========================================================= * FONCTIONS PRIVÉES
 * ================================================================ */

/**
 * @brief Vérifie si un caractère est un séparateur de mot.
 */
static bool is_separator(char c) {
    return c == ' ' || c == '\n' || c == '\r' ||
           c == '\t' || c == '.' || c == ',' ||
           c == '!' || c == '?' || c == ';' ||
           c == ':' || c == '"' || c == '(' ||
           c == ')' || c == '\'' || c == '-';
}

/**
 * @brief Détecte les erreurs de ponctuation française.
 *
 * En français : espace AVANT : ; ! ? »
 *               espace APRÈS  : « (
 *
 * TODO [DEV-C] : améliorer pour gérer les guillemets français
 */
static void detect_punctuation_errors(const char   *text,
                                       size_t        len,
                                       NlpResult    *out) {
    for (size_t i = 1; i < len && out->error_count < NLP_MAX_ERRORS; i++) {
        /* Détecter : ; ! ? sans espace avant */
        if ((text[i] == ':' || text[i] == ';' ||
             text[i] == '!' || text[i] == '?') &&
            text[i-1] != ' ' && text[i-1] != '\n') {

            NlpError *err = &out->errors[out->error_count++];
            err->type   = NLP_ERROR_PUNCTUATION;
            err->start  = i;
            err->length = 1;
            snprintf(err->original, NLP_MAX_WORD_LEN, "%c", text[i]);
            snprintf(err->message, sizeof(err->message),
                "Ponctuation française : espace manquante avant '%c'",
                text[i]);
            err->suggestion_count = 1;
            snprintf(err->suggestions[0].word, NLP_MAX_WORD_LEN,
                " %c", text[i]);
            err->suggestions[0].confidence = 1.0f;
        }
    }
}

/**
 * @brief Détecte les mots répétés trop proches (distance < 3 mots).
 */
static void detect_repetitions(const char *text,
                                size_t      len,
                                NlpResult  *out) {
    /* Tableau des 5 derniers mots vus */
    char  last_words[5][NLP_MAX_WORD_LEN] = {{0}};
    size_t last_pos[5] = {0};
    int   word_idx = 0;

    size_t i = 0;
    char   word[NLP_MAX_WORD_LEN];
    size_t word_len  = 0;
    size_t word_start = 0;

    while (i <= len && out->error_count < NLP_MAX_ERRORS) {
        char c = (i < len) ? text[i] : ' ';

        if (!is_separator(c) && word_len < NLP_MAX_WORD_LEN - 1) {
            if (word_len == 0) word_start = i;
            word[word_len++] = tolower((unsigned char)c);
        } else if (word_len > 2) { /* Ignorer mots très courts */
            word[word_len] = '\0';

            /* Vérifier si le mot apparaît dans les 5 derniers */
            for (int j = 0; j < 5; j++) {
                if (strcmp(last_words[j], word) == 0) {
                    /* Répétition détectée ! */
                    NlpError *err = &out->errors[out->error_count++];
                    err->type   = NLP_ERROR_REPETITION;
                    err->start  = word_start;
                    err->length = word_len;
                    strncpy(err->original, word, NLP_MAX_WORD_LEN - 1); /* patched */
                    snprintf(err->message, sizeof(err->message),
                        "Répétition : '%s' déjà utilisé récemment "
                        "(position %zu)", word, last_pos[j]);
                    err->suggestion_count = 0;
                    break;
                }
            }

            /* Enregistrer ce mot */
            strncpy(last_words[word_idx % 5], word, NLP_MAX_WORD_LEN - 1); /* patched */
            last_pos[word_idx % 5] = word_start;
            word_idx++;
            word_len = 0;
        } else {
            word_len = 0;
        }
        i++;
    }
}

/**
 * @brief Détecte les anglicismes dans le texte.
 */
static void detect_anglicisms(const char *text,
                               size_t      len,
                               NlpResult  *out) {
    size_t i = 0;
    char   word[NLP_MAX_WORD_LEN];
    size_t word_len  = 0;
    size_t word_start = 0;

    while (i <= len && out->error_count < NLP_MAX_ERRORS) {
        char c = (i < len) ? text[i] : ' ';

        if (!is_separator(c) && word_len < NLP_MAX_WORD_LEN - 1) {
            if (word_len == 0) word_start = i;
            word[word_len++] = tolower((unsigned char)c);
        } else if (word_len > 0) {
            word[word_len] = '\0';

            /* Chercher dans la liste des anglicismes */
            for (int j = 0; ANGLICISMES[j] != NULL; j++) {
                if (strcmp(word, ANGLICISMES[j]) == 0) {
                    NlpError *err = &out->errors[out->error_count++];
                    err->type   = NLP_ERROR_ANGLICISM;
                    err->start  = word_start;
                    err->length = word_len;
                    strncpy(err->original, word, NLP_MAX_WORD_LEN - 1); /* patched */
                    snprintf(err->message, sizeof(err->message),
                        "Anglicisme détecté : '%s' — "
                        "préférer un équivalent français", word);
                    err->suggestion_count = 0;
                    break;
                }
            }
            word_len = 0;
        }
        i++;
    }
}


/* ========================================================= * API PUBLIQUE
 * ================================================================ */

/**
 * @brief Analyse NLP complète d'un texte.
 *
 * Effectue dans l'ordre :
 *   1. Correction orthographique (Hunspell)
 *   2. Ponctuation française
 *   3. Répétitions de mots
 *   4. Anglicismes
 *
 * @param engine  Handle NLP (contient le SpellChecker Hunspell)
 * @param text    Texte UTF-8 à analyser
 * @param len     Longueur en octets
 * @return        NlpResult alloué (libérer avec nlp_result_destroy)
 */
NlpResult *nlp_analyze(NlpEngine *engine, const char *text, size_t len) {
    if (!text || len == 0) return NULL;

    NlpResult *result = calloc(1, sizeof(NlpResult));
    if (!result) return NULL;

    result->error_count = 0;
    result->is_complete = true;

    printf("[NLP] Analyse de %zu octets...\n", len);

    /* ── Étape 1 : Orthographe (Hunspell) ── */
    if (engine && engine->spell_checker) {
        spellcheck_analyze(engine->spell_checker, text, len, result);
        printf("[NLP] Orthographe : %zu erreurs trouvées\n",
               result->error_count);
    } else {
        fprintf(stderr, "[NLP] WARN: Hunspell non disponible — "
                        "orthographe ignorée\n");
    }

    /* ── Étape 2 : Ponctuation française ── */
    size_t before_punct = result->error_count;
    detect_punctuation_errors(text, len, result);
    printf("[NLP] Ponctuation : %zu erreurs trouvées\n",
           result->error_count - before_punct);

    /* ── Étape 3 : Répétitions ── */
    size_t before_rep = result->error_count;
    detect_repetitions(text, len, result);
    printf("[NLP] Répétitions : %zu trouvées\n",
           result->error_count - before_rep);

    /* ── Étape 4 : Anglicismes ── */
    size_t before_ang = result->error_count;
    detect_anglicisms(text, len, result);
    printf("[NLP] Anglicismes : %zu trouvés\n",
           result->error_count - before_ang);

    printf("[NLP] Total : %zu erreurs\n", result->error_count);

    /*
     * TODO [INTÉGRATION DEV-B] :
     * Quand DEV-B aura implémenté ui_apply_nlp_markers(), décommenter :
     *
     *   ui_apply_nlp_markers(app_ctx, result);
     *
     * Pour l'instant on retourne le résultat et c'est l'appelant
     * qui décide quoi en faire.
     */

    return result;
}

void nlp_result_destroy(NlpResult *result) {
    free(result);
}
