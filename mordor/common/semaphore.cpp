// Copyright (c) 2009 - Decho Corp.

#include "mordor/common/pch.h"

#include "semaphore.h"

#include "assert.h"
#include "exception.h"

#ifndef WINDOWS
#include <errno.h>
#endif

#ifdef FREEBSD
#include <sys/sem.h>
#endif

#ifdef OSX
#include <mach/mach_init.h>
#include <mach/task.h>
#endif

Semaphore::Semaphore(unsigned int count)
{
#ifdef WINDOWS
    m_semaphore = CreateSemaphore(NULL, count, 2147483647, NULL);
    if (m_semaphore == NULL) {
        THROW_EXCEPTION_FROM_LAST_ERROR_API("CreateSemaphore");
    }
#elif defined(OSX)
    m_task = mach_task_self();
    if (semaphore_create(m_task, &m_semaphore, SYNC_POLICY_FIFO, count)) {
        THROW_EXCEPTION_FROM_LAST_ERROR_API("semaphore_create");
    }
#elif defined(FREEBSD)
    m_semaphore = semget(IPC_PRIVATE, 1, IPC_CREAT | 0600);
    if (m_semaphore < 0) {
        THROW_EXCEPTION_FROM_LAST_ERROR_API("semget");
    }
    semun init;
    init.val = count;
    if (semctl(m_semaphore, 0, SETVAL, init) < 0) {
        semctl(m_semaphore, 0, IPC_RMID);
        THROW_EXCEPTION_FROM_LAST_ERROR_API("semctl");
    }
#else
    if (sem_init(&m_semaphore, 0, count)) {
        THROW_EXCEPTION_FROM_LAST_ERROR_API("sem_init");
    }
#endif
}

Semaphore::~Semaphore()
{
#ifdef WINDOWS
    BOOL bRet = CloseHandle(m_semaphore);
    ASSERT(bRet);
#elif defined(OSX)
    int rc = semaphore_destroy(m_task, m_semaphore);
    ASSERT(!rc);
#elif defined(FREEBSD)
    int rc = semctl(m_semaphore, 0, IPC_RMID);
    ASSERT(rc >= 0);
#else
    int rc = sem_destroy(&m_semaphore);
    ASSERT(!rc);
#endif
}

void
Semaphore::wait()
{
#ifdef WINDOWS
    DWORD dwRet = WaitForSingleObject(m_semaphore, INFINITE);
    if (dwRet != WAIT_OBJECT_0) {
        THROW_EXCEPTION_FROM_LAST_ERROR_API("WaitForSingleObject");
    }
#elif defined(OSX)
    while (true) {
        if (!semaphore_wait(m_semaphore))
            return;
        if (errno != EINTR) {
            THROW_EXCEPTION_FROM_LAST_ERROR_API("semaphore_wait");
        }
    }
#elif defined(FREEBSD)
    sembuf op;
    op.sem_num = 0;
    op.sem_op = -1;
    op.sem_flg = 0;
    while (true) {
        if (!semop(m_semaphore, &op, 1))
            return;
        if (errno != EINTR) {
            THROW_EXCEPTION_FROM_LAST_ERROR_API("semop");
        }       
    }
#else
    while (true) {
        if (!sem_wait(&m_semaphore))
            return;
        if (errno != EINTR) {
            THROW_EXCEPTION_FROM_LAST_ERROR_API("sem_wait");
        }
    }
#endif
}

void
Semaphore::notify()
{
#ifdef WINDOWS
    if (!ReleaseSemaphore(m_semaphore, 1, NULL)) {
        THROW_EXCEPTION_FROM_LAST_ERROR_API("ReleaseSemaphore");
    }
#elif defined(OSX)
    semaphore_signal(m_semaphore);
#elif defined(FREEBSD)
    sembuf op;
    op.sem_num = 0;
    op.sem_op = 1;
    op.sem_flg = 0;
    if (semop(m_semaphore, &op, 1)) {
        THROW_EXCEPTION_FROM_LAST_ERROR_API("semop");
    }
#else
    if (sem_post(&m_semaphore)) {
        THROW_EXCEPTION_FROM_LAST_ERROR_API("sem_post");
    }
#endif
}
