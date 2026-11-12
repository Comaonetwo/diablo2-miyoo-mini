// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
namespace d2mini {
// Bounded sparse color accents on the already lit world, before HUD/panels.
// No full-screen RGB grid, shadow map, extra framebuffer or per-light allocation.
struct ColoredLights {
 struct Lamp{int x,y,r;std::array<std::uint8_t,3> rgb;};
 std::array<Lamp,8> lamps{};unsigned count=0;
 void add(int x,int y,int radius,std::array<std::uint8_t,3> rgb){
  if(radius<=0||*std::max_element(rgb.begin(),rgb.end())==*std::min_element(rgb.begin(),rgb.end()))return;
  radius=std::clamp(radius,8,96);
  for(unsigned i=0;i<count;++i)if(lamps[i].rgb==rgb&&std::abs(lamps[i].x-x)<radius/2&&std::abs(lamps[i].y-y)<radius/4)return;
  if(count<lamps.size())lamps[count++]={x,y,radius,rgb};
 }
 void apply(std::span<std::uint8_t> f,int width,int height)const{
  if(f.size()!=std::size_t(width)*height*4)return;
  for(unsigned i=0;i<count;++i){const auto& l=lamps[i];const int rr=l.r*l.r,scale=24*65536/rr;
   for(int y=std::max(0,l.y-l.r/2);y<std::min(height,l.y+l.r/2+1);++y){const int yy=2*(y-l.y),base=rr-yy*yy;
    for(int x=std::max(0,l.x-l.r);x<std::min(width,l.x+l.r+1);++x){const int dx=x-l.x,d=base-dx*dx;if(d<=0)continue;auto*p=f.data()+(std::size_t(y)*width+x)*4;if(!(p[0]|p[1]|p[2]))continue;const int a=d*scale>>16;
     for(int c=0;c<3;++c)p[c]=std::uint8_t(std::min(255,int(p[c])+(int(l.rgb[c])*a>>8)));
    }
   }
  }
 }
};
}
