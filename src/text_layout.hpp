// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <font.hpp>
#include <algorithm>
#include <string>
#include <string_view>
#include <vector>
namespace d2mini {
struct TextLayout {int scale=100,line_height=0,line_step=0,width=0,height=0;bool fits=false;std::vector<std::string> lines;};
inline TextLayout fit_text(const d2d::font::Font& font,std::string_view text,int width,int height,int requested,int minimum=75,bool wrap=true,bool break_words=true,int line_gap=3){
 TextLayout result;
 for(int scale=std::max(requested,minimum);scale>=minimum;scale-=5){
  const d2d::font::TextScaleScope scope(scale);TextLayout candidate;candidate.scale=scale;candidate.line_height=font.line_height();candidate.line_step=candidate.line_height+std::max(0,line_gap);
  std::string row,word;
  const auto emit_word=[&]{
   if(word.empty())return;
   if(!row.empty()&&font.ink_width(row+" "+word)>width){candidate.lines.push_back(row);row.clear();}
   if(!row.empty())row+=' ';
   for(char c:word){if(wrap&&break_words&&!row.empty()&&font.ink_width(row+std::string(1,c))>width){candidate.lines.push_back(row);row.clear();}row+=c;}
   word.clear();
  };
  if(wrap){for(char c:text){if(c==' '||c=='\n'){emit_word();if(c=='\n'){candidate.lines.push_back(row);row.clear();}}else word+=c;}emit_word();if(!row.empty())candidate.lines.push_back(row);}
  else candidate.lines.emplace_back(text);
  for(const auto& line:candidate.lines)candidate.width=std::max(candidate.width,font.ink_width(line));
  candidate.height=candidate.lines.empty()?0:int(candidate.lines.size()-1)*candidate.line_step+candidate.line_height;
  candidate.fits=candidate.width<=width&&candidate.height<=height;result=std::move(candidate);if(result.fits)return result;
 }
 if(!wrap&&!result.lines.empty()){
  const d2d::font::TextScaleScope scope(result.scale);auto& line=result.lines.front();
  while(!line.empty()&&font.ink_width(line+"...")>width)line.pop_back();
  line+=font.ink_width("...")<=width?"...":"";
  result.width=font.ink_width(line);result.fits=result.width<=width&&result.height<=height;
 }
 return result;
}
}
