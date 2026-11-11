// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <cstdint>
#include <span>
namespace d2mini {
inline int globe_fill_rows(std::int64_t current,std::int64_t maximum,bool life){
 if(maximum<=0||current<=0)return 0;if(current>=maximum)return 80;
 const int rows=int(current*80/maximum);return life?std::max(rows,2):rows;
}
// Emit only filled, nontransparent texels. Empty glass preserves the art/world underneath.
template<class Draw> void globe_pixels(std::span<const std::uint8_t> source,int width,int height,int rows,Draw&& draw){
 if(width<=0||height<=0||source.size()/std::size_t(width)<std::size_t(height))return;
 rows=std::clamp(rows,0,height);
 for(int y=height-rows;y<height;++y){
  const auto row=source.subspan(std::size_t(y)*width,std::size_t(width));
  for(int x=0;x<width;++x)if(auto index=row[std::size_t(x)])draw(x,y,index);
 }
}
}
