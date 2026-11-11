// SPDX-License-Identifier: GPL-3.0-or-later
#include "name_keyboard.hpp"
#include "scene.hpp"
namespace d2mini {
void NameKeyboard::draw(std::vector<std::uint8_t>& f,const d2d::client::Scene& s)const{
 if(!open)return;const d2d::font::TextScaleScope scale(100);const auto& font=s.readable_font.glyph_count()?s.readable_font:s.font;const auto& pal=s.pal;
 const auto rect=[&](int x,int y,int w,int h,int r,int g,int b){for(int j=y;j<y+h;++j)for(int i=x;i<x+w;++i){auto*p=f.data()+(j*800+i)*4;p[0]=r;p[1]=g;p[2]=b;p[3]=255;}};
 rect(32,22,736,355,137,112,70);rect(35,25,730,349,16,14,12);
 font.draw_tinted(f,800,600,pal,58,42,"Character Name",223,199,140);
 const std::string text=draft.empty()?"_":draft+"_";font.draw(f,800,600,pal,58,76,text);
 if(!notice.empty())font.draw(f,800,600,pal,58,98,notice);
 for(int r=0;r<4;++r)for(int c=0;c<columns(r);++c){const int w=r==3?222:64,x=58+c*(r==3?230:69),y=120+r*53;const bool selected=r==row&&c==col;
  rect(x,y,w,42,selected?137:48,selected?112:42,selected?70:34);const auto key=label(r,c);font.draw(f,800,600,pal,x+(w-font.measure(key))/2,y+10,key);}
 font.draw(f,800,600,pal,58,340,"B: Select   A: Cancel   Y: Delete   X: Case");
}
}
