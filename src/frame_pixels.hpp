// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <span>
#include "frame_filter_neon.hpp"

namespace d2mini {
#if D2MINI_NEON_FILTER_AVAILABLE
namespace detail {
[[gnu::noinline]] std::size_t restore_opaque_alpha_neon(std::span<std::uint8_t> rgba) {
    const auto opacity = vreinterpretq_u8_u32(vdupq_n_u32(0xff000000u));
    const auto limit = rgba.size() & ~std::size_t{15};
    for (std::size_t offset = 0; offset < limit; offset += 16) {
        auto* pixels = rgba.data() + offset;
        vst1q_u8(pixels, vorrq_u8(vld1q_u8(pixels), opacity));
    }
    return limit;
}
}
#endif

inline void restore_opaque_alpha(std::span<std::uint8_t> rgba) {
    std::size_t offset = 0;
#if D2MINI_NEON_FILTER_AVAILABLE
    if (frame_filter_detail::available()) offset = detail::restore_opaque_alpha_neon(rgba);
#endif
    for (; offset + 4 <= rgba.size(); offset += 4) rgba[offset + 3] = 255;
}
}
