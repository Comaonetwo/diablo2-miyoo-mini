// SPDX-License-Identifier: GPL-3.0-or-later
// Frozen pre-0.2.8 rasterizer used only for pixel equivalence tests.
namespace reference {
using namespace d2d::client;
void blit_dt1_tile(std::vector<std::uint8_t>& framebuffer,
                   const d2d::dt1::Tile& tile,
                   const d2d::palette::Palette& pal,
                   int screen_x, int screen_y, int alpha_all , const Hole* hole) {
    thread_local std::vector<std::uint8_t> row_scratch;
    const int tile_height = std::abs(tile.height);
    const int screen_w = fb_width(framebuffer);
    for (int y = 0; y < tile_height; ++y) {
        const int pixel_y = screen_y + y;
        if (pixel_y < 0 || pixel_y >= int(kScreenHeight)) continue;
        const auto* row = tile.row(std::size_t(y), row_scratch);
        for (int x = 0; x < tile.width; ++x) {
            const std::uint8_t idx = row[x];
            if (idx == 0) continue;   // transparent
            const int pixel_x = screen_x + x;
            if (pixel_x < 0 || pixel_x >= screen_w) continue;
            const auto colour = pal[idx];
            auto* pixel = framebuffer.data() + (std::size_t(pixel_y) * std::size_t(screen_w) + std::size_t(pixel_x)) * 4;
            const int alpha = hole ? hole->alpha(pixel_x, pixel_y, alpha_all) : alpha_all;
            if (alpha >= 255) { pixel[0] = colour.r; pixel[1] = colour.g; pixel[2] = colour.b; }
            else { pixel[0] = std::uint8_t((pixel[0] * (255 - alpha) + colour.r * alpha) / 255); pixel[1] = std::uint8_t((pixel[1] * (255 - alpha) + colour.g * alpha) / 255); pixel[2] = std::uint8_t((pixel[2] * (255 - alpha) + colour.b * alpha) / 255); }
            pixel[3] = 0xFF;
        }
    }
}

void blit_dt1_shadow(std::vector<std::uint8_t>& framebuffer, const d2d::dt1::Tile& tile,
                     const d2d::palette::Palette& pal, int screen_x, int screen_y) {
    thread_local std::vector<std::uint8_t> row_scratch;
    const int tile_height = std::abs(tile.height);
    const int screen_w = fb_width(framebuffer);
    for (int y = 0; y < tile_height; ++y) {
        const int pixel_y = screen_y + y;
        if (pixel_y < 0 || pixel_y >= int(kScreenHeight)) continue;
        const auto* row = tile.row(std::size_t(y), row_scratch);
        for (int x = 0; x < tile.width; ++x) {
            const std::uint8_t idx = row[x];
            const int pixel_x = screen_x + x;
            if (idx == 0 || pixel_x < 0 || pixel_x >= screen_w) continue;
            const auto colour = pal[idx];
            auto* pixel = framebuffer.data() + (std::size_t(pixel_y) * std::size_t(screen_w) + std::size_t(pixel_x)) * 4;
            pixel[0] = std::uint8_t((pixel[0] * 3 + colour.r) / 4); pixel[1] = std::uint8_t((pixel[1] * 3 + colour.g) / 4); pixel[2] = std::uint8_t((pixel[2] * 3 + colour.b) / 4);
        }
    }
}

void blit_dt1_tile_lit(std::vector<std::uint8_t>& framebuffer, const d2d::dt1::Tile& tile, const Lighting& light,
                       int screen_x, int screen_y, int cell_x, int cell_y, int top_x, int top_y, bool floor, int alpha_all , const Hole* hole) {
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
    if (low >> 3 == high >> 3) { reference::blit_dt1_tile(framebuffer, tile, pals[std::size_t(low >> 3)], screen_x, screen_y, alpha_all, hole); return; }
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
    thread_local std::vector<std::uint8_t> row_scratch;
    const int tile_height = std::abs(tile.height);
    const int screen_w = fb_width(framebuffer);
    for (int y = 0; y < tile_height; ++y) {
        const int pixel_y = screen_y + y;
        if (pixel_y < 0 || pixel_y >= int(kScreenHeight)) continue;
        const auto* row = tile.row(std::size_t(y), row_scratch);
        int last_group = INT32_MIN; std::size_t cached_light = 0;
        for (int x = std::max(0, -screen_x); x < std::min(tile.width, screen_w - screen_x); ++x) {
            const std::uint8_t idx = row[x];
            if (idx == 0) continue;
            const int pixel_x = screen_x + x;
            if (pixel_x < 0 || pixel_x >= screen_w) continue;
            const int group = g_fast_render ? (pixel_x & ~3) : pixel_x;
            if (group != last_group) {
                cached_light = level(group - top_x, floor ? (g_fast_render ? (pixel_y & ~1) : pixel_y) - top_y : kIsoH / 2);
                last_group = group;
            }
            const auto lvl = cached_light;
            const auto col = pals[lvl][idx];
            auto* pixel = framebuffer.data() + (std::size_t(pixel_y) * std::size_t(screen_w) + std::size_t(pixel_x)) * 4;
            const int alpha = hole ? hole->alpha(pixel_x, pixel_y, alpha_all) : alpha_all;
            if (alpha >= 255) { pixel[0] = col.r; pixel[1] = col.g; pixel[2] = col.b; }
            else { pixel[0] = std::uint8_t((pixel[0] * (255 - alpha) + col.r * alpha) / 255); pixel[1] = std::uint8_t((pixel[1] * (255 - alpha) + col.g * alpha) / 255); pixel[2] = std::uint8_t((pixel[2] * (255 - alpha) + col.b * alpha) / 255); }
            pixel[3] = 0xFF;
        }
    }
}

}
