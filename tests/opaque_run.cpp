// SPDX-License-Identifier: GPL-3.0-or-later
#include "opaque_run.hpp"
#include <array>
#include <vector>
#include <chrono>
#include <cstdio>
#include <stdexcept>
#include <string_view>
struct Rgba{std::uint8_t r,g,b,a;};
int main(int argc,char**argv){
 const bool quick=argc>1&&std::string_view(argv[1])=="--quick";
 std::array<Rgba,256> pal;for(unsigned i=0;i<256;++i)pal[i]={std::uint8_t(i),std::uint8_t(i*7),std::uint8_t(i*19),std::uint8_t(i*31)};
 std::array<std::uint8_t,513> src;for(unsigned i=0;i<src.size();++i)src[i]=std::uint8_t(i*73+5);
 unsigned cases=0;
 for(int count=0;count<=257;++count)for(int offset=0;offset<16;++offset){
  if(quick&&(offset>2||(count>17&&count!=63&&count!=64&&count!=65&&count!=257)))continue;
  std::vector<std::uint8_t> expected(257*4+64,177),out=expected;
  for(int i=0;i<count;++i){auto c=pal[src[i]];c.a=255;std::memcpy(expected.data()+offset+i*4,&c,4);}
  d2mini::opaque_run(out.data()+offset,src.data(),count,pal.data());
  if(out!=expected)throw std::runtime_error("opaque span or guard bytes mismatch");++cases;
 }
 std::printf("PASS: %u opaque palette spans and guards\n",cases);
 if(argc>1&&!quick){
  for(int count:{4,16,64}){
   double times[2]{};std::uint64_t hashes[2]{};
   for(int old=0;old<2;++old){std::vector<std::uint8_t> out(1024*64*4);const auto start=std::chrono::steady_clock::now();
    for(unsigned i=0;i<2000000;++i){auto* p=out.data()+(i%1024)*64*4;const auto* s=src.data()+(i%256);if(old)d2mini::opaque_run_scalar(p,s,count,pal.data());else d2mini::opaque_run(p,s,count,pal.data());}
    times[old]=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();for(auto v:out)hashes[old]=hashes[old]*131+v;
   }
   if(hashes[0]!=hashes[1])throw std::runtime_error("benchmark mismatch");std::printf("BENCH count=%d old_ms=%.3f new_ms=%.3f speedup=%.3f\n",count,times[1],times[0],times[1]/times[0]);
  }
 }
}
