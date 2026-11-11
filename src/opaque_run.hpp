// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <cstring>
#include <cstddef>
#if defined(__ARM_NEON) && !defined(D2MINI_DISABLE_OPAQUE_NEON)
#include <arm_neon.h>
#endif
namespace d2mini {
// Opaque RLE spans only; clipping and transparent runs are handled by the caller.
template<class Colour>inline void opaque_run_scalar(std::uint8_t* dst,const std::uint8_t* src,int count,const Colour* palette){
 static_assert(sizeof(Colour)==4&&offsetof(Colour,a)==3);
 for(int i=0;i<count;++i,dst+=4){auto c=palette[src[i]];c.a=255;std::memcpy(dst,&c,4);}
}
template<class Colour>inline void opaque_run(std::uint8_t* dst,const std::uint8_t* src,int count,const Colour* palette){
 static_assert(sizeof(Colour)==4&&offsetof(Colour,a)==3);
 int i=0;
#if defined(__ARM_NEON) && !defined(D2MINI_DISABLE_OPAQUE_NEON)
 for(;i+4<=count;i+=4,src+=4,dst+=16){
  const auto packed=[&](unsigned n){std::uint32_t v;std::memcpy(&v,palette+n,4);return v;};
  auto c=vdupq_n_u32(packed(src[0]));c=vsetq_lane_u32(packed(src[1]),c,1);c=vsetq_lane_u32(packed(src[2]),c,2);c=vsetq_lane_u32(packed(src[3]),c,3);
  vst1q_u8(dst,vreinterpretq_u8_u32(vorrq_u32(c,vdupq_n_u32(0xff000000u))));
 }
#endif
 opaque_run_scalar(dst,src,count-i,palette);
}
}
