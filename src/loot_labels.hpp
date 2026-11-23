// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <vector>
namespace d2mini {
// Temporary coverage in the otherwise opaque world alpha channel: later walls
// overwrite it naturally. Reset before presenting; no extra full-frame buffer.
inline std::uint8_t actor_pixel_alpha=255;
struct ActorCoverageScope {std::uint8_t old;explicit ActorCoverageScope(bool protect):old(actor_pixel_alpha){actor_pixel_alpha=protect?254:255;}~ActorCoverageScope(){actor_pixel_alpha=old;}};
struct LootFade {
 std::uint32_t last=0;
 float value=0;
 bool started=false;
 float step(std::uint32_t now,bool active) {
  const auto dt=started ? std::min(now-last,300u) : 0u;
  started=true;
  last=now;
  const float delta=float(dt)/300.f;
  value=active ? std::min(1.f,value+delta) : std::max(0.f,value-delta);
  return value;
 }
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
 ~LootLabelComposite() {
  const int pixels = x1_ - x0_;
  if (pixels == 0) return;
  const auto row_bytes = std::size_t(pixels) * 4;
  const int actor_opacity = opacity_ * (255 - fade_) / 255;
  for (int y = y0_; y < y1_; ++y) {
   auto* drawn = frame_.data() + (y * width_ + x0_) * 4;
   const auto* before = saved_.data() + std::size_t(y - y0_) * row_bytes;
   if (std::memcmp(drawn, before, row_bytes) == 0) continue;
   for (int x = 0; x < pixels; ++x, drawn += 4, before += 4) {
    if (std::memcmp(drawn, before, 4) == 0) continue;
    const int alpha = before[3] == 254 ? actor_opacity : opacity_;
    for (int c = 0; c < 3; ++c)
     drawn[c] = std::uint8_t((drawn[c] * alpha + before[c] * (255 - alpha) + 127) / 255);
    drawn[3] = before[3];
   }
  }
 }
 static std::vector<std::uint8_t>& scratch(){static std::vector<std::uint8_t> pixels;return pixels;}
};
}
