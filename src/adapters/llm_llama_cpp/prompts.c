/**
 * @file prompts.c
 * @brief Templates de prompts pour le LLM — ADAPTER / llm_llama_cpp
 *
 * =============================================================================
 * RÔLE
 * =============================================================================
 * Construit les prompts structurés envoyés au LLM selon la tâche.
 * Le prompt engineering est centralisé ici pour faciliter les ajustements.
 *
 * PRINCIPE D'UN BON PROMPT :
 *   - Rôle système clair ("Tu es un correcteur de français...")
 *   - Instruction précise
 *   - Format de réponse attendu (JSON de préférence pour faciliter le parsing)
 *   - Exemple si nécessaire (few-shot)
 *
 * RESPONSABLE : DEV-C
 * =============================================================================
 */

#include "../../../include/llm.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief Prompt système commun à tous les appels LLM.
 *
 * TODO [DEV-C / TODO-PROMPT-001] :
 *   Affiner ce prompt système selon les tests avec le modèle choisi.
 *   Mistral 7B et Qwen2.5 comprennent bien le français mais ont des
 *   formats de chat différents (voir leurs templates Jinja2).
 */
static const char *SYSTEM_PROMPT =
    "Tu es un assistant expert en rédaction académique française. "
    "Tu corriges les fautes de grammaire et de style avec précision. "
    "Tes réponses sont toujours en français, concises et structurées.";

size_t llm_prompt_grammar_check(const char *text, char *out_buf, size_t buf_size) {
    /*
     * TODO [DEV-C / TODO-PROMPT-002] :
     *   Adapter le format selon le modèle utilisé.
     *   Pour Mistral Instruct : [INST] ... [/INST]
     *   Pour Qwen2.5 : <|im_start|>system\n...<|im_end|>\n<|im_start|>user\n...
     *
     *   Format de réponse souhaité (JSON) :
     *   {
     *     "errors": [
     *       {"original": "les règles est", "corrected": "les règles sont",
     *        "type": "accord", "position": 42}
     *     ]
     *   }
     */

    return (size_t)snprintf(out_buf, buf_size,
        "[INST] %s\n\n"
        "Analyse le texte suivant et identifie les fautes grammaticales. "
        "Réponds uniquement avec un objet JSON contenant un tableau 'errors'. "
        "Chaque erreur a les champs: original, corrected, type, position.\n\n"
        "Texte:\n%s\n[/INST]",
        SYSTEM_PROMPT, text);
}

size_t llm_prompt_reformulate(const char *text, char *out_buf, size_t buf_size) {
    /*
     * TODO [DEV-C / TODO-PROMPT-003] :
     *   Demander une reformulation stylistique en conservant le sens.
     *   Retourner JSON : {"reformulation": "...", "changes": [...]}
     */

    return (size_t)snprintf(out_buf, buf_size,
        "[INST] %s\n\n"
        "Reformule la phrase suivante de manière plus académique et claire, "
        "en conservant exactement le même sens. "
        "Réponds avec JSON: {\"reformulation\": \"...\"}\n\n"
        "Phrase: %s\n[/INST]",
        SYSTEM_PROMPT, text);
}

size_t llm_prompt_semantic_check(const char *question,
                                  const char *section,
                                  char       *out_buf,
                                  size_t      buf_size) {
    /*
     * TODO [DEV-C / TODO-PROMPT-004] :
     *   Construire un prompt qui demande une réponse Oui/Non + explication.
     *   Format : {"answer": "yes"|"no"|"partial", "explanation": "..."}
     */

    return (size_t)snprintf(out_buf, buf_size,
        "[INST] %s\n\n"
        "Question sur le texte suivant:\n"
        "Question: %s\n\n"
        "Réponds uniquement avec JSON: "
        "{\"answer\": \"yes\"|\"no\"|\"partial\", \"explanation\": \"...\"}\n\n"
        "Texte de la section:\n%s\n[/INST]",
        SYSTEM_PROMPT, question, section);
}
