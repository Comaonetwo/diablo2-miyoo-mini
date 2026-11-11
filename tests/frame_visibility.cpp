// SPDX-License-Identifier: GPL-3.0-or-later
#include "frame_visibility.hpp"
#include "shadow_blit.hpp"
#include <random>
#include <vector>
#include <cstdio>
int main(){std::mt19937 rng(104108);unsigned bodies=0,shadows=0;
 for(int i=0;i<1200;++i){const int w=1+rng()%150,h=1+rng()%230,bx=int(rng()%201)-100,by=int(rng()%151)-130,ax=int(rng()%451)-150,ay=int(rng()%451)-150;
  d2mini::WorldOcclusion clip;if(i%3==1){clip.clip_left=10;clip.clip_top=5;clip.clip_right=95;clip.clip_bottom=90;}else if(i%3==2)clip={40,70,110,103};
  const d2mini::WorldOcclusionScope scope(clip);
  for(bool shadow:{false,true}){
   const bool hidden=shadow?d2mini::shadow_hidden(w,h,bx,by,ax,ay,127,103):d2mini::body_hidden(w,h,bx,by,ax,ay,127,103);if(!hidden)continue;
   if(shadow)++shadows;else++bodies;
   std::vector<unsigned char> pixels(w*h,1),image(127*103*4,200);std::vector<std::uint16_t> mask(127*103);
   if(shadow)d2mini::blit_shadow_scalar(image,127,103,pixels,w,h,bx,by,ax,ay,mask,1);
   else for(int y=0;y<h;++y)for(int x=0;x<w;++x){int xx=ax+bx+x,yy=ay+by+y;if(xx>=0&&xx<127&&yy>=0&&yy<103)image[(yy*127+xx)*4]=50;}
   for(int y=0;y<103;++y)for(int x=0;x<127;++x)if(!clip.hides(x,y,1,1))for(int c=0;c<4;++c)if(image[(y*127+x)*4+c]!=200)return 2;
  }
 }
 if(!d2mini::body_hidden(0,10,0,0,0,0,127,103)||!d2mini::shadow_hidden(10,0,0,0,0,0,127,103))return 3;
 printf("PASS: %u hidden bodies and %u projected shadows change no visible pixel across screen/HUD/crop boundaries\n",bodies,shadows);return bodies>400&&shadows>400?0:1;}
