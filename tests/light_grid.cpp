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
 d2d::rules::LightGrid reused;
 reused.reset(12,18,5,7);
 reused.stamp(12*8,18*8,72,230);
 std::fill(reused.blocked.begin(),reused.blocked.end(),1);
 for(int side:{7,3,12}){
  reused.reset(-12,8,47,side);
  for(int y=0;y<side;++y)for(int x=0;x<side;++x){
   const int sx=reused.origin_x+x,sy=reused.origin_y+y;
   if(reused.is_blocked(sx,sy)||reused.at(sx,sy)!=47)
    throw std::runtime_error("previous room leaked into new light grid");
  }
  if(!reused.is_blocked(reused.origin_x-1,reused.origin_y))
   throw std::runtime_error("outside room considered unblocked");
  reused.blocked.back()=1;reused.values.front()=255;
 }
 std::puts("PASS: clipped/offscreen/saturated lights match legacy grid including negative coordinates");
}catch(const std::exception&e){std::fprintf(stderr,"%s\n",e.what());return 1;}
