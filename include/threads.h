/**
 * @file threads.h
 * @brief Contrat public — Abstraction des threads et synchronisation — INFRA
 *
 * =============================================================================
 * RESPONSABILITÉ
 * =============================================================================
 * Fournit une couche d'abstraction fine au-dessus des threads Win32.
 * Permet au code Core/Adapters de créer des threads et des mutex sans
 * dépendre directement de l'API Win32 (meilleure portabilité conceptuelle).
 *
 * APPARTIENT À LA COUCHE : INFRA
 * AUTEUR(S) RESPONSABLE(S) : DEV-A (intégration par DEV-C pour le LLM)
 *
 * =============================================================================
 */

#ifndef INTELLIEDITOR_THREADS_H
#define INTELLIEDITOR_THREADS_H

#include <stdbool.h>
#include <stdint.h>

/* Types opaques — les implémentations utilisent HANDLE Win32 en interne */
typedef struct IeThread  IeThread;
typedef struct IeMutex   IeMutex;
typedef struct IeCondVar IeCondVar;

/** Signature d'une fonction de thread. */
typedef void (*ThreadFunc)(void *arg);

/**
 * @brief Crée et démarre un thread.
 *
 * @param func  Fonction à exécuter dans le thread.
 * @param arg   Argument passé à la fonction.
 * @return      Handle du thread, ou NULL si erreur.
 */
IeThread *thread_create(ThreadFunc func, void *arg);

/**
 * @brief Attend la fin d'un thread et libère sa ressource.
 *
 * @param t  Handle du thread.
 */
void thread_join(IeThread *t);

/** Crée un mutex. */
IeMutex *mutex_create(void);

/** Détruit un mutex. */
void mutex_destroy(IeMutex *m);

/** Verrouille un mutex. */
void mutex_lock(IeMutex *m);

/** Déverrouille un mutex. */
void mutex_unlock(IeMutex *m);

/** Crée une variable de condition. */
IeCondVar *condvar_create(void);

/** Détruit une variable de condition. */
void condvar_destroy(IeCondVar *cv);

/** Attend sur une condition (déverrouille le mutex pendant l'attente). */
void condvar_wait(IeCondVar *cv, IeMutex *m);

/** Réveille un thread en attente sur la condition. */
void condvar_signal(IeCondVar *cv);

/** Réveille tous les threads en attente sur la condition. */
void condvar_broadcast(IeCondVar *cv);

#endif /* INTELLIEDITOR_THREADS_H */
