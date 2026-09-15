#include <sys/mman.h>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <iostream>

class alloc
{
  public:
    alloc(size_t soft_max_mem);
    void *allocate(size_t size);
    void unallocate(void *ptr);

  private:
    void *allocated;
    void *map;
    size_t offset = 0;
    size_t soft_max;
    size_t total_mem = 0;
    void *returning;
    size_t ret_off = 0;
    size_t ret_size = 0;

    std::thread wt;
};

static bool running = true;
static std::mutex mtx;
static std::condition_variable cv;
static std::queue<std::pair<void *, size_t>> freelist;

void
worker()
{
    while(running)
    {
        std::unique_lock<std::mutex> lock(mtx);

        cv.wait(lock, [] { return !freelist.empty() || !running; });

        if(!running && freelist.empty())
            return;

        int ret = madvise(freelist.front().first, freelist.front().second,
                          MADV_DONTNEED);
        freelist.pop();
    }
}

alloc::alloc(size_t soft_max_mem)
{
    map = mmap(nullptr, 1ULL << 40, PROT_READ | PROT_WRITE,
               MAP_ANON | MAP_PRIVATE, 0, 0);
    allocated = mmap(nullptr, 1ULL << 40, PROT_READ | PROT_WRITE,
                     MAP_ANON | MAP_PRIVATE, 0, 0);
    returning = mmap(nullptr, 1ULL << 40, PROT_READ | PROT_WRITE,
                     MAP_ANON | MAP_PRIVATE, 0, 0);
    soft_max = soft_max_mem;

    wt = std::thread(worker);
}

void *
alloc::allocate(size_t size)
{
    if(size < sizeof(size_t))
    {
        return nullptr;
    }
    if(offset + size < 1ULL << 50)
    {
        *(size_t *)((char *)allocated + offset) = size;
        void *ptr = (char *)map + offset;
        offset += size;
        total_mem += size;
        return ptr;
    }
    return 0;
}

void
alloc::unallocate(void *ptr)
{
    size_t size = *((char *)allocated + (intptr_t)ptr - (intptr_t)map);
    {
        std::lock_guard<std::mutex> lock(mtx);
        freelist.push({ ptr, size });
    }
    cv.notify_one();
}

/*void
alloc::unallocate(void *ptr)
{
    if(total_mem > soft_max)
    {
        size_t size = *((char *)allocated + (intptr_t)ptr - (intptr_t)map);
        int ret = madvise(ptr, size, MADV_FREE);
        ret = madvise((char *)allocated + (intptr_t)ptr - (intptr_t)map,
                      sizeof(size_t), MADV_FREE);
        total_mem -= size;
        size_t off = 0;
        for(size_t i = 0; i < ret_size; i++)
        {
            ret = madvise(
                *(void **)((char *)returning + off),
                *(size_t *)((char *)returning + off + sizeof(void *)),
                MADV_DONTNEED);
            total_mem -= *(size_t *)((char *)returning + off + sizeof(void *));
            off += sizeof(void *) + sizeof(size);
            ret_off = 0;
        }
        ret_size = 0;
    }
    else
    {
        size_t size = *((char *)allocated + (intptr_t)ptr - (intptr_t)map);
        *(void **)((char *)returning + ret_off) = ptr;
        ret_off += sizeof(ptr);
        *(size_t *)((char *)returning + ret_off) = size;
        ret_off += sizeof(size);
        ret_size++;
    }
}*/
