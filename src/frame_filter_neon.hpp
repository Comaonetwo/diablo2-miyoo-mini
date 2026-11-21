// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#if defined(__ARM_NEON) && defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ && !defined(D2MINI_FORCE_SCALAR_FILTER)
#define D2MINI_NEON_FILTER_AVAILABLE 1
#include <arm_neon.h>
#include <cstddef>
#include <cstdint>
#if defined(__linux__) && defined(__arm__) && !defined(__aarch64__)
#include <sys/auxv.h>
#endif
namespace d2mini::frame_filter_detail {
inline bool available(){
#if defined(__linux__) && defined(__arm__) && !defined(__aarch64__)
 // Linux ARM HWCAP_NEON. Keep the scalar path on CPUs/kernels without it.
 static const bool enabled=(getauxval(AT_HWCAP)&(1ul<<12))!=0;
 return enabled;
#else
 return true;
#endif
}
// Two output pixels share each 8-lane operation; loads stay inside
// the last source row/column even for the final pair.
// Keep explicit SIMD instructions inside the runtime-gated call.
template<bool Reverse>
[[gnu::noinline]] void convert(const std::uint8_t* rgba,std::uint8_t* dest,unsigned pitch){
 const auto alpha=vreinterpret_u8_u32(vdup_n_u32(0xff000000u));
 for (unsigned row = 0; row < 120; ++row) {
  const auto* source = rgba + std::size_t(row) * 5 * 3200;
  auto* line = dest + std::size_t(Reverse ? 479 - row * 4 : row * 4) * pitch;
  const auto stride = Reverse ? -std::ptrdiff_t(pitch) : std::ptrdiff_t(pitch);
  for (unsigned group = 0; group < 160; ++group) {
   const auto pair = [&]<unsigned P>() {
    const uint16x8_t c{4-P,4-P,4-P,4-P,3-P,3-P,3-P,3-P};
    const uint16x8_t d{1+P,1+P,1+P,1+P,2+P,2+P,2+P,2+P};
    const auto* input = source + (group * 5 + P) * 4;
    const auto horizontal = [&](unsigned y) {
     const auto* pixels = input + y * 3200;
     return vmlaq_u16(vmulq_u16(vmovl_u8(vld1_u8(pixels)), c),
                      vmovl_u8(vld1_u8(pixels+4)), d);
    };
    const unsigned x = group * 4 + P;
    auto* output = line + (Reverse ? 638-x : x) * 4;
    const auto vertical = [&]<unsigned Y>(uint16x8_t upper, uint16x8_t lower) {
     const auto n = vaddq_u16(vmlaq_n_u16(vmulq_n_u16(upper,4-Y),lower,Y+1),vdupq_n_u16(12));
     // n <= 6387, so floor(n / 25) equals (n * 5243) >> 17.
     const auto low = vmovn_u32(vshrq_n_u32(vmull_n_u16(vget_low_u16(n),5243),17));
     const auto high = vmovn_u32(vshrq_n_u32(vmull_n_u16(vget_high_u16(n),5243),17));
     auto out = vorr_u8(vtbl1_u8(vmovn_u16(vcombine_u16(low,high)),uint8x8_t{2,1,0,3,6,5,4,7}),alpha);
     if constexpr(Reverse) out=vreinterpret_u8_u32(vrev64_u32(vreinterpret_u32_u8(out)));
     vst1_u8(output + Y * stride, out);
    };
    const auto h0 = horizontal(0), h1 = horizontal(1);
    vertical.template operator()<0>(h0,h1);
    const auto h2 = horizontal(2);
    vertical.template operator()<1>(h1,h2);
    const auto h3 = horizontal(3);
    vertical.template operator()<2>(h2,h3);
    vertical.template operator()<3>(h3,horizontal(4));
   };
   pair.template operator()<0>();
   pair.template operator()<2>();
  }
 }
}
}
#else
#define D2MINI_NEON_FILTER_AVAILABLE 0
namespace d2mini::frame_filter_detail {inline bool available(){return false;}}
#endif
