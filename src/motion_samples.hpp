// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>
namespace d2mini {
// Only visual positions. Never writes back to simulation, paths or collision.
class MotionSamples {
 struct Sample { std::uint64_t key;float x,y; };
 const void* level_=nullptr;
 std::vector<Sample> samples_;
public:
 static constexpr std::size_t kMaxSamples=4096;
 enum Kind:unsigned {Monster=1,Pet=2,Merc=3,Missile=4,Npc=5};
 static std::uint64_t key(Kind kind,std::uint64_t id){return (std::uint64_t(kind)<<56)|(id&0x00ffffffffffffffULL);}
 void begin(const void* level){level_=level;samples_.clear();}
 void add(std::uint64_t id,float x,float y){if(id&&samples_.size()<kMaxSamples)samples_.push_back({id,x,y});}
 void finish(){std::sort(samples_.begin(),samples_.end(),[](const Sample&a,const Sample&b){return a.key<b.key;});}
 std::size_t size()const{return samples_.size();}
 std::size_t bytes()const{return samples_.capacity()*sizeof(Sample);}
 std::array<float,2> at(const void* level,std::uint64_t id,float x,float y,float blend)const{
  if(!id||level!=level_)return {x,y};
  const auto p=std::lower_bound(samples_.begin(),samples_.end(),id,[](const Sample&s,std::uint64_t k){return s.key<k;});
  if(p==samples_.end()||p->key!=id)return {x,y}; // newborn or newly visible
  const float dx=x-p->x,dy=y-p->y;
  if(!std::isfinite(dx)||!std::isfinite(dy)||dx*dx+dy*dy>4)return {x,y}; // teleport, not travel
  blend=std::clamp(blend,0.f,1.f);
  return {p->x+dx*blend,p->y+dy*blend};
 }
};
}
