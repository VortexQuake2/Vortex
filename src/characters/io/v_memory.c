#include "../../g_local.h"
#include "gds.h"

#include <pthread.h>

static pthread_once_t mem_mutex_once = PTHREAD_ONCE_INIT;
static pthread_mutex_t mem_mutex_free;
static pthread_mutex_t mem_mutex_malloc;

static void vrx_prepare_memory_mutexes(void)
{
    pthread_mutex_init(&mem_mutex_malloc, NULL);
    pthread_mutex_init(&mem_mutex_free, NULL);
}

void Mem_PrepareMutexes()
{
    pthread_once(&mem_mutex_once, vrx_prepare_memory_mutexes);
}

void *vrx_malloc(size_t Size, int Tag)
{
    void *memory;

    Mem_PrepareMutexes();
    pthread_mutex_lock(&mem_mutex_malloc);
    memory = gi.TagMalloc(Size, Tag);
    pthread_mutex_unlock(&mem_mutex_malloc);

    return memory;
}

void vrx_free(void *mem)
{
    if (!mem)
        return;

    Mem_PrepareMutexes();
    pthread_mutex_lock(&mem_mutex_free);
    gi.TagFree(mem);
    pthread_mutex_unlock(&mem_mutex_free);
}
