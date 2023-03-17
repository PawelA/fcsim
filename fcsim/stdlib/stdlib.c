#include <stddef.h>

extern unsigned char __heap_base;

static size_t heap_end = (size_t)&__heap_base;

void *malloc(size_t size)
{
	unsigned char *ptr;
	
	heap_end += -heap_end & 7;
	ptr = (void *)heap_end;
	heap_end += size;

	return ptr;
}

void free(void *ptr)
{
}
