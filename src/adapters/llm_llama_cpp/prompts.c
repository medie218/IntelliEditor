/**
 * @file prompts.c
 * @brief Templates de prompts pour le LLM — ADAPTER / llm_llama_cpp
 *
 * ====================================================================== * RÔLE
 * ====================================================================== * Construit les prompts structurés envoyés au LLM selon la tâche.
 * Le prompt engineering est centralisé ici pour faciliter les ajustements.
 *
 * PRINCIPE D'UN BON PROMPT :
 *   - Rôle système clair ("Tu es un correcteur de français...")
 *   - Instruction précise
 *   - Format de réponse attendu (JSON de préférence pour faciliter le parsing)
 *   - Exemple si nécessaire (few-shot)
 *
 * RESPONSABLE : DEV-C
 * ====================================================================== */

#include "llm.h"
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
        "<|im_start|>system\n%s<|im_end|>\n"
        "<|im_start|>user\n"
        "Analyse ce texte et retourne UNIQUEMENT ce JSON :\n"
        "{\"errors\": [{\"original\": \"mot fautif\", "
        "\"corrected\": \"correction\", "
        "\"type\": \"orthographe|grammaire|style\", "
        "\"position\": 0}]}\n\n"
        "Texte a analyser:\n%s\n"
        "<|im_end|>\n"
        "<|im_start|>assistant\n",
        SYSTEM_PROMPT, text);
}

size_t llm_prompt_reformulate(const char *text, char *out_buf, size_t buf_size) {
    /*
     * TODO [DEV-C / TODO-PROMPT-003] :
     *   Demander une reformulation stylistique en conservant le sens.
     *   Retourner JSON : {"reformulation": "...", "changes": [...]}
     */

   return (size_t)snprintf(out_buf, buf_size,
        "<|im_start|>system\n%s<|im_end|>\n"
        "<|im_start|>user\n"
        "Reformule cette phrase en français académique. "
        "Conserve le sens exact. "
        "Retourne UNIQUEMENT ce JSON :\n"
        "{\"reformulation\": \"ta reformulation ici\", "
        "\"changes\": [\"changement 1\", \"changement 2\"]}\n\n"
        "Phrase originale:\n%s\n"
        "<|im_end|>\n"
        "<|im_start|>assistant\n",
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
        "<|im_start|>system\n%s<|im_end|>\n"
        "<|im_start|>user\n"
        "Reponds a cette question sur le texte ci-dessous.\n"
        "Question: %s\n\n"
        "Retourne UNIQUEMENT ce JSON :\n"
        "{\"answer\": \"yes\", \"explanation\": \"explication courte\"}\n"
        "Les valeurs possibles pour 'answer' sont : yes, no, partial.\n\n"
        "Texte:\n%s\n"
        "<|im_end|>\n"
        "<|im_start|>assistant\n",
        SYSTEM_PROMPT, question, section);   
}

