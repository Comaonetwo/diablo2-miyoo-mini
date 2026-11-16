// SPDX-License-Identifier: GPL-3.0-or-later
#include "gold_pad.hpp"
#include "control_hint.hpp"
#include "scene.hpp"
namespace d2mini {
void GoldPad::draw(std::vector<std::uint8_t>& f,const d2d::client::Scene& s)const{
 if(!open)return;const d2d::font::TextScaleScope scale(100);const auto& font=s.readable_font.glyph_count()?s.readable_font:s.font;const auto& pal=s.pal;
 const auto rect=[&](int x,int y,int w,int h,int r,int g,int b){for(int j=y;j<y+h;++j)for(int i=x;i<x+w;++i){auto*p=f.data()+(j*800+i)*4;p[0]=r;p[1]=g;p[2]=b;p[3]=255;}};
 rect(238,80,324,450,137,112,70);rect(241,83,318,444,16,14,12);
 const auto text=[&](int y,std::string value,bool gold=false){const auto ink=font.ink_bounds(value);const int x=(800-font.measure(value))/2,at=y-(ink[1]+ink[3])/2;if(gold)font.draw_tinted(f,800,600,pal,x,at,value,255,214,139);else font.draw(f,800,600,pal,x,at,value);};
 text(108,withdraw?"Withdraw Gold":"Deposit Gold",true);text(136,notice.empty()?"Maximum: "+std::to_string(limit):notice);
 rect(261,156,278,34,90,76,52);rect(263,158,274,30,0,0,0);text(173,(draft.empty()?"0":draft)+"_");
 for(int r=0;r<5;++r)for(int c=0;c<columns(r);++c){const auto b=box(r,c);const bool pick=r==row&&c==col;rect(b[0],b[1],b[2],b[3],pick?137:48,pick?112:42,pick?70:34);const auto v=label(r,c);const auto ink=font.ink_bounds(v);font.draw(f,800,600,pal,b[0]+(b[2]-font.measure(v))/2,b[1]+(b[3]-ink[1]-ink[3])/2,v);}
 for(const auto& line:std::array<std::pair<int,std::string_view>,2>{{{466,"B: Select  A: Cancel"},{497,"Y: Delete  X: All  START: OK"}}}){
  const auto ink=font.ink_bounds(line.second);draw_control_hint(font,f,pal,(800-font.measure(line.second))/2,line.first-(ink[1]+ink[3])/2,line.second);
 }
}
}
