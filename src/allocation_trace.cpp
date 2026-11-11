// SPDX-License-Identifier: GPL-3.0-or-later
// Failed allocations only: no headers/counters or extra allocations on success.
#include <cstddef>
#include <cstdio>
#include <cerrno>
#include <unistd.h>
#include <unwind.h>
namespace d2mini {extern const char* allocation_phase;}
namespace {
void failed(const char* operation,std::size_t bytes,void* caller){
 const int saved=errno;char message[256];int n=std::snprintf(message,sizeof(message),"allocation_failure: op=%s bytes=%zu phase=%s errno=%d caller=%p\n",operation,bytes,__atomic_load_n(&d2mini::allocation_phase,__ATOMIC_RELAXED),saved,caller);
 if(n>0)write(STDERR_FILENO,message,std::size_t(n)<sizeof(message)?n:sizeof(message)-1);
 static thread_local bool tracing=false;
 if(!tracing){
  tracing=true;struct Frames{unsigned count=0;} frames;
  _Unwind_Backtrace([](_Unwind_Context* ctx,void* data){auto& f=*static_cast<Frames*>(data);if(f.count>=12)return _URC_END_OF_STACK;char line[80];int n=std::snprintf(line,sizeof(line),"allocation_frame %u ip=%p\n",f.count++,reinterpret_cast<void*>(_Unwind_GetIP(ctx)));if(n>0)write(STDERR_FILENO,line,std::size_t(n));return _URC_NO_REASON;},&frames);
  tracing=false;
 }
 errno=saved;
}
}
extern "C" {
void* __real_malloc(std::size_t);
void* __wrap_malloc(std::size_t n){auto*p=__real_malloc(n);if(!p&&n)failed("malloc",n,__builtin_return_address(0));return p;}
void* __real_calloc(std::size_t,std::size_t);
void* __wrap_calloc(std::size_t n,std::size_t size){auto*p=__real_calloc(n,size);if(!p&&n&&size)failed("calloc",n>std::size_t(-1)/size?std::size_t(-1):n*size,__builtin_return_address(0));return p;}
void* __real_realloc(void*,std::size_t);
void* __wrap_realloc(void*old,std::size_t n){auto*p=__real_realloc(old,n);if(!p&&n)failed("realloc",n,__builtin_return_address(0));return p;}
void* __real_aligned_alloc(std::size_t,std::size_t);
void* __wrap_aligned_alloc(std::size_t a,std::size_t n){auto*p=__real_aligned_alloc(a,n);if(!p&&n)failed("aligned_alloc",n,__builtin_return_address(0));return p;}
int __real_posix_memalign(void**,std::size_t,std::size_t);
int __wrap_posix_memalign(void**p,std::size_t a,std::size_t n){int r=__real_posix_memalign(p,a,n);if(r){int saved=errno;errno=r;failed("posix_memalign",n,__builtin_return_address(0));errno=saved;}return r;}
}
