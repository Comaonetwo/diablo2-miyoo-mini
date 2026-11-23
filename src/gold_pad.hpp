// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <array>
#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
namespace d2d::client {struct Scene;}
namespace d2mini {
struct GoldPad {
 bool open=false,withdraw=false,just_open=false,replace=true;int row=0,col=0,limit=0;
 std::string draft,notice;std::optional<int> result;std::array<bool,9> previous{};std::array<float,4> repeats{};
 static constexpr int columns(int r){return r==4?2:3;}
 static std::array<int,4> box(int r,int c){return {264+c*(r==4?140:94),204+r*48,r==4?132:84,40};}
 int amount()const{int n=0;for(char c:draft)n=n*10+c-'0';return n;}
 void show(bool take,int max){open=just_open=replace=true;withdraw=take;limit=std::max(0,max);draft=std::to_string(limit);notice.clear();result.reset();row=col=0;previous={};repeats={};}
 void append(std::string_view text){for(char c:text)if(c>='0'&&c<='9'){
  if(replace){draft.clear();replace=false;}if(draft=="0")draft.clear();if(draft.size()<7)draft+=c;
  if(amount()>limit){draft=std::to_string(limit);notice="Maximum: "+draft;}else notice.clear();
 }}
 void erase(){replace=false;notice.clear();if(!draft.empty())draft.pop_back();}
 void all(){draft=std::to_string(limit);replace=true;notice.clear();}
 void accept(){const int n=std::min(amount(),limit);if(n>0){result=n;open=false;}else notice="No gold to transfer";}
 std::string label(int r,int c)const{if(r==4)return c?"Confirm":"Cancel";if(r==3)return c==0?"DEL":c==1?"0":"All";return std::string(1,char('1'+r*3+c));}
 void choose(){if(row==4){if(col)accept();else open=false;}else if(row==3&&col==0)erase();else if(row==3&&col==2)all();else append(label(row,col));}
 void click(int x,int y){for(int r=0;r<5;++r)for(int c=0;c<columns(r);++c){const auto b=box(r,c);if(x>=b[0]&&x<b[0]+b[2]&&y>=b[1]&&y<b[1]+b[3]){row=r;col=c;choose();return;}}}
 void update(const std::array<bool,9>& held,float dt){
  if(just_open){
   previous=held;
   for(int i=0;i<4;++i)if(held[i])repeats[i]=.35f;
   just_open=false;return;
  }
  for(int i=0;i<4;++i){bool step=false;if(!held[i])repeats[i]=0;else if(!previous[i]){step=true;repeats[i]=.35f;}else if((repeats[i]-=dt)<=0){step=true;repeats[i]=.12f;}
   if(step){if(i<2){row=(row+(i?1:4))%5;col=std::min(col,columns(row)-1);}else col=(col+(i==2?columns(row)-1:1))%columns(row);}}
  if(held[5]&&!previous[5])open=false;else if(held[8]&&!previous[8])accept();else if(held[4]&&!previous[4])choose();
  if(held[6]&&!previous[6])erase();if(held[7]&&!previous[7])all();previous=held;
 }
 void draw(std::vector<std::uint8_t>&,const d2d::client::Scene&)const;
};
}
