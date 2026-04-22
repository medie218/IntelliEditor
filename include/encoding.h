/**
 * @file encoding.h
 * @brief Contrat public — Conversion d'encodage UTF-8 ↔ UTF-16 — INFRA
 *
 * =============================================================================
 * RESPONSABILITÉ
 * =============================================================================
 * Le pipeline interne d'IntelliEditor utilise EXCLUSIVEMENT UTF-8.
 * Ce module assure la conversion vers/depuis UTF-16 (requis par WinAPI).
 *
 * RÈGLE : Les conversions UTF-16 n'ont lieu QU'AUX POINTS D'ENTRÉE/SORTIE
 * Win32. Jamais dans le Core ou les modules NLP/Rules.
 *
 * APPARTIENT À LA COUCHE : INFRA
 * AUTEUR(S) RESPONSABLE(S) : DEV-A
 *
 * =============================================================================
 */

#ifndef INTELLIEDITOR_ENCODING_H
#define INTELLIEDITOR_ENCODING_H

#include <stddef.h>
#include <stdbool.h>
#include <wchar.h>

/**
 * @brief Convertit une chaîne UTF-8 en UTF-16 (wchar_t).
 *
 * Alloue un nouveau buffer wchar_t. L'appelant doit libérer avec free().
 *
 * @param utf8    Chaîne source UTF-8 (null-terminée).
 * @param out_len Pointeur de sortie pour la longueur (en wchar_t, optionnel).
 * @return        Chaîne UTF-16 allouée, ou NULL si erreur.
 */
wchar_t *encoding_utf8_to_utf16(const char *utf8, size_t *out_len);

/**
 * @brief Convertit une chaîne UTF-16 (wchar_t) en UTF-8.
 *
 * Alloue un nouveau buffer char. L'appelant doit libérer avec free().
 *
 * @param utf16   Chaîne source UTF-16 (null-terminée).
 * @param out_len Pointeur de sortie pour la longueur en octets (optionnel).
 * @return        Chaîne UTF-8 allouée, ou NULL si erreur.
 */
char *encoding_utf16_to_utf8(const wchar_t *utf16, size_t *out_len);

/**
 * @brief Vérifie si une séquence d'octets est du UTF-8 valide.
 *
 * @param data  Buffer à valider.
 * @param len   Longueur en octets.
 * @return true Si UTF-8 valide.
 */
bool encoding_is_valid_utf8(const char *data, size_t len);

/**
 * @brief Retourne la longueur en caractères Unicode (code points) d'une chaîne UTF-8.
 *
 * Diffère de strlen() qui compte les octets.
 *
 * @param utf8  Chaîne UTF-8 null-terminée.
 * @return      Nombre de code points Unicode.
 */
size_t encoding_utf8_char_count(const char *utf8);

/**
 * @brief Retourne le pointeur sur le N-ième caractère Unicode dans un UTF-8.
 *
 * @param utf8  Chaîne UTF-8.
 * @param index Index du caractère (0-based).
 * @return      Pointeur dans la chaîne, ou NULL si hors bornes.
 */
const char *encoding_utf8_char_at(const char *utf8, size_t index);

#endif /* INTELLIEDITOR_ENCODING_H */
