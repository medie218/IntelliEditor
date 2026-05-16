# =============================================================================
# Makefile — IntelliEditor
# =============================================================================
#
# Environnement cible : MSYS2 + MinGW-w64 (Windows x64)
# Compilateur         : GCC (C11)
# Outil de build      : GNU Make
#
# UTILISATION :
#   make           → compile le projet complet
#   make test      → compile et lance les tests unitaires
#   make clean     → supprime les fichiers générés
#   make help      → affiche ce message
#
# INSTALLATION MSYS2 :
#   pacman -S mingw-w64-x86_64-gcc
#   pacman -S mingw-w64-x86_64-make
#   pacman -S mingw-w64-x86_64-cmocka      (pour les tests)
#   pacman -S mingw-w64-x86_64-cjson       (à venir)
#   pacman -S mingw-w64-x86_64-pcre2       (à venir)
#   pacman -S mingw-w64-x86_64-hunspell    (à venir)
#
# =============================================================================

# -----------------------------------------------------------------------------
# COMPILATEUR ET FLAGS
# -----------------------------------------------------------------------------

CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -Wpedantic -Wno-unused-parameter
CFLAGS += -Iinclude
EXTRA_INCLUDES ?=
CFLAGS += $(EXTRA_INCLUDES)
CFLAGS += -g  # Symboles de debug (à retirer en release avec -O2)

# Flags Windows (nécessaires pour l'API Win32)
# -DUNICODE et -D_UNICODE permettent d'utiliser les versions Wide des API Win32
WFLAGS  = -DWIN32_LEAN_AND_MEAN -DUNICODE -D_UNICODE

# Bibliothèques de liaison
LIBS    = -lcomctl32 -lcomdlg32 -lgdi32 -luser32 -lkernel32

# ── Adapters externes optionnels ──────────────────────────────
# Usage : make ENABLE_HUNSPELL=1 ENABLE_LLAMA=1 etc.
ENABLE_HUNSPELL ?= 0
ENABLE_LLAMA    ?= 0
ENABLE_CJSON    ?= 0
ENABLE_PCRE2    ?= 0

ifeq ($(ENABLE_HUNSPELL),1)
    LIBS   += -lhunspell-1.7
    CFLAGS += -DHAVE_HUNSPELL
endif

ifeq ($(ENABLE_LLAMA),1)
    LIBS   += -lstdc++ -lm -lws2_32 -lgomp
    CFLAGS += -DHAVE_LLAMA
endif

ifeq ($(ENABLE_CJSON),1)
    LIBS   += -lcjson
    CFLAGS += -DHAVE_CJSON
endif

ifeq ($(ENABLE_PCRE2),1)
    LIBS   += -lpcre2-8
    CFLAGS += -DHAVE_PCRE2
endif

# -----------------------------------------------------------------------------
# RÉPERTOIRES
# -----------------------------------------------------------------------------

SRC_CORE     = src/core/editor src/core/rules src/core/nlp
SRC_ADAPTERS = src/adapters/ui_win32 src/adapters/ui_scintilla \
               src/adapters/llm_llama_cpp src/adapters/hunspell_wrap \
               src/adapters/rules_json_cjson src/adapters/regex_pcre2
SRC_INFRA    = src/infra/config_ini src/infra/encoding \
               src/infra/threads src/infra/storage

BUILD_DIR = build
BIN_DIR   = bin

# -----------------------------------------------------------------------------
# SOURCES ET OBJETS
# -----------------------------------------------------------------------------

# Collecter tous les fichiers .c automatiquement
SRCS_CORE     = $(wildcard $(addsuffix /*.c, $(SRC_CORE)))
SRCS_ADAPTERS = $(wildcard $(addsuffix /*.c, $(SRC_ADAPTERS)))
SRCS_INFRA    = $(wildcard $(addsuffix /*.c, $(SRC_INFRA)))

# Tous les sources sauf main_window.c (qui contient WinMain — ne pas inclure aux tests)
SRCS_LIB = $(SRCS_CORE) $(SRCS_INFRA) \
           $(filter-out src/adapters/ui_win32/main_window.c, $(SRCS_ADAPTERS))

# Source principal (contient WinMain)
SRC_MAIN = src/adapters/ui_win32/main_window.c

# Transformer les .c en .o dans le répertoire build/
OBJS_LIB = $(patsubst %.c, $(BUILD_DIR)/%.o, $(SRCS_LIB))
OBJ_MAIN = $(patsubst %.c, $(BUILD_DIR)/%.o, $(SRC_MAIN))

# Exécutable final
TARGET = $(BIN_DIR)/IntelliEditor.exe

# -----------------------------------------------------------------------------
# RÈGLES PRINCIPALES
# -----------------------------------------------------------------------------

.PHONY: all test clean help dirs

## Cible par défaut : compiler l'exécutable
all: dirs $(TARGET)

$(TARGET): $(OBJS_LIB) $(OBJ_MAIN)
	@echo "[LINK] $@"
	$(CC) $(CFLAGS) $(WFLAGS) -o $@ $^ $(LIBS) -mwindows
	@echo " Build terminé : $@"

## Compiler un fichier objet
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "[CC]   $<"
	$(CC) $(CFLAGS) $(WFLAGS) -c -o $@ $<

## Créer les répertoires nécessaires
dirs:
	@mkdir -p $(BUILD_DIR) $(BIN_DIR)
	@mkdir -p $(addprefix $(BUILD_DIR)/, $(SRC_CORE) $(SRC_ADAPTERS) $(SRC_INFRA))
	@mkdir -p $(BUILD_DIR)/src/adapters/ui_win32

# -----------------------------------------------------------------------------
# TESTS UNITAIRES
# -----------------------------------------------------------------------------

TEST_SRCS  = $(wildcard tests/core/*.c) $(wildcard tests/adapters/*.c) $(wildcard tests/infra/*.c) $(wildcard tests/*.c)
TEST_FLAGS = $(CFLAGS) -Iinclude
TEST_LIBS  = -lcmocka $(LIBS)

## Compiler et lancer tous les tests
test: dirs test_editor test_rules test_infra

test_editor: $(OBJS_LIB) $(BUILD_DIR)/tests/core/test_editor.o
	@echo "[LINK] test_editor"
	$(CC) $(TEST_FLAGS) -o $(BIN_DIR)/test_editor.exe $^ $(TEST_LIBS)
	@echo "[RUN]  test_editor"
	@$(BIN_DIR)/test_editor.exe || true

test_rules: $(OBJS_LIB) $(BUILD_DIR)/tests/core/test_rules.o
	@echo "[LINK] test_rules"
	$(CC) $(TEST_FLAGS) -o $(BIN_DIR)/test_rules.exe $^ $(TEST_LIBS)
	@echo "[RUN]  test_rules"
	@$(BIN_DIR)/test_rules.exe || true

test_infra: $(OBJS_LIB) $(BUILD_DIR)/tests/infra/test_infra.o
	@echo "[LINK] test_infra"
	$(CC) $(TEST_FLAGS) -o $(BIN_DIR)/test_infra.exe $^ $(TEST_LIBS)
	@echo "[RUN]  test_infra"
	@$(BIN_DIR)/test_infra.exe || true

$(BUILD_DIR)/tests/core/test_editor.o: tests/core/test_editor.c
	@mkdir -p $(BUILD_DIR)/tests/core
	$(CC) $(TEST_FLAGS) -c -o $@ $<

$(BUILD_DIR)/tests/core/test_rules.o: tests/core/test_rules.c
	@mkdir -p $(BUILD_DIR)/tests/core
	$(CC) $(TEST_FLAGS) -c -o $@ $<

$(BUILD_DIR)/tests/infra/test_infra.o: tests/infra/test_infra.c
	@mkdir -p $(BUILD_DIR)/tests/infra
	$(CC) $(TEST_FLAGS) -c -o $@ $<

# -----------------------------------------------------------------------------
# NETTOYAGE
# -----------------------------------------------------------------------------

## Supprimer les fichiers générés
clean:
	@echo "[CLEAN] Suppression de build/ et bin/"
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "✅ Nettoyage terminé"

# -----------------------------------------------------------------------------
# AIDE
# -----------------------------------------------------------------------------

help:
	@echo ""
	@echo "╔══════════════════════════════════════════════════════╗"
	@echo "║          IntelliEditor — Makefile                    ║"
	@echo "╠══════════════════════════════════════════════════════╣"
	@echo "║  make          Compiler le projet complet            ║"
	@echo "║  make test     Compiler et lancer les tests          ║"
	@echo "║  make clean    Supprimer les fichiers générés        ║"
	@echo "║  make help     Afficher cette aide                   ║"
	@echo "╚══════════════════════════════════════════════════════╝"
	@echo ""
	@echo "Prérequis MSYS2 :"
	@echo "  pacman -S mingw-w64-x86_64-gcc"
	@echo "  pacman -S mingw-w64-x86_64-make"
	@echo "  pacman -S mingw-w64-x86_64-cmocka  (tests)"
	@echo ""
