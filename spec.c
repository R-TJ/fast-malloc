#include <sys/mman.h>
#include <pthread.h>

#include <stdatomic.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#define EMPTY (void*)0

void *allocate(size_t size);
void unallocate(void *ptr);

static void *allocated;
static void *map;
static size_t offset;
pthread_t wt;

static bool running = true;
static pthread_mutex_t mtx;
static pthread_cond_t cond;
static void** _Atomic pointers;
static size_t* _Atomic sizes;
static size_t _Atomic return_off = 0;
static size_t _Atomic ret_size = 0;

void*
worker(void* call)
{
    while(running)
    {
        pthread_mutex_lock(&mtx);

        while(running && ret_size == 0)
        {
          pthread_cond_wait(&cond, &mtx);
        }

        if(!running)
            return NULL;

        for(size_t i = 0; i < ret_size; i++)
        {
          int ret = madvise(*(pointers + i), *(sizes + i),
                          MADV_DONTNEED);
        }
        int ret = madvise(pointers, 1ULL << 40, MADV_DONTNEED);
        ret = madvise(sizes, 1ULL << 40, MADV_DONTNEED);
        ret_size = 0;
        return_off = 0;

        pthread_mutex_unlock(&mtx);
    }

    return NULL;
}

void init()
{
    map = mmap(NULL, 1ULL << 40, PROT_READ | PROT_WRITE,
               MAP_ANON | MAP_PRIVATE, 0, 0);
    allocated = mmap(NULL, 1ULL << 40, PROT_READ | PROT_WRITE,
                     MAP_ANON | MAP_PRIVATE, 0, 0);
    pointers = mmap(NULL, 1ULL << 40, PROT_READ | PROT_WRITE,
                     MAP_ANON | MAP_PRIVATE, 0, 0);
    sizes = mmap(NULL, 1ULL << 40, PROT_READ | PROT_WRITE,
                     MAP_ANON | MAP_PRIVATE, 0, 0);

    pthread_create(&wt, NULL, worker, NULL);

    pthread_mutex_init(&mtx, NULL);

    pthread_cond_init(&cond, NULL);
}

void *
allocate(size_t size)
{
    if(size < sizeof(size_t))
    {
        return NULL;
    }
    if(offset + size < 1ULL << 40)
    {
        *(size_t *)((char *)allocated + offset) = size;
        void *ptr = (char *)map + offset;
        offset += size;
        return ptr;
    }
    return NULL;
}

void
unallocate(void *ptr)
{
    pthread_mutex_lock(&mtx);

    size_t size = *((char *)allocated + (intptr_t)ptr - (intptr_t)map);

    *(sizes + return_off) = size;
    *(pointers + return_off) = ptr;
    return_off++;
    ret_size++;

    *(sizes + return_off) = sizeof(size_t);
    *(pointers + return_off) = (char*)allocated + (intptr_t)ptr - (intptr_t)map;
    return_off++;
    ret_size++;

    pthread_mutex_unlock(&mtx);

    pthread_cond_signal(&cond);
}
