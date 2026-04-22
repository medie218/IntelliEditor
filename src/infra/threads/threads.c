/**
 * @file threads.c
 * @brief Abstraction threads et synchronisation (Win32) — INFRA
 *
 * Encapsule CreateThread, CreateMutex, etc. derrière des types opaques.
 * RESPONSABLE : DEV-A
 */

#include "../../include/threads.h"
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

/* ============================================================================
 * STRUCTURES INTERNES
 * ============================================================================ */

struct IeThread {
    HANDLE   handle;    /**< Handle Win32 du thread                          */
    ThreadFunc func;    /**< Fonction exécutée par le thread                 */
    void      *arg;     /**< Argument passé à la fonction                    */
};

struct IeMutex {
    CRITICAL_SECTION cs;  /**< Section critique Win32 (plus légère qu'un Mutex) */
};

struct IeCondVar {
    CONDITION_VARIABLE cv;  /**< Variable de condition Win32 (Vista+)        */
};


/* ============================================================================
 * THREAD
 * ============================================================================ */

/**
 * @brief Fonction wrapper appelée par Win32 pour démarrer le thread.
 *
 * Win32 attend une signature DWORD WINAPI func(LPVOID arg).
 * Notre signature à nous est void func(void *arg).
 * Ce wrapper fait la traduction.
 */
static DWORD WINAPI thread_win32_wrapper(LPVOID arg) {
    IeThread *t = (IeThread *)arg;
    t->func(t->arg);
    return 0;
}

IeThread *thread_create(ThreadFunc func, void *arg) {
    if (!func) return NULL;

    IeThread *t = malloc(sizeof(IeThread));
    if (!t) return NULL;

    t->func = func;
    t->arg  = arg;

    t->handle = CreateThread(
        NULL,                    /* Attributs de sécurité par défaut          */
        0,                       /* Taille de pile par défaut                 */
        thread_win32_wrapper,    /* Fonction de démarrage                     */
        t,                       /* Argument                                  */
        0,                       /* Créer immédiatement                       */
        NULL                     /* Ne pas retourner l'ID                     */
    );

    if (!t->handle) {
        fprintf(stderr, "[ERROR] thread_create: CreateThread échoué (%lu)\n", GetLastError());
        free(t);
        return NULL;
    }

    return t;
}

void thread_join(IeThread *t) {
    if (!t) return;
    WaitForSingleObject(t->handle, INFINITE);
    CloseHandle(t->handle);
    free(t);
}


/* ============================================================================
 * MUTEX (Section critique Win32 — plus efficace qu'un Mutex pour intra-process)
 * ============================================================================ */

IeMutex *mutex_create(void) {
    IeMutex *m = malloc(sizeof(IeMutex));
    if (!m) return NULL;
    InitializeCriticalSection(&m->cs);
    return m;
}

void mutex_destroy(IeMutex *m) {
    if (!m) return;
    DeleteCriticalSection(&m->cs);
    free(m);
}

void mutex_lock(IeMutex *m) {
    if (!m) return;
    EnterCriticalSection(&m->cs);
}

void mutex_unlock(IeMutex *m) {
    if (!m) return;
    LeaveCriticalSection(&m->cs);
}


/* ============================================================================
 * VARIABLE DE CONDITION (Windows Vista+)
 * ============================================================================ */

IeCondVar *condvar_create(void) {
    IeCondVar *cv = malloc(sizeof(IeCondVar));
    if (!cv) return NULL;
    InitializeConditionVariable(&cv->cv);
    return cv;
}

void condvar_destroy(IeCondVar *cv) {
    /* Les CONDITION_VARIABLE Win32 n'ont pas de fonction de destruction */
    free(cv);
}

void condvar_wait(IeCondVar *cv, IeMutex *m) {
    if (!cv || !m) return;
    SleepConditionVariableCS(&cv->cv, &m->cs, INFINITE);
}

void condvar_signal(IeCondVar *cv) {
    if (!cv) return;
    WakeConditionVariable(&cv->cv);
}

void condvar_broadcast(IeCondVar *cv) {
    if (!cv) return;
    WakeAllConditionVariable(&cv->cv);
}
