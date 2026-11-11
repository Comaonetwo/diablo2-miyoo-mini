// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <font.hpp>
namespace d2mini {
// Every colon-delimited legend starts with a highlighted physical button name.
inline void draw_control_hint(const d2d::font::Font& font,std::vector<std::uint8_t>& f,
 const d2d::palette::Palette& pal,int x,int y,std::string_view hint){
 while(!hint.empty()){
  const auto end=hint.find("  ");const auto part=hint.substr(0,end);const auto colon=part.find(':');
  if(colon!=part.npos){x=font.draw_tinted(f,800,600,pal,x,y,part.substr(0,colon),255,214,139);x=font.draw(f,800,600,pal,x,y,part.substr(colon));}
  else x=font.draw(f,800,600,pal,x,y,part);
  if(end==hint.npos)break;x=font.draw(f,800,600,pal,x,y,"  ");hint.remove_prefix(end+2);
 }
}
}
