// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <span>
#if defined(__ARM_NEON) && !defined(D2MINI_DISABLE_SHADOW_NEON)
#include <arm_neon.h>
#endif
namespace d2mini {
inline void blit_shadow_scalar(std::span<std::uint8_t> out,int sw,int sh,std::span<const std::uint8_t> indices,int w,int h,int box_left,int box_top,int ax,int ay,std::span<std::uint16_t> mask,std::uint16_t id){
 if(w<=0||h<=0)return;
 const int bottom=box_top+h-1,left=ax+box_left+bottom/2,top=ay+bottom/2;
 // Keep the original per-pixel algorithm on platforms without NEON.
 for(int row=0;row<h;row+=2){
  const int yy=top-row/2;if(yy<0||yy>=sh)continue;
  const auto* src=indices.data()+std::size_t(h-1-row)*w;
  for(int x=0;x<w;++x){
   const int xx=left+x-row/2;if(!src[x]||xx<0||xx>=sw)continue;
   auto& stamp=mask[std::size_t(yy)*sw+xx];if(stamp==id)continue;stamp=id;
   auto* p=out.data()+(std::size_t(yy)*sw+xx)*4;p[0]/=4;p[1]/=4;p[2]/=4;
  }
 }
}

inline void blit_shadow(std::span<std::uint8_t> out,int sw,int sh,std::span<const std::uint8_t> indices,int w,int h,int box_left,int box_top,int ax,int ay,std::span<std::uint16_t> mask,std::uint16_t id){
 if(w<=0||h<=0)return;
 const int bottom=box_top+h-1,left=ax+box_left+bottom/2,top=ay+bottom/2;
#if defined(__ARM_NEON) && !defined(D2MINI_DISABLE_SHADOW_NEON)
 // One source row in two projects to one screen row; clip before the pixel loop.
 const int first=std::max(0,top-sh+1),last=std::min((h+1)/2,top+1);
 for(int k=first;k<last;++k){
  const int xleft=left-k,x0=std::max(0,-xleft),x1=std::min(w,sw-xleft);
  if(x0>=x1)continue;
  auto* dst=out.data()+((top-k)*sw+xleft+x0)*4;
  auto* stamp=mask.data()+(top-k)*sw+xleft+x0;
  const auto* src=indices.data()+(h-1-2*k)*w+x0;
  int x=x0;
#if defined(__ARM_NEON) && !defined(D2MINI_DISABLE_SHADOW_NEON)
  for(;x+8<=x1;x+=8,src+=8,dst+=32,stamp+=8){
   const auto previous=vld1q_u16(stamp),own=vdupq_n_u16(id);
   const auto solid=vcgtq_u16(vmovl_u8(vld1_u8(src)),vdupq_n_u16(0));
   const auto active=vandq_u16(solid,vmvnq_u16(vceqq_u16(previous,own)));
   const auto bits=vreinterpretq_u64_u16(active);
   if(!(vgetq_lane_u64(bits,0)|vgetq_lane_u64(bits,1)))continue;
   const auto darken=[&](std::uint8_t* p,uint32x4_t select){
    const auto before=vld1q_u8(p);
    auto dark=vshrq_n_u8(before,2);
    dark=vbslq_u8(vreinterpretq_u8_u32(vdupq_n_u32(0xff000000u)),before,dark);
    vst1q_u8(p,vbslq_u8(vreinterpretq_u8_u32(select),dark,before));
   };
   darken(dst,vceqq_u32(vmovl_u16(vget_low_u16(active)),vdupq_n_u32(65535)));
   darken(dst+16,vceqq_u32(vmovl_u16(vget_high_u16(active)),vdupq_n_u32(65535)));
   vst1q_u16(stamp,vbslq_u16(active,own,previous));
  }
#endif
  for(;x<x1;++x,++src,dst+=4,++stamp)if(*src&&*stamp!=id){*stamp=id;dst[0]>>=2;dst[1]>>=2;dst[2]>>=2;}
 }
#else
 blit_shadow_scalar(out,sw,sh,indices,w,h,box_left,box_top,ax,ay,mask,id);
#endif
}
}
