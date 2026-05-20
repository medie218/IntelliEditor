/**
 * @file hunspell_wrap.c
 * @brief Wrapper Hunspell — ADAPTER / hunspell_wrap
 *
 * =============================================================================
 * RÔLE
 * =============================================================================
 * Intègre la bibliothèque Hunspell pour la correction orthographique du français.
 * Implémente l'interface SpellChecker définie dans nlp.h.
 *
 * HUNSPELL :
 *   Bibliothèque open source utilisée par LibreOffice, Firefox, etc.
 *   Elle a une API C (hunspell.h) et des dictionnaires .aff + .dic
 *
 *   Fichiers nécessaires (à placer dans data/dicts/) :
 *     - fr_FR.aff   (règles d'affixation)
 *     - fr_FR.dic   (dictionnaire)
 *   Téléchargeables sur : https://github.com/wooorm/dictionaries
 *
 * UTILISATION :
 *   Hunhandle *hh = Hunspell_create("fr_FR.aff", "fr_FR.dic");
 *   int ok = Hunspell_spell(hh, "bonjour");   // 1 = correct, 0 = incorrect
 *   Hunspell_destroy(hh);
 *
 * COMPILATION :
 *   Lier avec -lhunspell-1.7 (ou -lhunspell selon la version)
 *
 * RESPONSABLE : DEV-C
 * =============================================================================
 */

#include "nlp.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/*
 * TODO [DEV-C / TODO-HUNSPELL-001] :
 *   Décommenter l'include Hunspell quand disponible via MSYS2.
 *   Installation : pacman -S mingw-w64-x86_64-hunspell
 */
 #ifdef HAVE_HUNSPELL
#include <hunspell/hunspell.h>
#endif

/* ============================================================================
 * STRUCTURE INTERNE
 * ============================================================================ */

/**
 * @brief Implémentation concrète du SpellChecker (masquée derrière l'interface nlp.h).
 */
struct SpellChecker {
    void  *hunspell_handle;  /**< Pointeur vers Hunhandle (opaque)            */
    char   aff_path[512];    /**< Chemin vers le fichier .aff                 */
    char   dic_path[512];    /**< Chemin vers le fichier .dic                 */
    bool   loaded;           /**< Vrai si Hunspell est initialisé             */
};


/* ============================================================================
 * API
 * ============================================================================ */

/**
 * @brief Crée et initialise un correcteur orthographique Hunspell.
 *
 * TODO [DEV-C / TODO-HUNSPELL-002] :
 *   1. Vérifier que aff_path et dic_path existent (fopen + fclose)
 *   2. Appeler Hunspell_create(aff_path, dic_path)
 *   3. Vérifier que le handle retourné n'est pas NULL
 *   4. Ajouter des mots personnalisés si nécessaire (noms propres, termes techniques)
 *
 * @param aff_path  Chemin vers fr_FR.aff
 * @param dic_path  Chemin vers fr_FR.dic
 * @return          Handle SpellChecker, ou NULL si erreur
 */
SpellChecker *spellchecker_create(const char *aff_path, const char *dic_path) {
    SpellChecker *sc = calloc(1, sizeof(SpellChecker));
    if (!sc) return NULL;

    strncpy(sc->aff_path, aff_path ? aff_path : "", 511);
    sc->aff_path[511] = '\0';
    strncpy(sc->dic_path, dic_path ? dic_path : "", 511);
    sc->dic_path[511] = '\0';
    sc->loaded = false;
    sc->hunspell_handle = NULL;

    /*
     * TODO [DEV-C / TODO-HUNSPELL-002] :
     *   sc->hunspell_handle = Hunspell_create(aff_path, dic_path);
     *   if (!sc->hunspell_handle) { free(sc); return NULL; }
     *   sc->loaded = true;
     */

   sc->hunspell_handle = Hunspell_create(aff_path, dic_path);
    if (!sc->hunspell_handle) {
        fprintf(stderr, "[ERROR] Hunspell_create échoué pour: %s / %s\n", aff_path, dic_path);
        free(sc);
        return NULL;
    }
    sc->loaded = true;
    printf("[INFO] Hunspell chargé avec succès: %s\n", dic_path);

    return sc;
}

/**
 * @brief Libère un SpellChecker Hunspell.
 */
void spellchecker_destroy(SpellChecker *sc) {
    if (!sc) return;
    if (sc->hunspell_handle) Hunspell_destroy(sc->hunspell_handle);
    free(sc);
}

bool spellcheck_word(const SpellChecker *sc, const char *word) {
    if (!sc || !word) return true; /* Par défaut: ne pas signaler d'erreur si non initialisé */

    if (!sc->loaded || !sc->hunspell_handle) {
        /* STUB : accepter tout si Hunspell non chargé */
        return true;
    }

    /*
     * TODO [DEV-C / TODO-HUNSPELL-004] :
     *   int result = Hunspell_spell(sc->hunspell_handle, word);
     *   return result != 0;
     *
     * ATTENTION : Hunspell attend du Latin-1 par défaut pour le français.
     * Si les dictionnaires sont en UTF-8, utiliser Hunspell_spell() directement.
     * Vérifier l'encodage du .aff avec : head -1 fr_FR.aff (ligne "SET UTF-8")
     */
int result = Hunspell_spell(sc->hunspell_handle, word);
    return result != 0;
}

void spellcheck_suggest(const SpellChecker *sc,
                        const char         *word,
                        NlpSuggestion       suggestions[NLP_MAX_SUGGESTIONS],
                        size_t             *count) {
    if (!sc || !word || !suggestions || !count) return;
    *count = 0;

    if (!sc->loaded || !sc->hunspell_handle) {
        /* STUB : aucune suggestion si non initialisé */
        return;
    }

    char **hsuggestions = NULL;
    int n = Hunspell_suggest(sc->hunspell_handle, &hsuggestions, word);
    for (int i = 0; i < n && i < NLP_MAX_SUGGESTIONS; i++) {
        strncpy(suggestions[i].word, hsuggestions[i], NLP_MAX_WORD_LEN - 1);
        suggestions[i].word[NLP_MAX_WORD_LEN - 1] = '\0';
        suggestions[i].confidence = 1.0f - (float)i / (float)n;
    }
    *count = (size_t)(n < NLP_MAX_SUGGESTIONS ? n : NLP_MAX_SUGGESTIONS);
    Hunspell_free_list(sc->hunspell_handle, &hsuggestions, n);
}

void spellcheck_analyze(const SpellChecker *sc,
                        const char         *text,
                        size_t              len,
                        NlpResult          *out) {
    if (!sc || !text || !out) return;
    out->error_count = 0;
    out->is_complete = true;

    if (!sc->loaded) {
        fprintf(stderr, "[STUB] spellcheck_analyze: Hunspell non chargé\n");
        return;
    }

    /*
     * TODO [DEV-C / TODO-HUNSPELL-006] :
     *   Algorithme :
     *   1. Tokeniser le texte en mots (délimiteurs: espaces, ponctuation)
     *   2. Pour chaque mot, appeler spellcheck_word()
     *   3. Si incorrect, créer un NlpError et appeler spellcheck_suggest()
     *   4. Ajouter l'erreur dans out->errors[]
     *
     *   ATTENTION : gérer les mots avec apostrophes (l'homme → "l" + "homme")
     *   ATTENTION : ne pas signaler les nombres, URLs, emails comme erreurs
     */

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
            word_start = i + 1;
        } else {
            word_start = i + 1;
        }
        i++;
    }
}
   
