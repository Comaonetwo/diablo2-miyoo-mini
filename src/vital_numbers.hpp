// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "scene.hpp"
#include "compact_hud.hpp"
namespace d2mini {
inline void draw_vital_numbers(std::vector<std::uint8_t>& out,const d2d::client::Scene& scene,
                              const d2d::d2s::Stats& stats,bool compact){
 using namespace d2d::d2s;
 const auto& pal=scene.act_pal.entries().empty()?scene.pal:scene.act_pal;
 const auto& font=scene.readable(scene.font);
 for(int side=0;side<2;++side){
  const auto value=stats.get(side?kMana:kLife),maximum=stats.get(side?kMaxMana:kMaxLife);
  const auto current=std::max<std::int64_t>(side?0:value>0?1:0,value/256);
  const auto text=std::to_string(current)+"/"+std::to_string(std::max<std::int64_t>(maximum/256,0));
  int scale=scene.use_readable()?70:100;
  {const d2d::font::TextScaleScope size(scale);const auto box=font.ink_bounds(text);if(box[2]-box[0]>(compact?92:74))scale=std::max(40,scale*(compact?92:74)/(box[2]-box[0]));}
  const d2d::font::TextScaleScope size(scale);const auto box=font.ink_bounds(text);
  const int centre_x=compact?(side?713:90):(side?732:72),centre_y=compact?533:546;
  const int x=centre_x-(box[0]+box[2])/2,y=centre_y-(box[1]+box[3])/2;
  for(auto offset:{std::pair{-1,-1},std::pair{1,-1},std::pair{-1,1},std::pair{1,1}})
   font.draw_tinted(out,800,600,pal,x+offset.first,y+offset.second,text,0,0,0);
  font.draw(out,800,600,pal,x,y,text);
 }
}
}
