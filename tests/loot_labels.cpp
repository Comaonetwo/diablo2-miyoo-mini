// SPDX-License-Identifier: GPL-3.0-or-later
#include "loot_labels.hpp"
#include <cassert>
#include <cstdio>

static void actor_coverage_is_restored() {
    assert(d2mini::actor_pixel_alpha == 255);
    {
        d2mini::ActorCoverageScope outer(true);
        assert(d2mini::actor_pixel_alpha == 254);
        {
            d2mini::ActorCoverageScope inner(false);
            assert(d2mini::actor_pixel_alpha == 255);
        }
        assert(d2mini::actor_pixel_alpha == 254);
    }
    assert(d2mini::actor_pixel_alpha == 255);
}

static void inactive_or_offscreen_labels_leave_drawing_untouched() {
    constexpr int width = 16;
    std::vector<std::uint8_t> pixels(width * 600 * 4, 20);
    const auto check = [&](std::array<int, 4> rect, float fade) {
        std::fill(pixels.begin(), pixels.end(), 20);
        {
            d2mini::LootLabelComposite blend(pixels, width, rect, fade, false);
            std::fill(pixels.begin(), pixels.end(), 200);
        }
        assert(std::all_of(pixels.begin(), pixels.end(), [](auto value) { return value == 200; }));
    };
    check({0, 0, 8, 8}, 0.f);
    check({-30, 0, -20, 8}, 1.f);
    check({0, 610, 8, 620}, 1.f);
}

static void alpha_only_edits_restore_actor_coverage() {
    constexpr int width = 16;
    std::vector<std::uint8_t> frame(width * 600 * 4, 255);
    const int actor_alpha = (3 * width + 2) * 4 + 3;
    frame[actor_alpha] = 254;
    const auto expected = frame;
    {
        d2mini::LootLabelComposite blend(frame, width, {0, 0, 8, 8}, 1.f, false);
        frame[actor_alpha] = 255;
    }
    assert(frame == expected);
}

int main(){
 alpha_only_edits_restore_actor_coverage();
 actor_coverage_is_restored();
 inactive_or_offscreen_labels_leave_drawing_untouched();
 d2mini::LootFade fade;assert(fade.step(100,false)==0);assert(fade.step(250,true)==.5f);assert(fade.step(400,true)==1);assert(fade.step(550,false)==.5f);assert(fade.step(700,false)==0);
 for(bool selected:{false,true}){
  std::vector<std::uint8_t> pixels(8*600*4,20);for(std::size_t i=3;i<pixels.size();i+=4)pixels[i]=255;
  auto* actor=pixels.data()+(3*8+3)*4;actor[0]=90;actor[1]=70;actor[2]=50;actor[3]=254;
  {const d2mini::LootLabelComposite blend(pixels,8,{0,0,8,8},1,selected);for(int y=0;y<8;++y)for(int x=0;x<8;++x)for(int c=0;c<3;++c)pixels[(y*8+x)*4+c]=255;}
  assert(actor[0]==90&&actor[1]==70&&actor[2]==50&&actor[3]==254);
  assert(selected?pixels[0]==255:pixels[0]>20&&pixels[0]<130);
 }
 std::vector<std::uint8_t> frame(16*600*4,100);{const d2mini::LootLabelComposite blend(frame,16,{-20,-30,7,6},.5f,false);frame[0]=200;}assert(frame[0]>100&&frame[0]<200);
 std::puts("PASS loot combat fade: 300 ms both directions, selected label readability, exact actor preservation, faded background and clipped labels");
}
