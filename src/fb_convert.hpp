// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <initializer_list>
#include <cstring>
#include <span>
#include <stdexcept>
#include <vector>
#include "frame_filter_neon.hpp"

namespace d2mini {
inline bool g_smooth_frame=true;
struct Channel { unsigned offset, length; };
struct FrameFormat {
    unsigned width, height, pitch, bits;
    Channel red, green, blue, alpha;
};
inline void convert_frame(std::span<const std::uint8_t> rgba, unsigned width, unsigned height,
                          std::span<std::uint8_t> dest, FrameFormat f, bool rotate = true, bool smooth = false) {
    if (!width || !height || !f.width || !f.height || (f.bits != 16 && f.bits != 32) ||
        rgba.size() != std::uint64_t(width) * height * 4 || std::uint64_t(f.width) * (f.bits / 8) > f.pitch ||
        std::uint64_t(f.height - 1) * f.pitch + std::uint64_t(f.width) * (f.bits / 8) > dest.size())
        throw std::runtime_error("Invalid framebuffer bounds");
    for (auto c : {f.red, f.green, f.blue, f.alpha})
        if (c.length > 8 || c.offset + c.length > f.bits) throw std::runtime_error("Invalid channel");
    unsigned dw = f.width, dh = std::uint64_t(dw) * height / width;
    if (dh > f.height) { dh = f.height; dw = std::uint64_t(dh) * width / height; }
    if (!dw || !dh) throw std::runtime_error("Empty scaled frame");
    const auto bx = (f.width - dw) / 2, by = (f.height - dh) / 2;
    const auto pack = [](std::uint8_t value, Channel c) { return c.length ? (std::uint32_t(value) >> (8 - c.length)) << c.offset : 0; };
    const unsigned bpp = f.bits / 8;
    // Exact Mini 5:4 reduction. Four output pixels are the first four of
    // each five source pixels; no map allocation or division per frame.
    if(width==800&&height==600&&f.width==640&&f.height==480&&f.bits==32
        &&f.red.offset==16&&f.red.length==8&&f.green.offset==8&&f.green.length==8
        &&f.blue.offset==0&&f.blue.length==8&&f.alpha.offset==24&&f.alpha.length==8){
        if(smooth){
#if D2MINI_NEON_FILTER_AVAILABLE
            if(frame_filter_detail::available()&&f.pitch%4==0&&reinterpret_cast<std::uintptr_t>(dest.data())%4==0){
                if(rotate)frame_filter_detail::convert<true>(rgba.data(),dest.data(),f.pitch);
                else frame_filter_detail::convert<false>(rgba.data(),dest.data(),f.pitch);
                return;
            }
#endif
            // Exact 5:4 area reduction: adjacent source weights are 4:1,
            // 3:2, 2:3, 1:4. All five rows/columns contribute; no scratch
            // framebuffer or allocation is required. RGB channels are linear
            // integer averages, rounded identically to the preview model.
            const auto draw=[&]<bool Reverse>(){
                for(unsigned y=0;y<480;++y){
                    const unsigned sy=(y/4)*5+y%4, a=4-y%4, b=5-a;
                    const auto* row0=rgba.data()+std::size_t(sy)*3200;
                    const auto* row1=row0+3200;
                    auto* line=dest.data()+std::size_t(Reverse?479-y:y)*f.pitch;
                    for(unsigned group=0;group<160;++group){
                        for(unsigned phase=0;phase<4;++phase){
                            const unsigned sx=(group*5+phase)*4,c=4-phase,d=5-c;
                            const auto channel=[&](unsigned k){
                                const unsigned upper=unsigned(row0[sx+k])*c+unsigned(row0[sx+4+k])*d;
                                const unsigned lower=unsigned(row1[sx+k])*c+unsigned(row1[sx+4+k])*d;
                                return (upper*a+lower*b+12)/25;
                            };
                            const std::uint32_t value=0xff000000u|(channel(0)<<16)|(channel(1)<<8)|channel(2);
                            const unsigned x=group*4+phase;
                            std::memcpy(line+(Reverse?639-x:x)*4,&value,4);
                        }
                    }
                }
            };
            if(rotate)draw.template operator()<true>();else draw.template operator()<false>();return;
        }
        const auto pixel=[](const std::uint8_t* src){std::uint32_t c;std::memcpy(&c,src,4);return 0xff000000u|(c&0xff00u)|((c&255u)<<16)|((c>>16)&255u);};
        const auto draw=[&]<bool Reverse>(){
            for(unsigned y=0;y<480;++y){const auto* src=rgba.data()+std::size_t((y/4)*5+y%4)*3200;auto* line=dest.data()+std::size_t(Reverse?479-y:y)*f.pitch;
                for(unsigned group=0;group<160;++group,src+=20){
                    const auto put=[&](unsigned i){const auto value=pixel(src+i*4);const unsigned x=group*4+i;std::memcpy(line+(Reverse?639-x:x)*4,&value,4);};
                    put(0);put(1);put(2);put(3);
                }
            }
        };
        if(rotate)draw.template operator()<true>();else draw.template operator()<false>();return;
    }
    // Divide once per output column/row, rather than twice per pixel. ARMv7
    // otherwise calls software 64-bit division hundreds of thousands of times.
    std::vector<std::size_t> columns(dw);
    for (unsigned x = 0; x < dw; ++x) columns[x] = std::size_t(std::uint64_t(x) * width / dw) * 4;
    // Common Mini 32-bit BGRA scanout: fixed channel positions, no borders.
    // Avoid the per-pixel variable shifts and variable-size memcpy fallback.
    if (f.bits == 32 && dw == f.width && dh == f.height && f.red.offset == 16 && f.red.length == 8
        && f.green.offset == 8 && f.green.length == 8 && f.blue.offset == 0 && f.blue.length == 8
        && f.alpha.offset == 24 && f.alpha.length == 8) {
        for (unsigned y = 0; y < f.height; ++y) {
            const auto* row = rgba.data() + std::size_t(std::uint64_t(y) * height / dh) * width * 4;
            auto* line = dest.data() + std::size_t(rotate ? f.height - 1 - y : y) * f.pitch;
            for (unsigned x = 0; x < dw; ++x) {
                const auto* p = row + columns[x];
                const std::uint32_t pixel = std::uint32_t(p[0]) << 16 | std::uint32_t(p[1]) << 8 | p[2] | 0xff000000u;
                std::memcpy(line + std::size_t(rotate ? dw - 1 - x : x) * 4, &pixel, 4);
            }
        }
        return;
    }
    for (unsigned y = 0; y < f.height; ++y) {
        const bool content_y = y >= by && y < by + dh;
        const auto* source_row = content_y ? rgba.data() + std::size_t(std::uint64_t(y - by) * height / dh) * width * 4 : nullptr;
        auto* line = dest.data() + std::size_t(rotate ? f.height - 1 - y : y) * f.pitch;
        for (unsigned x = 0; x < f.width; ++x) {
            std::uint32_t pixel = 0;
            if (x >= bx && x < bx + dw && y >= by && y < by + dh) {
                const auto* source = source_row + columns[x - bx];
                pixel = pack(source[0], f.red) | pack(source[1], f.green) | pack(source[2], f.blue);
                if (f.alpha.length) pixel |= pack(255, f.alpha);
            }
            std::memcpy(line + std::size_t(rotate ? f.width - 1 - x : x) * bpp, &pixel, bpp);
        }
    }
}
}
