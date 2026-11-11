// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "world_occlusion.hpp"
#include <palette.hpp>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <span>
#if defined(__ARM_NEON) && !defined(D2MINI_DISABLE_SPRITE_NEON)
#include <arm_neon.h>
#endif
namespace d2mini {
enum class SpriteBlend {Palette,Opaque,Add,Multiply,Quarter,Half,ThreeQuarter};
inline void blit_indexed(std::span<std::uint8_t> out,int screen_w,int screen_h,
 std::span<const std::uint8_t> indices,int w,int h,int dx,int dy,
 const d2d::palette::Palette& pal,SpriteBlend mode=SpriteBlend::Palette,int shade=256){
 if(w<=0||h<=0||world_occlusion.hides(dx,dy,w,h))return;
 const int x0=std::max(0,-dx),x1=std::min(w,screen_w-dx),y0=std::max(0,-dy),y1=std::min(h,screen_h-dy);
 if(x0>=x1||y0>=y1)return;
 const auto* colours=pal.entries().data();
#if defined(__ARM_NEON) && !defined(D2MINI_DISABLE_SPRITE_NEON)
 // Additive spell effects: four indexed pixels share one saturated RGBA
 // operation. Index zero keeps the entire destination, including its alpha.
 if(mode==SpriteBlend::Add){
  for(int y=y0;y<y1;++y)world_occlusion.visible(dx+x0,dx+x1,dy+y,[&](int left,int right){
   const auto* src=indices.data()+y*w+left-dx;auto* dst=out.data()+((dy+y)*screen_w+left)*4;
   int x=left;
   for(;x+4<=right;x+=4,src+=4,dst+=16){
    if(!(src[0]|src[1]|src[2]|src[3]))continue;
    const auto packed=[&](unsigned i){std::uint32_t c;std::memcpy(&c,colours+i,4);return c;};
    auto colour=vdupq_n_u32(packed(src[0]));
    colour=vsetq_lane_u32(packed(src[1]),colour,1);colour=vsetq_lane_u32(packed(src[2]),colour,2);colour=vsetq_lane_u32(packed(src[3]),colour,3);
    auto index=vdupq_n_u32(src[0]);index=vsetq_lane_u32(src[1],index,1);index=vsetq_lane_u32(src[2],index,2);index=vsetq_lane_u32(src[3],index,3);
    const auto before=vld1q_u8(dst);
    auto sum=vqaddq_u8(before,vreinterpretq_u8_u32(colour));
    sum=vreinterpretq_u8_u32(vorrq_u32(vreinterpretq_u32_u8(sum),vdupq_n_u32(0xff000000u)));
    const auto transparent=vreinterpretq_u8_u32(vceqq_u32(index,vdupq_n_u32(0)));
    vst1q_u8(dst,vbslq_u8(transparent,before,sum));
   }
   for(;x<right;++x,++src,dst+=4)if(const auto n=*src){const auto c=colours[n];dst[0]=std::uint8_t(std::min(255,dst[0]+c.r));dst[1]=std::uint8_t(std::min(255,dst[1]+c.g));dst[2]=std::uint8_t(std::min(255,dst[2]+c.b));dst[3]=255;}
  });
  return;
 }
#endif

 const auto paint=[&](auto&& put){for(int y=y0;y<y1;++y)world_occlusion.visible(dx+x0,dx+x1,dy+y,[&](int left,int right){
   const auto* src=indices.data()+y*w+left-dx;auto* dst=out.data()+((dy+y)*screen_w+left)*4;
   for(int x=left;x<right;++x,++src,dst+=4)if(const auto n=*src)put(dst,colours[n]);
 });};
 if(mode==SpriteBlend::Palette){
  if(shade==256)paint([](auto* p,const auto& c){std::memcpy(p,&c,4);});
  else paint([&](auto* p,const auto& c){p[0]=std::uint8_t(std::min(255,c.r*shade/256));p[1]=std::uint8_t(std::min(255,c.g*shade/256));p[2]=std::uint8_t(std::min(255,c.b*shade/256));p[3]=c.a;});
 }else if(mode==SpriteBlend::Opaque)paint([](auto* p,auto c){c.a=255;std::memcpy(p,&c,4);});
 else if(mode==SpriteBlend::Add)paint([](auto* p,const auto& c){p[0]=std::uint8_t(std::min(255,p[0]+c.r));p[1]=std::uint8_t(std::min(255,p[1]+c.g));p[2]=std::uint8_t(std::min(255,p[2]+c.b));p[3]=255;});
 else if(mode==SpriteBlend::Multiply)paint([](auto* p,const auto& c){p[0]=std::uint8_t(p[0]*c.r/255);p[1]=std::uint8_t(p[1]*c.g/255);p[2]=std::uint8_t(p[2]*c.b/255);p[3]=255;});
 else {const int a=mode==SpriteBlend::Quarter?1:mode==SpriteBlend::Half?2:3;
  paint([&](auto* p,const auto& c){p[0]=std::uint8_t((p[0]*(4-a)+c.r*a)/4);p[1]=std::uint8_t((p[1]*(4-a)+c.g*a)/4);p[2]=std::uint8_t((p[2]*(4-a)+c.b*a)/4);p[3]=255;});
 }
}
}
