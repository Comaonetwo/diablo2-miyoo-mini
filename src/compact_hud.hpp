// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "globe_fill.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>
namespace d2d::client {struct Scene;struct Hud;struct LevelButtons;struct SkillBar;struct MiniPanel;}
namespace d2mini {
struct HudRect {int x,y,w,h;bool contains(int px,int py)const{return px>=x&&py>=y&&px<x+w&&py<y+h;}};
enum class HudPart {World,Life,Mana,Mini,Belt,Bars,LeftSkill,RightSkill,Stats,Skills,Blank,Popup,Stamina,Run};
struct HudPiece {HudPart id;HudRect source,target;};
struct HudHit {HudPart part;int x,y;};
// Native hit boxes and source-art coordinates share this one layout table.
// Original UI rules see native coordinates; the pointer remains in display coordinates.
struct CompactHudLayout {
 std::array<HudPiece,10> pieces;
 std::array<HudPiece,0> quick;
 std::array<HudPiece,4> belt_slots;
 std::array<HudPiece,0> ornaments;
 int top,core_x,core_w,popup_row_h,outer_x,outer_w,upper_x,upper_w;
 explicit CompactHudLayout(int = 125) {
  const int orb_w=148,orb_h=132,skill=84,xp_h=7,belt_h=84,bars_y=509;
  core_w=336;core_x=232;popup_row_h=84;
  outer_x=148;outer_w=504;upper_x=core_x;upper_w=core_w;top=499;
  pieces={{{HudPart::Life,{0,496,117,104},{0,600-orb_h,orb_w,orb_h}},
   {HudPart::Mana,{683,496,117,104},{800-orb_w,600-orb_h,orb_w,orb_h}},
   {HudPart::Mini,{323,528,156,24},{core_x,bars_y,0,0}},
   {HudPart::Belt,{421,561,125,32},{core_x,bars_y,core_w,belt_h}},
   {HudPart::Bars,{254,558,123,10},{outer_x,600-xp_h,outer_w,xp_h}},
   {HudPart::LeftSkill,{117,552,48,48},{outer_x,bars_y,skill,belt_h}},
   {HudPart::RightSkill,{635,552,48,48},{core_x+core_w,bars_y,skill,belt_h}},
   {HudPart::Stats,{40,459,35,39},{139*orb_w/234-18,426,36,40}},
   {HudPart::Skills,{727,459,35,39},{800-orb_w+93*orb_w/234-18,426,36,40}},
   {HudPart::Stamina,{273,573,102,18},{outer_x,top,outer_w,10}}}};
  for(int i=0;i<4;++i)belt_slots[i]={HudPart::Belt,{421+i*31,561,31,31},{core_x+i*84,bars_y,84,belt_h}};

 }
 HudPiece popup(int row)const {return {HudPart::Popup,{421,560-row*32,125,32},{upper_x,top-row*popup_row_h-3,upper_w,popup_row_h}};}
 HudHit native(int x,int y,bool stats,bool skills,int popup_rows=0)const {
  const auto hit=[&](const HudPiece&p){return HudHit{p.id,p.source.x+(x-p.target.x)*p.source.w/p.target.w,p.source.y+(y-p.target.y)*p.source.h/p.target.h};};
  for(int r=1;r<popup_rows;++r){auto p=popup(r);if(p.target.contains(x,y))return hit(p);}
  for(const auto&p:quick)if(p.target.contains(x,y))return hit(p);
  // Raised point notifications overlap the space above the HUD only when present.
  for(auto it=pieces.rbegin();it!=pieces.rend();++it){const auto&p=*it;if(p.id==HudPart::Mini)continue;if(p.id==HudPart::Stats&&!stats)continue;if(p.id==HudPart::Skills&&!skills)continue;if(p.target.contains(x,y))return hit(p);}
  return y>=top?HudHit{HudPart::Blank,400,599}:HudHit{HudPart::World,x,y};
 }
};
void draw_compact_hud(std::vector<std::uint8_t>&,const d2d::client::Scene&,const d2d::client::SkillBar&,
 const d2d::client::Hud&,const d2d::client::LevelButtons&,const d2d::client::MiniPanel&,bool popup,int mouse_x,int mouse_y);
}
