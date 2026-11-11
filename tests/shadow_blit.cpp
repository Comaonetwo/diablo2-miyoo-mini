// SPDX-License-Identifier: GPL-3.0-or-later
#include "shadow_blit.hpp"
#include <vector>
#include <string_view>
#include <cstdio>
#include <stdexcept>
#include <chrono>
void reference(std::vector<std::uint8_t>& out,int sw,int sh,const std::vector<std::uint8_t>& src,int w,int h,int bl,int bt,int ax,int ay,std::vector<std::uint16_t>& mask,std::uint16_t id){
 const int bottom=bt+h-1,left=ax+bl+bottom/2,top=ay+bottom/2;
 for(int row=0;row<h;row+=2){int yy=top-row/2;if(yy<0||yy>=sh)continue;for(int x=0;x<w;++x){const int xx=left+x-row/2;if(!src[(h-1-row)*w+x]||xx<0||xx>=sw)continue;auto& m=mask[yy*sw+xx];if(m==id)continue;m=id;auto* p=out.data()+(yy*sw+xx)*4;p[0]/=4;p[1]/=4;p[2]/=4;}}
}
int main(int argc,char**argv)try{
 const bool quick=argc>1&&std::string_view(argv[1])=="--quick";int cases=0;
 for(int w:{1,7,8,9,73})for(int h:{1,2,3,90,91})for(int bt:{-110,-9,0,3})for(int ax:{-100,-9,0,80,150})for(int ay:{-100,0,20,90,160})for(std::uint16_t id:{0,1,7,65535}){
  if(quick&&(w!=9||h!=91||bt!=-9))continue;
  std::vector<std::uint8_t> src(w*h),a(127*103*4),b;std::vector<std::uint16_t> ma(127*103),mb;
  for(std::size_t i=0;i<src.size();++i)src[i]=(i/13)%3?std::uint8_t(i%255+1):0;
  for(std::size_t i=0;i<a.size();++i)a[i]=std::uint8_t(i*31+17);
  for(std::size_t i=0;i<ma.size();++i)ma[i]=i%3==0?id:(id?0:1);
  b=a;mb=ma;
  for(int repeat=0;repeat<2;++repeat){reference(a,127,103,src,w,h,-17,bt,ax,ay,ma,id);d2mini::blit_shadow(b,127,103,src,w,h,-17,bt,ax,ay,mb,id);if(a!=b||ma!=mb)throw std::runtime_error("pixel or stamp mismatch");}
  ++cases;
 }
 // Whole-unit shadows commute: each unit's parts form a union, and every
 // covered RGB pixel is divided by four once for that unit. Reordering the
 // complete groups must preserve pixels even when units/parts overlap.
 for(int pattern=0;pattern<32;++pattern){
  std::vector<std::uint8_t> src(9*11),initial(31*23*4),ordered,reversed;
  for(std::size_t i=0;i<src.size();++i)src[i]=(i+pattern)%3?255:0;
  for(std::size_t i=0;i<initial.size();++i)initial[i]=std::uint8_t(i*17+pattern);
  ordered=initial;reversed=initial;
  for(int reverse=0;reverse<2;++reverse){
   auto& output=reverse?reversed:ordered;std::vector<std::uint16_t> stamps(31*23);
   for(int n=0;n<13;++n){
    int unit=reverse?12-n:n;
    for(int part=0;part<3;++part)d2mini::blit_shadow(output,31,23,src,9,11,-3+part,-8,10+(unit%5),12+(unit%3),stamps,std::uint16_t(n+1));
   }
  }
  if(ordered!=reversed)throw std::runtime_error("whole-unit shadow ordering changed pixels");
 }
 std::printf("PASS: %d shadow cases plus 32 whole-unit ordering cases, overlapping parts, clipping, odd heights and alpha\n",cases);
 if(argc>1&&!quick){
  std::vector<std::uint8_t> src(73*91);for(std::size_t i=0;i<src.size();++i)src[i]=i%3?std::uint8_t(i%255+1):0;
  std::uint64_t sums[2]{};double ms[2]{};
  for(int old=0;old<2;++old){std::vector<std::uint8_t> out(800*600*4,177);std::vector<std::uint16_t> mask(800*600);const auto t=std::chrono::steady_clock::now();for(unsigned n=1;n<=40000;++n){if(old)reference(out,800,600,src,73,91,-20,-70,300,260,mask,std::uint16_t(n));else d2mini::blit_shadow(out,800,600,src,73,91,-20,-70,300,260,mask,std::uint16_t(n));}ms[old]=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t).count();for(auto x:out)sums[old]=sums[old]*131+x;for(auto x:mask)sums[old]=sums[old]*131+x;}
  if(sums[0]!=sums[1])throw std::runtime_error("benchmark mismatch");std::printf("BENCH old_ms=%.3f new_ms=%.3f speedup=%.3f\n",ms[1],ms[0],ms[1]/ms[0]);
 }
}catch(const std::exception& e){std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}
