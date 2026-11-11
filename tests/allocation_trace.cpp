// SPDX-License-Identifier: GPL-3.0-or-later
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
namespace d2mini {const char* allocation_phase="allocation trace regression";}
static bool reject=false;
extern "C" {
void* __real_malloc(std::size_t n){if(reject){errno=ENOMEM;return nullptr;}return std::malloc(n);}
void* __real_calloc(std::size_t n,std::size_t s){if(reject){errno=ENOMEM;return nullptr;}return std::calloc(n,s);}
void* __real_realloc(void*p,std::size_t n){if(reject){errno=ENOMEM;return nullptr;}return std::realloc(p,n);}
void* __real_aligned_alloc(std::size_t a,std::size_t n){if(reject){errno=ENOMEM;return nullptr;}void*p=nullptr;return posix_memalign(&p,a,n)?nullptr:p;}
int __real_posix_memalign(void**p,std::size_t a,std::size_t n){if(reject)return ENOMEM;return posix_memalign(p,a,n);}
void* __wrap_malloc(std::size_t);void* __wrap_calloc(std::size_t,std::size_t);void* __wrap_realloc(void*,std::size_t);void* __wrap_aligned_alloc(std::size_t,std::size_t);int __wrap_posix_memalign(void**,std::size_t,std::size_t);
}
#define CHECK(x) do{if(!(x))return __LINE__;}while(false)
int main(){
 void*p=__wrap_malloc(31);CHECK(p);p=__wrap_realloc(p,100);CHECK(p);std::free(p);
 p=__wrap_calloc(10,4);CHECK(p&&static_cast<unsigned char*>(p)[39]==0);std::free(p);
 p=__wrap_aligned_alloc(16,32);CHECK(p&&std::size_t(p)%16==0);std::free(p);
 CHECK(__wrap_posix_memalign(&p,16,32)==0);std::free(p);
 reject=true;CHECK(!__wrap_malloc(42)&&errno==ENOMEM);CHECK(!__wrap_calloc(7,9)&&errno==ENOMEM);
 CHECK(!__wrap_realloc(nullptr,64)&&errno==ENOMEM);CHECK(!__wrap_aligned_alloc(16,64)&&errno==ENOMEM);
 errno=EINTR;CHECK(__wrap_posix_memalign(&p,16,64)==ENOMEM&&errno==EINTR);
 puts("PASS: allocation tracing forwards requests and preserves failure results/errno");return 0;
}
