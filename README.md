Uses memory mapping and a bump style allocator to allow for really fast allocation. Also allows keeping past memory unlike a normal bump allocator.

The class is initialised with a number that represents the maximum amount of memory,in bytes, that should be used at once. Once it goes past that it will return the memory to the OS which could take a long time depending on how many allocations are being returned.

If the program allocates more that 1pb total over the whole lifespan if will break. I will fix this at some time.

any allocation smaller than the size of a "size_t" will fail becuse of metadata limits.

 I will add multithread support sometime

**USAGE**

just include the "spec.cxx" file into your program and create a "alloc". 

alloc::allocate() is the equilavent of malloc

alloc::unallocate() is the equilavent of free  

ive given up on this. it works but there is a memory leak somewhere and i cant figure out where. this would save a couple nanoseconds per allocation compared to glibc so im no longer working on it.
