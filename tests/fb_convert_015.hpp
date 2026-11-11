// Baseline 0.2.15 scanout, retained only for benchmark comparison.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <initializer_list>
#include <cstring>
#include <span>
#include <stdexcept>
#include <vector>

namespace scanout015 {
struct Channel { unsigned offset, length; };
struct FrameFormat {
    unsigned width, height, pitch, bits;
    Channel red, green, blue, alpha;
};
inline void convert_frame(std::span<const std::uint8_t> rgba, unsigned width, unsigned height,
                          std::span<std::uint8_t> dest, FrameFormat f, bool rotate = true) {
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
