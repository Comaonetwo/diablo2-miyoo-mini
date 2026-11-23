// SPDX-License-Identifier: GPL-3.0-or-later
#include "motion_samples.hpp"
#include "roof_cutout.hpp"
#include "colored_lights.hpp"
#include <cassert>
#include <cstdio>
int main(){
 using M=d2mini::MotionSamples;M m;int level,other;
 const auto a=M::key(M::Monster,42),b=M::key(M::Missile,42);
 m.begin(&level);m.add(b,5,7);m.add(a,1,2);m.finish();
 auto p=m.at(&level,a,1.4f,2.8f,.5f);assert(std::abs(p[0]-1.2f)<.0001f&&std::abs(p[1]-2.4f)<.0001f);
 p=m.at(&level,b,5.4f,7,.5f);assert(std::abs(p[0]-5.2f)<.0001f); // types cannot collide
 assert(m.at(&other,a,1.4f,2.8f,.5f)[0]==1.4f); // change level snaps
 assert(m.at(&level,M::key(M::Missile,43),5.4f,7,0)[0]==5.4f); // newborn snaps
 assert(m.at(&level,a,9,2,.5f)[0]==9); // teleport snaps
 assert(m.at(&level,a,1.4f,2,-1)[0]==1);assert(m.at(&level,a,1.4f,2,2)[0]==1.4f);
 const auto capacity=m.bytes();for(int i=0;i<100;++i){m.begin(&level);m.add(a,float(i),2);m.finish();}assert(m.bytes()==capacity);
 // A multi-tick catchup replaces, rather than accumulates, the previous frame.
 m.begin(&level);m.add(a,2,2);m.finish();m.begin(&level);m.add(a,3,2);m.finish();assert(m.at(&level,a,4,2,.5f)[0]==3.5f);
 m.begin(&level);for(int i=0;i<10000;++i)m.add(M::key(M::Monster,i),0,0);m.finish();assert(m.size()==M::kMaxSamples&&m.bytes()<=65536);
 d2mini::RoofCutout hole{100,100,d2mini::RoofCutout::kRadius};
 assert(hole.alpha(100,100,255)==16&&hole.alpha(160,100,255)==16);
 int last=16;for(int x=100;x<=100+hole.radius;++x){const int v=hole.alpha(x,100,255);assert(v>=last&&v<=255);last=v;}
 assert(hole.alpha(100+hole.radius,100,255)==255&&hole.alpha(100,100,8)==8);
 d2mini::ColoredLights colors;for(int i=0;i<1000;++i)colors.add(32,32,24,{255,50,0});assert(colors.count==1);
 std::vector<std::uint8_t> pixels(64*64*4,40);colors.apply(pixels,64,64);const auto mid=(32*64+32)*4;assert(pixels[mid]>pixels[mid+2]&&pixels[mid+3]==40);assert(pixels[0]==40);
 // Compare complete RGBA buffers, including clipped and overlapping lamps,
 // black pixels and channels already near saturation.
 for(int radius=8;radius<=96;++radius){
  d2mini::ColoredLights lights;
  lights.add(-2,8,radius,{255,80,3});
  lights.add(28,18,radius,{2,134,254});
  std::vector<std::uint8_t> actual(32*24*4);
  for(std::size_t i=0;i<actual.size();++i)actual[i]=(i/4)%7?std::uint8_t(i*31):0;
  auto expected=actual;
  for(unsigned i=0;i<lights.count;++i){
   const auto& l=lights.lamps[i];
   for(int y=0;y<24;++y)for(int x=0;x<32;++x){
    auto* p=expected.data()+(y*32+x)*4;
    int dx=x-l.x,dy=2*(y-l.y),rr=l.r*l.r,d=rr-dx*dx-dy*dy;
    if(d<=0||!(p[0]|p[1]|p[2]))continue;
    int a=(d*(24*65536/rr))>>16;
    for(int c=0;c<3;++c)p[c]=std::min(255,p[c]+(l.rgb[c]*a>>8));
   }
  }
  lights.apply(actual,32,24);
  assert(actual==expected);
 }
 std::puts("PASS render-only interpolation: identities, newborns, teleport/level reset, blend bounds, multi-tick history and bounded storage; broader near-clear roof fade");
}
