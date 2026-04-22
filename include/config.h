/**
 * @file config.h
 * @brief Contrat public — Gestion de la configuration INI — INFRA
 *
 * Lit et écrit %APPDATA%\IntelliEditor\config.ini
 * Format INI simple, parsé manuellement en C11 sans bibliothèque externe.
 *
 * APPARTIENT À LA COUCHE : INFRA
 * AUTEUR(S) RESPONSABLE(S) : DEV-A
 */

#ifndef INTELLIEDITOR_CONFIG_H
#define INTELLIEDITOR_CONFIG_H

#include <stdbool.h>
#include <stddef.h>

#define CONFIG_MAX_VALUE_LEN  256
#define CONFIG_MAX_KEY_LEN    64
#define CONFIG_MAX_SECTION_LEN 32
#define CONFIG_MAX_ENTRIES     64

/**
 * @brief Une entrée clé=valeur de la configuration.
 */
typedef struct {
    char section[CONFIG_MAX_SECTION_LEN]; /**< Ex : "General", "LLM"         */
    char key[CONFIG_MAX_KEY_LEN];         /**< Ex : "theme", "model_path"    */
    char value[CONFIG_MAX_VALUE_LEN];     /**< Ex : "light", "C:\\models\\"  */
} ConfigEntry;

/**
 * @brief Configuration complète de l'application.
 */
typedef struct {
    ConfigEntry entries[CONFIG_MAX_ENTRIES];
    size_t      count;
    char        filepath[512]; /**< Chemin du fichier config chargé           */
} AppConfig;

/** Charge la config depuis le fichier INI. Crée le fichier s'il n'existe pas. */
bool config_load(AppConfig *cfg, const char *filepath);

/** Sauvegarde la config dans le fichier INI. */
bool config_save(const AppConfig *cfg);

/** Retourne la valeur d'une clé dans une section. NULL si non trouvée. */
const char *config_get(const AppConfig *cfg,
                       const char *section,
                       const char *key);

/** Définit ou met à jour une valeur. */
void config_set(AppConfig *cfg,
                const char *section,
                const char *key,
                const char *value);

/** Retourne la valeur entière d'une clé, ou default_val si absente. */
int config_get_int(const AppConfig *cfg,
                   const char *section,
                   const char *key,
                   int default_val);

#endif /* INTELLIEDITOR_CONFIG_H */
