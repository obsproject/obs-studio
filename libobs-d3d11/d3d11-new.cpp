#include <new>
#include "util/bmem.h"

void * operator new(std::size_t n) throw(std::bad_alloc)
{
	return bmalloc(n);
}

void operator delete(void * p) throw()
{
	bfree(p);
}

void * operator new[](std::size_t n) throw(std::bad_alloc)
{
	return bmalloc(n);
}

void operator delete[](void * p) throw()
{
	bfree(p);
}