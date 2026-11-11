// SPDX-License-Identifier: GPL-3.0-or-later
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <thread>
#include <vector>
extern "C" {void* dlmalloc(size_t);void* dlcalloc(size_t,size_t);void* dlrealloc(void*,size_t);void dlfree(void*);int dlposix_memalign(void**,size_t,size_t);int dlmalloc_trim(size_t);size_t dlmalloc_footprint();}
static void check(bool x){if(!x)std::abort();}
static void worker(){for(int r=0;r<100;++r){std::vector<void*> v;for(int n=0;n<64;++n){auto size=size_t(700+n*631);auto*p=(unsigned char*)dlcalloc(size,1);check(p&&p[size-1]==0);memset(p,n,size);auto*q=(unsigned char*)dlrealloc(p,size+513);check(q&&q[size-1]==n);v.push_back(q);}for(auto p:v)dlfree(p);void*p=nullptr;check(dlposix_memalign(&p,256,999)==0&&size_t(p)%256==0);dlfree(p);}}
int main(){std::vector<std::thread> t;for(int n=0;n<4;++n)t.emplace_back(worker);for(auto&x:t)x.join();dlmalloc_trim(0);check(dlmalloc_footprint()<8*1024*1024);auto*p=dlmalloc(12*1024*1024);check(p);memset(p,42,12*1024*1024);dlfree(p);dlmalloc_trim(0);check(dlmalloc_footprint()<8*1024*1024);puts("PASS: zeroing, realloc retention, alignment, concurrent reuse and large-region release");}
