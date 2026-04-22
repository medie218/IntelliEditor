# DEV_GUIDE.md — Guide du développeur IntelliEditor

> Ce guide explique **quoi faire, dans quel ordre, et comment** pour chaque membre de l'équipe.
> Il contient aussi des prompts prêts à l'emploi pour travailler avec une IA.

---

## Règles fondamentales (à lire absolument)

### Les 5 commandements de l'architecture

1. **Le Core ne dépend de rien.** Jamais `#include <windows.h>` dans `src/core/`.
2. **Le texte vit dans le gap buffer.** Scintilla est un miroir, pas la source de vérité.
3. **Le LLM est 100% asynchrone.** L'UI ne attend jamais une réponse LLM.
4. **Un module = un header = un contrat.** Ne pas modifier `include/` sans en parler à l'équipe.
5. **Pas de malloc sans free.** Chaque allocation a un responsable clair.

### Processus de travail

```
1. Lire le TODO → comprendre CE QUI doit être fait
2. Lire le header include/ → comprendre LES TYPES et LES CONTRATS
3. Lire ARCHITECTURE.md → comprendre LE CONTEXTE
4. Écrire le code dans le .c
5. Vérifier : make + make test
6. Commit atomique + Push + Pull Request
```

---

## DEV-A — Infrastructure & Éditeur de texte

### Votre mission

Vous êtes le **fondateur du projet**. Tout le reste repose sur votre travail.
Un bug dans le gap buffer = un bug dans l'application entière.

### Vos modules

| Fichier | Description | Priorité |
|---------|-------------|----------|
| `src/core/editor/gap_buffer.c` | Gap buffer, insertion, suppression, curseur | 🔴 P1 |
| `src/infra/encoding/encoding.c` | Conversion UTF-8/UTF-16, validation | 🔴 P1 |
| `src/infra/threads/threads.c` | Threads Win32 (déjà implémenté ✅) | ✅ Fait |
| `src/infra/config_ini/config.c` | Config INI (déjà implémenté ✅) | ✅ Fait |
| `src/infra/storage/storage.c` | Lecture/écriture fichiers | 🟡 P2 |

### Par où commencer ?

**Semaine 1 — Gap Buffer (Phase 1)**

```
TODO-GAPBUF-001 : gap_buffer_ensure_gap() → realloc si le gap est trop petit
TODO-GAPBUF-002 : gap_buffer_move_gap()   → déplacer le trou vers la position
TODO-EDITOR-001 : editor_create()         → allouer le buffer initial
TODO-EDITOR-003 : editor_insert()         → insérer du texte
TODO-EDITOR-004 : editor_delete()         → supprimer du texte
TODO-EDITOR-006 : editor_get_text()       → extraire le texte complet
```

**Comment tester votre gap buffer sans l'UI :**

Écrivez un petit `main()` de test dans un fichier temporaire :

```c
// test_manual.c — à NE PAS commiter, juste pour votre débogage perso
#include "include/editor.h"
#include <stdio.h>

int main(void) {
    EditorDocument *doc = editor_create();
    editor_insert(doc, "Bonjour", 7);
    editor_insert(doc, " monde", 6);

    char *text = editor_get_text(doc);
    printf("Texte: '%s'\n", text);  // Attendu : "Bonjour monde"
    free(text);

    editor_delete(doc, 0, 7);  // Supprimer "Bonjour"
    text = editor_get_text(doc);
    printf("Après delete: '%s'\n", text);  // Attendu : " monde"
    free(text);

    editor_destroy(doc);
    return 0;
}
```

**Semaine 2 — Undo/Redo (Phase 1)**

```
TODO-UNDO-001 : editor_undo() → inverser la dernière commande
TODO-UNDO-002 : editor_redo() → rejouer une commande annulée
```

**Semaine 3 — Encoding + Stats (Phase 1/2)**

```
TODO-ENCODING-001 : encoding_is_valid_utf8() → validation séquence UTF-8
TODO-ENCODING-002 : (déjà fait dans le stub, vérifier)
TODO-STATS-001    : editor_compute_stats()   → comptage mots/lignes/paragraphes
TODO-STORAGE-001  : storage_read_file()      → détection d'encodage (UTF-16 BOM)
```

### Prompt pour une IA

```
Tu es expert en C11 et structures de données.
Contexte : je développe un gap buffer en C11 pur pour un éditeur de texte.
Voici la structure : [coller GapBuffer depuis editor.h]
Voici la fonction à implémenter : gap_buffer_ensure_gap()
Elle doit :
  - Vérifier si (gap_end - gap_start) >= needed
  - Si oui, ne rien faire
  - Si non, calculer la nouvelle capacité (doubler si possible)
  - Appeler realloc() et décaler la partie "après le gap"
  - Retourner false si realloc() échoue
Génère uniquement le code C11 de cette fonction, avec des commentaires.
```

### Erreurs à éviter

- ❌ `memcpy(buf, buf + offset, size)` sur des zones qui se chevauchent → utiliser `memmove()`
- ❌ Oublier de mettre à jour `gap_end` quand on agrandit le buffer après realloc
- ❌ Ne pas vérifier que `position <= gap_start` dans `gap_buffer_move_gap()`
- ❌ Calculer la longueur avec `capacity` au lieu de `capacity - (gap_end - gap_start)`

---

## DEV-B — Interface utilisateur Win32 + Scintilla

### Votre mission

Vous rendez l'application utilisable. Sans vous, personne ne voit rien.
Votre code est le **pont entre l'utilisateur et le Core**.

### Vos modules

| Fichier | Description | Priorité |
|---------|-------------|----------|
| `src/adapters/ui_win32/main_window.c` | WinMain, WndProc, menus, layout | 🔴 P1 |
| `src/adapters/ui_scintilla/scintilla_wrapper.c` | Intégration Scintilla | 🔴 P1 |
| Barre de statut | Mots, ligne, colonne | 🟡 P2 |
| Panneau de règles | Affichage RuleReport | 🟡 P2 |
| Boîtes de dialogue | Ouvrir, sauvegarder, config | 🟡 P2 |

### Par où commencer ?

**Semaine 1 — Fenêtre + Scintilla (Phase 1)**

Avant tout : télécharger Scintilla.
- Aller sur [scintilla.org](https://www.scintilla.org/)
- Télécharger `scintilla574.zip` (ou version récente)
- Copier `SciLexer.dll` dans le dossier racine du projet

Ensuite :
```
TODO-STATUSBAR-001   : create_statusbar() → barre de statut Win32
TODO-TOOLBAR-001     : create_toolbar()   → barre d'outils
TODO-SCI-003         : scintilla_create() → configuration initiale
TODO-SCI-004         : configure_defaults → police, indicateurs NLP
```

**Semaine 2 — Synchronisation Core ↔ Scintilla (Phase 1)**

C'est la partie **la plus délicate** : synchroniser le gap buffer et Scintilla.

Règle d'or :
```
Scintilla modifie → intercepter SCN_MODIFIED → mettre à jour le gap buffer
Gap buffer modifie → appeler ui_sync_text() → envoyer vers Scintilla
```

Attention à la boucle infinie ! Utiliser un flag `bool syncing` :
```c
// Dans main_window.c
static bool g_syncing = false;

// Quand on met à jour Scintilla depuis le Core :
g_syncing = true;
SendMessage(ctx->hwnd_scintilla, SCI_SETTEXT, 0, (LPARAM)text);
g_syncing = false;

// Quand Scintilla notifie une modification :
case SCN_MODIFIED:
    if (!g_syncing) {
        // Répercuter dans le gap buffer
        editor_insert(ctx->doc, ...);
    }
    break;
```

```
TODO-SYNC-001          : ui_sync_text()         → gap buffer → Scintilla
TODO-WNDPROC-003       : WM_SIZE                → redimensionner les contrôles
TODO-CMD-002           : ID_FILE_OPEN           → dialogue + chargement fichier
TODO-CMD-003           : ID_FILE_SAVE           → sauvegarde
TODO-NLP-MARKERS-001   : ui_apply_nlp_markers() → soulignements dans Scintilla
TODO-RULES-PANEL-001   : ui_update_rules_panel()→ affichage du RuleReport
```

### Prompt pour une IA

```
Tu es expert en Win32 API et Scintilla en C11.
Je veux créer une fenêtre Win32 avec :
- Un contrôle Scintilla qui occupe 75% de la largeur
- Un panneau de règles (HWND ListBox) qui occupe 25% à droite
- Une barre de statut en bas
Voici mon AppContext : [coller la structure]
Voici ma WndProc actuelle : [coller le code]
Implémente uniquement le traitement de WM_SIZE pour redimensionner ces contrôles.
Utilise GetClientRect() et MoveWindow().
```

### Erreurs à éviter

- ❌ Stocker le texte dans Scintilla comme source de vérité → le texte est dans le gap buffer
- ❌ Appeler `editor_insert()` depuis un callback SCN_MODIFIED déclenché par `SCI_SETTEXT`
- ❌ Bloquer l'UI en attendant une réponse LLM → utiliser `PostMessage` depuis le callback
- ❌ Oublier de libérer les handles GDI (HFONT, HBRUSH) dans WM_DESTROY

---

## DEV-C — LLM & Moteur NLP

### Votre mission

Vous apportez l'**intelligence** au projet. C'est la partie la plus impressionnante
à l'oral. Maîtrisez bien le thread asynchrone et les prompts.

### Vos modules

| Fichier | Description | Priorité |
|---------|-------------|----------|
| `src/adapters/llm_llama_cpp/llm_thread.c` | Thread worker + file de requêtes | 🔴 P1 |
| `src/adapters/hunspell_wrap/hunspell_wrap.c` | Correcteur Hunspell FR | 🟡 P2 |
| `src/adapters/llm_llama_cpp/prompts.c` | Templates de prompts | 🟡 P2 |
| NLP engine (tokenizer, pipeline) | Analyse complète | 🔴 P2 |

### Par où commencer ?

**Semaine 1 — Thread LLM (Phase 1)**

Le thread worker est déjà structuré dans `llm_thread.c`.
Commencez par le faire tourner **sans llama.cpp** (en mode stub) :

1. Vérifier que `thread_create()`, `mutex_create()`, `condvar_wait()` fonctionnent
2. Soumettre une fausse requête avec `llm_submit_request()`
3. Vérifier que le callback est appelé avec la réponse stub

Puis compiler llama.cpp :
```
TODO-LLM-005 : llm_create() → Hunspell_create() + charger le modèle GGUF
TODO-LLM-004 : llm_worker_func() → appel llama_eval() et génération tokens
```

**Installer llama.cpp sous MSYS2 :**
```bash
# Télécharger les binaires pré-compilés pour Windows
# Ou compiler depuis les sources :
git clone https://github.com/ggerganov/llama.cpp
cd llama.cpp
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
mingw32-make
```

**Semaine 2 — Hunspell + Pipeline NLP (Phase 2)**

```bash
# Installation MSYS2
pacman -S mingw-w64-x86_64-hunspell

# Dictionnaire français
# Télécharger fr_FR.aff et fr_FR.dic depuis :
# https://github.com/wooorm/dictionaries/tree/main/dictionaries/fr
# Placer dans data/dicts/
```

```
TODO-HUNSPELL-002 : spellchecker_create() → Hunspell_create()
TODO-HUNSPELL-004 : spellcheck_word()     → Hunspell_spell()
TODO-HUNSPELL-005 : spellcheck_suggest()  → Hunspell_suggest()
TODO-HUNSPELL-006 : spellcheck_analyze()  → tokeniser + analyser tout le texte
```

**Semaine 3 — Prompts + Intégration (Phase 2/3)**

```
TODO-PROMPT-002 : llm_prompt_grammar_check()  → format Mistral/Qwen
TODO-PROMPT-003 : llm_prompt_reformulate()
TODO-PROMPT-004 : llm_prompt_semantic_check()
```

### Tester le thread LLM sans l'UI

```c
// Dans un fichier test_llm_manual.c
#include "include/llm.h"
#include <stdio.h>
#include <windows.h>

void my_callback(const LlmResponse *resp, void *userdata) {
    printf("Réponse LLM reçue !\n");
    printf("Status: %d\n", resp->status);
    printf("Texte: %s\n", resp->text);
}

int main(void) {
    LlmEngine *engine = llm_create("models/mistral.gguf", 4, 4096);
    llm_start_worker(engine);

    LlmRequestId id = llm_submit_request(
        engine,
        LLM_TASK_GRAMMAR_CHECK,
        "Vérifie cette phrase: les règles est importants.",
        my_callback,
        NULL
    );

    printf("Requête %u soumise, attente réponse...\n", id);
    Sleep(30000); // Attendre max 30s

    llm_destroy(engine);
    return 0;
}
```

### Prompt pour une IA

```
Tu es expert en llama.cpp (API C) et en threading Win32.
Je veux intégrer llama.cpp dans un worker thread C11 sur Windows.
Voici ma structure LlmEngine : [coller la struct]
Voici ma boucle worker actuelle (avec TODO) : [coller llm_worker_func]
Je veux implémenter la génération de tokens avec llama.cpp.
Mon modèle est Mistral 7B Instruct en GGUF.
Génère le code C11 pour :
  1. Tokeniser le prompt avec llama_tokenize()
  2. Évaluer avec llama_decode()
  3. Générer les tokens un par un jusqu'à EOS ou max_tokens
  4. Assembler la réponse dans response.text
N'inclure que ce qui remplace le commentaire "TODO-LLM-004".
```

### Erreurs à éviter

- ❌ Appeler `llama_eval()` depuis le thread UI → toujours dans le worker
- ❌ Accéder à `engine->queue` sans verrouiller le mutex
- ❌ Oublier de signaler la condition variable après avoir ajouté une requête
- ❌ Passer un pointeur vers une variable locale comme `userdata` dans le callback

---

## DEV-D — Moteur de règles & Intégration générale

### Votre mission

Vous êtes l'**intégrateur** : vous faites fonctionner tous les modules ensemble.
Vous êtes aussi responsable des tests de tout le projet.

### Vos modules

| Fichier | Description | Priorité |
|---------|-------------|----------|
| `src/adapters/rules_json_cjson/rule_parser.c` | Parser JSON → RuleSet | 🔴 P1 |
| `src/core/rules/section_checker.c` | Vérificateur sections | 🟡 P2 |
| `src/core/rules/count_checker.c` | Vérificateur comptage mots | 🟡 P2 |
| `tests/core/test_editor.c` | Tests module éditeur | 🟡 P2 |
| `tests/core/test_rules.c` | Tests moteur règles | 🟡 P2 |
| `tests/infra/test_infra.c` | Tests infra | 🟡 P2 |
| Intégration NLP → UI | Signaux d'erreur Scintilla | 🔴 P3 |

### Par où commencer ?

**Semaine 1 — Parser JSON (Phase 1)**

```bash
# Installer cJSON sous MSYS2
pacman -S mingw-w64-x86_64-cjson
```

```
TODO-PARSER-001 : Décommenter #include <cjson/cJSON.h>
TODO-PARSER-002 : parse_check_type() → compléter les cas manquants
TODO-PARSER-003 : ruleset_load_from_file() → parser complet JSON → RuleSet
```

**Plan d'implémentation de `ruleset_load_from_file` :**

```c
cJSON *root = cJSON_Parse(json_text);
// 1. Lire meta.document_type, meta.version, meta.author
// 2. Obtenir le tableau "rules"
// 3. Pour chaque règle :
//    - Lire id, description, category, check_type, severity
//    - Lire parameter (ATTENTION : peut être string, objet ou tableau)
//      → si objet/tableau : sérialiser avec cJSON_PrintUnformatted()
//    - Lire flags (case_insensitive)
//    - Lire target_section
cJSON_Delete(root);
```

**Semaine 2 — Checkers (Phase 2)**

```
TODO-CHECK-001 : check_section_exists() → chercher un titre dans le texte
TODO-CHECK-002 : check_word_count_min() → parser { "section": "X", "min_words": N }
TODO-COUNT-003 : check_word_count_max()
TODO-SECTION-004 : check_section_order()
TODO-SECTION-005 : check_heading_format()
```

Pour le vérificateur regex, vous aurez besoin de PCRE2 :
```bash
pacman -S mingw-w64-x86_64-pcre2
```

**Semaine 2 — Tests (Phase 2/3)**

Activez les tests commentés dans `test_editor.c` et `test_rules.c` au fur et
à mesure que les fonctions sont implémentées par vos collègues.

**Vérification d'intégration** — quand DEV-A et DEV-B ont fini Phase 1 :

```c
// Test d'intégration manuel : charger un fichier, évaluer les règles, afficher
EditorDocument *doc = editor_create();
// Charger le fichier avec storage_read_file() + editor_insert()

RuleSet *set = ruleset_load_from_file("data/rule_templates/memoire_licence.json");

char *text = editor_get_text(doc);
size_t len = editor_get_length(doc);

RuleReport *report = rules_evaluate(set, text, len);
// Afficher le rapport
for (size_t i = 0; i < report->result_count; i++) {
    printf("%s : %s — %s\n",
           report->results[i].rule_id,
           rule_status_to_string(report->results[i].status),
           report->results[i].message);
}
```

### Prompt pour une IA

```
Tu es expert en cJSON (bibliothèque C) et en C11.
Voici la structure RuleSet que je dois remplir : [coller RuleSet depuis rules.h]
Voici la structure Rule : [coller Rule]
Je veux parser ce fichier JSON : [coller memoire_licence.json]
La difficulté est que le champ "parameter" peut être :
  - une chaîne simple : "Introduction"
  - un objet JSON : {"level": 1, "case": "uppercase"}
  - un tableau JSON : ["Résumé", "Introduction", ...]
Pour les objets et tableaux, je veux les sérialiser en JSON string avec
cJSON_PrintUnformatted() et les stocker dans rule->parameter.
Génère la fonction ruleset_load_from_file() complète en C11.
```

### Erreurs à éviter

- ❌ Oublier `cJSON_Delete(root)` → fuite mémoire
- ❌ Ne pas tester les NULL de cJSON_GetObjectItem() → segfault sur JSON malformé
- ❌ Utiliser `strlen()` sur `parameter` sans vérifier que ce n'est pas un objet JSON
- ❌ Modifier rule_engine.c pour ajouter de la logique métier → aller dans les checkers

---

## Phases de développement

### Phase 1 — Fondations (DEV-A + DEV-B + DEV-C)

Objectif : application qui démarre, fenêtre qui s'affiche, on peut taper du texte.

| Dev | Livrable |
|-----|----------|
| DEV-A | Gap buffer qui compile et passe les tests unitaires basiques |
| DEV-B | Fenêtre Win32 + Scintilla visible, menus fonctionnels |
| DEV-C | Thread LLM qui démarre et accepte des requêtes (même stub) |
| DEV-D | Parser JSON qui charge memoire_licence.json correctement |

### Phase 2 — Intelligence de base

Objectif : correction ortho + règles structurelles dans l'UI.

### Phase 3 — Fonctionnalités avancées

Objectif : LLM grammaire + règles sémantiques + reformulation.

### Phase 4 — Stabilisation

Objectif : tests passent, pas de crash, rendu propre.

---

## Communication entre développeurs

### Points de synchronisation obligatoires

1. **DEV-A → DEV-B** : dès que `editor_create()` + `editor_insert()` + `editor_get_text()` fonctionnent → DEV-B peut intégrer
2. **DEV-D → DEV-B** : dès que `ruleset_load_from_file()` fonctionne → DEV-B peut afficher le panneau
3. **DEV-C → DEV-B** : dès que `llm_submit_request()` + callback fonctionnent → DEV-B peut afficher les résultats

### Si vous bloquez sur quelque chose

1. Relire le TODO concerné dans le code
2. Relire le header `include/` correspondant
3. Vérifier `ARCHITECTURE.md` pour comprendre le contexte
4. Demander à DEV-D (intégrateur) qui a une vue globale du projet
5. Utiliser un prompt IA (exemples ci-dessus)

---

## Checklist avant la soutenance

Pour chaque développeur, vérifier :

- [ ] Je peux expliquer ce qu'est un gap buffer / l'architecture hexagonale / le thread asynchrone / cJSON
- [ ] Je peux montrer MON code et expliquer chaque ligne
- [ ] Je connais les TODO que je n'ai PAS implémentés et pourquoi
- [ ] Les tests de mon module passent (au moins les tests de base)
- [ ] Mes commits sont réguliers et bien décrits sur GitHub
- [ ] J'ai lu `ARCHITECTURE.md` en entier et je peux le ré-expliquer avec mes propres mots
