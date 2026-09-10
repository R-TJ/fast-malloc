#include "spec.cxx"
#include <chrono>
#include <iostream>

int
main()
{
    size_t **test = (size_t **)malloc(sizeof(void *) * 1000);

    alloc al(2ULL << 25);

    auto start = std::chrono::steady_clock::now();

    for(size_t i = 0; i < 10000; i++)
    {
        for(size_t j = 0; j < 1000; j++)
        {
            *(test + j) = (size_t *)malloc(sizeof(size_t));
        }

        for(size_t j = 0; j < 1000; j++)
        {
            free(test + j);
        }
    }

    auto end = std::chrono::steady_clock::now();
    auto dur
        = std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
              .count();
    std::cout << dur << "ms\n";

    return 0;
}
