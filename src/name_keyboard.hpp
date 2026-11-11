// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <array>
#include <string>
#include <string_view>
#include <optional>
#include <algorithm>
#include <cstdint>
#include <vector>
namespace d2d::client {struct Scene;}
namespace d2mini {
struct NameKeyboard {
 bool open=false,upper=false;int row=0,col=0;
 std::string draft,notice;std::optional<std::string> result;
 std::array<bool,8> previous{};std::array<float,4> repeats{};
 static int columns(int row){return row==3?3:10;}
 void show(std::string_view value){open=true;draft=value;notice.clear();result.reset();row=col=0;previous={};repeats={};}
 void append(std::string_view text){notice.clear();for(char c:text){const bool alpha=(c>='a'&&c<='z')||(c>='A'&&c<='Z');const bool mark=(c=='-'||c=='_'||c=='\'')&&!draft.empty()&&draft.find_first_of("'-_")==std::string::npos;if((alpha||mark)&&draft.size()<15)draft+=c;}}
 void erase(){notice.clear();if(!draft.empty())draft.pop_back();}
 bool valid()const {if(draft.size()<2||draft.size()>15)return false;const char c=draft.back();return (c>='a'&&c<='z')||(c>='A'&&c<='Z');}
 void accept(){if(valid()){result=draft;open=false;}else notice="Use 2-15 letters; a separator goes inside.";}
 std::string label(int r,int c)const {
  if(r==3)return c==0?(upper?"Lowercase":"Uppercase"):c==1?"Done":"Cancel";
  if(r==2&&c==9)return "DEL";
  constexpr std::string_view chars="abcdefghijklmnopqrstuvwxyz-_'";
  char ch=chars[std::size_t(r*10+c)];if(upper&&ch>='a'&&ch<='z')ch=char(ch-'a'+'A');return std::string(1,ch);
 }
 void choose(){if(row==3){if(col==0)upper=!upper;else if(col==1)accept();else open=false;}else if(row==2&&col==9)erase();else append(label(row,col));}
 void update(const std::array<bool,8>& held,float seconds){
  for(int i=0;i<4;++i){bool step=false;if(!held[i])repeats[i]=0;else if(!previous[i]){step=true;repeats[i]=.35f;}else if((repeats[i]-=seconds)<=0){step=true;repeats[i]=.12f;}
   if(step){if(i<2){row=(row+(i?1:3))%4;col=std::min(col,columns(row)-1);}else col=(col+(i==2?columns(row)-1:1))%columns(row);}}
  if(held[5]&&!previous[5])open=false;
  else if(held[4]&&!previous[4])choose();
  if(held[6]&&!previous[6])erase();if(held[7]&&!previous[7])upper=!upper;
  previous=held;
 }
 void draw(std::vector<std::uint8_t>&,const d2d::client::Scene&)const;
};
}
