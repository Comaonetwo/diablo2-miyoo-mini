/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Strong C allocation symbols replace Zig's weak SmpAllocator-backed libc.
 * Its small-allocation slabs never return to the OS and cannot coalesce across
 * size classes, exhausting this handheld during repeated room rebuilds. */
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
extern void* dlmalloc(size_t);
extern void* dlcalloc(size_t,size_t);
extern void* dlrealloc(void*,size_t);
extern void dlfree(void*);
extern void* dlmemalign(size_t,size_t);
extern int dlposix_memalign(void**,size_t,size_t);
extern void* dlvalloc(size_t);
extern void* dlpvalloc(size_t);
extern size_t dlmalloc_usable_size(const void*);
extern int dlmalloc_trim(size_t);
void* malloc(size_t n){return dlmalloc(n);}
void free(void* p){dlfree(p);}
void* calloc(size_t n,size_t s){return dlcalloc(n,s);}
void* realloc(void* p,size_t n){return dlrealloc(p,n);}
void* reallocarray(void* p,size_t n,size_t s){if(n&&s>SIZE_MAX/n){errno=ENOMEM;return NULL;}return dlrealloc(p,n*s);}
void* aligned_alloc(size_t a,size_t n){if(!a||(a&(a-1))||n%a){errno=EINVAL;return NULL;}return dlmemalign(a,n);}
int posix_memalign(void** p,size_t a,size_t n){int saved=errno;int r=dlposix_memalign(p,a,n);errno=saved;return r;}
void* memalign(size_t a,size_t n){if(!a||(a&(a-1))){errno=EINVAL;return NULL;}return dlmemalign(a,n);}
void* valloc(size_t n){return dlvalloc(n);}
void* pvalloc(size_t n){return dlpvalloc(n);}
size_t malloc_usable_size(void* p){return dlmalloc_usable_size(p);}
int malloc_trim(size_t n){return dlmalloc_trim(n);}
