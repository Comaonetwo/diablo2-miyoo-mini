// SPDX-License-Identifier: GPL-3.0-or-later
#include "panel_zoom.hpp"
#include <chrono>
#include <cstdio>
#include <random>
#include <vector>

using Frame = std::vector<std::uint8_t>;
using Zoom = void (*)(std::span<std::uint8_t>, int);

static void previous_zoom(std::span<std::uint8_t> frame, int mode) {
 const int width = d2mini::world_width(mode);
 const int height = width * 3 / 4;
 d2mini::expand_rect(frame, (800 - width) / 2, (600 - height) / 2,
                     width, height, 0, 0, 800, 600);
}

double sample(Zoom zoom, const Frame& input, int mode) {
 Frame buffer(input);
 std::chrono::nanoseconds total{};
 for (int i = 0; i < 64; ++i) {
  buffer = input;
  const auto start = std::chrono::steady_clock::now();
  zoom(buffer, mode);
  asm volatile("" : : "r"(buffer.data()) : "memory");
  total += std::chrono::steady_clock::now() - start;
 }
 return total.count() / 64000.0;
}

int main(int argc, char**) {
 std::mt19937 rng(24119);
 Frame input(800 * 600 * 4);
 for (int image = 0; image < 16; ++image) {
  for (auto& byte : input) byte = static_cast<std::uint8_t>(rng());
  for (int mode : {0, 1, 2}) {
   auto expected = input, actual = input;
   previous_zoom(expected, mode);
   d2mini::zoom_world(actual, mode);
   if (expected != actual) return 1;
  }
 }
 std::puts("PASS 48 complete RGBA frame comparisons");
 if (argc > 1) return 0;
 for (int mode : {1, 2}) {
  sample(previous_zoom, input, mode);
  sample(d2mini::zoom_world, input, mode);
  std::array<double, 16> old{}, improved{}, saving{};
  for (int i = 0; i < 16; ++i) {
   if (i % 2 == 0) old[i] = sample(previous_zoom, input, mode);
   improved[i] = sample(d2mini::zoom_world, input, mode);
   if (i % 2 != 0) old[i] = sample(previous_zoom, input, mode);
   saving[i] = 100 * (1 - improved[i] / old[i]);
  }
  for (auto* series : {&old, &improved, &saving}) std::sort(series->begin(), series->end());
  std::printf("mode %d: %.1f -> %.1f us; median paired saving %.1f%% (range %.1f..%.1f%%)\n",
              mode, (old[7]+old[8])/2, (improved[7]+improved[8])/2,
              (saving[7]+saving[8])/2, saving.front(), saving.back());
 }
}
