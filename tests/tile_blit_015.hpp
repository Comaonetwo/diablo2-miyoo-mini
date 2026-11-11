// Baseline 0.2.15 raster path, retained only for controlled performance comparison.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "world_view.hpp"
#include <cstring>
namespace reference015 {
using namespace d2d::client;
void blit_dt1_tile(std::vector<std::uint8_t>& framebuffer,
                   const d2d::dt1::Tile& tile,
                   const d2d::palette::Palette& pal,
                   int screen_x, int screen_y, int alpha_all , const Hole* hole) {
    if(d2mini::world_occlusion.hides(screen_x,screen_y,int(tile.width),std::abs(tile.height)))return;
    const int screen_w = fb_width(framebuffer);
    const int left = std::max(0, -screen_x), right = std::min(int(tile.width), screen_w - screen_x);
    const int first = std::max(0, -screen_y), last = std::min(std::abs(tile.height), int(kScreenHeight) - screen_y);
    if (left >= right || first >= last) return;
    for (int y = first; y < last; ++y) {
        const int pixel_y = screen_y + y;
        d2mini::world_occlusion.visible(screen_x+left,screen_x+right,screen_y+y,[&](int x0,int x1){
        tile.spans(std::size_t(y), x0-screen_x, x1-screen_x, [&](int x, const std::uint8_t* indices, int count) {
            auto* pixel = framebuffer.data() + (std::size_t(pixel_y) * screen_w + screen_x + x) * 4;
            if (!hole && alpha_all >= 255) {
                for (int i = 0; i < count; ++i, pixel += 4) {
                    auto c = pal[indices[i]]; c.a = 255; std::memcpy(pixel, &c, 4);
                }
            } else for (int i = 0; i < count; ++i, pixel += 4) {
                const auto c = pal[indices[i]];
                const int alpha = hole ? hole->alpha(screen_x + x + i, pixel_y, alpha_all) : alpha_all;
                pixel[0] = std::uint8_t((pixel[0] * (255 - alpha) + c.r * alpha) / 255);
                pixel[1] = std::uint8_t((pixel[1] * (255 - alpha) + c.g * alpha) / 255);
                pixel[2] = std::uint8_t((pixel[2] * (255 - alpha) + c.b * alpha) / 255);
                pixel[3] = 255;
            }
        });
        });
    }
}

void blit_dt1_shadow(std::vector<std::uint8_t>& framebuffer, const d2d::dt1::Tile& tile,
                     const d2d::palette::Palette& pal, int screen_x, int screen_y) {
    if(d2mini::world_occlusion.hides(screen_x,screen_y,int(tile.width),std::abs(tile.height)))return;
    const int screen_w = fb_width(framebuffer);
    const int left = std::max(0, -screen_x), right = std::min(int(tile.width), screen_w - screen_x);
    const int first = std::max(0, -screen_y), last = std::min(std::abs(tile.height), int(kScreenHeight) - screen_y);
    if (left >= right || first >= last) return;
    for (int y = first; y < last; ++y) {
        d2mini::world_occlusion.visible(screen_x+left,screen_x+right,screen_y+y,[&](int x0,int x1){
        tile.spans(std::size_t(y), x0-screen_x, x1-screen_x, [&](int x, const std::uint8_t* indices, int count) {
            auto* pixel = framebuffer.data() + (std::size_t(screen_y + y) * screen_w + screen_x + x) * 4;
            for (int i = 0; i < count; ++i, pixel += 4) {
                const auto c = pal[indices[i]];
                pixel[0] = std::uint8_t((pixel[0] * 3 + c.r) / 4);
                pixel[1] = std::uint8_t((pixel[1] * 3 + c.g) / 4);
                pixel[2] = std::uint8_t((pixel[2] * 3 + c.b) / 4);
            }
        });
        });
    }
}

void blit_dt1_tile_lit(std::vector<std::uint8_t>& framebuffer, const d2d::dt1::Tile& tile, const Lighting& light,
                       int screen_x, int screen_y, int cell_x, int cell_y, int top_x, int top_y, bool floor, int alpha_all , const Hole* hole) {
    if(d2mini::world_occlusion.hides(screen_x,screen_y,int(tile.width),std::abs(tile.height)))return;
    // The subtile corners round the cell: a tile's pixels reach up to 6
    // subtiles before its top corner (tall floors) and 8 past.
    if (screen_x >= fb_width(framebuffer) || screen_x + tile.width <= 0
        || screen_y >= int(kScreenHeight) || screen_y + std::abs(tile.height) <= 0) return;
    constexpr int kPatch = 16, kOrigin = 6;
    std::array<float, kPatch * kPatch> corners{};
    int low = 255, high = 0;
    for (int j = 0; j < kPatch; ++j)
        for (int i = 0; i < kPatch; ++i) {
            const int value = light.grid.at(cell_x * 5 - kOrigin + i, cell_y * 5 - kOrigin + j);
            corners[std::size_t(j * kPatch + i)] = float(value);
            low = std::min(low, value); high = std::max(high, value);
        }
    const auto& pals = *light.pal;
    if (low >> 3 == high >> 3) { reference015::blit_dt1_tile(framebuffer, tile, pals[std::size_t(low >> 3)], screen_x, screen_y, alpha_all, hole); return; }
    // Screen offset from the top corner → subtiles into the cell: dx - dy =
    // x / 80 cells, dx + dy = y / 40.
    auto level = [&](int offset_x, int offset_y) {
        const float along_x = float(offset_x) / (kIsoW / 2), along_y = float(offset_y) / (kIsoH / 2);
        const float grid_a = std::clamp((along_y + along_x) * 2.5f + kOrigin, 0.f, kPatch - 1.001f), grid_b = std::clamp((along_y - along_x) * 2.5f + kOrigin, 0.f, kPatch - 1.001f);
        const int column = int(grid_a), row = int(grid_b);
        const float frac_a = grid_a - float(column), fb2 = grid_b - float(row);
        const float* corner = &corners[std::size_t(row * kPatch + column)];
        return std::size_t(int((corner[0] + (corner[1] - corner[0]) * frac_a) * (1 - fb2) + (corner[kPatch] + (corner[kPatch + 1] - corner[kPatch]) * frac_a) * fb2) >> 3);
    };
    // Wall lighting depends only on the screen column; reuse it across rows.
    // Zero means not sampled yet, levels 0..31 are stored as 1..32.
    std::array<std::uint8_t, kScreenWidth> wall_levels{};
    const int screen_w = fb_width(framebuffer);
    const int left = std::max(0, -screen_x), right = std::min(int(tile.width), screen_w - screen_x);
    const int first = std::max(0, -screen_y), last = std::min(std::abs(tile.height), int(kScreenHeight) - screen_y);
    for (int y = first; y < last; ++y) {
        const int pixel_y = screen_y + y;
        int last_group = INT32_MIN; std::size_t cached_light = 0;
        d2mini::world_occlusion.visible(screen_x+left,screen_x+right,screen_y+y,[&](int x0,int x1){
        tile.spans(std::size_t(y), x0-screen_x, x1-screen_x, [&](int x, const std::uint8_t* indices, int count) {
            auto* pixel = framebuffer.data() + (std::size_t(pixel_y) * screen_w + screen_x + x) * 4;
            for (int i = 0; i < count; ++i, pixel += 4) {
                const int pixel_x = screen_x + x + i;
                const int group = g_fast_render ? (pixel_x & ~3) : pixel_x;
                if (group != last_group) {
                    if (floor) cached_light = level(group - top_x, (g_fast_render ? (pixel_y & ~1) : pixel_y) - top_y);
                    else if (std::size_t(group) >= wall_levels.size()) cached_light = level(group - top_x, kIsoH / 2);
                    else {
                        auto& sample = wall_levels[std::size_t(group)];
                        if (!sample) sample = std::uint8_t(level(group - top_x, kIsoH / 2) + 1);
                        cached_light = sample - 1;
                    }
                    last_group = group;
                }
                auto col = pals[cached_light][indices[i]];
                if (!hole && alpha_all >= 255) {col.a = 255; std::memcpy(pixel, &col, 4);}
                else {
                    const int alpha = hole ? hole->alpha(pixel_x, pixel_y, alpha_all) : alpha_all;
                    pixel[0] = std::uint8_t((pixel[0] * (255 - alpha) + col.r * alpha) / 255);
                    pixel[1] = std::uint8_t((pixel[1] * (255 - alpha) + col.g * alpha) / 255);
                    pixel[2] = std::uint8_t((pixel[2] * (255 - alpha) + col.b * alpha) / 255);
                    pixel[3] = 255;
                }
            }
        });
        });
    }
}

}
