# Guide de contribution — IntelliEditor

## Règles d'architecture

### Includes
- Toujours utiliser `#include "X.h"` en comptant sur `-Iinclude` du Makefile
- **Jamais** de chemins relatifs en cascade comme `../../../include/rules.h`
- Les modules CORE ne doivent pas inclure de headers UI, Win32 ou adapters

### Dépendances externes
Chaque bibliothèque externe est optionnelle et protégée par un guard :
- `#ifdef HAVE_CJSON` pour cJSON
- `#ifdef HAVE_PCRE2` pour PCRE2
- `#ifdef HAVE_HUNSPELL` pour Hunspell
- `#ifdef HAVE_LLAMA` pour llama.cpp

Pour activer une lib : `make ENABLE_CJSON=1`

### Sécurité mémoire
- Toujours forcer la terminaison NUL après strncpy :
```c
strncpy(dest, src, sizeof(dest) - 1);
dest[sizeof(dest) - 1] = '\0';


