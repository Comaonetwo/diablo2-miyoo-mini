// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <span>
namespace d2mini {
// Separable in-place expansion stages one row and one column (5 KiB),
// instead of allocating up to 1.15 MiB for a second panel framebuffer.
inline void zoom_panel(std::span<std::uint8_t> f,int percent,bool left,bool right,int mx,int my) {
 if(percent<=100||(!left&&!right)||f.size()!=800*600*4)return;
 const int sx=left?80:400,sy=48,sw=left&&right?640:320,sh=450;
 const int w=sw*percent/100,h=sh*percent/100;
 const int dx=w<800?(800-w)/2:-std::clamp((mx-sx)*percent/100-400,0,w-800);
 const int dy=h<560?(560-h)/2:-std::clamp((my-sy)*percent/100-280,0,h-560);
 const int x0=std::max(0,dx),x1=std::min(800,dx+w),y0=std::max(0,dy),y1=std::min(560,dy+h);
 const auto source_x=[&](int x){return sx+(x-dx)*sw/w;};
 const auto source_y=[&](int y){return sy+(y-dy)*sh/h;};
 std::array<std::uint8_t,800*4> row;
 std::array<std::uint8_t,450*4> column;
 for(int y=sy;y<sy+sh;++y){
  std::memcpy(row.data(),f.data()+y*800*4,row.size());
  for(int x=x0;x<x1;++x)std::memcpy(f.data()+(y*800+x)*4,row.data()+source_x(x)*4,4);
 }
 for(int x=x0;x<x1;++x){
  for(int y=0;y<sh;++y)std::memcpy(column.data()+y*4,f.data()+((sy+y)*800+x)*4,4);
  for(int y=y0;y<y1;++y)std::memcpy(f.data()+(y*800+x)*4,column.data()+(source_y(y)-sy)*4,4);
 }
 for(int y=0;y<600;++y)for(int x=0;x<800;++x)if(y<y0||y>=y1||x<x0||x>=x1){auto*p=f.data()+(y*800+x)*4;p[0]=12;p[1]=10;p[2]=9;p[3]=255;}
}
// Safe separable copy with a bounded row/column. Preserves pixels outside the
// destination, so large panels remain overlays on the world.
inline void expand_rect(std::span<std::uint8_t> f,int sx,int sy,int sw,int sh,int dx,int dy,int w,int h){
 if(f.size()!=800*600*4)return;
 std::array<std::uint8_t,800*4> row;std::array<std::uint8_t,600*4> column;
 if(w%5==0&&sw==w/5*4){
  // The world and large panels expand four source pixels into five:
  // [a,b,c,d] -> [a,a,b,c,d]. Stage only the source rectangle so overlap
  // stays safe, then copy each group without a division per output pixel.
  for(int y=sy;y<sy+sh;++y){
   std::memcpy(row.data(),f.data()+(y*800+sx)*4,std::size_t(sw)*4);
   auto* to=f.data()+(y*800+dx)*4;
   const auto* from=row.data();
   for(int x=0;x<w;x+=5,to+=20,from+=16){
    std::memcpy(to,from,4);std::memcpy(to+4,from,16);
   }
  }
 }else{
  for(int y=sy;y<sy+sh;++y){std::memcpy(row.data(),f.data()+y*3200,3200);for(int x=0;x<w;++x)std::memcpy(f.data()+(y*800+dx+x)*4,row.data()+(sx+x*sw/w)*4,4);}
 }
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
inline void large_panels(std::span<std::uint8_t> f,bool left,bool right,bool tree=false){
 if(left)expand_rect(f,80,60,320,432,0,0,400,540);
 if(right)expand_rect(f,400,60,320,tree?480:432,400,0,400,tree?600:540);
}
// Preserve old bool callers as mode 1; mode 2 crops a centered 480x360.
inline int world_width(int mode){return mode==2?480:mode?640:800;}
inline void zoom_world(std::span<std::uint8_t> f,int mode=1){const int w=world_width(mode),h=w*3/4;expand_rect(f,(800-w)/2,(600-h)/2,w,h,0,0,800,600);}
inline int world_input_x(int x,int mode){const int w=world_width(mode);return (800-w)/2+x*w/800;}
inline int world_input_y(int y,int mode){const int h=world_width(mode)*3/4;return (600-h)/2+y*h/600;}

}
