/**
 * @file storage.h
 * @brief Contrat public — Lecture/écriture de fichiers — INFRA
 *
 * =============================================================================
 * RESPONSABILITÉ
 * =============================================================================
 * Gère la persistance des documents : chargement et sauvegarde en .txt, .rtf,
 * et dans le format binaire propriétaire .ie
 *
 * RÈGLE : Ce module ne connaît pas la structure interne du gap buffer.
 * Il travaille avec des chaînes UTF-8 et des chemins de fichiers.
 *
 * APPARTIENT À LA COUCHE : INFRA
 * AUTEUR(S) RESPONSABLE(S) : DEV-A
 *
 * =============================================================================
 */

#ifndef INTELLIEDITOR_STORAGE_H
#define INTELLIEDITOR_STORAGE_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Formats de fichiers supportés.
 */
typedef enum {
    FILE_FORMAT_TXT,  /**< Texte brut UTF-8               */
    FILE_FORMAT_RTF,  /**< Rich Text Format               */
    FILE_FORMAT_IE,   /**< Format binaire IntelliEditor   */
    FILE_FORMAT_UNKNOWN,
} FileFormat;

/**
 * @brief Lit un fichier et retourne son contenu UTF-8.
 *
 * Alloue un buffer. L'appelant doit libérer avec free().
 * Gère automatiquement la détection d'encodage (UTF-8, UTF-16 avec BOM).
 *
 * @param filepath  Chemin du fichier (UTF-8).
 * @param out_len   Longueur du contenu retourné (en octets).
 * @return          Contenu UTF-8 alloué, ou NULL si erreur.
 */
char *storage_read_file(const char *filepath, size_t *out_len);

/**
 * @brief Écrit du texte UTF-8 dans un fichier .txt.
 *
 * @param filepath  Chemin de destination.
 * @param text      Texte UTF-8 à écrire.
 * @param len       Longueur en octets.
 * @return true     Si l'écriture a réussi.
 */
bool storage_write_txt(const char *filepath, const char *text, size_t len);

/**
 * @brief Exporte un texte au format RTF.
 *
 * Génère les balises RTF manuellement (pas de bibliothèque externe).
 *
 * @param filepath  Chemin de destination.
 * @param text      Texte UTF-8 source.
 * @param len       Longueur en octets.
 * @return true     Si l'export a réussi.
 */
bool storage_write_rtf(const char *filepath, const char *text, size_t len);

/**
 * @brief Détecte automatiquement le format d'un fichier.
 *
 * @param filepath  Chemin du fichier.
 * @return          Format détecté.
 */
FileFormat storage_detect_format(const char *filepath);

#endif /* INTELLIEDITOR_STORAGE_H */
