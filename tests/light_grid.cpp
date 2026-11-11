// SPDX-License-Identifier: GPL-3.0-or-later
#include <light.hpp>
#include <cstdio>
#include <stdexcept>
int main()try{
 std::uint32_t seed=17;
 const auto random=[&](int bound){seed=seed*1664525u+1013904223u;return int(seed%std::uint32_t(bound));};
 for(int size:{1,7,48,110})for(int pass=0;pass<150;++pass){
  d2d::rules::LightGrid a,b;const int cx=random(1001)-500,cy=random(1001)-500,ambient=random(256);
  a.reset(cx,cy,ambient,size);b.reset(cx,cy,ambient,size);
  for(int k=0;k<40;++k){
   const int x=cx*8+random(2401)-1200,y=cy*8+random(2401)-1200,r=random(258),power=random(256);
   a.stamp(x,y,r,power);b.stamp_legacy(x,y,r,power);
   if(a.values!=b.values)throw std::runtime_error("light grid differs from original lattice");
  }
  for(int k=0;k<40;++k){a.stamp(cx*8+3,cy*8+7,144,255);b.stamp_legacy(cx*8+3,cy*8+7,144,255);}
  if(a.values!=b.values)throw std::runtime_error("saturated light grid differs");
 }
 std::puts("PASS: clipped/offscreen/saturated lights match legacy grid including negative coordinates");
}catch(const std::exception&e){std::fprintf(stderr,"%s\n",e.what());return 1;}
