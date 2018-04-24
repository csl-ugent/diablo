# Memory management
To facilitate dynamic memory management, diablo provides an interface
which can help in the detection of memory leaks. To use this
functionality, a programmer should not use the default functions to
allocate and free dynamic memory, but should revert to the diablo
variants. Their usage is identical to that of the standard c functions.
The only difference is that they start with capital letters.

~~~~
void * Calloc(size_t nmemb, size_t size);
void * Malloc(size_t size);
void * Realloc(void *ptr, size_t size);
void Free(void *ptr);
~~~~
