# 🚀 Consignes de Reprise — Projet IntelliEditor

Ce document est destiné au développeur qui reprend le projet avec **Gemini CLI**. Il résume l'état actuel, les défis techniques et les étapes pour une présentation réussie à l'université.

## 📌 État Actuel du Projet
- **Core :** Gap Buffer opérationnel avec Undo/Redo, recherche et remplacement.
- **UI :** Interface Win32 + Scintilla stable. La barre d'outils a été améliorée avec des libellés textuels pour éviter les "petits carreaux".
- **Règles :** Moteur fonctionnel. J'ai ajouté un fichier de règles universelles en français (`data/rule_templates/regles_universelles_fr.json`).
- **IA :** Pipeline LLM (llama.cpp) intégré avec parsing JSON des réponses.

## 🛠️ Défis Techniques Immédiats
1. **Compilation Silencieuse :** `gcc` échoue actuellement sans message d'erreur sur `gap_buffer.c`. 
   - *Piste :* Vérifier les encodages de fichiers (UTF-8 vs ANSI) ou un conflit de headers dans `include/editor.h`.
   - *Action :* Utiliser `mingw32-make -v` ou tester avec un autre compilateur (Clang) pour obtenir les logs.
2. **Polissage de l'UI :** L'utilisateur souhaite une interface plus "traitement de texte" (palettes de navigation, personnalisation du texte).
   - *Piste :* Étendre `main_window.c` pour inclure des boîtes de dialogue de police (`ChooseFont`) et de recherche.

## 🎯 Objectifs pour la Présentation (Université)
- **Moteur de Règles :** Montrer le chargement du fichier `regles_universelles_fr.json` via le menu "Outils > Charger règles". Cela validera la structure (Introduction/Conclusion) et le style (évitement du "Je").
- **IA Locale :** Faire une démonstration de correction grammaticale "100% Offline" pour souligner la confidentialité.
- **Export :** Montrer l'export RTF fonctionnel pour prouver la compatibilité avec Word/LibreOffice.

## 📦 Instructions Git pour ton collègue
1. **Pousser le travail actuel :**
   ```bash
   git add .
   git commit -m "relais: core stable, règles FR ajoutées, nettoyage UI"
   git push origin main
   ```
2. **Reprendre avec Gemini CLI :**
   Une fois sur son PC, il doit lancer :
   `gemini "Lis CONSIGNES_REPRISE.md et aide-moi à résoudre l'erreur de compilation de gap_buffer.c puis à finaliser l'UI pour la présentation."`

## 💡 Note sur les Règles Universelles
Le fichier `data/rule_templates/regles_universelles_fr.json` contient :
- Vérification de l'espace avant `: ; ! ?`
- Alerte sur l'usage de "Je" (style académique).
- Vérification de la présence des sections "Introduction" et "Conclusion".
- Compte de mots minimum (100 mots).

---
*Signé : Gemini CLI (Relais technique)*
