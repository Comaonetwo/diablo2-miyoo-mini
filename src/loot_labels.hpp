// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>
namespace d2mini {
// Temporary coverage in the otherwise opaque world alpha channel: later walls
// overwrite it naturally. Reset before presenting; no extra full-frame buffer.
inline std::uint8_t actor_pixel_alpha=255;
struct ActorCoverageScope {std::uint8_t old;explicit ActorCoverageScope(bool protect):old(actor_pixel_alpha){actor_pixel_alpha=protect?254:255;}~ActorCoverageScope(){actor_pixel_alpha=old;}};
struct LootFade {
 std::uint32_t last=0;float value=0;
 float step(std::uint32_t now,bool active){const auto dt=last?std::min(now-last,300u):0u;last=now;const float d=float(dt)/300.f;value=active?std::min(1.f,value+d):std::max(0.f,value-d);return value;}
};
// Blend only this label's small rectangle, preserving actual visible actor
// pixels, including correct wall occlusion and selected-label protection.
class LootLabelComposite {
 std::vector<std::uint8_t>& frame_;int width_,x0_,y0_,x1_,y1_,fade_,opacity_;
 std::vector<std::uint8_t>& saved_;
public:
 LootLabelComposite(std::vector<std::uint8_t>& f,int width,std::array<int,4> r,float fade,bool selected):frame_(f),width_(width),fade_(int(std::clamp(fade,0.f,1.f)*255)),opacity_(selected?255:255-fade_*160/255),saved_(scratch()){
  x0_=std::clamp(r[0]-4,0,width);x1_=std::clamp(r[2]+4,0,width);y0_=std::clamp(r[1]-4,0,600);y1_=std::clamp(r[3]+6,0,600);
  if(!fade_||x1_<=x0_||y1_<=y0_){x1_=x0_;return;}
  saved_.resize(std::size_t(x1_-x0_)*(y1_-y0_)*4);
  for(int y=y0_;y<y1_;++y)std::copy_n(f.data()+(y*width+x0_)*4,(x1_-x0_)*4,saved_.data()+std::size_t(y-y0_)*(x1_-x0_)*4);
 }
 ~LootLabelComposite(){for(int y=y0_;y<y1_;++y)for(int x=x0_;x<x1_;++x){auto*p=frame_.data()+(y*width_+x)*4;const auto*b=saved_.data()+(std::size_t(y-y0_)*(x1_-x0_)+x-x0_)*4;const int a=b[3]==254?opacity_*(255-fade_)/255:opacity_;for(int c=0;c<3;++c)p[c]=std::uint8_t((int(p[c])*a+int(b[c])*(255-a)+127)/255);p[3]=b[3];}}
 static std::vector<std::uint8_t>& scratch(){static std::vector<std::uint8_t> pixels;return pixels;}
};
}
