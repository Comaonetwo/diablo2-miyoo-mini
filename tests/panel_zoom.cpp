// SPDX-License-Identifier: GPL-3.0-or-later
#include "panel_zoom.hpp"
#include <vector>
#include <cstdio>
#include <stdexcept>
int main(){
 std::vector<std::uint8_t> original(800*600*4);for(std::size_t i=0;i<original.size();++i)original[i]=(i*67+i/13)%251;
 int cases=0;
 for(int scale:{125,150,175,200,225})for(int panels=1;panels<=3;++panels)for(int mx:{0,80,200,399,400,600,799})for(int my:{0,48,240,497,599}){
  bool left=panels&1,right=panels&2;auto actual=original,expected=original;
  int sx=left?80:400,sy=48,sw=left&&right?640:320,sh=450,w=sw*scale/100,h=sh*scale/100;
  int dx=w<800?(800-w)/2:-std::clamp((mx-sx)*scale/100-400,0,w-800);
  int dy=h<560?(560-h)/2:-std::clamp((my-sy)*scale/100-280,0,h-560);
  for(int y=0;y<600;++y)for(int x=0;x<800;++x){auto*p=expected.data()+(y*800+x)*4;
   if(y>=std::max(0,dy)&&y<std::min(560,dy+h)&&x>=std::max(0,dx)&&x<std::min(800,dx+w))std::memcpy(p,original.data()+((sy+(y-dy)*sh/h)*800+sx+(x-dx)*sw/w)*4,4);
   else{p[0]=12;p[1]=10;p[2]=9;p[3]=255;}
  }
  d2mini::zoom_panel(actual,scale,left,right,mx,my);if(actual!=expected)throw std::runtime_error("in-place panel zoom changed pixels");++cases;
 }
 for(int panels=1;panels<=3;++panels)for(bool tree:{false,true}){
  auto actual=original,expected=original;
  auto reference=[&](int sx,int sh,int dx,int h){for(int y=0;y<h;++y)for(int x=0;x<400;++x)std::memcpy(expected.data()+(y*800+dx+x)*4,original.data()+((60+y*sh/h)*800+sx+x*320/400)*4,4);};
  if(panels&1)reference(80,432,0,540);if(panels&2)reference(400,tree?480:432,400,tree?600:540);
  d2mini::large_panels(actual,panels&1,panels&2,tree);if(actual!=expected)throw std::runtime_error("large overlay differs from reference");
 }
 for(int mode:{0,1,2}){
  auto actual=original,expected=original;const int w=d2mini::world_width(mode),h=w*3/4,sx=(800-w)/2,sy=(600-h)/2;
  for(int y=0;y<600;++y)for(int x=0;x<800;++x){
   const int ix=sx+x*w/800,iy=sy+y*h/600;
   if(d2mini::world_input_x(x,mode)!=ix||d2mini::world_input_y(y,mode)!=iy)throw std::runtime_error("world cursor mapping differs");
   std::memcpy(expected.data()+(y*800+x)*4,original.data()+(iy*800+ix)*4,4);
  }
  d2mini::zoom_world(actual,mode);if(actual!=expected)throw std::runtime_error("world zoom differs from reference");
 }
 // Exercise the grouped 5:4 copy at offsets beyond the fixed UI layouts,
 // with different containing destinations. Outside pixels remain intact.
 for(int sw:{160,320,640})for(int sx:{0,80})for(int dx:{0,16})for(int h:{300,480}){
  const int w=sw*5/4;if(dx+w>800)continue;
  auto actual=original,expected=original;
  for(int y=0;y<h;++y)for(int x=0;x<w;++x)
   std::memcpy(expected.data()+(((20+y)*800+dx+x)*4),original.data()+((60+y*240/h)*800+sx+x*4/5)*4,4);
  d2mini::expand_rect(actual,sx,60,sw,240,dx,20,w,h);
  if(actual!=expected)throw std::runtime_error("grouped 5:4 copy differs from immutable reference");++cases;
 }
 for(int sy:{0,30,60,120})for(int dy:{0,20,60,100})for(int sh:{200,300,400}){
  const int h=480;if(sy+sh>600||dy+h>600||sy<dy||sy+sh>dy+h)continue;
  auto actual=original,expected=original;
  for(int y=0;y<h;++y)for(int x=0;x<600;++x)std::memcpy(expected.data()+(((dy+y)*800+40+x)*4),original.data()+((sy+y*sh/h)*800+80+x*400/600)*4,4);
  d2mini::expand_rect(actual,80,sy,400,sh,40,dy,600,h);if(actual!=expected)throw std::runtime_error("row-order expansion differs from immutable reference");++cases;
 }
 std::printf("PASS: %d single/dual panel transforms match immutable reference without scratch allocation\n",cases);
}
