#include "spec.c"

int
main()
{
    void **test = malloc(sizeof(void *) * 1000);

    init();

    for(size_t i = 0; i < SIZE_MAX; i++)
    {
        for(size_t j = 0; j < 1000; j++)
        {
            *(test + j) = allocate(100);
        }

        for(size_t j = 0; j < 1000; j++)
        {
            unallocate(*(test + j));
        }
    }

    return 0;
}
