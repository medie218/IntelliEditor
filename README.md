# IntelliEditor

> Éditeur de texte intelligent hors ligne pour Windows — Projet C Avancé L3 GL / UDBL 2025-2026

[![Langage](https://img.shields.io/badge/Langage-C11-blue)]()
[![Plateforme](https://img.shields.io/badge/Plateforme-Windows%2010%2F11-blue)]()
[![Architecture](https://img.shields.io/badge/Architecture-Hexagonale-green)]()
[![Statut](https://img.shields.io/badge/Statut-En%20cours%20(base%20de%20projet)-yellow)]()

## Build

Pour lancer la compilation sous MSYS2 / MinGW64 :

- Avec Bash : `./make`
- Avec cmd.exe : `make.bat`

Ces wrappers pointent vers le `make` disponible dans `C:\msys64\usr\bin\make.exe` ou `mingw32-make.exe`.

---

---

## Présentation

IntelliEditor est un logiciel de traitement de texte intelligent développé en **C11 pur**,
fonctionnant **entièrement hors ligne** sur Windows. Il combine :

- Un **éditeur de texte** performant (gap buffer, undo/redo illimité)
- Une **correction orthographique** en temps réel (Hunspell FR)
- Un **moteur de règles métier** configurable en JSON
- Une **assistance par LLM local** (Mistral, Qwen2.5 via llama.cpp)
- Une **interface Win32 + Scintilla** native

Ce dépôt contient la **base de projet** : structure, contrats, stubs et guides.
Le code métier est à compléter progressivement par l'équipe.

---

## Équipe

| Identifiant | Rôle | Modules principaux |
|-------------|------|-------------------|
| **DEV-A** | Infrastructure & Éditeur | `src/core/editor/`, `src/infra/` |
| **DEV-B** | Interface utilisateur | `src/adapters/ui_win32/`, `src/adapters/ui_scintilla/` |
| **DEV-C** | LLM & NLP | `src/adapters/llm_llama_cpp/`, `src/adapters/hunspell_wrap/` |
| **DEV-D** | Règles & Intégration | `src/core/rules/`, `src/adapters/rules_json_cjson/`, `tests/` |

---

## Structure du projet

```
IntelliEditor/
│
├── include/                   ← CONTRATS (headers publics)
│   ├── editor.h               ← API éditeur (gap buffer, undo/redo, stats)
│   ├── rules.h                ← API moteur de règles
│   ├── nlp.h                  ← API NLP (interfaces abstraites)
│   ├── llm.h                  ← API LLM asynchrone
│   ├── ui.h                   ← API interface utilisateur
│   ├── config.h               ← API configuration INI
│   ├── encoding.h             ← API encodage UTF-8/UTF-16
│   ├── threads.h              ← API threads et synchronisation
│   └── storage.h              ← API fichiers
│
├── src/
│   ├── core/                  ← CŒUR MÉTIER (aucune dépendance externe)
│   │   ├── editor/
│   │   │   └── gap_buffer.c   ← Gap buffer, undo/redo, stats
│   │   ├── rules/
│   │   │   ├── rule_engine.c  ← Orchestrateur d'évaluation
│   │   │   ├── section_checker.c
│   │   │   └── count_checker.c
│   │   └── nlp/               ← Interfaces NLP (implémentées par les adapters)
│   │
│   ├── adapters/              ← INTÉGRATIONS EXTERNES
│   │   ├── ui_win32/
│   │   │   └── main_window.c  ← WinMain, WndProc, menus
│   │   ├── ui_scintilla/
│   │   │   └── scintilla_wrapper.c
│   │   ├── llm_llama_cpp/
│   │   │   ├── llm_thread.c   ← Thread worker LLM + file de requêtes
│   │   │   └── prompts.c      ← Templates de prompts
│   │   ├── hunspell_wrap/
│   │   │   └── hunspell_wrap.c
│   │   ├── rules_json_cjson/
│   │   │   └── rule_parser.c  ← Parser JSON → RuleSet
│   │   └── regex_pcre2/       ← Wrapper PCRE2 (à créer)
│   │
│   └── infra/                 ← SERVICES TECHNIQUES
│       ├── config_ini/
│       │   └── config.c
│       ├── encoding/
│       │   └── encoding.c
│       ├── threads/
│       │   └── threads.c
│       └── storage/
│           └── storage.c
│
├── tests/
│   ├── core/
│   │   ├── test_editor.c
│   │   └── test_rules.c
│   └── infra/
│       └── test_infra.c
│
├── data/
│   └── rule_templates/
│       └── memoire_licence.json   ← Exemple de fichier de règles
│
├── docs/
│   └── (assets, diagrammes)
│
├── Makefile
├── README.md                  ← Ce fichier
├── ARCHITECTURE.md            ← Explication de l'architecture hexagonale
└── DEV_GUIDE.md               ← Guide de travail pour chaque développeur
```

---

## Architecture en un coup d'œil

```
┌─────────────────────────────────────────────────────────────────┐
│                      ADAPTERS (UI)                              │
│              Win32 + Scintilla + Panneau règles                 │
├──────────────────┬──────────────────┬───────────────────────────┤
│   CORE / editor  │   CORE / rules   │    CORE / nlp             │
│   gap_buffer     │   rule_engine    │    (interfaces)           │
│   undo_redo      │   checkers       │                           │
├──────────────────┴──────────────────┴───────────────────────────┤
│                    ADAPTERS (IA & données)                      │
│        llm_llama_cpp  |  hunspell_wrap  |  rules_json_cjson     │
├─────────────────────────────────────────────────────────────────┤
│                    INFRA (services)                             │
│         config_ini  |  encoding  |  threads  |  storage        │
└─────────────────────────────────────────────────────────────────┘
```

La règle d'or : **les flèches de dépendance ne remontent jamais.**
Le Core ne dépend de rien. Les Adapters dépendent du Core. L'Infra dépend de rien.

---

## Prérequis et installation

### 1. Installer MSYS2

Télécharger depuis [https://www.msys2.org/](https://www.msys2.org/), puis dans le terminal MSYS2 MinGW64 :

```bash
# Mettre à jour les paquets
pacman -Syu

# Outils de développement essentiels
pacman -S mingw-w64-x86_64-gcc
pacman -S mingw-w64-x86_64-make
pacman -S mingw-w64-x86_64-gdb

# Dépendances du projet (à installer au fur et à mesure des besoins)
pacman -S mingw-w64-x86_64-cmocka        # Tests unitaires
# À venir :
# pacman -S mingw-w64-x86_64-cjson       # Parser JSON (DEV-D)
# pacman -S mingw-w64-x86_64-pcre2       # Regex (DEV-D)
# pacman -S mingw-w64-x86_64-hunspell    # Correcteur ortho (DEV-C)
```

### 2. Cloner le projet

```bash
git clone https://github.com/VOTRE_ORG/IntelliEditor.git
cd IntelliEditor
```

### 3. Bibliothèques tierces à placer manuellement

| Bibliothèque | Où l'obtenir | Où placer |
|---|---|---|
| `SciLexer.dll` | [Scintilla.org](https://www.scintilla.org/) | Même dossier que `.exe` |
| Modèle LLM (`.gguf`) | [HuggingFace](https://huggingface.co/) | Chemin dans `config.ini` |
| `fr_FR.aff` + `fr_FR.dic` | [wooorm/dictionaries](https://github.com/wooorm/dictionaries) | `data/dicts/` |

---

## Compilation

```bash
# Terminal MSYS2 MinGW64 — à la racine du projet

# Compiler le projet complet
make

# Lancer les tests unitaires
make test

# Nettoyer les fichiers générés
make clean

# Aide
make help
```

L'exécutable est généré dans `bin/IntelliEditor.exe`.

---

## Lancer les tests

```bash
make test
```

Cela compile et exécute :
- `bin/test_editor.exe` → tests du gap buffer et undo/redo
- `bin/test_rules.exe`  → tests du moteur de règles
- `bin/test_infra.exe`  → tests de l'encodage, config, storage

Au début, la plupart des tests retourneront `SKIP` ou `FAIL` car les stubs
ne sont pas encore implémentés. C'est **normal et attendu**.

---

## Workflow Git recommandé

```bash
# Chaque développeur travaille sur sa branche
git checkout -b dev-a   # ou dev-b, dev-c, dev-d

# Commits atomiques et bien décrits
git add src/core/editor/gap_buffer.c
git commit -m "feat(editor): implémenter gap_buffer_ensure_gap (TODO-GAPBUF-001)"

# Pull Request vers main quand une fonctionnalité est prête
# → demander une review à DEV-D (intégrateur)
```

**Convention de commits :**
```
feat(module): description courte
fix(module): ce qui a été corrigé
test(module): tests ajoutés ou modifiés
docs(module): documentation mise à jour
refactor(module): refactorisation sans changement fonctionnel
```

---

## Références

- [Architecture hexagonale](https://alistair.cockburn.us/hexagonal-architecture/) — Alistair Cockburn
- [Gap Buffer](https://www.emacswiki.org/emacs/GapBuffer) — Wikipedia
- [Scintilla documentation](https://www.scintilla.org/ScintillaDoc.html)
- [llama.cpp](https://github.com/ggerganov/llama.cpp) — API C pour LLM GGUF
- [cmocka](https://cmocka.org/) — Framework de tests unitaires pour C
- [cJSON](https://github.com/DaveGamble/cJSON) — Parser JSON léger en C

---

## Licence

Projet pédagogique — Université Don Bosco de Lubumbashi, L3 Génie Logiciel 2025-2026.
