// SPDX-License-Identifier: GPL-3.0-or-later
#include "panel_zoom.hpp"
#include <chrono>
#include <cstdio>
#include <vector>
void previous(std::span<std::uint8_t> f,int sx,int sy,int sw,int sh,int dx,int dy,int w,int h){
 if(f.size()!=800*600*4)return;
 std::array<std::uint8_t,800*4> row;std::array<std::uint8_t,600*4> column;
 for(int y=sy;y<sy+sh;++y){std::memcpy(row.data(),f.data()+y*3200,3200);for(int x=0;x<w;++x)std::memcpy(f.data()+(y*800+dx+x)*4,row.data()+(sx+x*sw/w)*4,4);}
 if(h>=sh){
  // Copy contiguous rows instead of walking the framebuffer by columns.
  // Above the fixed point the source is below its destination; below it,
  // read bottom-up. This preserves every source row during in-place growth.
  int split=dy;while(split<dy+h&&sy+(split-dy)*sh/h>=split)++split;
  const auto copy=[&](int y){const int from=sy+(y-dy)*sh/h;std::memmove(f.data()+(y*800+dx)*4,f.data()+(from*800+dx)*4,std::size_t(w)*4);};
  for(int y=dy;y<split;++y)copy(y);
  for(int y=dy+h-1;y>=split;--y)copy(y);
 }else{
  for(int x=dx;x<dx+w;++x){for(int y=0;y<sh;++y)std::memcpy(column.data()+y*4,f.data()+((sy+y)*800+x)*4,4);for(int y=0;y<h;++y)std::memcpy(f.data()+((dy+y)*800+x)*4,column.data()+y*sh/h*4,4);}
 }
}
int main(){
 std::vector<std::uint8_t> original(800*600*4),f(original.size());for(std::size_t i=0;i<original.size();++i)original[i]=(i*67+i/13)%251;
 auto expected=original,actual=original;previous(expected,80,60,640,480,0,0,800,600);d2mini::zoom_world(actual);if(expected!=actual)return 1;
 for(bool old:{true,false}){
  std::array<long long,7> samples{};
  for(auto& sample:samples){std::chrono::nanoseconds elapsed{};for(int i=0;i<100;++i){f=original;const auto start=std::chrono::steady_clock::now();if(old)previous(f,80,60,640,480,0,0,800,600);else d2mini::zoom_world(f);elapsed+=std::chrono::steady_clock::now()-start;}sample=elapsed.count()/100;}
  std::sort(samples.begin(),samples.end());std::printf("%s world_zoom_median_ns=%lld\n",old?"0.2.28":"0.2.29",samples[3]);
 }
}
