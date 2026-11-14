// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "scene.hpp"
#include "gamepad_map.hpp"
#include "platform.hpp"
#include <array>
#include <vector>
#include <string>
namespace d2mini {
std::string inspect_hud_assets(const d2d::client::Scene&,const std::filesystem::path&);
struct PortUI {
    bool open=false, controls=false, options=false, changed=false, quit=false, return_menu=false;
    int row=0, scroll=0, sfx_volume=100, music_volume=50, text_scale=100, ui_scale=125, map_opacity=100;
    bool smooth_camera=true;
    bool closing=false, close_quit=false, close_return=false;
    std::uint64_t fade_start=0;float close_from=1;
    float fade_alpha()const {const float t=std::clamp(float(SDL_GetTicks()-fade_start)/100.f,0.f,1.f);return closing?close_from*(1-t):t;}
    void request_close(bool quit_game=false,bool to_menu=false){if(closing)return;close_from=fade_alpha();closing=true;close_quit=quit_game;close_return=to_menu;fade_start=SDL_GetTicks();}
    std::vector<std::uint8_t> backdrop;
    void capture(const std::vector<std::uint8_t>& frame);
    int world_big=0; // 0: 800, 1: 640, 2: 480 logical world width
    bool fast=true, sound=true,compact=true,dim_loot=true,readable_font=true,smooth=true,belt_sequential=false,belt_refill=false,sharp_panel_text=false;
    std::array<bool,std::size_t(d2d::client::pad::Button::Count)> previous{};
    std::array<float,4> repeats{};
    void show(const d2d::client::pad::Sample& sample) {open=true;controls=false;options=false;row=2;scroll=0;return_menu=false;closing=close_quit=close_return=false;fade_start=SDL_GetTicks();smooth=true;sharp_panel_text=true;previous=sample.held;repeats={};}
    void input(const d2d::client::pad::Sample&,std::span<const SDL_Keycode>,float seconds);
    void draw(std::vector<std::uint8_t>&,const d2d::client::Scene&) const;
    static int valid_scale(int value) { return value>=150?200:100; }
};
// All input remains in the original coordinates. The focused panel and its
// cursor are transformed together, so controller selection still hits cells.
struct InterfaceView {
    void present(std::vector<std::uint8_t>& fb,int scale,bool left,bool right,int mouse_x,int mouse_y,bool tree=false);
};
}
